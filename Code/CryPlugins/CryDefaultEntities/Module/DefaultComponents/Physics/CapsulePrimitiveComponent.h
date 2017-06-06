#pragma once

#include "PhysicsPrimitiveComponent.h"

namespace Cry
{
	namespace DefaultComponents
	{
		class CCapsulePrimitiveComponent
			: public CPhysicsPrimitiveComponent
		{
		public:
			CCapsulePrimitiveComponent() {}
			virtual ~CCapsulePrimitiveComponent() {}

			virtual IGeometry* CreateGeometry() const final
			{
				primitives::capsule primitive;
				primitive.center = ZERO;
				primitive.axis = Vec3(0, 0, 1);
				primitive.r = m_radius;
				primitive.hh = m_height * 0.5f;

				return gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive((int)primitive.type, &primitive);
			}

			static CryGUID& IID()
			{
				static CryGUID id = "{6657F34C-D32A-469F-B9E3-62C7A0A0CCF7}"_cry_guid;
				return id;
			}

			Schematyc::Range<0, 1000000> m_radius = 0.5f;
			Schematyc::Range<0, 1000000> m_height = 1.f;
		};
	}
}