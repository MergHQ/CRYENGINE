// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.Components;
using CryEngine.Common;
using CryEngine.Sydewinder.UI;
using CryEngine.Sydewinder.Framework;

namespace CryEngine.Sydewinder.Types
{	
	public class Enemy : DestroyableBase
	{
		/// <summary>
		/// The enemy types.
		/// </summary>
		private readonly ObjectSkin[] _enemyTypes = new ObjectSkin[]
		{
			new ObjectSkin("objects/ships/enemy_01.cgf"),
			new ObjectSkin("objects/ships/enemy_02.cgf"),
			new ObjectSkin("objects/ships/enemy_03.cgf")
		};

		private const int HitDamage = 100;
		private float _flightAngleIncrement = 180;

		public WeaponBase Weapon { get; set; }
		public float FlightDirectionAngle { get; set; }

		public Enemy(int type, Vec3 position)
		{
			Entity = EntitySystem.Entity.Instantiate (position, Quat.Identity, 0.5f, _enemyTypes [type].Geometry);

			if (_enemyTypes[type].Material != null)
				Entity.Material = Env.Engine.GetMaterialManager().LoadMaterial(_enemyTypes[type].Material);

			// Rotate Z-Axis in degrees to have enemy facing forward.
			Entity.Rotation = Quat.CreateRotationZ(Utils.Deg2Rad(90f));

			var pfx = Env.ParticleManager.FindEffect("spaceship.Trails.blue_fire_trail");
			Entity.LoadParticleEmitter(1, pfx, 0.5f);
		
			// Get position of jet on enemy (Note: Non-Existing position means x, y and z are all 0).
			Vec3 jetPosition = Entity.GetHelperPos(0, "particle_01");

			// NOTE ON MATRIX34
			// ----------------
			// First Vec3 parameter indicates scale
			// Second Quat parameter indicate rotation
			// Third Vec3 parameter indicates position

			// Scale, Rotate and Position particle effect to ensure it's shown at the back of the ship.
			Entity.SetTM(1, Matrix34.Create(Vec3.One, Quat.CreateRotationX(Utils.Deg2Rad(270f)), jetPosition));	

			// Attach to entity collision event
			EntityCollisionCheck.SubscribeToEntityCollision(Entity.ID);

			// Put into game loop.
			GamePool.AddObjectToPool(this);
		}

		public override Vec3 Move ()
		{
			var entPos = Entity.Position;
			Vec3 screenRelativePosition = Env.Renderer.ProjectToScreen(entPos.x, entPos.y, entPos.z);

			if (screenRelativePosition.x < -10f) 
			{
				Destroy(false);
				return Entity.Position;
			}

			// Flying in a smooth wave form
			Speed = new Vec3 (Speed.x, Speed.y, 2.2f * (float)Math.Sin(Utils.Deg2Rad(FlightDirectionAngle)));
			FlightDirectionAngle += FrameTime.Normalize(_flightAngleIncrement);

			// Spawn projectile in front of Ship to avoid collision with self
			if (Weapon != null) 
				Weapon.Fire(Entity.Position - new Vec3(0,3,0));

			return base.Move ();
		}

		private void OnEntityCollision(IEntity entity, SEntityEvent arg)
		{		
			arg.GetEventPhysCollision ().GetFirstVelocity (); 	
			Collision();
		}

		public override void Collision()
		{
			DrainLife (HitDamage);
			if(Life <= 0)
				Destroy();
			// ToDo: Check what has caused the collision.
			Program.GameApp.MakeScore(100 * Hud.CurrentHud.Stage);
		}

		public override void Destroy(bool withEffect = true)
		{
			DrainLife(MaxLife);
			base.Destroy (withEffect);
			GamePool.FlagForPurge(Entity.ID);
			if(Weapon != null)
				GamePool.FlagForPurge (Weapon.Entity.ID);
		}

		public enum WaveType
		{
			VerticalLine,
			HorizontalLine,
			Diagonal,
		}

		/// <summary>
		/// Spawns a wave of enemies.
		/// </summary>
		/// <param name="position"> Top Left Position.</param>
		/// <param name="enemyType">Enemy type.</param>
		/// <param name="waveType">Wave type.</param>
		/// <param name="speed">Speed.</param>
		public static void SpawnWave(Vec3 position, int enemyType, WaveType waveType, Vec3 speed)
		{
			// Spawn distance should be calculated based on geometry measurement.
			// Not swigged yet!
			// AABB bBox = null;
			// spawnedEntity.GetLocalBounds(bBox);

			switch (waveType ) 
			{
			case WaveType.VerticalLine:
				for (int i = 0; i < 5; i++) 
				{
					var spawnPos = new Vec3 (position.x, position.y, position.z - (4 * i));
					var enemy = new Enemy (enemyType, spawnPos);
					var projectileSpeed = new Vec3(0f, speed.y - 4f, 0f);
					enemy.Speed = speed;
					enemy.Weapon = new LightGun (projectileSpeed);
				}
				break;

			case WaveType.HorizontalLine: 
				for (int i = 0; i < 5; i++) 
				{
					var xVariation = 0.001f * (i+1); // Minimal offset to avoid flickering of Particles which overlapping.
					var spawnPos = new Vec3 (position.x + xVariation, position.y + (4 * i), position.z);
					var enemy = new Enemy (enemyType, spawnPos);
					enemy.Speed = speed;
					enemy.FlightDirectionAngle = -45 * i;
				}
				break;
			case WaveType.Diagonal:				
				for (int i = 0; i < 4; i++) 
				{
					var xVariation = 0.001f * (i+1); // Minimal offset to avoid flickering of Particles which overlapping.
					var spawnPos = new Vec3 (position.x + xVariation, position.y + (4 * i), position.z - (4 * i));
					var enemy = new Enemy (enemyType, spawnPos);
					enemy.Speed = speed;
					enemy.FlightDirectionAngle = -45 * i;
				}
				break;
			default:
				break;
			}
		}
	}
}
