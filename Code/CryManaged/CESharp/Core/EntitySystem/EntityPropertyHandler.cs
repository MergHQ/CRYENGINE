// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using CryEngine.Common;
using CryEngine.Attributes;

namespace CryEngine.EntitySystem
{	
	internal sealed class EntityProperty
	{
		#region Fields
		/// <summary>
		/// Registered property converters. Key corresponds to the managed property type.
		/// </summary>
		private static Dictionary<Type,IEntityPropertyConverter> s_converters = new Dictionary<Type, IEntityPropertyConverter>();
		/// <summary>
		/// Managed property information.
		/// </summary>
		private PropertyInfo _managedInfo;
		/// <summary>
		/// Native CRYENGINE entity properties, storing information like minimum and maximum values, as well as the Sandbox EditType.
		/// </summary>
		private IEntityPropertyHandler.SPropertyInfo _engineProperty;
		/// <summary>
		/// Default value for the property in string representation.
		/// </summary>
		private string _default;
		#endregion

		#region Properties
		/// <summary>
		/// Managed property information.
		/// </summary>
		internal PropertyInfo ManagedInfo { get { return _managedInfo; } }
		/// <summary>
		/// Native CRYENGINE entity properties, storing information like minimum and maximum values, as well as the Sandbox EditType.
		/// </summary>
		internal IEntityPropertyHandler.SPropertyInfo EngineInfo { get { return _engineProperty; } }
		/// <summary>
		/// Default value for the property in string representation.
		/// </summary>
		internal string Default { get { return _default; } }
		#endregion

		#region Constructors
		/// <summary>
		/// Only intended to be used by the EntityFramework itself!
		/// </summary>
		/// <param name="managed">Reflected property information.</param>
		/// <param name="engine">Native CRYENGINE property infromation.</param>
		/// <param name="defaultValue">Default value for the property.</param>
		private EntityProperty(PropertyInfo managed, IEntityPropertyHandler.SPropertyInfo engine, string defaultValue)
		{
			_managedInfo = managed;
			_engineProperty = engine;
			_default = defaultValue;
		}
		#endregion

		#region Methods
		/// <summary>
		/// Only intended to be used by the EntityFramework itself! Creates the native property information for a reflected managed property and returns the created managed EntityProperty. Returns 'null', if the property is not marked as 'EntityProperty', or no property converter can be found for the type of the property.
		/// </summary>
		/// <param name="managed">Managed.</param>
		internal static EntityProperty Create(PropertyInfo managed)
		{
			EntityPropertyAttribute attrib = (EntityPropertyAttribute)managed.GetCustomAttributes (typeof(EntityPropertyAttribute), true).FirstOrDefault ();
			if (attrib == null)
				return null;
			
			if (!s_converters.ContainsKey (managed.PropertyType))
				return null;

			EEditTypes editType = attrib.EditType;
			if (editType == EEditTypes.Default)
				editType = s_converters [managed.PropertyType].DefaultEditType;
			string strEditType = StringValueAttribute.GetStringValue (editType);
			
			IEntityPropertyHandler.SPropertyInfo engine = new IEntityPropertyHandler.SPropertyInfo ();
			engine.name = managed.Name;
			engine.description = attrib.Description;
			engine.editType = strEditType;
			engine.limits = new IEntityPropertyHandler.SPropertyInfo.SLimits ();
			engine.limits.min = attrib.Min;
			engine.limits.max = attrib.Max;
			engine.type = s_converters [managed.PropertyType].EngineType;

			return new EntityProperty (managed, engine, attrib.Default);
		}

		/// <summary>
		/// Get the current property value from the given managed entity. Used by the engine to read the value from the entity.
		/// </summary>
		/// <param name="entity">Managed entity instance.</param>
		internal string Get(BaseEntity entity)
		{
			return s_converters [_managedInfo.PropertyType].GetValue (_managedInfo.GetValue (entity, null));
		}

		/// <summary>
		/// Set the given values to the given managed entity. Used by the engine to set the value for the entity.
		/// </summary>
		/// <param name="entity">Managed entity instance.</param>
		/// <param name="value">Value in string representation.</param>
		internal void Set(BaseEntity entity, string value)
		{
			_managedInfo.SetValue (entity, s_converters [_managedInfo.PropertyType].SetValue (value), null);
		}

		/// <summary>
		/// Discovers all classes implementing 'IEntityPropertyConverter' and registers an instance of them.
		/// </summary>
		internal static void RegisterConverters()
		{
			Type tProto = typeof(IEntityPropertyConverter);

			// Collect all types implementing IEntityPropertyConverter
			foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies()) {
				foreach (Type t in asm.GetTypes()) {
					if (!t.IsAbstract && !t.IsInterface && t.GetInterfaces ().Contains (tProto)) {
						IEntityPropertyConverter instance = (IEntityPropertyConverter)Activator.CreateInstance (t);
						s_converters.Add (instance.PropertyType, instance);
					}
				}
			}

			Type[] keys = s_converters.Keys.ToArray ();
			StringBuilder sbTypes = new StringBuilder ();
			for (int i = 0; i < keys.Length; i++) {
				sbTypes.Append (keys [i].Name);

				if (i < keys.Length - 1)
					sbTypes.Append (", ");
			}
			Log.Info ("[EntityFramework] Available property converters: {0}", sbTypes.ToString ());
		}
		#endregion
	}

	internal sealed class EntityPropertyHandler : IMonoEntityPropertyHandler
	{
		#region Fields
		/// <summary>
		/// Reference to the managed entity class this PropertyHandler belongs to.
		/// </summary>
		private EntityClass _parent;
		/// <summary>
		/// Storage of available properties, handled by this PropertyHandler.
		/// </summary>
		private List<EntityProperty> _properties;
		#endregion

		#region Constructors
		/// <summary>
		/// Only intended to be used by the EntityFramework itself!
		/// </summary>
		/// <param name="parent">Parent.</param>
		private EntityPropertyHandler (EntityClass parent)
		{
			_parent = parent;
		}
		#endregion

		#region Methods
		/// <summary>
		/// Attempt to create a property handler for the given managed entity class.
		/// </summary>
		/// <returns>'Null' if the given entity class prototype doesn't expose any properties, that can be handled. Otherwise an instance of the PropertyHandler for the given managed entity class.</returns>
		/// <param name="parent">Parent.</param>
		internal static EntityPropertyHandler CreateHandler(EntityClass parent)
		{
			EntityPropertyHandler instance = new EntityPropertyHandler (parent);
			if (!instance.Init ()) {
				instance.Dispose ();
				instance = null;
			}
			return instance;
		}

		/// <summary>
		/// Attempt to register all properties with the PropertyHandler. Will return 'false' if the parent's entity class prototype doesn't expose any properties that can be handled.
		/// </summary>
		private bool Init()
		{
			_properties = new List<EntityProperty> ();
			PropertyInfo[] properties = _parent.ProtoType.GetProperties (BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty | BindingFlags.SetProperty);
			foreach(PropertyInfo pi in properties)
			{
				EntityProperty prop = EntityProperty.Create (pi);
				if (prop == null)
					continue;

				_properties.Add (prop);
			}
			return _properties.Count > 0;
		}

		/// <summary>
		/// Set the properties of the given managed entity instance to the default value.
		/// </summary>
		/// <param name="entity">Managed entity instance.</param>
		public void SetDefaults(BaseEntity entity)
		{
			foreach (EntityProperty prop in _properties)
				prop.Set (entity, prop.Default);
		}

		public override void Dispose ()
		{
			base.Dispose ();
		}

		/// <summary>
		/// Return the default property value for the property with the given index
		/// </summary>
		/// <returns>'Null' if the index doesn't correspond to a valid index - otherwise the default value in string representation.</returns>
		/// <param name="index">Property index.</param>
		public override string GetDefaultProperty (int index)
		{
			if (index >= _properties.Count)
				return null;

			return _properties [index].Default;
		}

		/// <summary>
		/// Not implemented!
		/// </summary>
		/// <param name="pSizer">CRYENGINE memory sizer.</param>
		public override void GetMemoryUsage (ICrySizer pSizer)
		{
		}

		/// <summary>
		/// Return the property info for the proeprty with the given index.
		/// </summary>
		/// <returns>'Null' if the index doesn't correspond to a valid index - otherwise the native property information.</returns>
		/// <param name="index">Property index.</param>
		public override SPropertyInfo GetMonoPropertyInfo (int index)
		{
			if (index >= _properties.Count)
				return null;
			
			return _properties [index].EngineInfo;
		}

		/// <summary>
		/// Get the current property value for the given native entity at the given property index.
		/// </summary>
		/// <returns>'Null' if the index doesn't correspond to a valid index - otherwise the current property value in string representation.</returns>
		/// <param name="entity">Native entity instance.</param>
		/// <param name="index">Property index.</param>
		public override string GetProperty (IEntity entity, int index)
		{
			if (index >= _properties.Count)
				return null;
			
			uint id = entity.GetId ();
			BaseEntity ent = EntityFramework.GetEntity (id);
			if (ent == null)
				return null;
			
			return _properties [index].Get (ent);
		}

		/// <summary>
		/// Return the number of properties managed by the PropertyHandler.
		/// </summary>
		public override int GetPropertyCount ()
		{
			return _properties.Count;
		}

		/// <summary>
		/// Not implemented!
		/// </summary>
		public override uint GetScriptFlags ()
		{
			return 0;
		}

		/// <summary>
		/// Not implemented!
		/// </summary>
		public override void InitArchetypeEntity (IEntity entity, string archetypeName, SEntitySpawnParams spawnParams)
		{
			// TODO: Implement?
		}

		/// <summary>
		/// Not implemented!
		/// </summary>
		public override void LoadArchetypeXMLProperties (string archetypeName, XmlNodeRef xml)
		{
			// TODO: Implement?
		}

		/// <summary>
		/// Not implemented!
		/// </summary>
		public override void LoadEntityXMLProperties (IEntity entity, XmlNodeRef xml)
		{
			// TODO: Implement?
		}

		/// <summary>
		/// Triggered when properties have been changed inside Sandbox. Will notify the managed entity instance (if it exists).
		/// </summary>
		/// <param name="entity">Entity.</param>
		public override void PropertiesChanged (IEntity entity)
		{
			uint id = entity.GetId ();
			BaseEntity ent = EntityFramework.GetEntity (id);
			if (ent != null)
				ent.OnPropertiesChanged ();
		}

		/// <summary>
		/// Not implemented!
		/// </summary>
		public override void RefreshProperties ()
		{
			// TODO: Implement?
		}

		/// <summary>
		/// Set the property of the given native entity at the given property index to the given value in string representation. Won't do anything if the index is invalid, no managed entity instance exists for the native entity or the value cannot be converted.
		/// </summary>
		/// <param name="entity">Native entity instance.</param>
		/// <param name="index">Property index.</param>
		/// <param name="value">Value in string representation.</param>
		public override void SetProperty (IEntity entity, int index, string value)
		{
			if (index >= _properties.Count)
				return;

			uint id = entity.GetId ();
			BaseEntity ent = EntityFramework.GetEntity (id);
			if (ent == null)
				return;
			
			_properties [index].Set (ent, value);
		}

		/// <summary>
		/// Retrieve entity property values from the provided entity cache entry. Used when hot-reloading entities.
		/// </summary>
		/// <param name="ent">Ent.</param>
		/// <param name="cacheEntry">Cache entry.</param>
		public void Retrieve(BaseEntity ent, Dictionary<string, string> cacheEntry)
		{
			foreach (KeyValuePair<string, string> kvp in cacheEntry) {
				foreach (EntityProperty prop in _properties) {
					if (!prop.EngineInfo.name.Equals (kvp.Key, StringComparison.InvariantCultureIgnoreCase))
						continue;

					prop.Set (ent, kvp.Value);
				}
			}
		}

		/// <summary>
		/// Write the current property values to an entity cache entry and return it. Used when hot-reloading entities.
		/// </summary>
		public Dictionary<string, string> Store(BaseEntity ent)
		{
			Dictionary<string, string> cacheEntry = new Dictionary<string, string> ();

			foreach (EntityProperty prop in _properties) {
				string key = prop.EngineInfo.name;
				string value = prop.Get (ent);
				cacheEntry.Add (key, value);
			}
			return cacheEntry;
		}

		/// <summary>
		/// Read property values from the given xml node and apply them to the given managed entity instance. Used when handling entities loaded from xml files (e.g. levels/layers).
		/// </summary>
		/// <param name="managedEntity">Managed entity instance.</param>
		/// <param name="xml">Entity xml node (only the 'Properties' child-node is relevant).</param>
		public void SetXMLProperties(BaseEntity managedEntity, XmlNodeRef xml)
		{
			int nChildren = xml.getChildCount ();
			for (int i = 0; i < nChildren; i++) {
				XmlNodeRef child = xml.getChild (i);
				if (String.Equals (child.getTag (), "Properties", StringComparison.InvariantCultureIgnoreCase)) {
					foreach (EntityProperty prop in _properties) {
						if (!child.haveAttr (prop.EngineInfo.name))
							continue;

						prop.Set (managedEntity, child.getAttr (prop.EngineInfo.name));
					}
				}
			}
		}
		#endregion
	}
}

