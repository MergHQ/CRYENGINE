// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	public struct CollisionEvent
	{
		public PhysicsObject Source { get; set; }
		public PhysicsObject Target { get; set; }
	}

	/// <summary>
	/// Representation of an object in the physics engine
	/// </summary>
	public class PhysicsObject
	{
		protected Entity _entity;

		internal virtual IPhysicalEntity NativeHandle { get; private set; }

		/// <summary>
		/// Gets the Entity that this PhysicsObject belongs to.
		/// </summary>
		public virtual Entity OwnerEntity
		{
			get
			{
				if (_entity == null)
				{
					var entityHandle = Global.gEnv.pEntitySystem.GetEntityFromPhysics(NativeHandle);
					if (entityHandle != null)
					{
						_entity = new Entity(entityHandle, entityHandle.GetId());
					}
				}

				return _entity;
			}
		}

		internal PhysicsObject() { }

		internal PhysicsObject(IPhysicalEntity handle)
		{
			NativeHandle = handle;
		}

		/// <summary>
		/// Get or set the velocity of this Entity.
		/// </summary>
		public Vector3 Velocity
		{
			get
			{
				var status = new pe_status_dynamics();
				NativeHandle.GetStatus(status);
				return status.v;
			}
			set
			{
				var action = new pe_action_set_velocity();
				action.v = value;
				NativeHandle.Action(action);
			}
		}

		/// <summary>
		/// Adds an impulse to this Entity.
		/// </summary>
		[Obsolete("Impulse is obsolete, use AddImpulse instead.")]
		public Vector3 Impulse
		{
			set
			{
				var action = new pe_action_impulse();
				action.impulse = value;
				NativeHandle.Action(action);
			}
		}

		/// <summary>
		/// Set the velocity of this PhysicsObject.
		/// </summary>
		/// <param name="velocity"></param>
		public void SetVelocity(Vector3 velocity)
		{
			var action = new pe_action_set_velocity();
			action.v = velocity;
			NativeHandle.Action(action);
		}

		/// <summary>
		/// Adds an impulse to this PhysicsObject.
		/// </summary>
		/// <param name="impulse">Direction and length of the impulse.</param>
		public void AddImpulse(Vector3 impulse)
		{
			var action = new pe_action_impulse();
			action.impulse = impulse;
			NativeHandle.Action(action);
		}

		/// <summary>
		/// Adds an impulse to this PhysicsObject. The impulse will be applied to the point in world-space.
		/// </summary>
		/// <param name="impulse">Direction and length of the impulse.</param>
		/// <param name="point">Point of application, in world-space.</param>
		public void AddImpulse(Vector3 impulse, Vector3 point)
		{
			var action = new pe_action_impulse();
			action.impulse = impulse;
			action.point = point;
			NativeHandle.Action(action);
		}


		/// <summary>
		/// Move this PhysicsObject in a direction.
		/// </summary>
		/// <param name="direction"></param>
		public void Move(Vector3 direction)
		{
			var action = new pe_action_move();

			//Jump mode 2 - just adds to current velocity
			action.iJump = 2;
			action.dir = direction;

			NativeHandle.Action(action);
		}

		/// <summary>
		/// Move this PhysicsObject in a direction, but apply the velocity instantly.
		/// </summary>
		/// <param name="direction"></param>
		public void Jump(Vector3 direction)
		{
			var action = new pe_action_move();

			//Jump mode 1 - instant velocity change
			action.iJump = 1;
			action.dir = direction;
			
			NativeHandle.Action(action);
		}

		/// <summary>
		/// Adds and executes a physics action of type T.
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <param name="setParams">Sets the parameters of the action to be performed.</param>
		public void Action<T>(Action<T> setParams) where T : pe_action
		{
			var actionParams = Activator.CreateInstance<T>();
			setParams(actionParams);
			NativeHandle.Action(actionParams);
		}

		public T GetStatus<T>() where T : BasePhysicsStatus, new()
		{
			var status = new T();
			var nativeStatus = status.ToBaseNativeStatus();
			NativeHandle.GetStatus(nativeStatus);
			status.NativeToManaged(nativeStatus);
			return status;
		}
	}

	/// <summary>
	/// Representation of an Entity in the physics engine
	/// </summary>
	public sealed class PhysicsEntity : PhysicsObject
	{
		/// <summary>
		/// Gets the Entity that this PhysicsEntity belongs to.
		/// </summary>
		public override Entity OwnerEntity { get { return _entity; } }

		internal override IPhysicalEntity NativeHandle
		{
			get
			{
				return OwnerEntity.NativeHandle.GetPhysicalEntity();
			}
		}

		internal PhysicsEntity(Entity entity)
		{
			_entity = entity;
		}

		/// <summary>
		/// Physicalize an entity with the specified mass and of the specified type.
		/// </summary>
		/// <param name="mass"></param>
		/// <param name="type"></param>
		public void Physicalize(float mass, PhysicalizationType type)
		{
			if(type == PhysicalizationType.None)
			{
				Log.Error("Can't Physicalize and entity with PhysicalizationType.None!");
				return;
			}

			var physParams = new SEntityPhysicalizeParams();
			physParams.mass = mass;
			physParams.type = (int)type;
			OwnerEntity.NativeHandle.Physicalize(physParams);
		}

		/// <summary>
		/// Physicalize entity by creating based on the specified physical parameters.
		/// </summary>
		/// <param name="mass">Mass of the entity in Kilograms.</param>
		/// <param name="type"></param>
		[Obsolete("Using EPhysicalizizationType is obsolete. Use PhysicalizationType instead.")]
		public void Physicalize(float mass, EPhysicalizationType type)
		{
			Physicalize(mass, -1, type);
		}

		/// <summary>
		/// Physicalize entity by creating based on the specified physical parameters.
		/// </summary>
		/// <param name="mass">Mass of the entity in Kilograms.</param>
		/// <param name="density"></param>
		/// <param name="type"></param>
		[Obsolete("Using EPhysicalizizationType is obsolete. Use PhysicalizationType instead.")]
		public void Physicalize(float mass, int density, EPhysicalizationType type)
		{
			var physParams = new SEntityPhysicalizeParams();
			physParams.mass = mass;
			physParams.density = density;
			physParams.type = (int)type;

			OwnerEntity.NativeHandle.Physicalize(physParams);
		}

		/// <summary>
		/// Physicalize entity by creating based on the specified physical parameters.
		/// </summary>
		/// <param name="phys"></param>
		[Obsolete("Using SEntityPhysicalizeParams is obsolete. Use EntityPhysicalizeParams instead.")]
		public void Physicalize(SEntityPhysicalizeParams phys)
		{
			OwnerEntity.NativeHandle.Physicalize(phys);
		}

		/// <summary>
		/// Physicalize the Entity as a rigid-entity using default values.
		/// </summary>
		public void Physicalize()
		{
			Physicalize(-1, PhysicalizationType.Rigid);
		}

		/// <summary>
		/// Physicalize the entity based on Physicalize parameters.
		/// </summary>
		/// <param name="parameters"></param>
		public void Physicalize(EntityPhysicalizeParams parameters)
		{
			if(parameters == null)
			{
				throw new ArgumentNullException(nameof(parameters));
			}

			//We could do a using here, but this could result in strange behaviour if these parameters are re-used to physicalize another entity.
			var nativeParameters = parameters.ToNativeParams();
			OwnerEntity.NativeHandle.Physicalize(nativeParameters);
		}
	}
}
