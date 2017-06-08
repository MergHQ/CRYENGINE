#pragma once

#include "PhysicsPrimitiveComponent.h"

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CBoxPrimitiveComponent
			: public CPhysicsPrimitiveComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);
			
		public:
			static void ReflectType(Schematyc::CTypeDesc<CBoxPrimitiveComponent>& desc);

			CBoxPrimitiveComponent() {}
			virtual ~CBoxPrimitiveComponent() {}

			virtual IGeometry* CreateGeometry() const final
			{
				primitives::box primitive;
				primitive.size = m_size;
				primitive.Basis = IDENTITY;
				primitive.bOriented = 1;
				primitive.center = Vec3(0, 0, m_size.z);

				return gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive((int)primitive.type, &primitive);
			}

			static CryGUID& IID()
			{
				static CryGUID id = "{52431C11-77EE-410A-A7A7-A7FA69E79685}"_cry_guid;
				return id;
			}

			Vec3 m_size = Vec3(1, 1, 1);
		};
	}
}