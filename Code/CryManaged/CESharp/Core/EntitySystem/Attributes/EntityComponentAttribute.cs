// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;


namespace CryEngine
{
	/// <summary>
	/// Used to specify specific parameters for entity components, if not used defaults will be in place
	/// Default name is the entity component class name
	/// </summary>
	[AttributeUsage(AttributeTargets.Class)]
	public sealed class EntityComponentAttribute : Attribute
	{
		#region Properties
		/// <summary>
		/// Display name inside CRYENGINE's EntitySystem as well as Sandbox.
		/// </summary>
		public string Name { get; set; }
		/// <summary>
		/// Entity category inside Sandbox.
		/// </summary>
		public string Category { get; set; }
		/// <summary>
		/// User friendly description of what the component is for.
		/// </summary>
		public string Description { get; set; }
		/// <summary>
		/// Editor icon string, i.e. "icons:designer/Designer_Box.ico"
		/// </summary>
		public string Icon { get; set; }
		/// <summary>
		/// Systems reference entity components by GUID, for example when serializing to file to detect which type a component belongs to.
		/// By default we generate a GUID automatically based on the EntityComponent implementation type, however this will result in serialization breaking if you rename it.
		/// To circumvent this you can explicitly specify your desired GUID.
		/// Default GUID format is <c>"C47DF64B-E1F9-40D1-8063-2C533A1CE7D5"</c>.
		/// </summary>
		/// <value>The string value of the GUID.</value>
		public string Guid { get; set; }
		#endregion

		#region Constructors
		public EntityComponentAttribute(string uiName = "", string category = "", string description = "", string icon = "", string guid = "")
		{
			Name = uiName;
			Category = category;
			Description = description;
			Icon = icon;
			Guid = guid;
		}
		#endregion
	}
}
