// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.EntitySystem;
using System;

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

		public static Enemy Create(int type, Vector3 pos)
		{
			var enemy = Entity.Spawn<Enemy>(pos, Quaternion.Identity, 0.5f);

            enemy.LoadGeometry(0, EnemyTypes[type].Geometry);

            enemy.Physics.Physicalize(0, 1, EPhysicalizationType.ePT_Rigid);

			if (EnemyTypes [type].Material != null) 
			{
				enemy.Material = Engine.Engine3D.GetMaterialManager ().LoadMaterial (EnemyTypes [type].Material);
			}

			// Rotate Z-Axis in degrees to have enemy facing forward.
			enemy.Rotation = Quaternion.CreateRotationZ(MathHelpers.DegreesToRadians(90f));

			var pfx = Engine.ParticleManager.FindEffect("spaceship.Trails.blue_fire_trail");
			enemy.LoadParticleEmitter(1, pfx, 0.5f);
		
			// Get position of jet on enemy (Note: Non-Existing position means x, y and z are all 0).
			Vector3 jetPosition = enemy.GetHelperPos(0, "particle_01");

			// NOTE ON Matrix3x4
			// ----------------
			// First Vector3 parameter indicates scale
			// Second Quaternion parameter indicate rotation
			// Third Vector3 parameter indicates position

			// Scale, Rotate and Position particle effect to ensure it's shown at the back of the ship.
			enemy.SetGeometrySlotLocalTransform(1, new Matrix3x4(Vector3.One, Quaternion.CreateRotationX(MathHelpers.DegreesToRadians(270f)), jetPosition));	

			// Put into game loop.
			GamePool.AddObjectToPool(enemy);
			return enemy;
		}

		protected override void OnCollision(DestroyableBase hitEnt)
		{
			if (hitEnt is Player || (hitEnt is DefaultAmmo && !(hitEnt as DefaultAmmo).IsHostile))
				ProcessHit ();
		}

		public override Vector3 Move ()
		{
			if (Camera.ProjectToScreen(Position).x < -0.1f) 
			{
				Destroy(false);
				return Position;
			}

			// Flying in a smooth wave form
			Speed = new Vector3 (Speed.x, Speed.y, 2.2f * (float)Math.Sin(MathHelpers.DegreesToRadians(FlightDirectionAngle)));
			FlightDirectionAngle += FrameTime.Delta * _flightAngleIncrement;

			// Spawn projectile in front of Ship to avoid collision with self
			if (Weapon != null) 
				Weapon.Fire(Position - new Vector3(0,3,0));

			return base.Move ();
		}

		public void ProcessHit()
		{
			DrainLife (HitDamage);
			if(Life <= 0)
				Destroy();

			SydewinderApp.Instance.MakeScore(100 * Hud.CurrentHud.Stage);
		}

		public override void Destroy(bool withEffect = true)
		{
			DrainLife(MaxLife);
			base.Destroy (withEffect);
			GamePool.FlagForPurge(Id);
			if(Weapon != null)
				GamePool.FlagForPurge (Weapon.Id);
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
		public static void SpawnWave(Vector3 pos, int enemyType, WaveType waveType, Vector3 speed)
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
					var spawnPos = new Vector3 (pos.x, pos.y, pos.z - (4 * i));
					var enemy = Enemy.Create (enemyType, spawnPos);
					var projectileSpeed = new Vector3(0f, speed.y - 4f, 0f);
					enemy.Speed = speed;
					enemy.Weapon = LightGun.Create (projectileSpeed);
				}
				break;

			case WaveType.HorizontalLine: 
				for (int i = 0; i < 5; i++) 
				{
					var xVariation = 0.001f * (i+1); // Minimal offset to avoid flickering of Particles which overlapping.
					var spawnPos = new Vector3 (pos.x + xVariation, pos.y + (4 * i), pos.z);
					var enemy = Enemy.Create (enemyType, spawnPos);
					enemy.Speed = speed;
					enemy.FlightDirectionAngle = -45 * i;
				}
				break;
			case WaveType.Diagonal:				
				for (int i = 0; i < 4; i++) 
				{
					var xVariation = 0.001f * (i+1); // Minimal offset to avoid flickering of Particles which overlapping.
					var spawnPos = new Vector3 (pos.x + xVariation, pos.y + (4 * i), pos.z - (4 * i));
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
