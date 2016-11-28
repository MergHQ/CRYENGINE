using CryEngine.Common;
using System;

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

        public virtual IPhysicalEntity NativeHandle { get; private set; }

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

        public Vector3 Velocity
        {
            set
            {
                var action = new pe_action_set_velocity();
                action.v = value;
                NativeHandle.Action(action);
            }
        }

        internal PhysicsObject() { }

        internal PhysicsObject(IPhysicalEntity handle)
        {
            NativeHandle = handle;
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
    }

    /// <summary>
    /// Representation of an Entity in the physics engine
    /// </summary>
	public sealed class PhysicsEntity : PhysicsObject
    {
		public override Entity OwnerEntity { get { return _entity; } }

        public override IPhysicalEntity NativeHandle
        {
            get
            {
                return OwnerEntity.NativeHandle.GetPhysicalEntity();
            }
        }

        public PhysicsEntity(Entity entity)
		{
            _entity = entity;
		}

		public void Physicalize(float mass, EPhysicalizationType type)
		{
			Physicalize(mass, -1, type);
		}

		public void Physicalize(float mass, int density, EPhysicalizationType type)
		{
			var physParams = new SEntityPhysicalizeParams();
			physParams.mass = mass;
			physParams.density = density;
			physParams.type = (int)type;

			Physicalize(physParams);
		}

		public void Physicalize(SEntityPhysicalizeParams phys)
		{
			OwnerEntity.NativeHandle.Physicalize(phys);
		}

		/// <summary>
		/// Physicalize the Entity using default values.
		/// </summary>
		public void Physicalize()
		{
			Physicalize(-1, -1, EPhysicalizationType.ePT_Rigid);
		}
	}
}
