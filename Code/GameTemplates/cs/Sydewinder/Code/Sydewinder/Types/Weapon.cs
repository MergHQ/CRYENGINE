// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Sydewinder.Types;
using CryEngine.Sydewinder.UI;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.EntitySystem;

namespace CryEngine.Sydewinder
{	
	/// <summary>
	/// Weapon base.
	/// </summary>
	public abstract class WeaponBase : DestroyableBase
	{
		/// <summary>
		/// Gets or sets the cool down time.
		/// </summary>
		/// <value>The cool down time in seconds.</value>
		public float CoolDownTime { get; set; }

		/// <summary>
		/// Fire this instance.
		/// </summary>
		public abstract void Fire(Vec3 position);
	}

	/// <summary>
	/// Gun - Specific WeaponBase implementation.
	/// Fire linear moving projectiles. No cool down.
	/// </summary>
	public class Gun : WeaponBase
	{
		private IParticleEffect _shot = Env.ParticleManager.FindEffect("spaceship.Weapon.player_shot");
		private IParticleEffect _shotSmoke = Env.ParticleManager.FindEffect("spaceship.Weapon.player_shot_smoke");

		public static Gun Create(Vec3 firingDirection)
		{
			// We need an ID. Otherwise we can not add this to pool.
			var gun = Entity.Instantiate<Gun>(Vec3.Zero, Quat.Identity);
			gun.Speed = firingDirection;

			// This is currently used to register Gun for receiving update calls (Move) which decreases the cool-downtimer.
			GamePool.AddObjectToPool(gun);
			return gun;
		}

		public override void Fire(Vec3 position)//, Vec3 fireDirection)
		{	
			// Do not allow to fire weapon while preparing for next shot.
			if (CoolDownTime > 0)
				return;

			// Creating an instance of the ammo will automatically fire the ammo
			DefaultAmmo.Create(position, Speed, false, _shot, _shotSmoke);
			AudioManager.PlayTrigger ("laser_player");
		}

		public override Vec3 Move()
		{
			if (CoolDownTime > 0)
				CoolDownTime -= FrameTime.Delta;

			return base.Move();
		}

		/*protected override void OnCollision(DestroyableBase hitEnt)
		{
			if (!(hitEnt is ProjectileBase))
				GamePool.FlagForPurge(ID);
		}*/
	}

	/// <summary>
	/// LightGun - Specific WeaponBase implementation.
	/// Used by Enemy because of its long cool down. Fires linear moving projectiles. No cool down.
	/// </summary>
	public class LightGun : WeaponBase
	{
		private IParticleEffect _shot = Env.ParticleManager.FindEffect("spaceship.Weapon.enemy_shot");
		private IParticleEffect _shotSmoke = Env.ParticleManager.FindEffect("spaceship.Weapon.enemy_shot_smoke");

		public static LightGun Create(Vec3 firingDirection)
		{
			var lightGun = Entity.Instantiate<LightGun> (Vec3.Zero, Quat.Identity);
			lightGun.Speed = firingDirection;

			// Weapon is usded by enemy. Prevent from firing immediatelly after spawn.
			lightGun.CoolDownTime = 0.3f;

			// This is currently used to register Gun for receiving update calls (Move) which decreases the cool-downtimer.
			GamePool.AddObjectToPool(lightGun);
			return lightGun;
		}

		public override void Fire(Vec3 position)
		{	
			// Do not allow to fire weapon while preparing for next shot.
			if (CoolDownTime > 0)
				return;

			AudioManager.PlayTrigger ("laser_charged");

			// Creating an instance of the ammo will automatically fire the ammo
			DefaultAmmo.Create(position, Speed, true, _shot, _shotSmoke);

			// Start cool down timer again (if necessary).
			CoolDownTime = 1.5f;
		}

		public override Vec3 Move()
		{
			if (CoolDownTime > 0)
				CoolDownTime -= FrameTime.Delta;

			return base.Move();
		}

		/*protected override void OnCollision(DestroyableBase hitEnt)
		{
			if (!(hitEnt is ProjectileBase))
				GamePool.FlagForPurge(ID);
		}*/
	}

	/// <summary>
	/// Projectile base.
	/// </summary>
	public abstract class ProjectileBase : DestroyableBase
	{
		protected IParticleEffect WeaponBulletParticleEffect { get; set; }
		protected IParticleEffect WeaponSmokeParticleEffect { get; set; }

		public bool IsHostile { get; protected set;}

		/// <summary>
		/// Gets or sets the life time until instance should disapear.
		/// </summary>
		/// <value>The life time in seconds.</value>
		public float LifeTime { get; protected set; }

		protected void SpawnParticles()
		{ 
			if (WeaponBulletParticleEffect != null)
				LoadParticleEmitter(1, WeaponBulletParticleEffect);

			if (WeaponSmokeParticleEffect != null)
				LoadParticleEmitter(2, WeaponSmokeParticleEffect);
		}

		public override Vec3 Move()
		{			
			if (LifeTime > 0) 
			{
				LifeTime -= FrameTime.Delta;			
			}
			else				
				GamePool.FlagForPurge(ID); // Let destroy current projectile.

			return base.Move();
		}

		protected override void OnCollision(DestroyableBase hitEnt)
		{
			// Always remove projectile on collision.
			GamePool.FlagForPurge (ID);

			// Kill particle trail (bullet)
			if (WeaponBulletParticleEffect != null)
				GetParticleEmitter (1).Kill ();
		}
	}

	/// <summary>
	/// Default ammo.
	/// </summary>
	public class DefaultAmmo : ProjectileBase
	{
		public static DefaultAmmo Create(Vec3 pos, Vec3 speed, bool isHostile, IParticleEffect weaponTrailParticleEffect, IParticleEffect weaponSmokeParticleEffect) 
		{
			var ammo = Entity.Instantiate<DefaultAmmo> (pos, Quat.Identity, 0.5f, "objects/default/primitive_sphere.cgf");
			ammo.LifeTime = 3f;
			ammo.Speed = speed;
			ammo.IsHostile = isHostile;
			ammo.WeaponSmokeParticleEffect = weaponSmokeParticleEffect;
			ammo.WeaponBulletParticleEffect = weaponTrailParticleEffect;

			// Causes geometry not to be rendered, as the entity particle effect is used as bullet
			ammo.SetSlotFlag(0, EEntitySlotFlags.ENTITY_SLOT_RENDER_NEAREST);
			ammo.SpawnParticles();
			GamePool.AddObjectToPool(ammo);
			return ammo;
		}
	}
}
