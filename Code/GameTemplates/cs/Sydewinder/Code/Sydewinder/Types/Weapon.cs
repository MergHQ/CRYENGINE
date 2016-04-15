// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Sydewinder.Types;
using CryEngine.Sydewinder.UI;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Sydewinder.Framework;

namespace CryEngine.Sydewinder
{	
	/// <summary>
	/// Weapon base.
	/// </summary>
	public abstract class WeaponBase : DestroyableBase
	{
		/// <summary>
		/// The firing sound of the weapon.
		/// </summary>
		protected MultiSoundPlayer _firingSound;

		/// <summary>
		/// Gets or sets the cool down time.
		/// </summary>
		/// <value>The cool down time in seconds.</value>
		public float CoolDownTime { get; set; }

		/// <summary>
		/// Fire this instance.
		/// </summary>
		public virtual void Fire(Vec3 position)
		{
			if (_firingSound != null)
				_firingSound.Play();
		}
	}

	/// <summary>
	/// Gun - Specific WeaponBase implementation.
	/// Fire linear moving projectiles. No cool down.
	/// </summary>
	public class Gun : WeaponBase
	{
		private IParticleEffect _shot = Env.ParticleManager.FindEffect("spaceship.Weapon.player_shot");
		private IParticleEffect _shotSmoke = Env.ParticleManager.FindEffect("spaceship.Weapon.player_shot_smoke");

		public Gun(Vec3 firingDirection)
		{
			Speed = firingDirection;

			// Audio taken from Y:\SOUND\Crytek Sound Library\Spfx\Synth Fx\Laser.
			string laserSoundFile = System.IO.Path.Combine(Application.DataPath, "sounds/laser_nrm.wav");
			_firingSound = new MultiSoundPlayer(laserSoundFile);

			// We need an ID. Otherwise we can not add this to pool.
			var _sEntitySpawnParams = new SEntitySpawnParams();
			_sEntitySpawnParams.pClass = Env.EntitySystem.GetClassRegistry().GetDefaultClass();

			var spawnedEntity = Env.EntitySystem.SpawnEntity(_sEntitySpawnParams, true);
			Entity = new EntitySystem.Entity (spawnedEntity);

			// This is currently used to register Gun for receiving update calls (Move) which decreases the cool-downtimer.
			GamePool.AddObjectToPool(this);
		}

		public override void Fire(Vec3 position)//, Vec3 fireDirection)
		{	
			// Do not allow to fire weapon while preparing for next shot.
			if (CoolDownTime > 0)
				return;

			// Creating an instance of the ammo will automatically fire the ammo
			new DefaultAmmo(position, Speed, false, _shot, _shotSmoke);

			base.Fire(position);
		}

		public override Vec3 Move()
		{
			if (CoolDownTime > 0)
				CoolDownTime -= FrameTime.Current;

			return base.Move();
		}

		public override void Collision()
		{			
			GamePool.FlagForPurge(Entity.ID);
		}
	}

	/// <summary>
	/// LightGun - Specific WeaponBase implementation.
	/// Used by Enemy because of its long cool down. Fires linear moving projectiles. No cool down.
	/// </summary>
	public class LightGun : WeaponBase
	{
		private IParticleEffect _shot = Env.ParticleManager.FindEffect("spaceship.Weapon.enemy_shot");
		private IParticleEffect _shotSmoke = Env.ParticleManager.FindEffect("spaceship.Weapon.enemy_shot_smoke");

		public LightGun(Vec3 firingDirection)
		{
			Speed = firingDirection;

			string laserSoundFile = System.IO.Path.Combine(Application.DataPath, "sounds/laser_charged.wav");
			_firingSound = new MultiSoundPlayer(laserSoundFile);

			// Weapon is usded by enemy. Prevent from firing immediatelly after spawn.
			CoolDownTime = 0.3f;

			// We need an ID. Otherwise we can not add this to pool (Because Dictionary chrashing while adding 0).
			var _sEntitySpawnParams = new SEntitySpawnParams();
			_sEntitySpawnParams.pClass = Env.EntitySystem.GetClassRegistry().GetDefaultClass();
			var spawnedEntity = Env.EntitySystem.SpawnEntity(_sEntitySpawnParams, true);
			Entity = new EntitySystem.Entity (spawnedEntity);

			// This is currently used to register Gun for receiving update calls (Move) which decreases the cool-downtimer.
			GamePool.AddObjectToPool(this);
		}

		public override void Fire(Vec3 position)
		{	
			// Do not allow to fire weapon while preparing for next shot.
			if (CoolDownTime > 0)
				return;

			if (_firingSound != null)
				_firingSound.Play();

			// Creating an instance of the ammo will automatically fire the ammo
			new DefaultAmmo(position, Speed, true, _shot, _shotSmoke);

			// Start cool down timer again (if necessary).
			CoolDownTime = 1.5f;

			base.Fire(position);
		}

		public override Vec3 Move()
		{
			if (CoolDownTime > 0)
				CoolDownTime -= FrameTime.Current;

			return base.Move();
		}

		public override void Collision()
		{			
			GamePool.FlagForPurge(Entity.ID);
		}
	}

	/// <summary>
	/// Projectile base.
	/// </summary>
	public abstract class ProjectileBase : DestroyableBase
	{
		protected IParticleEffect WeaponBulletParticleEffect { get; set; }
		protected IParticleEffect WeaponSmokeParticleEffect { get; set; }

		public bool IsHostile { get; private set;}

		/// <summary>
		/// Gets or sets the life time until instance should disapear.
		/// </summary>
		/// <value>The life time in seconds.</value>
		public float LifeTime { get; protected set; }

		public ProjectileBase(bool isHostile, IParticleEffect weaponTrailParticleEffect, IParticleEffect weaponSmokeParticleEffect)
		{
			IsHostile = isHostile;
			WeaponSmokeParticleEffect = weaponSmokeParticleEffect;
			WeaponBulletParticleEffect = weaponTrailParticleEffect;
		}

		protected void SpawnParticles()
		{ 
			if (WeaponBulletParticleEffect != null)
				Entity.LoadParticleEmitter(1, WeaponBulletParticleEffect);

			if (WeaponSmokeParticleEffect != null)
				Entity.LoadParticleEmitter(2, WeaponSmokeParticleEffect);
		}

		public override Vec3 Move()
		{			
			if (LifeTime > 0) 
			{
				LifeTime -= FrameTime.Current;			
			}
			else				
				GamePool.FlagForPurge(Entity.ID); // Let destroy current projectile.

			return base.Move();
		}

		public override void Collision()
		{
			// Always remove projectile on collision.
			GamePool.FlagForPurge(Entity.ID);

			// Kill particle trail (bullet)
			if (WeaponBulletParticleEffect != null)
				Entity.GetParticleEmitter(1).Kill();
		}
	}

	/// <summary>
	/// Default ammo.
	/// </summary>
	public class DefaultAmmo : ProjectileBase
	{
		public DefaultAmmo(Vec3 position, Vec3 speed, bool isHostile, IParticleEffect weaponTrailParticleEffect, IParticleEffect weaponSmokeParticleEffect) 
			: base(isHostile, weaponTrailParticleEffect, weaponSmokeParticleEffect)
		{
			LifeTime = 3f;
			Speed = speed;
			Entity = EntitySystem.Entity.Instantiate (position, Quat.Identity, 0.5f, "objects/default/primitive_sphere.cgf");

			// Causes geometry not to be rendered, as the entity particle effect is used as bullet
			Entity.SetSlotFlag(0, EEntitySlotFlags.ENTITY_SLOT_RENDER_NEAREST);

			// Attach to entity collision event
			EntityCollisionCheck.SubscribeToEntityCollision(Entity.ID);
			
			// Spawn particles on bullet
			base.SpawnParticles();

			GamePool.AddObjectToPool(this);
		}
	}
}
