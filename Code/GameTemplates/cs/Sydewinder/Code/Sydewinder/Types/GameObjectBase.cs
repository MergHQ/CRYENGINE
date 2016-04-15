// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;
using CryEngine.Sydewinder.UI;
using System.Collections.Generic;
using CryEngine.Sydewinder.Types;
using CryEngine.Components;
using CryEngine.EntitySystem;

namespace CryEngine.Sydewinder
{
	/// <summary>
	/// Object skin. Storing path to Object geometry and material.
	/// </summary>
	public struct ObjectSkin
	{
		public string Geometry;
		public string Material;

		public ObjectSkin(string geometryPath)
		{
			Geometry = geometryPath;
			Material = null;
		}

		public ObjectSkin(string geometryPath, string materialPath)
		{
			Geometry = geometryPath;
			Material = materialPath;
		}
	}

	public abstract class DestroyableBase
	{
		/// <summary>
		/// The is velocity is already submit to the physics system.
		/// Object which are require to fligh through a straight line can be moved by using the physics system.
		/// Velocity has to be set just once.
		/// </summary>
		private bool _isVelocitySet = false;

		private Vec3 _speed = Vec3.Zero;
		private bool _pausePositionSwitch = false;

		protected IParticleEffect DestroyParticleEffect;
		protected MultiSoundPlayer ExplosionSound;

		public Entity Entity { get; protected set; }
		public Int32 Life { get; private set;}
		public Int32 MaxLife { get; private set; }
		public bool IsAlive { get{ return Life > 0; } }

		public Vec3 Position 
		{ 
			get { return Entity.Position; } 
			set { Entity.Position = value; }
		}

		public Vec3 Speed 
		{ 
			get { return _speed; } 
			set { _speed = value ?? Vec3.Zero; }
		}

		public DestroyableBase(int maxLife = 100, IParticleEffect defaultDestroyEffect = null)
		{
			Life = MaxLife = maxLife;
			if(defaultDestroyEffect == null)
				DestroyParticleEffect = Env.ParticleManager.FindEffect("spaceship.Destruction.explosion_blue");

			var r = new Random ();
			string explosionSoundPath = System.IO.Path.Combine(Application.DataPath,
			string.Format("sounds/enemy_explosion_0{0}.wav", 1 + (int)(r.NextDouble()*3)));
			ExplosionSound = new MultiSoundPlayer(explosionSoundPath);
		}

		#region Moving Behaviour
		/// <summary>
		/// Move this instance. May be overriden in inheriting classes.
		/// </summary>
		public virtual Vec3 Move()
		{
			// HAndle null?
			if (!Entity.Exists)
				return Vec3.Zero;

			// Empty jetPosition means x, y and z are all 0.
			if (this is ProjectileBase)
			{
				if(!_isVelocitySet)
				{
					IPhysicalEntity phyEntity = Entity.PhysicalEntity;
					if (phyEntity != null) 
					{
						var peASV = new pe_action_set_velocity();
						peASV.v = Speed;
						phyEntity.Action(peASV);
					}
					_isVelocitySet = true;
				}
				return Entity.Position;
			}

			Vec3 newPosition = Entity.Position + FrameTime.Normalize(Speed); 
			Entity.Position = newPosition;
			return newPosition;
		}

		/// <summary>
		/// Used when the game is paused to avoid entitiy movement.
		/// As the game needs to set new positions in pause mode to avoid physics, this will switch between adding and substracting values for each update operation.
		/// </summary>
		public void KeepPosition()
		{
			if (!Entity.Exists)
				return;

			_pausePositionSwitch = !_pausePositionSwitch;
			Entity.Position = Entity.Position + new Vec3(_pausePositionSwitch ? 0.00001f : -0.00001f, 0, 0);
		}
		#endregion

		#region Destroyable class methods
		public void GainLife(int life)
		{
			Life = Math.Min(Life + life, MaxLife);
		}

		public void DrainLife(int damage)
		{			
			Life = Math.Max(Life - damage, 0);
		}

		/// <summary>
		/// Collision this instance.
		/// Should be implemented by inheriting class
		/// </summary>
		public abstract void Collision();

		public virtual void Destroy(bool withEffect = true)
		{
			if (withEffect && Entity.Exists && DestroyParticleEffect != null)
			{
				DestroyParticleEffect.Spawn (Entity.Position);
				ExplosionSound.Play();
			}
		}
		#endregion
	}
}
