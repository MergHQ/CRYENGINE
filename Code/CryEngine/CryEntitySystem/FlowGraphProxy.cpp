// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   FlowGraphProxy.h
//  Version:     v1.00
//  Created:     6/6/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FlowGraphProxy.h"
#include "Entity.h"
#include <CryFlowGraph/IFlowSystem.h>
#include <CryNetwork/ISerialize.h>

//////////////////////////////////////////////////////////////////////////
CFlowGraphProxy::CFlowGraphProxy()
{
	m_pEntity = NULL;
	m_pFlowGraph = 0;
}

//////////////////////////////////////////////////////////////////////////
CFlowGraphProxy::~CFlowGraphProxy()
{
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphProxy::Initialize(const SComponentInitializer& init)
{
	m_pEntity = (CEntity*)init.m_pEntity;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphProxy::Done()
{
	if (m_pFlowGraph)
		m_pFlowGraph->Release();
	m_pFlowGraph = 0;
};

//////////////////////////////////////////////////////////////////////////
void CFlowGraphProxy::SetFlowGraph(IFlowGraph* pFlowGraph)
{
	if (m_pFlowGraph)
		m_pFlowGraph->Release();
	m_pFlowGraph = pFlowGraph;
	if (m_pFlowGraph)
		m_pFlowGraph->AddRef();
}

//////////////////////////////////////////////////////////////////////////
IFlowGraph* CFlowGraphProxy::GetFlowGraph()
{
	return m_pFlowGraph;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphProxy::ProcessEvent(SEntityEvent& event)
{
	// Assumes only 1 current listener can be deleted as a result of the event.
	Listeners::iterator next;
	Listeners::iterator it = m_listeners.begin();
	while (it != m_listeners.end())
	{
		next = it;
		++next;
		(*it)->OnEntityEvent(m_pEntity, event);
		it = next;
	}
	// respond to entity activation/deactivation. enable/disable flowgraph
	switch (event.event)
	{
	case ENTITY_EVENT_INIT:
	case ENTITY_EVENT_DONE:
		{
			if (m_pFlowGraph)
			{
				const bool bActive = (event.event == ENTITY_EVENT_INIT) ? true : false;
				const bool bIsActive = m_pFlowGraph->IsActive();
				if (bActive != bIsActive)
					m_pFlowGraph->SetActive(bActive);
			}
		}
		break;
	case ENTITY_EVENT_POST_SERIALIZE:
		if (m_pFlowGraph)
			m_pFlowGraph->PostSerialize();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphProxy::AddEventListener(IEntityEventListener* pListener)
{
	// Does not check uniquiness due to performance reasons.
	m_listeners.push_back(pListener);
}

void CFlowGraphProxy::RemoveEventListener(IEntityEventListener* pListener)
{
	stl::find_and_erase(m_listeners, pListener);
}

void CFlowGraphProxy::Update(SEntityUpdateContext& ctx)
{
	//	if (m_pFlowGraph)
	//		m_pFlowGraph->Update();
}

void CFlowGraphProxy::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
	m_pEntity = (CEntity*)pEntity;

	SAFE_RELEASE(m_pFlowGraph);

	m_listeners.clear();
}

void CFlowGraphProxy::SerializeXML(XmlNodeRef& entityNode, bool bLoading)
{
	// don't serialize flowgraphs from the editor
	// (editor needs more information, so it saves them specially)
	if (gEnv->IsEditor() && !bLoading)
		return;

	XmlNodeRef flowGraphNode;
	if (bLoading)
	{
		flowGraphNode = entityNode->findChild("FlowGraph");
		if (!flowGraphNode)
			return;

		// prevents loading of disabled flowgraphs for optimization
		// only in game mode, of course
#if 1
		bool bEnabled = true;
		if (flowGraphNode->getAttr("enabled", bEnabled) && bEnabled == false)
		{
			EntityWarning("[FlowGraphProxy] Skipped loading of FG for Entity %d '%s'. FG was disabled on export.", m_pEntity->GetId(), m_pEntity->GetName());
			return;
		}
#endif

		enum EMultiPlayerType
		{
			eMPT_ServerOnly = 0,
			eMPT_ClientOnly,
			eMPT_ClientServer
		};

		EMultiPlayerType mpType = eMPT_ServerOnly;
		const char* mpTypeAttr = flowGraphNode->getAttr("MultiPlayer");

		if (mpTypeAttr)
		{
			if (strcmp("ClientOnly", mpTypeAttr) == 0)
				mpType = eMPT_ClientOnly;
			else if (strcmp("ClientServer", mpTypeAttr) == 0)
				mpType = eMPT_ClientServer;
		}

		const bool bIsServer = gEnv->bServer;
		const bool bIsClient = gEnv->IsClient();

		if (mpType == eMPT_ServerOnly && !bIsServer)
			return;
		else if (mpType == eMPT_ClientOnly && !bIsClient)
			return;

		if (gEnv->pFlowSystem)
		{
			SetFlowGraph(gEnv->pFlowSystem->CreateFlowGraph());
		}
	}
	else
	{
		flowGraphNode = entityNode->createNode("FlowGraph");
		entityNode->addChild(flowGraphNode);
	}
	assert(!!flowGraphNode);
	if (m_pFlowGraph)
	{
		m_pFlowGraph->SetGraphEntity(m_pEntity->GetId());
		m_pFlowGraph->SerializeXML(flowGraphNode, bLoading);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphProxy::NeedSerialize()
{
	return m_pFlowGraph != 0;
};

//////////////////////////////////////////////////////////////////////////
bool CFlowGraphProxy::GetSignature(TSerialize signature)
{
	signature.BeginGroup("FlowGraphProxy");
	signature.EndGroup();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphProxy::Serialize(TSerialize ser)
{
	bool hasFlowGraph = m_pFlowGraph != 0;
	if (ser.BeginOptionalGroup("FlowGraph", hasFlowGraph))
	{
		if (ser.IsReading() && m_pFlowGraph == 0)
		{
			EntityWarning("Unable to reload flowgraph here");
			return;
		}

		if (m_pFlowGraph)
			m_pFlowGraph->Serialize(ser);

		ser.EndGroup();
	}
}
