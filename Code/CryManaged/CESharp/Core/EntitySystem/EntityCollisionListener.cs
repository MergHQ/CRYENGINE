using CryEngine.Common;
using System;

namespace CryEngine.EntitySystem
{
	/// <summary>
	/// Listens for a Entity collision and invokes the OnCollide callback passing in the CE Sharp entity that this object collided with.
	/// </summary>
	public class EntityCollisionListener : EntityComponent
	{
		public event EventHandler<CollisionEvent> OnCollide;

		EntityEventCallback collisionCallback;

		public class CollisionEvent : EventArgs
		{
			/// <summary>
			/// The Collision listener that sent this event.
			/// </summary>
			public EntityCollisionListener Sender { get; private set; }

			/// <summary>
			/// The native physic collision event.
			/// </summary>
			public EventPhysCollision PhysCollision { get; private set; }

			/// <summary>
			/// The native Entity that invoked the collision.
			/// </summary>
			/// <value>The native entity.</value>
			public IEntity NativeEntityHandle { get; private set; }

			/// <summary>
			/// The CESharp Entity that invoked the collision.
			/// </summary>
			public Entity Entity { get { return NativeEntityHandle != null ? Entity.Get(NativeEntityHandle.GetId()) : null; } }

			public Vector3 Point { get { return PhysCollision.pt; } }
			public Vector3 Normal { get { return PhysCollision.n; } }

			public CollisionEvent(EntityCollisionListener sender, EventPhysCollision physCollision, IEntity entity)
			{
				Sender = sender;
				PhysCollision = physCollision;
				NativeEntityHandle = entity;
			}
		}

		public override void OnInitialize()
		{
			base.OnInitialize();
			RegisterForCollision();
		}

		public override void OnStart()
		{
			RegisterForCollision();
		}

		void RegisterForCollision()
		{
			if (collisionCallback != null && Entity.EventListener.HasCallback(collisionCallback))
				Entity.EventListener.RemoveCallback(collisionCallback);

			collisionCallback = Entity.EventListener.AddCallback(new EntityEventCallback(OnCollision, EEntityEvent.ENTITY_EVENT_COLLISION));
		}

		void OnCollision(SEntityEvent arg)
		{
			if (arg._event != EEntityEvent.ENTITY_EVENT_COLLISION)
			{
				return;
			}

			// As the only attached Entity event is the collision event, get physics of the collision.
			var collision = arg.GetEventPhysCollision();

			// Retrieve both entities which are part of the collision.
			var entOne = Global.gEnv.pEntitySystem.GetEntityFromPhysics(collision.GetFirstEntity());
			var entTwo = Global.gEnv.pEntitySystem.GetEntityFromPhysics(collision.GetSecondEntity());

			IEntity collider = null;
			if (entOne != null && entTwo != null)
			{
				if (entOne.GetId() == Entity.Id)
				{
					collider = entTwo;
				}
				else
				{
					collider = entOne;
				}
			}

			OnCollide?.Invoke(new CollisionEvent(this, collision, collider));
		}

		public override void OnRemove()
		{
			Entity.EventListener.RemoveCallback(collisionCallback);
		}
	}
}
