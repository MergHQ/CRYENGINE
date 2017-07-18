#include "StdAfx.h"
#include "StaticMeshComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{
void CStaticMeshComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
}

void CStaticMeshComponent::Initialize()
{
	LoadFromDisk();
	ResetObject();
}

void CStaticMeshComponent::LoadFromDisk()
{
	if (m_filePath.value.size() > 0)
	{
		m_pCachedStatObj = gEnv->p3DEngine->LoadStatObj(m_filePath.value);
	}
	else
	{
		m_pCachedStatObj = nullptr;
	}
}

void CStaticMeshComponent::SetObject(IStatObj* pObject, bool bSetDefaultMass)
{
	m_pCachedStatObj = pObject;
	m_filePath.value = m_pCachedStatObj->GetFilePath();

	if (bSetDefaultMass)
	{
		if (!m_pCachedStatObj->GetPhysicalProperties(m_physics.m_mass, m_physics.m_density))
		{
			m_physics.m_mass = 10;
		}
	}
}

void CStaticMeshComponent::ResetObject()
{
	if (m_pCachedStatObj != nullptr)
	{
		m_pEntity->SetStatObj(m_pCachedStatObj, GetOrMakeEntitySlotId(), false);
	}
	else
	{
		FreeEntitySlot();
	}
}

void CStaticMeshComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
		LoadFromDisk();
		ResetObject();
	}

	CBaseMeshComponent::ProcessEvent(event);
}

void CStaticMeshComponent::SetFilePath(const char* szPath)
{
	m_filePath.value = szPath;
}
}} // namespace Cry::DefaultComponents
