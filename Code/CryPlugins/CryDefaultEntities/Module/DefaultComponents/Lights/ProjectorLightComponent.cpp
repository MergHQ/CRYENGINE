#include "StdAfx.h"
#include "ProjectorLightComponent.h"

#include <CrySystem/IProjectManager.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>
#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{
static void RegisterProjectorLightComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CProjectorLightComponent));
		// Functions
		/*{
		   auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEnvironmentProbeComponent::LoadFromDisk, "122049AA-015F-4F30-933D-BF2E7C6BA0BC"_cry_guid, "LoadFromDisk");
		   pFunction->SetDescription("Loads a cube map texture from disk and applies it");
		   pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		   pFunction->BindInput(1, 'path', "Cube map Texture Path");
		   componentScope.Register(pFunction);
		   }*/
	}
}

void CProjectorLightComponent::ReflectType(Schematyc::CTypeDesc<CProjectorLightComponent>& desc)
{
	desc.SetGUID(CProjectorLightComponent::IID());
	desc.SetEditorCategory("Lights");
	desc.SetLabel("Projector Light");
	desc.SetDescription("Emits light from its position in a general direction, constrained to a specified angle");
	desc.SetIcon("icons:ObjectTypes/light.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	desc.AddMember(&CProjectorLightComponent::m_bActive, 'actv', "Active", "Active", "Determines whether the light is enabled", true);
	desc.AddMember(&CProjectorLightComponent::m_radius, 'radi', "Radius", "Range", "Determines whether the range of the point light", 10.f);
	desc.AddMember(&CProjectorLightComponent::m_angle, 'angl', "Angle", "Angle", "Maximum angle to emit light to, from the light's forward axis.", 45.0_degrees);

	desc.AddMember(&CProjectorLightComponent::m_projectorOptions, 'popt', "ProjectorOptions", "Projector Options", nullptr, CProjectorLightComponent::SProjectorOptions());

	desc.AddMember(&CProjectorLightComponent::m_color, 'colo', "Color", "Color", "Color emission information", CPointLightComponent::SColor());
	desc.AddMember(&CProjectorLightComponent::m_shadows, 'shad', "Shadows", "Shadows", "Shadow casting settings", CPointLightComponent::SShadows());
	desc.AddMember(&CProjectorLightComponent::m_options, 'opt', "Options", "Options", "Specific Light Options", CPointLightComponent::SOptions());
	desc.AddMember(&CProjectorLightComponent::m_animations, 'anim', "Animations", "Animations", "Light style / animation properties", CPointLightComponent::SAnimations());
}

static void ReflectType(Schematyc::CTypeDesc<CProjectorLightComponent::SProjectorOptions>& desc)
{
	desc.SetGUID("{705FA6D1-CC00-45A5-8E51-78AF6CA14D2D}"_cry_guid);
	desc.AddMember(&CProjectorLightComponent::SProjectorOptions::m_nearPlane, 'near', "NearPlane", "Near Plane", nullptr, 0.f);
	desc.AddMember(&CProjectorLightComponent::SProjectorOptions::m_texturePath, 'tex', "Texture", "Projected Texture", "Path to a texture we want to emit", "");
	desc.AddMember(&CProjectorLightComponent::SProjectorOptions::m_materialPath, 'mat', "Material", "Material", "Path to a material we want to apply to the projector", "");
}

static void ReflectType(Schematyc::CTypeDesc<CProjectorLightComponent::SFlare>& desc)
{
	desc.SetGUID("{DE4B89DD-B436-47EC-861F-4A5F3E831594}"_cry_guid);
	desc.AddMember(&CProjectorLightComponent::SFlare::m_angle, 'angl', "Angle", "Angle", nullptr, 360.0_degrees);
	desc.AddMember(&CProjectorLightComponent::SFlare::m_texturePath, 'tex', "Texture", "Flare Texture", "Path to the flare texture we want to use", "");
}

void CProjectorLightComponent::Run(Schematyc::ESimulationMode simulationMode)
{
	Initialize();
}

void CProjectorLightComponent::SProjectorOptions::SetTexturePath(const char* szPath)
{
	m_texturePath = szPath;
}

void CProjectorLightComponent::SProjectorOptions::SetMaterialPath(const char* szPath)
{
	m_materialPath = szPath;
}

void CProjectorLightComponent::SFlare::SetTexturePath(const char* szPath)
{
	m_texturePath = szPath;
}
}
}
