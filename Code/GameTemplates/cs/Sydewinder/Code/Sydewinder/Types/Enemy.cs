// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.Components;
using CryEngine.Common;
using CryEngine.Sydewinder.UI;
using CryEngine.EntitySystem;

namespace CryEngine.Sydewinder.Types
{	
	public class Enemy : DestroyableBase
	{
		/// <summary>
		/// The enemy types.
		/// </summary>
		private static readonly ObjectSkin[] EnemyTypes = new ObjectSkin[]
		{
			new ObjectSkin("objects/ships/enemy_01.cgf"),
			new ObjectSkin("objects/ships/enemy_02.cgf"),
			new ObjectSkin("objects/ships/enemy_03.cgf")
		};

		private const int HitDamage = 100;
		private float _flightAngleIncrement = 180;

		public WeaponBase Weapon { get; set; }
		public float FlightDirectionAngle { get; set; }

		public static Enemy Create(int type, Vec3 pos)
		{
			var enemy = Entity.Instantiate<Enemy>(pos, Quat.Identity, 0.5f, EnemyTypes [type].Geometry);

			if (EnemyTypes[type].Material != null)
				enemy.Material = Env.Engine.GetMaterialManager().LoadMaterial(EnemyTypes[type].Material);

			// Rotate Z-Axis in degrees to have enemy facing forward.
			enemy.Rotation = Quat.CreateRotationZ(Utils.Deg2Rad(90f));

			var pfx = Env.ParticleManager.FindEffect("spaceship.Trails.blue_fire_trail");
			enemy.LoadParticleEmitter(1, pfx, 0.5f);
		
			// Get position of jet on enemy (Note: Non-Existing position means x, y and z are all 0).
			Vec3 jetPosition = enemy.GetHelperPos(0, "particle_01");

			// NOTE ON MATRIX34
			// ----------------
			// First Vec3 parameter indicates scale
			// Second Quat parameter indicate rotation
			// Third Vec3 parameter indicates position

			// Scale, Rotate and Position particle effect to ensure it's shown at the back of the ship.
			enemy.SetTM(1, Matrix34.Create(Vec3.One, Quat.CreateRotationX(Utils.Deg2Rad(270f)), jetPosition));	

			// Put into game loop.
			GamePool.AddObjectToPool(enemy);
			return enemy;
		}

		protected override void OnCollision(DestroyableBase hitEnt)
		{
			if (hitEnt is Player || (hitEnt is DefaultAmmo && !(hitEnt as DefaultAmmo).IsHostile))
				ProcessHit ();
		}

		public override Vec3 Move ()
		{
			if (Env.Renderer.ProjectToScreen(Position).x < -0.1f) 
			{
				Destroy(false);
				return Position;
			}

			// Flying in a smooth wave form
			Speed = new Vec3 (Speed.x, Speed.y, 2.2f * (float)Math.Sin(Utils.Deg2Rad(FlightDirectionAngle)));
			FlightDirectionAngle += FrameTime.Delta * _flightAngleIncrement;

			// Spawn projectile in front of Ship to avoid collision with self
			if (Weapon != null) 
				Weapon.Fire(Position - new Vec3(0,3,0));

			return base.Move ();
		}

		public void ProcessHit()
		{
			DrainLife (HitDamage);
			if(Life <= 0)
				Destroy();

			Program.GameApp.MakeScore(100 * Hud.CurrentHud.Stage);
		}

		public override void Destroy(bool withEffect = true)
		{
			DrainLife(MaxLife);
			base.Destroy (withEffect);
			GamePool.FlagForPurge(ID);
			if(Weapon != null)
				GamePool.FlagForPurge (Weapon.ID);
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
		public static void SpawnWave(Vec3 pos, int enemyType, WaveType waveType, Vec3 speed)
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
					var spawnPos = new Vec3 (pos.x, pos.y, pos.z - (4 * i));
					var enemy = Enemy.Create (enemyType, spawnPos);
					var projectileSpeed = new Vec3(0f, speed.y - 4f, 0f);
					enemy.Speed = speed;
					enemy.Weapon = LightGun.Create (projectileSpeed);
				}
				break;

			case WaveType.HorizontalLine: 
				for (int i = 0; i < 5; i++) 
				{
					var xVariation = 0.001f * (i+1); // Minimal offset to avoid flickering of Particles which overlapping.
					var spawnPos = new Vec3 (pos.x + xVariation, pos.y + (4 * i), pos.z);
					var enemy = Enemy.Create (enemyType, spawnPos);
					enemy.Speed = speed;
					enemy.FlightDirectionAngle = -45 * i;
				}
				break;
			case WaveType.Diagonal:				
				for (int i = 0; i < 4; i++) 
				{
					var xVariation = 0.001f * (i+1); // Minimal offset to avoid flickering of Particles which overlapping.
					var spawnPos = new Vec3 (pos.x + xVariation, pos.y + (4 * i), pos.z - (4 * i));
					var enemy = Enemy.Create (enemyType, spawnPos);
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
