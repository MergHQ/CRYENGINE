using System;
using CryEngine.EntitySystem;
using CryEngine.Common;

namespace CryEngine.Sydewinder
{
	public class Plane : Entity
	{
		[EntityProperty]
		public string Geometry { get; set; } = "objects/default/primitive_plane_small.cgf";

		public Plane()
		{
			var x = Geometry;
		}

		public override void OnStart()
		{
			base.OnStart();
			NativeHandle.LoadGeometry(0, Geometry);
			Physics.Physicalize(10, EPhysicalizationType.ePT_Rigid);
		}
	}
}

