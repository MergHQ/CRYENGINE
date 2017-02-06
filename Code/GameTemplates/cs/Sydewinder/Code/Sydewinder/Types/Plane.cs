using System;
using CryEngine.EntitySystem;
using CryEngine.Common;

namespace CryEngine.Sydewinder
{
	[EntityClass]
	public class Plane : EntityComponent
	{
		[EntityProperty(EntityPropertyType.Object)]
		public string Geometry { get; set; } = "objects/default/primitive_box.cgf";

		public Plane()
		{
			var x = Geometry;

			Entity.LoadGeometry(0, Geometry);
			Entity.Physics.Physicalize(10, EPhysicalizationType.ePT_Rigid);
		}
	}
}

