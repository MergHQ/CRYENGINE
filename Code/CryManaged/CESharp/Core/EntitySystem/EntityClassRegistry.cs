// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.Serialization;
using CryEngine.Attributes;
using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	public sealed class EntityClass
	{
		#region Fields
		/// <summary>
		/// Native entity class description structure, storing general information (such as name), as well as editor relevant metadata (e.g. EditType).
		/// </summary>
		private IEntityClassRegistry.SEntityClassDesc _description;
		/// <summary>
		/// Managed prototype class, that will be instanced, when the entity is being spawned.
		/// </summary>
		private Type _protoType;
		/// <summary>
		/// Native entity class, created as an in-engine handle for the managed entity class.
		/// </summary>
		private IEntityClass _nativeClass;
		/// <summary>
		/// Managed property handler, responsible for handling of loaded entities and property changes through Sandbox.
		/// </summary>
		private EntityPropertyHandler _propertyHandler;
		#endregion

		#region Properties
		/// <summary>
		/// Native entity class description structure, storing general information (such as name), as well as editor relevant metadata (e.g. EditType).
		/// </summary>
		public IEntityClassRegistry.SEntityClassDesc Description { get { return _description; } set{ _description = value; } }
		/// <summary>
		/// Managed prototype class, that will be instanced, when the entity is being spawned.
		/// </summary>
		public Type ProtoType { get { return _protoType; } set{ _protoType = value; } }
		/// <summary>
		/// Native entity class, created as an in-engine handle for the managed entity class.
		/// </summary>
		public IEntityClass NativeClass { get { return _nativeClass; } set{ _nativeClass = value; } }
		/// <summary>
		/// Managed property handler, responsible for handling of loaded entities and property changes through Sandbox.
		/// </summary>
		internal EntityPropertyHandler PropertyHandler { get { return _propertyHandler; } set { _propertyHandler = value; } }
		#endregion

		#region Constructors
		/// <summary>
		/// Only intended to be used by the EntityFramework itself! This consutrctor will setup the property handler, as well as the sadnbox-relevant metadata description for the engine.
		/// </summary>
		/// <param name="t">Prototype the entity class should be created for.</param>
		internal EntityClass(Type t)
		{
			_nativeClass = null;
			_protoType = t;
			_description = new IEntityClassRegistry.SEntityClassDesc ();
			_description.editorClassInfo = new SEditorClassInfo ();
			_description.sScriptFile = "";

			_propertyHandler = EntityPropertyHandler.CreateHandler (this);
			if (_propertyHandler != null)
				_description.pPropertyHandler = _propertyHandler;
			
			EntityClassAttribute attrib = (EntityClassAttribute)t.GetCustomAttributes (typeof(EntityClassAttribute), true).FirstOrDefault ();
			if (attrib == null) {
				_description.sName = t.Name;
				_description.editorClassInfo.sCategory = "Game";
			} else {
				_description.sName = attrib.Name;
				_description.editorClassInfo.sCategory = attrib.EditorPath;
				_description.editorClassInfo.sHelper = attrib.Helper;
				_description.editorClassInfo.sIcon = attrib.Icon;

				if (attrib.Hide)
					_description.flags |= (int)EEntityClassFlags.ECLF_INVISIBLE;
			}
		}
		#endregion

		#region Methods
		/// <summary>
		/// Only intended to be used by the EntityFramework itself! Spawn a managed entity instance of this entity class around the provided native entity. For entities that are being loaded from XML files an XmlNodeRef can be provided.
		/// </summary>
		/// <returns>The instance.</returns>
		/// <param name="pEntity">Native CRYENGINE entity.</param>
		/// <param name="entityNode">(Optional) xml node for entities loaded from files (e.g. levels/layers).</param>
		internal BaseEntity CreateInstance(IEntity pEntity, XmlNodeRef entityNode = null)
		{
			object instance = FormatterServices.GetUninitializedObject (_protoType);
			FieldInfo fEntity = _protoType.GetField ("_entity", BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);
			if (fEntity != null)
				fEntity.SetValue (instance, pEntity);
			FieldInfo fEntityClass = _protoType.GetField ("_entityClass", BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);
			if (fEntityClass != null)
				fEntityClass.SetValue (instance, this);
			
			BaseEntity managedInstance = (BaseEntity)instance;
			if (_propertyHandler != null) {
				_propertyHandler.SetDefaults (managedInstance);
				if (entityNode != null)
					_propertyHandler.SetXMLProperties (managedInstance, entityNode);
			}
			managedInstance.Initialize ();
			return managedInstance;
		}
		#endregion
	}
	
	public sealed class EntityClassRegistry
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
		/// Discovers all types implementing 'BaseEntity' and registers them.
		/// </summary>
		public void RegisterAll()
		{
			Type tProto = typeof(BaseEntity);

			// Collect all types inheriting from BaseEntity
			foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies()) {
				foreach (Type t in asm.GetTypes()) {
					if (t == tProto || !tProto.IsAssignableFrom (t))
						continue;

					Register (t);
				}
			}
		}

		/// <summary>
		/// Register the given entity prototype class. 
		/// </summary>
		/// <param name="entityType">Entity class prototype.</param>
		public bool Register(Type entityType)
		{
			if (!typeof(BaseEntity).IsAssignableFrom (entityType))
				return false;

			EntityClass entry = new EntityClass (entityType);
			if (_registry.ContainsKey (entry.Description.sName)) {
				Log.Warning<EntityClassRegistry>("Managed entity class with name '{0}' already registered!", entry.Description.sName);
				return false;
			}

            if (entityType.IsAbstract) {
                Log.Info<EntityClassRegistry>("Managed entity class with name '{0}' is abstract and will not be registered.", entry.Description.sName);
                return false;
            }

			IEntityClass native = Global.gEnv.pEntitySystem.GetClassRegistry ().FindClass (entry.Description.sName);
			if(native != null)
				entry.Description.flags |= (int)EEntityClassFlags.ECLF_MODIFY_EXISTING;

			entry.NativeClass = Global.gEnv.pEntitySystem.GetClassRegistry ().RegisterStdClass (entry.Description);
			_registry [entry.Description.sName] = entry;
			Log.Info<EntityClassRegistry>("Registered managed entity class: {0}", entry.ProtoType.Name);
			return true;
		}

		/// <summary>
		/// Register the given entity prototype class.
		/// </summary>
		/// <typeparam name="T">Entity class prototype.</typeparam>
		public bool Register<T>() where T : BaseEntity
		{
			return Register (typeof(T));
		}

		/// <summary>
		/// Hide the given entity class from Sandbox.
		/// </summary>
		/// <param name="className">Entity class name.</param>
		public bool Disable(string className)
		{
			if (String.IsNullOrEmpty (className) || !_registry.ContainsKey (className))
				return false;

			EntityClass managedClass = _registry [className];

			managedClass.Description.flags |= (int)EEntityClassFlags.ECLF_MODIFY_EXISTING | (int)EEntityClassFlags.ECLF_INVISIBLE;
			managedClass.Description.pPropertyHandler = null;
			managedClass.NativeClass = Global.gEnv.pEntitySystem.GetClassRegistry ().RegisterStdClass (managedClass.Description);

			//TODO: Disable all entity instance handlers

			return true;
		}

		/// <summary>
		/// Hide the given entity class from Sandbox.
		/// </summary>
		/// <param name="entityType">Entity class prototype.</param>
		public bool Disable(Type entityType)
		{
			return Disable (FindClassName (entityType));
		}

		/// <summary>
		/// Hide the given entity class from Sandbox.
		/// </summary>
		/// <typeparam name="T">Entity class prototype.</typeparam>
		public bool Disable<T>() where T : BaseEntity
		{
			return Disable (typeof(T));
		}

		/// <summary>
		/// Hide all registered entity classes from Sandbox.
		/// </summary>
		public void DisableAll()
		{
			foreach (string className in _registry.Keys.ToList())
				Disable (className);
		}

		/// <summary>
		/// Delete the given entity class from the registry and also unregister the native entity class.
		/// </summary>
		/// <param name="className">Entity class name.</param>
		public bool Unregister(string className)
		{
			if (String.IsNullOrEmpty (className))
				return false;
			
			//TODO: Handle all entity instances using that entity class!!!!

			bool result = Global.gEnv.pEntitySystem.GetClassRegistry ().UnregisterEntityClass (_registry [className].NativeClass);
			Log.Info<EntityClassRegistry> ("Unregister entity class: {0} = {1}", className, result);
			_registry.Remove (className);
			return true;
		}

		/// <summary>
		/// Delete the given entity class from the registry and also unregister the native entity class.
		/// </summary>
		/// <param name="entityType">Entity class prototype.</param>
		public bool Unregister(Type entityType)
		{
			return Unregister (FindClassName (entityType));
		}

		/// <summary>
		/// Delete the given entity class from the registry and also unregister the native entity class.
		/// </summary>
		/// <typeparam name="T">Entity class prototype.</typeparam>
		public bool Unregister<T>() where T : BaseEntity
		{
			return Unregister(typeof(T));
		}

		/// <summary>
		/// Delete all registered entity classes from the registry and also unregister the native entity classes.
		/// </summary>
		public void UnregisterAll()
		{
			foreach (string className in _registry.Keys.ToList())
				Unregister (className);
		}

		/// <summary>
		/// Retrieve the registered classname for a given entity class prototype
		/// </summary>
		/// <returns>Class name with which the type is registered.</returns>
		/// <param name="entityType">Entity class prototype.</param>
		public string FindClassName(Type entityType)
		{
			foreach (string className in _registry.Keys)
				if (_registry [className].ProtoType == entityType)
					return className;

			return String.Empty;
		}

		/// <summary>
		/// Retrieve the registered classname for a given entity class prototype
		/// </summary>
		/// <returns>Class name with which the type is registered.</returns>
		/// <typeparam name="T">Entity class prototype.</typeparam>
		public string FindClassName<T>()
		{
			return FindClassName (typeof(T));
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

			IEntityClass pClass = pEntity.GetClass ();
			if (pClass == null)
				return null;

			string className = pClass.GetName ();
			if (!_registry.ContainsKey (className))
				return null;

			return _registry [className];
		}

		/// <summary>
		/// Retrieve the managed entity class for a set of entity spawn parameters.
		/// </summary>
		/// <returns>'Null' if the intended entity class is not a managed entity class - otherwise the managed entity class.</returns>
		/// <param name="spawnParams">Entity spawn paramters.</param>
		public EntityClass GetEntityClass(SEntitySpawnParams spawnParams)
		{
			if (spawnParams == null || spawnParams.pClass == null)
				return null;

			string className = spawnParams.pClass.GetName ();
			if (!_registry.ContainsKey (className))
				return null;

			return _registry [className];
		}

		/// <summary>
		/// Retrieve the registered managed entity class for an entity class prototype.
		/// </summary>
		/// <returns>'Null' if no entity class is registerd for the given prototype - otherwise the managed entity class.</returns>
		/// <typeparam name="T">Entity class prototype.</typeparam>
		public EntityClass GetEntityClass<T>() where T : BaseEntity
		{
			return GetEntityClass (typeof(T));
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

