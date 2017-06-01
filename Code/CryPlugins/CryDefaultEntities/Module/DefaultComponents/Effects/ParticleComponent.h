#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

#include <CryParticleSystem/IParticlesPfx2.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CParticleComponent
			: public IEntityComponent
		{
		public:
			virtual ~CParticleComponent() {}

			// IEntityComponent
			virtual void Initialize() override
			{
				if (m_effectName.value.size() > 0 && m_bEnabledByDefault)
				{
					if (IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(m_effectName.value, "CParticleComponent"))
					{
						if (IParticleEmitter* pEmitter = m_pEntity->GetParticleEmitter(GetEntitySlotId()))
						{
							if (pEmitter->GetEffect() != pEffect)
							{
								FreeEntitySlot();
								m_bCurrentlyActive = false;
							}
						}

						if(!m_bCurrentlyActive)
						{
							m_pEntity->LoadParticleEmitter(GetOrMakeEntitySlotId(), pEffect, &m_spawnParams.m_spawnParams);
						}

						if (IParticleEmitter* pEmitter = m_pEntity->GetParticleEmitter(GetEntitySlotId()))
						{
							m_bCurrentlyActive = true;

							pEmitter->GetAttributes().Reset(m_attributes.m_pAttributes.get());

							if (m_emitterGeomFileName.value.size() > 0)
							{
								if (IStatObj* pObject = gEnv->p3DEngine->LoadStatObj(m_emitterGeomFileName.value))
								{
									m_emitterGeomReference.Set(pObject);
									pEmitter->SetEmitGeom(m_emitterGeomReference);
								}
							}
						}
					}
					else
					{
						FreeEntitySlot();
					}
				}
				else
				{
					FreeEntitySlot();
				}
			}

			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CParticleComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{7925BC64-521F-4A2A-9068-C9C9A1D3499E}"_cry_guid;
				return id;
			}

			void Activate(bool bActive)
			{
				if (m_bCurrentlyActive == bActive)
				{
					return;
				}

				if (IParticleEmitter* pParticleEmitter = GetEntity()->GetParticleEmitter(GetEntitySlotId()))
				{
					pParticleEmitter->Activate(bActive);
					if (m_bCurrentlyActive)
					{
						pParticleEmitter->Restart();
					}
					m_bCurrentlyActive = bActive;
				}
				else
				{
					m_bCurrentlyActive = false;
				}
			}

			bool IsActive() const { return m_bCurrentlyActive; }
			void SetSpawnGeometry(const Schematyc::GeomFileName& geomName)
			{
				if (IStatObj* pObject = gEnv->p3DEngine->LoadStatObj(geomName.value.c_str()))
				{
					m_emitterGeomReference.Set(pObject);

					if (IParticleEmitter* pEmitter = GetEntity()->GetParticleEmitter(GetEntitySlotId()))
					{
						pEmitter->SetEmitGeom(m_emitterGeomReference);
					}
				}
			}

			struct SAttributes
			{
				SAttributes()
				{
					m_pAttributes = pfx2::GetIParticleSystem()->CreateParticleAttributes();
				}

				inline bool operator==(const SAttributes &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }
				
				TParticleAttributesPtr m_pAttributes;
			};

			struct SSpawnParameters
			{
				inline bool operator==(const SSpawnParameters &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }
				
				SpawnParams m_spawnParams;
			};

			void EnableAutomaticActivation(bool bEnable) { m_bEnabledByDefault = bEnable; }
			bool IsAutomaticActivationEnabled() const { return m_bEnabledByDefault; }

			SSpawnParameters& GetSpawnParameters() { return m_spawnParams; }
			const SSpawnParameters& GetSpawnParameters() const { return m_spawnParams; }

			SAttributes& GetAttributes() { return m_attributes; }
			const SAttributes& GetAttributes() const { return m_attributes; }

			virtual void SetEffectName(const char* szPath);
			const char* GetEffectName() const { return m_effectName.value.c_str(); }
			virtual void SetEmitterGeomFileName(const char* szPath);
			const char* GetEmitterGeomFileName() const { return m_emitterGeomFileName.value.c_str(); }

		protected:
			bool m_bEnabledByDefault = true;
			
			SSpawnParameters m_spawnParams;
			SAttributes m_attributes;

			GeomRef m_emitterGeomReference;
			bool m_bCurrentlyActive = false;

			Schematyc::ParticleEffectName m_effectName;
			Schematyc::GeomFileName m_emitterGeomFileName;
		};
	}
}