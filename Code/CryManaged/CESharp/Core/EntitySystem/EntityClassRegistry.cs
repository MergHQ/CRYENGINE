// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace CryEngine.EntitySystem
{
	internal sealed class EntityClassRegistry
	{
		#region Fields
		/// <summary>
		/// Internal entity class storage. String key represents the entity class name.
		/// </summary>
		private Dictionary<string, EntityClass> _registry = new Dictionary<string, EntityClass>();
		#endregion

		#region Constructors
		/// <summary>
		/// Only intended to be used by the EntityFramework itself!
		/// </summary>
		internal EntityClassRegistry()
		{
		}
		#endregion

		#region Methods
		/// <summary>
		/// Register the given entity prototype class. 
		/// </summary>
		/// <param name="entityType">Entity class prototype.</param>
		public void TryRegister(Type entityType)
		{
			if (!typeof(Entity).IsAssignableFrom(entityType) || entityType.IsAbstract)
				return;

			EntityClass entry = new EntityClass(entityType);
			if (_registry.ContainsKey(entry.Description.sName))
			{
				Log.Warning<EntityClassRegistry>("Managed entity class with name '{0}' already registered!", entry.Description.sName);
				return;
			}

			var playerAttribute = (PlayerEntityAttribute)entityType.GetCustomAttributes(typeof(PlayerEntityAttribute), true).FirstOrDefault();
			if (playerAttribute != null)
			{
				Global.gEnv.pMonoRuntime.RegisterManagedActor(entry.Description.sName);
			}

			entry.NativeClass = Global.gEnv.pEntitySystem.GetClassRegistry().FindClass(entry.Description.sName);
			if (entry.NativeClass == null)
			{
				Log.Info<EntityClassRegistry>("Registered managed entity class: {0}", entry.ProtoType.Name);
				entry.NativeClass = Global.gEnv.pEntitySystem.GetClassRegistry().RegisterStdClass(entry.Description);
			}

			_registry[entry.Description.sName] = entry;

			// Register all properties
			var propertyHandler = entry.NativeClass.GetPropertyHandler();
			if (propertyHandler != null)
			{
				PropertyInfo[] properties = entityType.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty | BindingFlags.SetProperty);
				foreach (PropertyInfo pi in properties)
				{
					EntityProperty prop = EntityProperty.Create(pi);
					if (prop == null)
						continue;

					// Register the property with the native property handler
					var propertyId = propertyHandler.RegisterProperty(prop.EngineInfo);

					// Add the id to our dictionary
					entry.Properties[propertyId] = prop;
				}
			}
		}

		/// <summary>
		/// Hide all registered entity classes from Sandbox.
		/// </summary>
		public void DisableAll()
		{
			foreach (string className in _registry.Keys.ToList())
			{
				if (String.IsNullOrEmpty(className) || !_registry.ContainsKey(className))
					continue;

				EntityClass managedClass = _registry[className];

				managedClass.Description.flags |= (int)EEntityClassFlags.ECLF_MODIFY_EXISTING | (int)EEntityClassFlags.ECLF_INVISIBLE;
				managedClass.NativeClass = Global.gEnv.pEntitySystem.GetClassRegistry().RegisterStdClass(managedClass.Description);

				//TODO: Disable all entity instance handlers
			}
		}

		/// <summary>
		/// Retrieve the managed entity class for a native CRYENGINE entity.
		/// </summary>
		/// <returns>'Null' if the native entity is not using a managed entity class - otherwise the managed entity class.</returns>
		/// <param name="pEntity">Native CRYENGINE entity.</param>
		public EntityClass GetEntityClass(IEntity pEntity)
		{
			if (pEntity == null || pEntity.IsGarbage())
				return null;

			IEntityClass pClass = pEntity.GetClass();
			if (pClass == null)
				return null;

			string className = pClass.GetName();
			if (!_registry.ContainsKey(className))
				return null;

			return _registry[className];
		}

		/// <summary>
		/// Retrieve the managed entity class for a set of entity spawn parameters.
		/// </summary>
		/// <returns>'Null' if the intended entity class is not a managed entity class - otherwise the managed entity class.</returns>
		/// <param name="spawnParams">Entity spawn paramters.</param>
		public EntityClass GetEntityClass(IEntityClass pEntityClass)
		{
			if (pEntityClass == null)
				return null;

			string className = pEntityClass.GetName();

			EntityClass foundClass = null;
			_registry.TryGetValue(className, out foundClass);

			return foundClass;
		}

		/// <summary>
		/// Retrieve the registered managed entity class for an entity class prototype.
		/// </summary>
		/// <returns>'Null' if no entity class is registerd for the given prototype - otherwise the managed entity class.</returns>
		/// <typeparam name="T">Entity class prototype.</typeparam>
		public EntityClass GetEntityClass<T>() where T : Entity
		{
			return GetEntityClass(typeof(T));
		}

		/// <summary>
		/// Retrieve the registered managed entity class for an entity class prototype.
		/// </summary>
		/// <returns>'Null' if no entity class is registerd for the given prototype - otherwise the managed entity class.</returns>
		/// <param name="t">Entity class prototype.</param>
		public EntityClass GetEntityClass(Type t)
		{
			foreach (EntityClass cls in _registry.Values)
				if (cls.ProtoType == t)
					return cls;

			return null;
		}
		#endregion
	}
}

