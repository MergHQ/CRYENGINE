using CryEngine.Common;
using System;

namespace CryEngine.EntitySystem
{
	/// <summary>
	/// Wrapper class for a Native Entity's physics methods.
	/// </summary>
	public class EntityPhysics
	{
		/// <summary>
		/// Returns the Native Entity's physical entity.
		/// </summary>
		public IPhysicalEntity NativeHandle { get { return Owner.NativeHandle.GetPhysics(); } }
		public Entity Owner { get; private set; }

		public Vector3 Velocity
		{
			set
			{
				var action = new pe_action_set_velocity();
				action.v = value;
				NativeHandle.Action(action);
			}
		}

		public EntityPhysics(Entity entity)
		{
			Owner = entity;
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
			Owner.NativeHandle.Physicalize(phys);
		}

		/// <summary>
		/// Physicalize the Entity using default values.
		/// </summary>
		public void Physicalize()
		{
			Physicalize(-1, -1, EPhysicalizationType.ePT_Rigid);
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
}
