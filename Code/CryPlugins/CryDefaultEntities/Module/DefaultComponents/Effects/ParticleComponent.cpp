#include "StdAfx.h"
#include "ParticleComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
	namespace DefaultComponents
	{
		void RegisterParticleComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CParticleComponent));
				{
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CParticleComponent::Activate, "{14F978C0-2C56-40C9-95FB-6967936DBFF9}"_cry_guid, "Activate");
					pFunction->SetDescription("Enables / disables particle emitter");
					pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
					pFunction->BindInput(1, 'actv', "Active");
					componentScope.Register(pFunction);
				}
				{
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CParticleComponent::IsActive, "{6B376186-B2AB-46FD-86C9-9ED772159590}"_cry_guid, "IsActive");
					pFunction->SetDescription("Is particle emitter enabled?");
					pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
					pFunction->BindOutput(0, 'actv', "Active");
					componentScope.Register(pFunction);
				}
				{
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CParticleComponent::SetSpawnGeometry, "{7C3E62F0-73B9-45B6-B7E1-1EFAEB5C5D3E}"_cry_guid, "SetSpawnGeometry");
					pFunction->SetDescription("Sets the geometry to be used for emitters that spawn from geometry");
					pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
					pFunction->BindInput(1, 'geom', "Geometry", "Geometry");
					componentScope.Register(pFunction);
				}
			}
		}

		void CParticleComponent::ReflectType(Schematyc::CTypeDesc<CParticleComponent>& desc)
		{
			desc.SetGUID(CParticleComponent::IID());
			desc.SetEditorCategory("Effects");
			desc.SetLabel("Particle Emitter");
			desc.SetDescription("Emits a particle effect");
			desc.SetIcon("icons:schematyc/entity_particle_emitter_component.png");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CParticleComponent::m_bEnabledByDefault, 'actv', "Enabled", "Enabled", "Whether or not the particle should emit by default", true);
			desc.AddMember(&CParticleComponent::m_effectName, 'file', "FilePath", "Effect", "Determines the particle effect to load");
			desc.AddMember(&CParticleComponent::m_emitterGeomFileName, 'emge', "EmitterGeo", "Emitter Geometry", "Sets specific geometry used to spawn particles");

			desc.AddMember(&CParticleComponent::m_spawnParams, 'spaw', "SpawnParams", "Spawn Parameters", nullptr, CParticleComponent::SSpawnParameters());
			desc.AddMember(&CParticleComponent::m_attributes, 'attr', "Attributes", "Attributes", nullptr, CParticleComponent::SAttributes());
		}

		static void ReflectType(Schematyc::CTypeDesc<CParticleComponent::SAttributes>& desc)
		{
			desc.SetGUID("{24AE6687-F855-4736-9C71-3419083BAECB}"_cry_guid);
			desc.SetLabel("Particle Attributes");
		}

		static void ReflectType(Schematyc::CTypeDesc<CParticleComponent::SSpawnParameters>& desc)
		{
			desc.SetGUID("{9E4544F0-3E5A-4479-B618-E9CB46905149}"_cry_guid);
			desc.SetLabel("Particle Spawn Parameters");
		}

		void CParticleComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			Initialize();
		}

		void CParticleComponent::SetEffectName(const char* szPath)
		{
			m_effectName = szPath;
		}

		void CParticleComponent::SetEmitterGeomFileName(const char* szPath)
		{
			m_emitterGeomFileName = szPath;
		}

		bool Serialize(Serialization::IArchive& archive, CParticleComponent::SAttributes& value, const char* szName, const char* szLabel)
		{
			return archive(*value.m_pAttributes, szName, szLabel);
		}

		bool Serialize(Serialization::IArchive& archive, CParticleComponent::SSpawnParameters& value, const char* szName, const char* szLabel)
		{
			return archive(value.m_spawnParams, szName, szLabel);
		}
	}
}