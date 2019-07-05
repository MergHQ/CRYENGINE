#include "StdAfx.h"
#include "WaterVolumeComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{

void CWaterVolumeComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CWaterVolumeComponent::SetDensity, "{B033BB4B-41B5-40FB-90ED-2D0BCC60FC3F}"_cry_guid, "SetDensity");
		pFunction->SetDescription("Sets water density for the volume");
		pFunction->BindInput(1, 'dens', "Water Density", "Water density, in kg/m^3; default water has 1000", 1000.0f);
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CWaterVolumeComponent::SetResistance, "{2B8A521F-FA43-44EB-8AB3-3A31A4AEA426}"_cry_guid, "SetResistance");
		pFunction->SetDescription("Sets water resistance for the volume");
		pFunction->BindInput(1, 'resi', "Water Resistance", "Water resistance, in kg/(m^2*s); default water has 1000", 1000.0f);
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CWaterVolumeComponent::SetFlow, "{1A601581-ABBA-42CC-8502-FC0FA38832AF}"_cry_guid, "SetFlow");
		pFunction->SetDescription("Sets water flow vector for the volume");
		pFunction->BindInput(1, 'flow', "Water Flow", "Water flow velocity vector", Vec3(ZERO));
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CWaterVolumeComponent::SetFixedVolume, "{D9E19A23-657D-496D-9690-072109D20544}"_cry_guid, "SetFixedVolume");
		pFunction->SetDescription("Sets fixed volume of the water (used to procedurally fill vessel geometries)");
		pFunction->BindInput(1, 'volu', "Fixed Volume", "Dynamically changes 'fixed volume' setting of the water volume", 0.0f);
		componentScope.Register(pFunction);
	}
}

void CWaterVolumeComponent::Attach()
{
	Vec3 ptc = GetEntity()->GetWorldTM() * (GetTransform() ? GetTransform()->GetTranslation() : Vec3(ZERO));
	IPhysicalEntity **pents;
	pe_params_buoyancy pb;
	int i = gEnv->pPhysicalWorld->GetEntitiesInBox(ptc - Vec3(m_distAttach), ptc + Vec3(m_distAttach), pents, ent_areas);
	for(--i; i >= 0 && !(pents[i]->GetParams(&pb) && !pb.iMedium && !is_unused(pb.waterPlane.origin) && fabs(pb.waterPlane.n * (pb.waterPlane.origin - ptc)) <= m_distAttach); i--)
		;
	m_volume = i >= 0 ? (IWaterVolumeRenderNode*)pents[i]->GetForeignData(PHYS_FOREIGN_ID_WATERVOLUME) : nullptr;
}

void CWaterVolumeComponent::ProcessEvent(const SEntityEvent& event)
{
	switch (event.event)
	{
		case ENTITY_EVENT_RESET:
			if (m_volume)
				m_volume->Physicalize();
			m_volume = nullptr;
			break;
		case ENTITY_EVENT_START_GAME:
			Attach();
			break;
	}
}

Cry::Entity::EventFlags CWaterVolumeComponent::GetEventMask() const
{
	Cry::Entity::EventFlags bitFlags = ENTITY_EVENT_START_GAME | ENTITY_EVENT_RESET;
	return bitFlags;
}

}
}
