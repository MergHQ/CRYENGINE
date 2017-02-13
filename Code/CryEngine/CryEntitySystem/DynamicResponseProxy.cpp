// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "DynamicResponseProxy.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryNetwork/ISerialize.h>

CRYREGISTER_CLASS(CEntityComponentDynamicResponse);

//////////////////////////////////////////////////////////////////////////
CEntityComponentDynamicResponse::CEntityComponentDynamicResponse()
	: m_pResponseActor(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
CEntityComponentDynamicResponse::~CEntityComponentDynamicResponse()
{
	gEnv->pDynamicResponseSystem->ReleaseResponseActor(m_pResponseActor);
	m_pResponseActor = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentDynamicResponse::Initialize()
{
	const char* szEntityName = m_pEntity->GetName();
	m_pResponseActor = gEnv->pDynamicResponseSystem->GetResponseActor(szEntityName);
	if (m_pResponseActor)
	{
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "DrsActor with name '%s' already exists. Actors need to have unique names to be referenced correctly", szEntityName);
	}
	else
	{
		m_pResponseActor = gEnv->pDynamicResponseSystem->CreateResponseActor(szEntityName, m_pEntity->GetId());
	}
	SET_DRS_USER_SCOPED("DrsProxy Initialize");
	DRS::IVariableCollection* pVariables = m_pResponseActor->GetLocalVariables();
	if (pVariables)
	{
		pVariables->SetVariableValue("Name", CHashedString(szEntityName));
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentDynamicResponse::ProcessEvent(SEntityEvent& event)
{
	if (m_pResponseActor && event.event == ENTITY_EVENT_RESET)
	{
		SET_DRS_USER_SCOPED("DrsProxy Reset Event");
		DRS::IVariableCollection* pVariables = m_pResponseActor->GetLocalVariables();
		if (pVariables)
		{
			pVariables->SetVariableValue("Name", m_pResponseActor->GetName());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
uint64 CEntityComponentDynamicResponse::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_RESET);
}

//////////////////////////////////////////////////////////////////////////
bool CEntityComponentDynamicResponse::NeedGameSerialize()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentDynamicResponse::GameSerialize(TSerialize ser)
{

}

//////////////////////////////////////////////////////////////////////////
DRS::IVariableCollection* CEntityComponentDynamicResponse::GetLocalVariableCollection() const
{
	CRY_ASSERT_MESSAGE(m_pResponseActor, "DRS Proxy without an Actor detected. Should never happen.");
	return m_pResponseActor->GetLocalVariables();
}

//////////////////////////////////////////////////////////////////////////
DRS::IResponseActor* CEntityComponentDynamicResponse::GetResponseActor() const
{
	CRY_ASSERT_MESSAGE(m_pResponseActor, "DRS Proxy without an Actor detected. Should never happen.");
	return m_pResponseActor;
}
