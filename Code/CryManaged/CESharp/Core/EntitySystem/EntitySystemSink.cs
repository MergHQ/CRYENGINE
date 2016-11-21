// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	internal sealed class EntitySystemSink : IEntitySystemSink
	{
		#region Events
		/// <summary>
		/// Triggered when a native CRYNEIGNE entity receives an entity event.
		/// </summary>
		public event EventHandler<IEntity, SEntityEvent> Event;
		/// <summary>
		/// Triggered after a native CRYENGINE entity was spawned.
		/// </summary>
		public event EventHandler<IEntity, SEntitySpawnParams> Spawn;
		/// <summary>
		/// Triggered before a native CRYENGINE entity is being removed.
		/// </summary>
		public event EventHandler<IEntity> Remove;
		#endregion

		#region Constructors
		/// <summary>
		/// This constructor will automatically register the EntitySystemSink with CRYENGINE's entity system.
		/// </summary>
		internal EntitySystemSink()
		{
			AddListener();

			Engine.EndReload += AddListener;
		}
		#endregion

		#region Methods
		void AddListener()
		{
			var subscribedEvents = (uint)(IEntitySystem.SinkEventSubscriptions.OnEvent | IEntitySystem.SinkEventSubscriptions.OnSpawn | IEntitySystem.SinkEventSubscriptions.OnRemove);

			Global.gEnv.pEntitySystem.AddSink(this, subscribedEvents, 0);
		}

		/// <summary>
		/// Upon dispose, the EntitySystemSink will automatically be unregistered from CRYENGINE's entity system.
		/// </summary>
		public override void Dispose()
		{
			Global.gEnv.pEntitySystem.RemoveSink(this);

			base.Dispose();
		}

		/// <summary>
		/// Translate 'OnEvent' callback to managed 'Event' event.
		/// </summary>
		public override void OnEvent(IEntity pEntity, SEntityEvent arg1)
		{
			if (Event != null)
				Event(pEntity, arg1);
		}

		/// <summary>
		/// Translate 'OnSpawn' callback to managed 'Spawn' event.
		/// </summary>
		public override void OnSpawn(IEntity pEntity, SEntitySpawnParams arg1)
		{
			if (Spawn != null)
				Spawn(pEntity, arg1);
		}

		/// <summary>
		/// Translate 'OnRemove' callback to managed 'Remove' event.
		/// </summary>
		public override bool OnRemove(IEntity pEntity)
		{
			if (Remove != null)
				Remove(pEntity);
			return true;
		}
		#endregion
	}
}

