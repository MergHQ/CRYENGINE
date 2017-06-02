#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CPhysicsPrimitiveComponent
			: public IEntityComponent
#ifndef RELEASE
			, public IEntityComponentPreviewer
#endif
		{
		public:
			CPhysicsPrimitiveComponent() {}
			virtual ~CPhysicsPrimitiveComponent()
			{
				if (m_physicsSlotId != -1)
				{
					if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
					{
						pPhysicalEntity->RemoveGeometry(m_physicsSlotId);
					}
				}
			}
			
			// IEntityComponent
			virtual void Initialize() final
			{
				AddPrimitive();
			}

			virtual void ProcessEvent(SEntityEvent& event) final
			{
				if (event.event == ENTITY_EVENT_PHYSICAL_TYPE_CHANGED)
				{
					AddPrimitive();
				}
			}

			virtual uint64 GetEventMask() const final { return (m_mass > 0 || m_density > 0) ? BIT64(ENTITY_EVENT_PHYSICAL_TYPE_CHANGED) : 0; }

#ifndef RELEASE
			virtual IEntityComponentPreviewer* GetPreviewer() final { return this; }
#endif
			// ~IEntityComponent

#ifndef RELEASE
			// IEntityComponentPreviewer
			virtual void SerializeProperties(Serialization::IArchive& archive) override {}

			virtual void Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const override
			{
				if (context.bSelected)
				{
					Matrix34 slotTransform = entity.GetSlotWorldTM(GetEntitySlotId());

					IGeometry* pPrimGeom = CreateGeometry();

					geom_world_data geomWorldData;
					geomWorldData.R = Matrix33(slotTransform);
					geomWorldData.scale = slotTransform.GetUniformScale();
					geomWorldData.offset = slotTransform.GetTranslation();

					gEnv->pSystem->GetIPhysRenderer()->DrawGeometry(pPrimGeom, &geomWorldData, -1, 0, ZERO, context.debugDrawInfo.color);

					pPrimGeom->Release();
				}
			}
			// ~IEntityComponentPreviewer
#endif

			virtual IGeometry* CreateGeometry() const = 0;

			void AddPrimitive()
			{
#ifndef RELEASE
				Validate();
#endif

				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					if (m_physicsSlotId != -1)
					{
						pPhysicalEntity->RemoveGeometry(m_physicsSlotId);
					}

					if (m_mass > 0 || m_density > 0)
					{
						IGeometry* pPrimGeom = CreateGeometry();
						phys_geometry* pPhysGeom = gEnv->pPhysicalWorld->GetGeomManager()->RegisterGeometry(pPrimGeom, 0);

						pe_geomparams geomParams;

						if (m_mass > 0)
						{
							geomParams.mass = m_mass;
						}
						else
						{
							geomParams.mass = -1.f;
						}

						if (m_density > 0)
						{
							geomParams.density = m_density;
						}
						else
						{
							geomParams.density = -1.f;
						}

						if (m_surfaceTypeName.value.size() > 0)
						{
							string surfaceType = "mat_" + m_surfaceTypeName.value;

							if (ISurfaceType* pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeByName(surfaceType))
							{
								geomParams.surface_idx = pSurfaceType->GetId();
							}
							else
							{
								CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Invalid surface type %s specified on primitive component in entity %s!", m_surfaceTypeName.value.c_str(), m_pEntity->GetName());
							}
						}

						// Force create a dummy entity slot to allow designer transformation change
						SEntitySlotInfo slotInfo;
						if (!m_pEntity->GetSlotInfo(GetOrMakeEntitySlotId(), slotInfo))
						{
							m_pEntity->SetSlotRenderNode(GetOrMakeEntitySlotId(), nullptr);
						}

						Matrix34 slotTransform = m_pEntity->GetSlotLocalTM(GetEntitySlotId(), false);
						geomParams.pMtx3x4 = &slotTransform;

						m_physicsSlotId = pPhysicalEntity->AddGeometry(pPhysGeom, &geomParams);
					}
				}
			}

		protected:
#ifndef RELEASE
			void Validate()
			{
				// Reset mass or density to 0 in the UI if the other is changed to be positive.
				// It is not possible to use both at the same time, this makes that clearer for the designer.
				if (m_mass != m_prevMass && m_mass > 0)
				{
					m_density = m_prevDensity = 0;
					m_prevMass = m_mass;
				}
				if (m_density != m_prevDensity && m_density > 0)
				{
					m_mass = m_prevMass = 0;
					m_prevDensity = m_density;
				}
			}
#endif

		public:
			Schematyc::PositiveFloat m_mass = 10.f;
			Schematyc::PositiveFloat m_density = 0.f;

			// Used to reset mass or density to -1 in the UI
#ifndef RELEASE
			float m_prevMass = 0.f;
			float m_prevDensity = 0.f;
#endif

			Schematyc::SurfaceTypeName m_surfaceTypeName;

		protected:
			int m_physicsSlotId = -1;
		};
	}
}