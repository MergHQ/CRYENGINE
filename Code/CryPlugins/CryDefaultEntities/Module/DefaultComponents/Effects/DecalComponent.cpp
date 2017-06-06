#include "StdAfx.h"
#include "DecalComponent.h"

#include <Cry3DEngine/IRenderNode.h>

static void ReflectType(Schematyc::CTypeDesc<SDecalProperties::EProjectionType>& desc)
{
	desc.SetGUID("{8556D9B6-F2D4-41DC-9C5C-897801C1D8CA}"_cry_guid);
	desc.SetLabel("Decal Projection Type");
	desc.SetDescription("Determines the method of projecting decals to objects");
	desc.SetDefaultValue(SDecalProperties::ePlanar);
	desc.AddConstant(SDecalProperties::ePlanar, "Planar", "Planar");
	desc.AddConstant(SDecalProperties::eProjectOnStaticObjects, "StaticObjects", "Static Objects");
	desc.AddConstant(SDecalProperties::eProjectOnTerrain, "Terrain", "Terrain");
	desc.AddConstant(SDecalProperties::eProjectOnTerrainAndStaticObjects, "TerrainAndStaticObjects", "Terrain and Static Objects");
}

namespace Cry
{
namespace DefaultComponents
{
static void RegisterDecalComponent(Schematyc::IEnvRegistrar& registrar)
{
	Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
	{
		Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CDecalComponent));
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CDecalComponent::Spawn, "{39ED7682-5C39-47EE-BEFF-0A39EB801EC2}"_cry_guid, "Spawn");
			pFunction->SetDescription("Spaws the decal");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			componentScope.Register(pFunction);
		}
		{
			auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CDecalComponent::Remove, "{7A31C5D3-51E2-4091-9A4D-A2459D37F67A}"_cry_guid, "Remove");
			pFunction->SetDescription("Removes the spawned decal, if any");
			pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
			componentScope.Register(pFunction);
		}
	}
}

void CDecalComponent::ReflectType(Schematyc::CTypeDesc<CDecalComponent>& desc)
{
	desc.SetGUID(CDecalComponent::IID());
	desc.SetEditorCategory("Effects");
	desc.SetLabel("Decal");
	desc.SetDescription("Exposes support for spawning decals");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	desc.AddMember(&CDecalComponent::m_bAutoSpawn, 'auto', "AutoSpawn", "Default Spawn", "Whether or not to automatically spawn the decal", true);
	desc.AddMember(&CDecalComponent::m_materialFileName, 'matf', "Material", "Material", "The material we want to use on decals", "");
	desc.AddMember(&CDecalComponent::m_bFollowEntityAfterSpawn, 'folo', "FollowEntity", "Follow Entity", "Whether or not the decal follows the entity", true);
	desc.AddMember(&CDecalComponent::m_projectionType, 'proj', "ProjectionType", "Projection Type", "The method of projection we want to use", SDecalProperties::ePlanar);
	desc.AddMember(&CDecalComponent::m_depth, 'dept', "Depth", "Projection Depth", "The depth at which we should project the decal", 1.f);
	desc.AddMember(&CDecalComponent::m_sortPriority, 'sort', "SortPriority", "Sort Priority", "Sorting priority, used to resolve depth issues with other decals", 16);
}

void CDecalComponent::Initialize()
{
	if (m_bAutoSpawn)
	{
		Spawn();
	}
	else
	{
		Remove();
	}
}

void CDecalComponent::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_XFORM:
		{
			Spawn();
		}
		break;
	case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
		{
			if(m_bSpawned)
			{
				Spawn();
			}
		}
		break;
	}
}

uint64 CDecalComponent::GetEventMask() const
{
	uint64 bitFlags = (m_bFollowEntityAfterSpawn && m_bSpawned) ? BIT64(ENTITY_EVENT_XFORM) : 0;
	if (m_bSpawned)
	{
		bitFlags |= BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
	}

	return bitFlags;
}

void CDecalComponent::SetMaterialFileName(const char* szPath)
{
	m_materialFileName = szPath;
}
}
}
