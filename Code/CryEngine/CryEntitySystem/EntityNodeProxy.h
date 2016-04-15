// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   EntityNodeProxy.h
//  Version:     v1.00
//  Created:     23/11/2010 by Benjamin B.
//  Description:
// -------------------------------------------------------------------------
//  History:	The EntityNodeProxy handles events that are specific to EntityNodes
//						(e.g. Footsteps).
//
////////////////////////////////////////////////////////////////////////////

#ifndef __EntityNodeProxy_h__
#define __EntityNodeProxy_h__
#pragma once

#include "EntitySystem.h"
#include <CryNetwork/ISerialize.h>

class CEntityNodeProxy :
	public IEntityProxy
{
public:
	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize(const SComponentInitializer& init);
	virtual void ProcessEvent(SEntityEvent& event);
	//////////////////////////////////////////////////////////////////////////

	void         GetMemoryUsage(ICrySizer* pSizer) const              {};
	EEntityProxy GetType()                                            { return ENTITY_PROXY_ENTITYNODE; };

	bool         Init(IEntity* pEntity, SEntitySpawnParams& params)   { m_pEntity = pEntity; return true; };
	void         Reload(IEntity* pEntity, SEntitySpawnParams& params) { m_pEntity = pEntity; };

	void         Done()                                               {};
	void         Release()                                            { delete this; }
	void         Update(SEntityUpdateContext& ctx)                    {};

	void         SerializeXML(XmlNodeRef& entityNode, bool bLoading)  {};
	void         Serialize(TSerialize ser)                            {};
	bool         NeedSerialize()                                      { return false; };
	bool         GetSignature(TSerialize signature)                   { return true; };

private:
	IEntity* m_pEntity;
};

#endif
