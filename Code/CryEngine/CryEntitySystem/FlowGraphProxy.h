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

#ifndef __FlowGraphProxy_h__
#define __FlowGraphProxy_h__
#pragma once

#include <CryNetwork/ISerialize.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Handles sounds in the entity.
//////////////////////////////////////////////////////////////////////////
class CFlowGraphProxy : public IEntityFlowGraphProxy
{
public:
	CFlowGraphProxy();
	~CFlowGraphProxy();

	CEntity* GetEntity() const { return m_pEntity; };

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize(const SComponentInitializer& init);
	virtual void ProcessEvent(SEntityEvent& event);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType() { return ENTITY_PROXY_FLOWGRAPH; }
	virtual void         Release() { delete this; };
	virtual void         Done();
	virtual void         Update(SEntityUpdateContext& ctx);
	virtual bool         Init(IEntity* pEntity, SEntitySpawnParams& params) { return true; }
	virtual void         Reload(IEntity* pEntity, SEntitySpawnParams& params);
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading);
	virtual void         Serialize(TSerialize ser);
	virtual bool         NeedSerialize();
	virtual bool         GetSignature(TSerialize signature);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityFlowGraphProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void        SetFlowGraph(IFlowGraph* pFlowGraph);
	virtual IFlowGraph* GetFlowGraph();
	virtual void        AddEventListener(IEntityEventListener* pListener);
	virtual void        RemoveEventListener(IEntityEventListener* pListener);
	//////////////////////////////////////////////////////////////////////////

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	void OnMove();

private:
	//////////////////////////////////////////////////////////////////////////
	// Private member variables.
	//////////////////////////////////////////////////////////////////////////
	// Host entity.
	CEntity*    m_pEntity;
	IFlowGraph* m_pFlowGraph;

	typedef std::list<IEntityEventListener*> Listeners;
	Listeners m_listeners;
};

#endif // __FlowGraphProxy_h__
