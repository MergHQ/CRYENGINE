using CryEngine.Common;
using System;
using System.Collections.Generic;
using System.Linq;

namespace CryEngine.EntitySystem
{
	/// <summary>
	/// Provides callbacks for when an event occurs with a specified entity.
	/// </summary>
	public class EntityEventListener : IEntityEventListener
	{
		Entity owningEntity;
		List<EntityEventCallback> callbacks;

		public EntityEventListener(Entity owningEntity)
		{
			this.owningEntity = owningEntity;
			callbacks = new List<EntityEventCallback>();

			Engine.EndReload += AddCallbacks;
		}

		public EntityEventCallback AddCallback(EntityEventCallback callback)
		{
			Global.gEnv.pEntitySystem.AddEntityEventListener(owningEntity.Id, callback.Trigger, this);
			callbacks.Add(callback);
			return callback;
		}

		public void RemoveCallback(EntityEventCallback listener)
		{
			if (callbacks.Contains(listener))
			{
				Global.gEnv.pEntitySystem.RemoveEntityEventListener(owningEntity.Id, listener.Trigger, this);
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

		void AddCallbacks()
		{
			callbacks.ForEach(x => Global.gEnv.pEntitySystem.AddEntityEventListener(owningEntity.Id, x.Trigger, this));
		}

		public override void Dispose()
		{
			callbacks.ForEach(x => Global.gEnv.pEntitySystem.RemoveEntityEventListener(owningEntity.Id, x.Trigger, this));

			base.Dispose();
		}

		public bool HasCallback(EntityEventCallback callback)
		{
			return callbacks.Contains(callback);
		}
	}
}

