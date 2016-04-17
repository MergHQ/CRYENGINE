// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Sydewinder.Framework;
using CryEngine.Components;
using CryEngine.EntitySystem;

namespace CryEngine.Sydewinder.Types
{
	/// <summary>
	/// Manages tunnels with doors and lights which are currently spawned within a level.
	/// </summary>
	public class LevelGeometry
	{
		public static Vec3 GlobalGeomSpeed { get; private set; } = new Vec3(0, -10f, 0); 

		/// <summary>
		/// Currently spawned tunnel elements (with doors + lights within).
		/// </summary>
		private Queue<Tunnel> _tunnelElementsQueue = null;

		/// <summary>
		/// Currently last tunnel element (new tunnels are spawned after that one).
		/// </summary>
		private Tunnel _lastTunnel = null;

		/// <summary>
		/// Will be populated with the name of the last tunnel (eg. "tunnel_01") to avoid using the same tunnel again.
		/// </summary>
		private int _lastTunnelNumber = 1;

		private Entity _rotatingRingOne = null;
		private Entity _rotatingRingTwo = null;

		/// <summary>
		/// To randomly generate tunnel and door numbers
		/// </summary>
		private static Random _randomizer = new Random();

		public static Random Randomizer
		{
			get { return _randomizer;}
		}

		public LevelGeometry()
		{
			_tunnelElementsQueue = new Queue<Tunnel>();

			// Get position.
			Vec3 firstTunnelPos = EntitySystem.Entity.ByName("TunnelSpawnPoint").Position;

			_lastTunnel = new Tunnel(firstTunnelPos, 0);
			_lastTunnel.Speed = LevelGeometry.GlobalGeomSpeed;

			_tunnelElementsQueue.Enqueue(_lastTunnel);

			_rotatingRingOne = EntitySystem.Entity.ByName("ring_2");
			_rotatingRingTwo = EntitySystem.Entity.ByName("ring_3");
		}

		/// <summary>
		/// Handles spawning of new tunnel elements and despawning of tunnel elements that are out of view.
		/// </summary>
		public void UpdateGeometry()
		{
			// Get first tunnel in Queue without removing the tunnel from the Queue.
			Tunnel firstTunnel = _tunnelElementsQueue.Peek();

			if (firstTunnel.Position.y < 450)
			{
				// Remove the tunnel from the Queue.
				_tunnelElementsQueue.Dequeue();

				// Purge door with related geometry (Door, Lights, ...).
				firstTunnel.Purge();
			}

			Vec3 lastTunnelPos = _lastTunnel.Position;
			if (lastTunnelPos.y < 700)
			{
				// Spawn new tunnel with door.
				int newTunnelNumber = _lastTunnelNumber;

				// Ensure the next tunnel element is different from the previous one.
				while (newTunnelNumber == _lastTunnelNumber)
					newTunnelNumber = _randomizer.Next(0, 4);

				_lastTunnelNumber = newTunnelNumber;
				_lastTunnel = new Tunnel(new Vec3(lastTunnelPos.x, (lastTunnelPos.y + 100f), lastTunnelPos.z), newTunnelNumber);
				_lastTunnel.Speed = LevelGeometry.GlobalGeomSpeed;
				_tunnelElementsQueue.Enqueue(_lastTunnel);
			}

			// Update background satelite rings
			RotateSatelliteRings();
		}

		private void RotateSatelliteRings()
		{
			Quat ringOneRot = new Quat(_rotatingRingOne.Rotation);
			Quat ringTwoRot = new Quat(_rotatingRingTwo.Rotation);

			_rotatingRingOne.Rotation = ringOneRot * Quat.CreateRotationZ(Utils.Deg2Rad(FrameTime.Normalize(10f)));
			_rotatingRingTwo.Rotation = ringTwoRot * Quat.CreateRotationZ(Utils.Deg2Rad(FrameTime.Normalize(-10f)));
		}
	}

	public class Tunnel : DestroyableBase
	{
		private Random _randomizer = new Random();

		/// <summary>
		/// The tunnel door at the beginning of the tunnel.
		/// </summary>
		private Door _tunnelDoor = null;

		/// <summary>
		/// The tunnel types.
		/// </summary>
		private readonly ObjectSkin[] _tunnelTypes = new ObjectSkin[]
		{			
			new ObjectSkin("objects/levelgeometry/tunnel_01.cgf"),
			new ObjectSkin("objects/levelgeometry/tunnel_02.cgf"),
			new ObjectSkin("objects/levelgeometry/tunnel_03.cgf"),
			new ObjectSkin("objects/levelgeometry/tunnel_04.cgf"),
		};

		/// <summary>
		/// Initializes a new instance of the <see cref="CryEngine.Sydewinder.Tunnel"/> class.
		/// </summary>
		/// <param name="position">Position.</param>
		/// <param name="tunnelType">Tunnel type.</param>
		public Tunnel(Vec3 position, int tunnelType)
		{
			if (tunnelType < 0 && tunnelType >= _tunnelTypes.Length)
				throw new ArgumentOutOfRangeException (
					string.Format ("tunnelType must betwenn 0 and {0}", (_tunnelTypes.Length - 1).ToString()));

			var tunnelSpawnParams = new SEntitySpawnParams() 
			{
				vPosition = position,
				pClass = Env.EntitySystem.GetClassRegistry().GetDefaultClass()
			};

			var spawnedEntity = Env.EntitySystem.SpawnEntity(tunnelSpawnParams);
			Entity = new EntitySystem.Entity(spawnedEntity);

			// Load geometry.
			spawnedEntity.LoadGeometry(0, _tunnelTypes[tunnelType].Geometry);

			if (tunnelType == 1)
			{
				var physics = new SEntityPhysicalizeParams() 
				{
					density = -1f,
					mass = 1f,
					type = (int)EPhysicalizationType.ePT_Rigid,
				};

				spawnedEntity.Physicalize(physics);
			}

			AddToGamePoolWithDoor(position);
		}

		private void AddToGamePoolWithDoor(Vec3 position = null)
		{
			GamePool.AddObjectToPool(this);

			if (position == null) // Get position via Engine EntitySystem.
				position = this.Position;

			// Create a door case as connector between tunnels.
			_tunnelDoor = new Door(new Vec3(position.x, position.y, position.z), 0);
			_tunnelDoor.Speed = LevelGeometry.GlobalGeomSpeed;

			// Create random door types
			int rand = LevelGeometry.Randomizer.Next(0, 3);
			if (rand != 0) {
				_tunnelDoor = new Door (new Vec3 (position.x, position.y, position.z), rand);
				_tunnelDoor.Speed = LevelGeometry.GlobalGeomSpeed;
			}

			ColorF lightColor = new ColorF(_randomizer.Next(0, 255), _randomizer.Next(0, 255), _randomizer.Next(0, 255));

			// Till 'MaxValue' as the for-loop is quit when once the first empty position is discovered.
			for (int i = 1; i < int.MaxValue; i++)
			{
				Vec3 helperPos = Entity.GetHelperPos(0, "Light_0" + i.ToString());

				if (helperPos.x != 0 || helperPos.y != 0 || helperPos.z != 0)
				{
					CDLight light = new CDLight() 
					{ 
						m_fRadius = 25f,
						m_fAttenuationBulbSize = 0.1f,
						m_BaseColor = lightColor,
						m_BaseSpecMult = 1f
					};

					Entity.LoadLight (i, light);
					Entity.SetTM(i, Matrix34.Create(Vec3.One, Quat.Identity, helperPos));
				}
				else
					break;
			}
		}

		public override void Collision()
		{
			// Undestroyable
			return;
		}

		public void Purge()
		{
			GamePool.FlagForPurge(this.Entity.ID);
			GamePool.FlagForPurge(_tunnelDoor.Entity.ID);
		}
	}

	public class Door : DestroyableBase
	{
		private const int HitDamage = 10;

		/// <summary>
		/// The door types.
		/// </summary>
		private readonly ObjectSkin[] _doorTypes = new ObjectSkin[]
		{			
			new ObjectSkin("objects/levelgeometry/door_01.cgf"), // Door Case
			new ObjectSkin("objects/levelgeometry/door_02.cgf"), // Bottom Door
			new ObjectSkin("objects/levelgeometry/door_03.cgf"), // Top Door
			//new ObjectSkin("objects/levelgeometry/door_04.cgf") // 2 Doors,Top, Bottom
		};

		/// <summary>
		/// Initializes a new instance of the <see cref="CryEngine.Sydewinder.Door"/> class.
		/// </summary>
		/// <param name="position">Position.</param>
		/// <param name="doorType">Door type.</param>
		public Door(Vec3 position, int doorType)
		{
			if (doorType < 0 && doorType >= _doorTypes.Length)
				throw new ArgumentOutOfRangeException (
					string.Format ("doorType must betwenn 0 and {0}", (_doorTypes.Length - 1).ToString()));

			Entity = EntitySystem.Entity.Instantiate (position, Quat.Identity, 1, _doorTypes [doorType].Geometry, _doorTypes[doorType].Material);

			GamePool.AddObjectToPool(this);

			string explosionSoundPath = System.IO.Path.Combine(Application.DataPath, "sounds/door_smash.wav");
			ExplosionSound = new MultiSoundPlayer(explosionSoundPath);
		}

		public override void Collision()
		{
			// Destroying Doors not allowed yet.
			return;
		}

		public void Destroy()
		{
			// Destroying Doors not allowed yet.
			return;
		}
	}
}
