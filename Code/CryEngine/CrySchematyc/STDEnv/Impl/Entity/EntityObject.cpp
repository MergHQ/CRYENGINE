// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntityObject.h"

#include <CryEntitySystem/IEntityClass.h>
#include <CryEntitySystem/IEntityAttributesProxy.h>
#include <Schematyc/Entity/EntityUserData.h>

#include "STDEnv.h"
#include "Entity/EntityClassProperties.h"
#include "Entity/EntityObjectAttribute.h"
#include "Entity/EntityObjectMap.h"
#include "Utils/SystemStateMonitor.h"

namespace Schematyc
{
CEntityObject::~CEntityObject()
{
	DestroyObject();
}

bool CEntityObject::Init(IEntity* pEntity, bool bIsPreview)
{
	if (!pEntity->GetProxy(ENTITY_PROXY_ATTRIBUTES))
	{
		pEntity->CreateProxy(ENTITY_PROXY_ATTRIBUTES);
	}

	m_pEntity = pEntity;
	m_bIsPreview = bIsPreview;
	
	return true;
}

void CEntityObject::PostInit()
{
	
	const bool bEditing = gEnv->IsEditing() && !CSTDEnv::GetInstance()->GetSystemStateMonitor().LoadingLevel();
	CreateObject(m_bIsPreview ? ESimulationMode::Preview : bEditing ? ESimulationMode::Editor : ESimulationMode::Idle);
}

void CEntityObject::CreateObject(ESimulationMode simulationMode)
{
	SCHEMATYC_ENV_ASSERT(m_pEntity && !m_pObject);
	if (m_pEntity && !m_pObject)
	{
		CEntityObjectAttributePtr pEntityObjectAttribute = GetEntityObjectAttribute();
		SCHEMATYC_ENV_ASSERT(pEntityObjectAttribute);
		if (pEntityObjectAttribute)
		{
			SObjectParams objectParams(pEntityObjectAttribute->GetGUID());
			objectParams.pCustomData = this;
			objectParams.pProperties = pEntityObjectAttribute->GetProperties();
			objectParams.simulationMode = simulationMode;

			m_pObject = GetSchematycCore().CreateObject(objectParams);
			if (m_pObject)
			{
				pEntityObjectAttribute->GetChangeSignalSlots().Connect(Delegate::Make(*this, &CEntityObject::OnPropertiesChange), m_connectionScope);
				CSTDEnv::GetInstance()->GetEntityObjectMap().AddEntity(m_pEntity->GetId(), m_pObject->GetId());
			}
			else
			{
				SCHEMATYC_ENV_ERROR("Failed to create Schematyc object for entity: name = %d", m_pEntity->GetName());
			}
		}
	}
}

void CEntityObject::DestroyObject()
{
	if (m_pObject)
	{
		CSTDEnv::GetInstance()->GetEntityObjectMap().RemoveEntity(m_pEntity->GetId());
		GetSchematycCore().DestroyObject(m_pObject->GetId());

		m_pObject = nullptr;
		m_connectionScope.Release();
	}
}

void CEntityObject::ProcessEvent(const SEntityEvent& event)
{	
	if (m_pObject && ((m_pEntity->GetFlags() & ENTITY_FLAG_SPAWNED) == 0))
	{
		switch (event.event)
		{
		case ENTITY_EVENT_RESET:
			{
				m_pObject->Reset(m_bIsPreview ? ESimulationMode::Preview : event.nParam[0] == 1 ? ESimulationMode::Game : ESimulationMode::Editor, EObjectResetPolicy::Always);
				break;
			}
		case ENTITY_EVENT_START_LEVEL:
			{
				if (!m_bIsPreview && !gEnv->IsEditor())
				{
					m_pObject->Reset(m_bIsPreview ? ESimulationMode::Preview : ESimulationMode::Game, EObjectResetPolicy::OnChange);
				}
				break;
			}
		case ENTITY_EVENT_XFORM:
		case ENTITY_EVENT_DONE:
		case ENTITY_EVENT_HIDE:
		case ENTITY_EVENT_UNHIDE:
		case ENTITY_EVENT_SCRIPT_EVENT:
		case ENTITY_EVENT_ENTERAREA:
		case ENTITY_EVENT_LEAVEAREA:
			{
				m_eventSignal.Send(event);
				break;
			}
		}
	}
}

void CEntityObject::OnPropertiesChange()
{
	if (m_pObject && gEnv->IsEditing())
	{
		DestroyObject();
		CreateObject(m_bIsPreview ? ESimulationMode::Preview : ESimulationMode::Editor);
	}
}

IEntity& CEntityObject::GetEntity()
{
	SCHEMATYC_CORE_ASSERT(m_pEntity);
	return *m_pEntity;
}

EntityEventSignal::Slots& CEntityObject::GetEventSignalSlots()
{
	return m_eventSignal.GetSlots();
}

GeomUpdateSignal& CEntityObject::GetGeomUpdateSignal()
{
	return m_geomUpdateSignal;
}

CEntityObjectAttributePtr CEntityObject::GetEntityObjectAttribute()
{
	SCHEMATYC_CORE_ASSERT(m_pEntity);
	if (m_pEntity)
	{
		IEntityAttributesProxy* pAttributesProxy = static_cast<IEntityAttributesProxy*>(m_pEntity->GetProxy(ENTITY_PROXY_ATTRIBUTES));
		if (pAttributesProxy)
		{
			return std::static_pointer_cast<CEntityObjectAttribute>(EntityAttributeUtils::FindAttribute(pAttributesProxy->GetAttributes(), CEntityObjectAttribute::ms_szAttributeName));
		}
	}
	return CEntityObjectAttributePtr();
}

} // Schematyc
