// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using System.Collections.Generic;
using System.Linq;

namespace CryEngine.EntitySystem
{
	public partial class Entity
	{
		#region Fields
		/// <summary>
		/// Event sink responsible to forward native entity system events to the managed EntityFramework.
		/// </summary>
		private static EntitySystemSink s_entitySystemSink;
		/// <summary>
		/// IMonoListener instance, forwarding an Update-event for each frame.
		/// </summary>
		private static EntityUpdateListener s_entityUpdateListener;
		/// <summary>
		/// Registry of created managed entity instances. Key corresponds to the native EntityId
		/// </summary>
		private static Dictionary<EntityId, Entity> s_managedEntities;
		#endregion

		#region Properties
		internal static EntityClassRegistry ClassRegistry { get; set; }
		#endregion

		#region Methods
		/// <summary>
		/// Setup the CESharp EntityFramework, including registering of discovered CESharp entity classes and assumption of already spawned CESharp entites.
		/// </summary>
		internal static void OnEngineStart()
		{
			EntityProperty.RegisterConverters();
			s_managedEntities = new Dictionary<EntityId, Entity>();
			ClassRegistry = new EntityClassRegistry();
			s_entitySystemSink = new EntitySystemSink();
			s_entitySystemSink.Spawn += OnSpawn;
			s_entitySystemSink.Event += OnEvent;
			s_entitySystemSink.Remove += OnRemove;
			s_entityUpdateListener = new EntityUpdateListener();
			s_entityUpdateListener.Update += OnUpdate;

			CollisionEventListener.Instance = new CollisionEventListener();
		}

		/// <summary>
		/// Finds an entity by its entity id
		/// First attempts to find the entity in the managed cache, otherwise queries native code
		/// </summary>
		/// <param name="id">EntityId</param>
		public static Entity Get(EntityId id)
		{
			Entity foundEntity;
			if (s_managedEntities.TryGetValue(id, out foundEntity))
			{
				return foundEntity;
			}

			var nativeEntity = Global.gEnv.pEntitySystem.GetEntity(id);
			if (nativeEntity != null && !nativeEntity.IsGarbage())
			{
				return new NativeEntity(nativeEntity, id);
			}

			return null;
		}

		internal static bool HasManagedInstanceOf(EntityId id)
		{
			Entity foundEntity;
			if (s_managedEntities.TryGetValue(id, out foundEntity))
			{
				return true;
			}

			return false;
		}

		public static T Get<T>(EntityId id) where T : Entity
		{
			return Get(id) as T;
		}

		/// <summary>
		/// Queries an entity by name
		/// First attempts to find the entity in the managed cache, otherwise queries native code
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		public static Entity Find(string name)
		{
			var ent = s_managedEntities.Values.FirstOrDefault(x => x.Exists && x.Name == name);

			if (ent != null)
			{
				return ent;
			}

			var nativeEntity = Global.gEnv.pEntitySystem.FindEntityByName(name);
			if (nativeEntity != null && !nativeEntity.IsGarbage())
			{
				return new NativeEntity(nativeEntity, nativeEntity.GetId());
			}

			return null;
		}

		/// <summary>
		/// Get managed CESharp entity instances for a given Type. Returns an empty list if no entities of the specified Type exist.
		/// </summary>
		/// <returns>The entities.</returns>
		/// <typeparam name="T">The 1st type parameter.</typeparam>
		public static IEnumerable<T> GetByType<T>() where T : Entity
		{
			return s_managedEntities.Values.OfType<T>();
		}

		/// <summary>
		/// Spawn a new instance of a managed entity
		/// </summary>
		/// <param name="name">Name.</param>
		public static T Spawn<T>(Vector3 pos, Quaternion rot, float scale = 1f) where T : Entity, new()
		{
			EntityClass cls = ClassRegistry.GetEntityClass<T>();
			if (cls == null)
			{
				Log.Warning("[EntityFramework] Could not find managed CESharp entity class for {0}", typeof(T).Name);
				return null;
			}

			SEntitySpawnParams sParams = new SEntitySpawnParams();
			sParams.pClass = cls.NativeClass;

			sParams.vPosition = pos;
			sParams.qRotation = rot;
			sParams.vScale = new Vec3(scale);
			IEntity pEnt = Global.gEnv.pEntitySystem.SpawnEntity(sParams);

			Entity spawnedEntity = null;
			if (s_managedEntities.TryGetValue(pEnt.GetId(), out spawnedEntity))
			{
				return spawnedEntity as T;
			}

			return null;
		}

		public static void Remove(EntityId id)
		{
			Global.gEnv.pEntitySystem.RemoveEntity(id);
		}

		/// <summary>
		/// Called when CRYENGINE's entity system is spawning an entity. Will check if the entity class is registered with the managed entity class registry and if yes, it will create a managed CESharp entity instance.
		/// </summary>
		private static void OnSpawn(IEntity pEntity, SEntitySpawnParams spawnParams)
		{
			if (spawnParams == null)
				return;

			EntityClass managedClass = ClassRegistry.GetEntityClass(spawnParams.pClass);
			if (managedClass == null)
				return;

			managedClass.CreateManagedInstance(pEntity);
		}

		/// <summary>
		/// Called when CRYENGINE's entity system issues an entity event. If a managed CESharp entity instance exists the event will be forwarded to it.
		/// </summary>
		private static void OnEvent(IEntity pEntity, SEntityEvent arg1)
		{
			EntityId id = pEntity.GetId();
			if (!s_managedEntities.ContainsKey(id))
				return;

			s_managedEntities[id].OnEvent(arg1);
		}

		/// <summary>
		/// Called when CRYENGINE's entity system is removing an entity. Will remove the managed CESharp entity instance if it exists.
		/// </summary>
		private static void OnRemove(IEntity pEntity)
		{
			EntityId id = pEntity.GetId();

			Entity foundEntity;
			if (!s_managedEntities.TryGetValue(id, out foundEntity))
			{
				return;
			}

			foundEntity.OnRemove();
			s_managedEntities.Remove(id);
		}

		/// <summary>
		/// Called once per frame. Will forward the update to all managed CESharp entity instances - unless the user is currently 'editing'.
		/// </summary>
		private static void OnUpdate(int updateFlags, int pauseMode)
		{
			if (Global.gEnv.IsEditor() && !Global.gEnv.IsEditorGameMode())
				return;
			
			foreach (Entity entity in s_managedEntities.Values.ToList())
				entity.OnUpdate();
		}
		#endregion
	}
}

