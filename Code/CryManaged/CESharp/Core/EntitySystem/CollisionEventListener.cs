using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	/// <summary>
	/// Checks collisions between registered entities. Will raise collision event on them if a collision occurs.
	/// </summary>
	internal class CollisionEventListener : IEntityEventListener
	{
		internal static CollisionEventListener Instance { get; set; }

		List<EntityId> _listeners = new List<EntityId>();

		public CollisionEventListener()
		{
			Engine.EndReload += AddListeners;
		}

		public void Subscribe(EntityId id)
		{
			_listeners.Add(id);

			Global.gEnv.pEntitySystem.AddEntityEventListener(id, EEntityEvent.ENTITY_EVENT_COLLISION, this);
		}

		public void Unsubscribe(EntityId id)
		{
			_listeners.Remove(id);

			Global.gEnv.pEntitySystem.RemoveEntityEventListener(id, EEntityEvent.ENTITY_EVENT_COLLISION, CollisionEventListener.Instance);
		}

		void AddListeners()
		{
			foreach (var listenerId in _listeners)
			{
				Global.gEnv.pEntitySystem.AddEntityEventListener(listenerId, EEntityEvent.ENTITY_EVENT_COLLISION, this);
			}
		}

		public override void Dispose()
		{
			foreach (var listenerId in _listeners)
			{
				Global.gEnv.pEntitySystem.RemoveEntityEventListener(listenerId, EEntityEvent.ENTITY_EVENT_COLLISION, CollisionEventListener.Instance);
			}

			base.Dispose();
		}

		/// <summary>
		/// Global event on entity collision. Forwards event to C# Entities.
		/// </summary>
		/// <param name="pEntity">CRYENGINE entity.</param>
		/// <param name="arg1">Collision information.</param>
		public override void OnEntityEvent(IEntity pEntity, SEntityEvent arg1)
		{
			// As the only attached Entity event is the collision event, get physics of the collision.
			EventPhysCollision collision = arg1.GetEventPhysCollision();

			// Retrieve both entities which are part of the collision.
			IEntity entOne = Global.gEnv.pEntitySystem.GetEntityFromPhysics(collision.GetFirstEntity());
			IEntity entTwo = Global.gEnv.pEntitySystem.GetEntityFromPhysics(collision.GetSecondEntity());

			if (entOne == null || entTwo == null)
				return;

			var e1 = Entity.Get(entOne.GetId());
			var e2 = Entity.Get(entTwo.GetId());
			e1.RaiseCollision(e2);
			e2.RaiseCollision(e1);
		}
	}
}
