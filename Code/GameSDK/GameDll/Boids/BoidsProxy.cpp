// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   BoidsProxy.cpp
//  Version:     v1.00
//  Created:     2/10/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <CryEntitySystem/IEntitySystem.h>
#include "BoidsProxy.h"
#include "Flock.h"
#include <CryNetwork/ISerialize.h>

//////////////////////////////////////////////////////////////////////////
CBoidsProxy::CBoidsProxy()
	: m_pEntity(NULL)
	, m_pFlock(NULL)
	, m_playersInCount(0)
{
}

//////////////////////////////////////////////////////////////////////////
CBoidsProxy::~CBoidsProxy()
{
	if (m_pFlock)
		delete m_pFlock;
}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::Initialize( const SComponentInitializer& init )
{
	m_pEntity = init.m_pEntity;

	// Make sure render and trigger proxy also exist.
	m_pEntity->CreateProxy( ENTITY_PROXY_RENDER );
}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::Reload( IEntity *pEntity,SEntitySpawnParams &params )
{
	m_pEntity = pEntity;

	if (m_pFlock)
		m_pFlock->SetPos(pEntity->GetWorldPos());
	
	// Make sure render and trigger proxy also exist.
	pEntity->CreateProxy( ENTITY_PROXY_RENDER );
	m_playersInCount = 0;
}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::Release()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::Update( SEntityUpdateContext &ctx )
{
	if (m_pFlock)
		m_pFlock->Update( ctx.pCamera );
}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::ProcessEvent( SEntityEvent &event )
{
	switch (event.event) {
	case ENTITY_EVENT_XFORM:
		if (m_pFlock)
			m_pFlock->SetPos( m_pEntity->GetWorldPos() );
		break;
	case ENTITY_EVENT_PRE_SERIALIZE:
		if (m_pFlock)
			m_pFlock->DeleteEntities(true);
		break;
	case ENTITY_EVENT_ENTERAREA:
		OnTrigger(true,event);
		break;
	case ENTITY_EVENT_LEAVEAREA:
		OnTrigger(false,event);
		break;
	case ENTITY_EVENT_RESET:
		if (m_pFlock)
			m_pFlock->Reset();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBoidsProxy::GetSignature( TSerialize signature )
{
	signature.BeginGroup("BoidsProxy");
	if (m_pFlock)
	{
		uint32 type = (uint32)m_pFlock->GetType();
		signature.Value("type", type);
	}
	signature.EndGroup();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::Serialize( TSerialize ser )
{
}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::SetFlock( CFlock *pFlock )
{
	m_pFlock = pFlock;

	if (!pFlock)
		return;
	
	// Update trigger based on new visibility distance settings.
	float fMaxDist = pFlock->GetMaxVisibilityDistance();

	/*
	CTriggerProxy *pTrigger = (CTriggerProxy*)m_pEntity->CreateProxy( ENTITY_PROXY_TRIGGER );
	if (!pTrigger)
		return;

	AABB bbox;
	bbox.min = -Vec3(fMaxDist,fMaxDist,fMaxDist);
	bbox.max = Vec3(fMaxDist,fMaxDist,fMaxDist);
	pTrigger->SetTriggerBounds( bbox );
	*/

	IEntityAreaProxyPtr pArea = crycomponent_cast<IEntityAreaProxyPtr>(m_pEntity->CreateProxy( ENTITY_PROXY_AREA ));
	if (!pArea)
		return;

	pArea->SetFlags( pArea->GetFlags() & IEntityAreaProxy::FLAG_NOT_SERIALIZE );
	pArea->SetSphere( Vec3(0,0,0),fMaxDist );
	if(gEnv->pEntitySystem->EntitiesUseGUIDs())
		pArea->AddEntity( m_pEntity->GetGuid() );
	else
		pArea->AddEntity( m_pEntity->GetId() ); // add itself.

}

//////////////////////////////////////////////////////////////////////////
void CBoidsProxy::OnTrigger( bool bEnter,SEntityEvent &event )
{
	EntityId whoId = (EntityId)event.nParam[0];
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(whoId);

	if (pEntity)
	{
		if (pEntity->GetFlags() & ENTITY_FLAG_LOCAL_PLAYER)
		{
			if (bEnter)
				m_playersInCount++;
			else
				m_playersInCount--;

			if (m_playersInCount == 1)
			{
				// Activates entity when player is nearby.
				m_pEntity->Activate(true);
				if (m_pFlock)
					m_pFlock->SetEnabled(true);
			}
			if (m_playersInCount <= 0)
			{
				// Activates entity when player is nearby.
				m_playersInCount = 0;
				m_pEntity->Activate(false);
				if (m_pFlock)
					m_pFlock->SetEnabled(false);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CBoidObjectProxy::CBoidObjectProxy()
	: m_pBoid(NULL)
	, m_pEntity(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
CBoidObjectProxy::~CBoidObjectProxy()
{
}

//////////////////////////////////////////////////////////////////////////
void CBoidObjectProxy::Initialize( const SComponentInitializer& init )
{
	m_pEntity = init.m_pEntity;
}

//////////////////////////////////////////////////////////////////////////
void CBoidObjectProxy::ProcessEvent( SEntityEvent &event )
{
	if (m_pBoid)
		m_pBoid->OnEntityEvent( event );
}

///////////////////////////////////////////////////////////78//////////////
bool CBoidObjectProxy::GetSignature( TSerialize signature )
{
	signature.BeginGroup("BoidObjectProxy");
	if (m_pBoid && m_pBoid->m_flock)
	{
		uint32 type = (uint32)m_pBoid->m_flock->GetType();
		signature.Value("type", type);
	}
	signature.EndGroup();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CBoidObjectProxy::Serialize( TSerialize ser )
{

}
