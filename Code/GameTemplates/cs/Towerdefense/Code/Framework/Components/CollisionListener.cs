using System;
using CryEngine.Common;

namespace CryEngine.Framework
{
    /// <summary>
    /// Listens for a Entity collision and invokes the OnCollide callback passing in the GameObject that this object collided with.
    /// </summary>
    public class CollisionListener : GameComponent
    {
        public event EventHandler<CollisionEvent> OnCollide;

        EntityEventCallback collisionCallback;

        public class CollisionEvent : EventArgs
        {
            /// <summary>
            /// The native physic collision event.
            /// </summary>
            public EventPhysCollision PhysCollision { get; private set; }

            /// <summary>
            /// The native entity that invoked the collision.
            /// </summary>
            public IEntity Entity { get; private set; }

            /// <summary>
            /// The GameObject that invoked the collision.
            /// </summary>
            public GameObject GameObject { get { return GameObject.ById(Entity.GetId()); } }

            public CollisionEvent(EventPhysCollision physCollision, IEntity entity)
            {
                PhysCollision = physCollision;
                Entity = entity;
            }
        }

        protected override void OnInitialize()
        {
            base.OnInitialize();
            RegisterForCollision();
        }

        protected override void OnStart()
        {
            RegisterForCollision();
        }

        void RegisterForCollision()
        {
            if (collisionCallback != null && EventListener.HasCallback(collisionCallback))
                EventListener.RemoveCallback(collisionCallback);

            collisionCallback = EventListener.AddCallback(new EntityEventCallback(OnCollision, EEntityEvent.ENTITY_EVENT_COLLISION));
        }

        void OnCollision(SEntityEvent arg)
        {
            if (arg._event != EEntityEvent.ENTITY_EVENT_COLLISION)
            {
                Logger.LogError("Event passed into OnCollision was not a Collision.");
                return;
            }

            // As the only attached Entity event is the collision event, get physics of the collision.
            var collision = arg.GetEventPhysCollision();

            // Retrieve both entities which are part of the collision.
            var entOne = Env.EntitySystem.GetEntityFromPhysics(collision.GetFirstEntity());
            var entTwo = Env.EntitySystem.GetEntityFromPhysics(collision.GetSecondEntity());

            if (entOne == null || entTwo == null)
                return;

            IEntity collider;
            if (entOne.GetId() == GameObject.Id)
            {
                collider = entTwo;
            }
            else
            {
                collider = entOne;
            }

            OnCollide?.Invoke(new CollisionEvent(collision, collider));
        }

        protected override void OnDestroy()
        {
            GameObject.EventListener.RemoveCallback(collisionCallback);
        }
    }
}
