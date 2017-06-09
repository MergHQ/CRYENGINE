#pragma once

#include "PhysicsPrimitiveComponent.h"

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CCylinderPrimitiveComponent
			: public CPhysicsPrimitiveComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

		public:
			static void ReflectType(Schematyc::CTypeDesc<CCylinderPrimitiveComponent>& desc);

			CCylinderPrimitiveComponent() {}
			virtual ~CCylinderPrimitiveComponent() {}

			virtual IGeometry* CreateGeometry() const final
			{
				primitives::cylinder primitive;
				primitive.center = ZERO;
				primitive.axis = Vec3(0, 0, 1);
				primitive.r = m_radius;
				primitive.hh = m_height * 0.5f;

				return gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive((int)primitive.type, &primitive);
			}

			static CryGUID& IID()
			{
				static CryGUID id = "{00B840E8-FEFF-45B0-8C1E-B3B3F1D9E0EE}"_cry_guid;
				return id;
			}

			Schematyc::Range<0, 1000000> m_radius = 0.5f;
			Schematyc::Range<0, 1000000> m_height = 1.f;
		};
	}
}