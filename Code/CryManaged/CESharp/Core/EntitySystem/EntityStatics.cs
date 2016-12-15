// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.EntitySystem;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace CryEngine
{
	public partial class Entity
	{
        #region Fields
        private static FieldInfo _ientitySwigPtrField;
        #endregion
        
		#region Methods
        static Entity()
        {
            // Special case, always get native pointers for SWIG director types
            _ientitySwigPtrField  = typeof(IEntity).GetField("swigCPtr", BindingFlags.NonPublic | BindingFlags.DeclaredOnly | BindingFlags.Instance);
        }

        /// <summary>
        /// Finds an entity by its entity id
        /// First attempts to find the entity in the managed cache, otherwise queries native code
        /// </summary>
        /// <param name="id">EntityId</param>
        public static Entity Get(EntityId id)
		{
			var nativeEntity = Global.gEnv.pEntitySystem.GetEntity(id);
			if (nativeEntity != null && !nativeEntity.IsGarbage())
			{
                return new Entity(nativeEntity, id);
			}

			return null;
		}
        
		/// <summary>
		/// Queries an entity by name
		/// First attempts to find the entity in the managed cache, otherwise queries native code
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		public static Entity Find(string name)
		{
			var nativeEntity = Global.gEnv.pEntitySystem.FindEntityByName(name);
			if (nativeEntity != null && !nativeEntity.IsGarbage())
			{
				return new Entity(nativeEntity, nativeEntity.GetId());
			}

			return null;
		}
        
		/// <summary>
		/// Spawn a new instance of a managed entity
		/// </summary>
		/// <param name="name">Name.</param>
		public static T SpawnWithComponent<T>(Vector3 pos, Quaternion rot, float scale = 1f) where T : EntityComponent, new()
		{
            SEntitySpawnParams spawnParams = new SEntitySpawnParams();

            string className;

            if(EntityComponent._componentClassMap.TryGetValue(typeof(T), out className))
            {
                spawnParams.pClass = Global.gEnv.pEntitySystem.GetClassRegistry().FindClass(className);
            }
            
            if(spawnParams.pClass == null)
            {
                spawnParams.pClass = Global.gEnv.pEntitySystem.GetClassRegistry().GetDefaultClass();
            }

            spawnParams.vPosition = pos;
            spawnParams.qRotation = rot;
            spawnParams.vScale = new Vec3(scale);

			IEntity nativeEntity = Global.gEnv.pEntitySystem.SpawnEntity(spawnParams);
            var entity = new Entity(nativeEntity, nativeEntity.GetId());

            if (entity != null)
            {
                return entity.GetOrCreateComponent<T>();
            }

            return null;
		}

		public static void Remove(EntityId id)
		{
			Global.gEnv.pEntitySystem.RemoveEntity(id);
		}
		#endregion
	}
}

