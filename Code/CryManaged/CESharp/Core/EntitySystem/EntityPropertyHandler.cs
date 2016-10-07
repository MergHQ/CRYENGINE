// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Attributes;
using CryEngine.Common;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;

namespace CryEngine.EntitySystem
{
	internal sealed class EntityProperty
	{
		#region Fields
		/// <summary>
		/// Registered property converters. Key corresponds to the managed property type.
		/// </summary>
		private static Dictionary<Type, IEntityPropertyConverter> s_converters = new Dictionary<Type, IEntityPropertyConverter>();
		/// <summary>
		/// Managed property information.
		/// </summary>
		private PropertyInfo _managedInfo;
		/// <summary>
		/// Native CRYENGINE entity properties, storing information like minimum and maximum values, as well as the Sandbox EditType.
		/// </summary>
		private IEntityPropertyHandler.SPropertyInfo _engineProperty;
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
		#endregion

		#region Constructors
		/// <summary>
		/// Only intended to be used by the EntityFramework itself!
		/// </summary>
		/// <param name="managed">Reflected property information.</param>
		/// <param name="engine">Native CRYENGINE property infromation.</param>
		/// <param name="defaultValue">Default value for the property.</param>
		private EntityProperty(PropertyInfo managed, IEntityPropertyHandler.SPropertyInfo engine)
		{
			_managedInfo = managed;
			_engineProperty = engine;
		}
		#endregion

		#region Methods
		/// <summary>
		/// Only intended to be used by the EntityFramework itself! Creates the native property information for a reflected managed property and returns the created managed EntityProperty. Returns 'null', if the property is not marked as 'EntityProperty', or no property converter can be found for the type of the property.
		/// </summary>
		/// <param name="managed">Managed.</param>
		internal static EntityProperty Create(PropertyInfo managed)
		{
			EntityPropertyAttribute attrib = (EntityPropertyAttribute)managed.GetCustomAttributes(typeof(EntityPropertyAttribute), true).FirstOrDefault();
			if (attrib == null)
				return null;

			if (!s_converters.ContainsKey(managed.PropertyType))
				return null;

			EEditTypes editType = attrib.EditType;
			if (editType == EEditTypes.Default)
				editType = s_converters[managed.PropertyType].DefaultEditType;
			string strEditType = StringValueAttribute.GetStringValue(editType);

			IEntityPropertyHandler.SPropertyInfo engine = new IEntityPropertyHandler.SPropertyInfo();
			engine.name = managed.Name;
			engine.description = attrib.Description;
			engine.editType = strEditType;
			engine.limits = new IEntityPropertyHandler.SPropertyInfo.SLimits();
			engine.limits.min = attrib.Min;
			engine.limits.max = attrib.Max;
			engine.type = s_converters[managed.PropertyType].EngineType;
			engine.defaultValue = attrib.Default;

			return new EntityProperty(managed, engine);
		}

		/// <summary>
		/// Get the current property value from the given managed entity. Used by the engine to read the value from the entity.
		/// </summary>
		/// <param name="entity">Managed entity instance.</param>
		internal string Get(Entity entity)
		{
			return s_converters[_managedInfo.PropertyType].GetValue(_managedInfo.GetValue(entity, null));
		}

		/// <summary>
		/// Set the given values to the given managed entity. Used by the engine to set the value for the entity.
		/// </summary>
		/// <param name="entity">Managed entity instance.</param>
		/// <param name="value">Value in string representation.</param>
		internal void Set(Entity entity, string value)
		{
			_managedInfo.SetValue(entity, s_converters[_managedInfo.PropertyType].SetValue(value), null);
		}

		/// <summary>
		/// Discovers all classes implementing 'IEntityPropertyConverter' and registers an instance of them.
		/// </summary>
		internal static void RegisterConverters()
		{
			Type tProto = typeof(IEntityPropertyConverter);

			// Collect all types implementing IEntityPropertyConverter
			foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies())
			{
				foreach (Type t in asm.GetTypes())
				{
					if (!t.IsAbstract && !t.IsInterface && t.GetInterfaces().Contains(tProto))
					{
						IEntityPropertyConverter instance = (IEntityPropertyConverter)Activator.CreateInstance(t);
						s_converters.Add(instance.PropertyType, instance);
					}
				}
			}

			Type[] keys = s_converters.Keys.ToArray();
			StringBuilder sbTypes = new StringBuilder();
			for (int i = 0; i < keys.Length; i++)
			{
				sbTypes.Append(keys[i].Name);

				if (i < keys.Length - 1)
					sbTypes.Append(", ");
			}
			Log.Info("[EntityFramework] Available property converters: {0}", sbTypes.ToString());
		}
		#endregion
	}
}

