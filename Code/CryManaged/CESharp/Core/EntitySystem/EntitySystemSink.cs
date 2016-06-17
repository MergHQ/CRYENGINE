// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	internal sealed class EntitySystemSink : IEntitySystemSink
	{
		#region Events
		/// <summary>
		/// Triggered before a native CRYENGINE entity is being spawned.
		/// </summary>
		public event EventHandler<SEntitySpawnParams> BeforeSpawn;
		/// <summary>
		/// Triggered when a native CRYENGINE entity is being reused.
		/// </summary>
		public event EventHandler<IEntity, SEntitySpawnParams> Reused;
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
		/// This constructor will automatically regiter the EntitySystemSink with CRYENGINE's entity system.
		/// </summary>
		internal EntitySystemSink()
		{
			Global.gEnv.pEntitySystem.AddSink (this, (uint)IEntitySystem.SinkEventSubscriptions.AllSinkEvents, 0);
		}
		#endregion

		#region Methods
		/// <summary>
		/// Upon dispose, the EntitySystemSink will automatically be unregistered from CRYENGINE's entity system.
		/// </summary>
		public override void Dispose ()
		{
			Global.gEnv.pEntitySystem.RemoveSink (this);
			base.Dispose ();
		}

		/// <summary>
		/// Translate 'OnBeforeSpawn' callback to managed 'BeforeSpawn' event.
		/// </summary>
		public override bool OnBeforeSpawn (SEntitySpawnParams arg0)
		{
			if (BeforeSpawn != null)
				BeforeSpawn (arg0);
			
			return true;
		}

		/// <summary>
		/// Translate 'OnReused' callback to managed 'Reused' event.
		/// </summary>
		public override void OnReused (IEntity pEntity, SEntitySpawnParams arg1)
		{
			if (Reused != null)
				Reused (pEntity, arg1);
		}

		/// <summary>
		/// Translate 'OnEvent' callback to managed 'Event' event.
		/// </summary>
		public override void OnEvent (IEntity pEntity, SEntityEvent arg1)
		{
			if (Event != null)
				Event (pEntity, arg1);
		}

		/// <summary>
		/// Translate 'OnSpawn' callback to managed 'Spawn' event.
		/// </summary>
		public override void OnSpawn (IEntity pEntity, SEntitySpawnParams arg1)
		{
			if (Spawn != null)
				Spawn (pEntity, arg1);
		}

		/// <summary>
		/// Translate 'OnRemove' callback to managed 'Remove' event.
		/// </summary>
		public override bool OnRemove (IEntity pEntity)
		{
			if (Remove != null)
				Remove (pEntity);
			return true;
		}
		#endregion
	}
}

