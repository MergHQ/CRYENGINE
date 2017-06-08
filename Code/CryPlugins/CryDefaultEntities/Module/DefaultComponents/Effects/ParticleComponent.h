#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

#include <CryParticleSystem/IParticlesPfx2.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CParticleComponent
			: public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			virtual ~CParticleComponent() {}

			static void ReflectType(Schematyc::CTypeDesc<CParticleComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{7925BC64-521F-4A2A-9068-C9C9A1D3499E}"_cry_guid;
				return id;
			}

			virtual void LoadEffect(bool bActivate)
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

					if (!m_bCurrentlyActive)
					{
						m_pEntity->LoadParticleEmitter(GetOrMakeEntitySlotId(), pEffect, &m_spawnParams.m_spawnParams);
					}

					if (IParticleEmitter* pEmitter = m_pEntity->GetParticleEmitter(GetEntitySlotId()))
					{
						pEmitter->GetAttributes().Reset(m_attributes.m_pAttributes.get());
						pEmitter->Activate(bActivate);
					}
				}
				else
				{
					FreeEntitySlot();
				}
			}

			virtual void Activate(bool bActive)
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

			virtual SSpawnParameters& GetSpawnParameters() { return m_spawnParams; }
			const SSpawnParameters& GetSpawnParameters() const { return m_spawnParams; }

			virtual SAttributes& GetAttributes() { return m_attributes; }
			const SAttributes& GetAttributes() const { return m_attributes; }

			virtual void SetEffectName(const char* szPath);
			const char* GetEffectName() const { return m_effectName.value.c_str(); }
			
		protected:
			bool m_bEnabled = true;
			bool m_bCurrentlyActive = false;
			
			SSpawnParameters m_spawnParams;
			SAttributes m_attributes;

			Schematyc::ParticleEffectName m_effectName;
		};
	}
}