// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "DynamicResponseProxy.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryNetwork/ISerialize.h>

//////////////////////////////////////////////////////////////////////////
CDynamicResponseProxy::CDynamicResponseProxy()
	: m_pResponseActor(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
CDynamicResponseProxy::~CDynamicResponseProxy()
{
}

//////////////////////////////////////////////////////////////////////////
void CDynamicResponseProxy::Initialize(SComponentInitializer const& init)
{
	const char* szEntityName = init.m_pEntity->GetName();
	m_pResponseActor = gEnv->pDynamicResponseSystem->GetResponseActor(szEntityName);
	if (m_pResponseActor)
	{
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_ERROR_DBGBRK, "DrsActor with name '%s' already exists. Actors need to have unique names to be referenced correctly", szEntityName);
	}
	else
	{
		m_pResponseActor = gEnv->pDynamicResponseSystem->CreateResponseActor(szEntityName, init.m_pEntity->GetId());
	}
	SET_DRS_USER_SCOPED("DrsProxy Initialize");
	m_pResponseActor->GetLocalVariables()->SetVariableValue("Name", CHashedString(szEntityName));
}

//////////////////////////////////////////////////////////////////////////
void CDynamicResponseProxy::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
}

//////////////////////////////////////////////////////////////////////////
void CDynamicResponseProxy::Release()
{
	gEnv->pDynamicResponseSystem->ReleaseResponseActor(m_pResponseActor);
	m_pResponseActor = nullptr;
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CDynamicResponseProxy::Update(SEntityUpdateContext& ctx)
{
}

//////////////////////////////////////////////////////////////////////////
void CDynamicResponseProxy::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_RESET)
	{
		SET_DRS_USER_SCOPED("DrsProxy Reset Event");
		m_pResponseActor->GetLocalVariables()->SetVariableValue("Name", m_pResponseActor->GetName());
	}
}

//////////////////////////////////////////////////////////////////////////
bool CDynamicResponseProxy::NeedSerialize()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CDynamicResponseProxy::GetSignature(TSerialize signature)
{
	signature.BeginGroup("DynamicResponseProxy");
	signature.EndGroup();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CDynamicResponseProxy::Serialize(TSerialize ser)
{

}

//////////////////////////////////////////////////////////////////////////
void CDynamicResponseProxy::SerializeXML(XmlNodeRef& entityNode, bool bLoading)
{

}

//////////////////////////////////////////////////////////////////////////
DRS::IVariableCollection* CDynamicResponseProxy::GetLocalVariableCollection() const
{
	CRY_ASSERT_MESSAGE(m_pResponseActor, "DRS Proxy without an Actor detected. Should never happen.");
	return m_pResponseActor->GetLocalVariables();
}

//////////////////////////////////////////////////////////////////////////
DRS::IResponseActor* CDynamicResponseProxy::GetResponseActor() const
{
	CRY_ASSERT_MESSAGE(m_pResponseActor, "DRS Proxy without an Actor detected. Should never happen.");
	return m_pResponseActor;
}
