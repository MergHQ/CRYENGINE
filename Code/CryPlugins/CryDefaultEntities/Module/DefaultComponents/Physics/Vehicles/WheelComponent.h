#pragma once

#include "DefaultComponents/Physics/PhysicsPrimitiveComponent.h"

namespace Cry
{
	namespace DefaultComponents
	{
		class CWheelComponent
			: public CPhysicsPrimitiveComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

		public:
			CWheelComponent() {}
			virtual ~CWheelComponent() {}

			// CPhysicsPrimitiveComponent
			virtual std::unique_ptr<pe_geomparams> GetGeomParams() const final 
			{
				std::unique_ptr<pe_geomparams> pParams = stl::make_unique<pe_cargeomparams>();
				pe_cargeomparams* pWheelParams = static_cast<pe_cargeomparams*>(pParams.get());
				pWheelParams->bDriving = 1;
				pWheelParams->flagsCollider = geom_colltype_vehicle;
				pWheelParams->flags &= ~geom_floats;
				pWheelParams->iAxle = m_axleIndex;
				pWheelParams->pivot = m_pTransform->GetTranslation();
				pWheelParams->lenMax = m_suspensionLength;
				pWheelParams->lenInitial = 0.f;
				pWheelParams->kStiffness = 0.f;
				pWheelParams->kDamping = -1.f;
				pWheelParams->bRayCast = m_bRaycast ? 1 : 0;

				return pParams;
			}

			virtual IGeometry* CreateGeometry() const final
			{
				primitives::cylinder primitive;
				primitive.center = ZERO;
				primitive.axis = Vec3(1, 0, 0);
				primitive.r = m_radius;
				primitive.hh = m_height * 0.5f;

				return gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive((int)primitive.type, &primitive);
			}
			// ~CPhysicsPrimitiveComponent

			static CryGUID& IID()
			{
				static CryGUID id = "{1988C96E-194F-48F4-B54D-467A851AB728}"_cry_guid;
				return id;
			}

			virtual void SetAxleIndex(int index) { m_axleIndex = index; }

			Schematyc::Range<0, 1000000> m_radius = 0.5f;
			Schematyc::Range<0, 1000000> m_height = 1.f;

			int m_axleIndex = 0;
			float m_suspensionLength = 0.5f;

			bool m_bRaycast = false;
		};
	}
}