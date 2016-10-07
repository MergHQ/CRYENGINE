using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Runtime.Serialization;

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

		private Dictionary<int, EntityProperty> _properties = new Dictionary<int, EntityProperty>();
		#endregion

		#region Properties
		/// <summary>
		/// Native entity class description structure, storing general information (such as name), as well as editor relevant metadata (e.g. EditType).
		/// </summary>
		public IEntityClassRegistry.SEntityClassDesc Description { get { return _description; } set { _description = value; } }
		/// <summary>
		/// Managed prototype class, that will be instanced, when the entity is being spawned.
		/// </summary>
		public Type ProtoType { get { return _protoType; } set { _protoType = value; } }
		/// <summary>
		/// Native entity class, created as an in-engine handle for the managed entity class.
		/// </summary>
		public IEntityClass NativeClass { get { return _nativeClass; } set { _nativeClass = value; } }

		internal Dictionary<int, EntityProperty> Properties { get { return _properties; } set { _properties = value; } }
		#endregion

		#region Constructors
		/// <summary>
		/// Only intended to be used by the EntityFramework itself! This constructor will setup the property handler, as well as the Editor relevant metadata description for the engine.
		/// </summary>
		/// <param name="t">Prototype the entity class should be created for.</param>
		internal EntityClass(Type t)
		{
			_nativeClass = null;
			_protoType = t;
			_description = new IEntityClassRegistry.SEntityClassDesc();
			_description.editorClassInfo = new SEditorClassInfo();
			_description.sScriptFile = "";

			EntityClassAttribute attrib = (EntityClassAttribute)t.GetCustomAttributes(typeof(EntityClassAttribute), true).FirstOrDefault();
			if (attrib == null)
			{
				_description.sName = t.Name;
				_description.editorClassInfo.sCategory = "Game";
			}
			else
			{
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
		internal Entity CreateManagedInstance(IEntity pEntity)
		{
			var instance = FormatterServices.GetUninitializedObject(_protoType) as Entity;

			instance.NativeHandle = pEntity;
			instance.Id = pEntity.GetId();
			instance.EntityClass = this;

			// Get the native property values (most definitely defaults) and assign to our managed properties
			var propertyHandler = pEntity.GetClass().GetPropertyHandler();
			if (propertyHandler != null)
			{
				foreach (var property in instance.EntityClass.Properties)
				{
					var propertyValue = propertyHandler.GetProperty(pEntity, property.Key);

					property.Value.Set(instance, propertyValue);
				}
			}

			instance.InitializeManagedEntity(_protoType);
			return instance;
		}
		#endregion
	}
}
