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

	public abstract class DestroyableBase : Entity
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
		protected string ExplosionTrigger;

		public Int32 Life { get; private set;}
		public Int32 MaxLife { get; private set; }
		public bool IsAlive { get{ return Life > 0; } }

		public Vec3 Speed 
		{ 
			get { return _speed; } 
			set { _speed = value ?? Vec3.Zero; }
		}

		public DestroyableBase()
		{
			Life = MaxLife = 100;
			DestroyParticleEffect = Env.ParticleManager.FindEffect("spaceship.Destruction.explosion_blue");
			ExplosionTrigger = string.Format ("enemy_explosion_0{0}", 1 + Rand.NextInt(3));

			SubscribeToCollision();
			OnEntityCollision += Collision;
		}

		private void Collision(Entity hitEnt)
		{
			var hitObj = GamePool.GetMoveableByEntityId(hitEnt.ID);

			// Check if one of the objects is already dead.
			if (IsAlive && hitObj != null)
				OnCollision (hitObj);
		}

		protected virtual void OnCollision (DestroyableBase hitEnt)
		{
			// Collision event was not overridden, so we can unsubscribe.
			UnsubscribeFromCollision();
		}

		#region Moving Behaviour
		/// <summary>
		/// Move this instance. May be overriden in inheriting classes.
		/// </summary>
		public virtual Vec3 Move()
		{
			// HAndle null?
			if (!Exists)
				return Vec3.Zero;

			// Empty jetPosition means x, y and z are all 0.
			if (this is ProjectileBase)
			{
				if(!_isVelocitySet)
				{
					if (PhysicalEntity != null) 
					{
						var peASV = new pe_action_set_velocity();
						peASV.v = Speed;
						PhysicalEntity.Action(peASV);
					}
					_isVelocitySet = true;
				}
				return Position;
			}

			Vec3 newPosition = Position + FrameTime.Normalize(Speed); 
			Position = newPosition;
			return newPosition;
		}

		/// <summary>
		/// Used when the game is paused to avoid entitiy movement.
		/// As the game needs to set new positions in pause mode to avoid physics, this will switch between adding and substracting values for each update operation.
		/// </summary>
		public void KeepPosition()
		{
			if (!Exists)
				return;

			_pausePositionSwitch = !_pausePositionSwitch;
			Position = Position + new Vec3(_pausePositionSwitch ? 0.00001f : -0.00001f, 0, 0);
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

		public virtual void Destroy(bool withEffect = true)
		{
			if (withEffect && Exists && DestroyParticleEffect != null)
			{
				DestroyParticleEffect.Spawn (Position);
				AudioManager.PlayTrigger (ExplosionTrigger);
			}
		}
		#endregion
	}
}
