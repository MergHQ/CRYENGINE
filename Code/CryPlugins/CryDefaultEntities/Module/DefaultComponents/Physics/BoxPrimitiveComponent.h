#pragma once

#include "PhysicsPrimitiveComponent.h"

namespace Cry
{
	namespace DefaultComponents
	{
		class CBoxPrimitiveComponent
			: public CPhysicsPrimitiveComponent
		{
		public:
			CBoxPrimitiveComponent() {}
			virtual ~CBoxPrimitiveComponent() {}

			// IEntityComponent
			virtual void Run(Schematyc::ESimulationMode simulationMode) final;
			// ~IEntityComponent

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