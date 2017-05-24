#include "StdAfx.h"
#include "PointLightComponent.h"

#include <CrySystem/IProjectManager.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>
#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
	namespace DefaultComponents
	{
		void RegisterPointLightComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CPointLightComponent));
				// Functions
			}
		}

		CRY_STATIC_AUTO_REGISTER_FUNCTION(&RegisterPointLightComponent)

		void CPointLightComponent::ReflectType(Schematyc::CTypeDesc<CPointLightComponent>& desc)
		{
			desc.SetGUID(CPointLightComponent::IID());
			desc.SetEditorCategory("Lights");
			desc.SetLabel("Point Light");
			desc.SetDescription("Emits light from its origin into all directions");
			desc.SetIcon("icons:ObjectTypes/light.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CPointLightComponent::m_bActive, 'actv', "Active", "Active", "Determines whether the light is enabled", true);
			desc.AddMember(&CPointLightComponent::m_radius, 'radi', "Radius", "Range", "Determines whether the range of the point light", 10.f);

			desc.AddMember(&CPointLightComponent::m_color, 'colo', "Color", "Color", "Color emission information", CPointLightComponent::SColor());
			desc.AddMember(&CPointLightComponent::m_shadows, 'shad', "Shadows", "Shadows", "Shadow casting settings", CPointLightComponent::SShadows());
			desc.AddMember(&CPointLightComponent::m_options, 'opt', "Options", "Options", "Specific Light Options", CPointLightComponent::SOptions());
			desc.AddMember(&CPointLightComponent::m_animations, 'anim', "Animations", "Animations", "Light style / animation properties", CPointLightComponent::SAnimations());
		}

		void CPointLightComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			Initialize();
		}
	}
}