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

class CEntityComponentTrackViewNode :
	public IEntityComponent
{
public:
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CEntityComponentTrackViewNode,"CEntityComponentTrackViewNode",0x60F18291C2A146F7,0xBA7F02F390C85BB2);
	
	virtual ~CEntityComponentTrackViewNode() {}

	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize() final;
	virtual void ProcessEvent(SEntityEvent& event) final;
	virtual uint64 GetEventMask() const final;
	//////////////////////////////////////////////////////////////////////////

	virtual void         GetMemoryUsage(ICrySizer* pSizer) const final {};
	virtual EEntityProxy GetProxyType() const final { return ENTITY_PROXY_ENTITYNODE; };

	virtual void         Release() final                                      { delete this; }
	virtual void         Update(SEntityUpdateContext& ctx) final {};

	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) final {};
};

#endif
