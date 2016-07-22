using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.EntitySystem;
using CryEngine.Common;

namespace CryEngine.Framework
{
    /// <summary>
    /// Wraps an Action that is invoked when an Entity Event of the specified Trigger occurs.
    /// </summary>
    public class EntityEventCallback
    {
        public Action<SEntityEvent> Callback { get; private set; }
        public EEntityEvent Trigger { get; private set; }

        public EntityEventCallback(Action<SEntityEvent> onCallback, EEntityEvent trigger)
        {
            Callback = onCallback;
            Trigger = trigger;
        }
    }

    /// <summary>
    /// Provides callbacks for when an event occurs with a specified entity.
    /// </summary>
    public class EntityEventListener : IEntityEventListener
    {
        BaseEntity owningEntity;
        List<EntityEventCallback> callbacks;

        public EntityEventListener(BaseEntity owningEntity)
        {
            this.owningEntity = owningEntity;
            callbacks = new List<EntityEventCallback>();
        }

        public EntityEventCallback AddCallback(EntityEventCallback callback)
        {
            Env.EntitySystem.AddEntityEventListener(owningEntity.Id, callback.Trigger, this);
            callbacks.Add(callback);
            return callback;
        }

        public void RemoveCallback(EntityEventCallback listener)
        {
            if (callbacks.Contains(listener))
            {
                Env.EntitySystem.RemoveEntityEventListener(owningEntity.Id, listener.Trigger, this);
                callbacks.Remove(listener);
            }
            else
            {
                Log.Error("[EntityEventListener] Cannot remove callback as the specified callback was never registered.");
            }
        }

        public override void OnEntityEvent(IEntity pEntity, SEntityEvent entityEvent)
        {
            callbacks
                .Where(x => x.Trigger == entityEvent._event)
                .ToList()
                .ForEach(x => x.Callback(entityEvent));
        }

        public void Destroy()
        {
            callbacks.ForEach(x => Env.EntitySystem.RemoveEntityEventListener(owningEntity.Id, x.Trigger, this));
        }

        public bool HasCallback(EntityEventCallback callback)
        {
            return callbacks.Contains(callback);
        }
    }
}

