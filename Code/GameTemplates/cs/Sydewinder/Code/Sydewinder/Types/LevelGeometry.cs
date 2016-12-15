// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.EntitySystem;
using System;
using System.Collections.Generic;

namespace CryEngine.Sydewinder.Types
{
	/// <summary>
	/// Manages tunnels with doors and lights which are currently spawned within a level.
	/// </summary>
	public class LevelGeometry
	{
		public static Vector3 GlobalGeomSpeed { get; private set; }

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

		public LevelGeometry()
		{
			GlobalGeomSpeed = new Vector3 (0, -10f, 0);
			_tunnelElementsQueue = new Queue<Tunnel>();

			// Get position.
			Vector3 firstTunnelPos = Entity.Find("TunnelSpawnPoint").Position;

			_lastTunnel = Tunnel.Create(firstTunnelPos, 0);
			_lastTunnel.Speed = LevelGeometry.GlobalGeomSpeed;

			_tunnelElementsQueue.Enqueue(_lastTunnel);

			_rotatingRingOne = Entity.Find("ring_2");
			_rotatingRingTwo = Entity.Find("ring_3");
		}

		/// <summary>
		/// Handles spawning of new tunnel elements and despawning of tunnel elements that are out of view.
		/// </summary>
		public void UpdateGeometry()
		{
			// Get first tunnel in Queue without removing the tunnel from the Queue.
			Tunnel firstTunnel = _tunnelElementsQueue.Peek();

			if (firstTunnel.Entity.Position.y < 450)
			{
				// Remove the tunnel from the Queue.
				_tunnelElementsQueue.Dequeue();

				// Purge door with related geometry (Door, Lights, ...).
				firstTunnel.Purge();
			}

			Vector3 lastTunnelPos = _lastTunnel.Entity.Position;
			if (lastTunnelPos.y < 700)
			{
				// Spawn new tunnel with door.
				int newTunnelNumber = _lastTunnelNumber;

				// Ensure the next tunnel element is different from the previous one.
				while (newTunnelNumber == _lastTunnelNumber)
					newTunnelNumber = Random.Next(4);

				_lastTunnelNumber = newTunnelNumber;
				_lastTunnel = Tunnel.Create(new Vector3(lastTunnelPos.x, (lastTunnelPos.y + 100f), lastTunnelPos.z), newTunnelNumber);
				_lastTunnel.Speed = LevelGeometry.GlobalGeomSpeed;
				_tunnelElementsQueue.Enqueue(_lastTunnel);
			}

			// Update background satelite rings
			RotateSatelliteRings();
		}

		private void RotateSatelliteRings()
		{
			_rotatingRingOne.Rotation *= Quaternion.CreateRotationZ(MathHelpers.DegreesToRadians(FrameTime.Delta * 10f));
			_rotatingRingTwo.Rotation *= Quaternion.CreateRotationZ(MathHelpers.DegreesToRadians(FrameTime.Delta * -10f));
		}
	}

	public class Tunnel : DestroyableBase
	{
		/// <summary>
		/// The tunnel door at the beginning of the tunnel.
		/// </summary>
		private Door _tunnelDoor = null;

		/// <summary>
		/// The tunnel types.
		/// </summary>
		private static readonly ObjectSkin[] TunnelTypes = new ObjectSkin[]
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
		public static Tunnel Create(Vector3 pos, int tunnelType)
		{
			if (tunnelType < 0 && tunnelType >= TunnelTypes.Length)
				throw new ArgumentOutOfRangeException (
					string.Format ("tunnelType must betwenn 0 and {0}", (TunnelTypes.Length - 1).ToString()));

			var tunnel = Entity.SpawnWithComponent<Tunnel> (pos, Quaternion.Identity, 1.0f);

			tunnel.Entity.LoadGeometry(0, TunnelTypes[tunnelType].Geometry);

			if (tunnelType == 1)
			{
				tunnel.Entity.Physics.Physicalize(1f, EPhysicalizationType.ePT_Rigid);
			}

			tunnel.AddToGamePoolWithDoor(pos);
			return tunnel;
		}

		private void AddToGamePoolWithDoor(Vector3 ?pos = null)
		{
			GamePool.AddObjectToPool(this);

			Vector3 position;
			if (pos.HasValue)
			{
				position = pos.Value;
			}
			else
			{
				position = this.Entity.Position;
			}

			// Create a door case as connector between tunnels.
			_tunnelDoor = Door.Create(new Vector3(position.x, position.y, position.z), 0);
			_tunnelDoor.Speed = LevelGeometry.GlobalGeomSpeed;

			// Create random door types
			int rand = Random.Next(3);
			if (rand != 0) {
				_tunnelDoor = Door.Create (new Vector3 (position.x, position.y, position.z), rand);
				_tunnelDoor.Speed = LevelGeometry.GlobalGeomSpeed;
			}

			ColorF lightColor = new ColorF(Random.Next(0, 255), Random.Next(0, 255), Random.Next(0, 255));

			// Till 'MaxValue' as the for-loop is quit when once the first empty position is discovered.
			for (int i = 1; i < int.MaxValue; i++)
			{
				Vector3 helperPos = Entity.GetHelperPos(0, "Light_0" + i.ToString());

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
					Entity.SetGeometrySlotLocalTransform(i, new Matrix3x4(Vector3.One, Quaternion.Identity, helperPos));
				}
				else
					break;
			}
		}

		public void Purge()
		{
			GamePool.FlagForPurge(Entity.Id);
			GamePool.FlagForPurge(_tunnelDoor.Entity.Id);
		}
	}

	public class Door : DestroyableBase
	{
		private const int HitDamage = 10;

		/// <summary>
		/// The door types.
		/// </summary>
		private static readonly ObjectSkin[] DoorTypes = new ObjectSkin[]
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
		public static Door Create(Vector3 pos, int doorType)
		{
			if (doorType < 0 && doorType >= DoorTypes.Length)
				throw new ArgumentOutOfRangeException (
					string.Format ("doorType must betwenn 0 and {0}", (DoorTypes.Length - 1).ToString()));

			var door = Entity.SpawnWithComponent<Door> (pos, Quaternion.Identity, 1);
            
            var material = DoorTypes[doorType].Material;
            if (material != null)
            {
				door.Entity.LoadMaterial(material);
            }

            var geometry = DoorTypes[doorType].Geometry;
            if (geometry != null)
            {
				door.Entity.LoadGeometry(0, geometry);
            }

			door.Entity.Physics.Physicalize(0, 1, EPhysicalizationType.ePT_Rigid);

            GamePool.AddObjectToPool(door);
			return door;
		}
	}
}
