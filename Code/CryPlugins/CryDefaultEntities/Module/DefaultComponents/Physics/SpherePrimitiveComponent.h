#pragma once

#include "PhysicsPrimitiveComponent.h"

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CSpherePrimitiveComponent
			: public CPhysicsPrimitiveComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

		public:
			static void ReflectType(Schematyc::CTypeDesc<CSpherePrimitiveComponent>& desc);

			CSpherePrimitiveComponent() {}
			virtual ~CSpherePrimitiveComponent() {}

			virtual IGeometry* CreateGeometry() const final
			{
				primitives::sphere primitive;
				primitive.center = ZERO;
				primitive.r = m_radius;

				return gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive((int)primitive.type, &primitive);
			}

			static CryGUID& IID()
			{
				static CryGUID id = "{106FACAD-4099-4993-A9CC-F558A8586307}"_cry_guid;
				return id;
			}

			Schematyc::Range<0, 100000> m_radius = 1.f;
		};
	}
}