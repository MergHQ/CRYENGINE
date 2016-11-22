// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   TriggerProxy.h
//  Version:     v1.00
//  Created:     5/12/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TriggerProxy_h__
#define __TriggerProxy_h__
#pragma once

#include "Entity.h"
#include "EntitySystem.h"

struct SProximityElement;

//////////////////////////////////////////////////////////////////////////
// Description:
//    Handles sounds in the entity.
//////////////////////////////////////////////////////////////////////////
class CEntityComponentTriggerBounds : public IEntityTriggerComponent
{
	CRY_ENTITY_COMPONENT_CLASS(CEntityComponentTriggerBounds,IEntityTriggerComponent,"CEntityComponentTriggerBounds",0x1C58115AA18E446E,0x8E82B3B4C6DD6F55);

public:
	//CTriggerProxy();
	//~CTriggerProxy();

	CEntity* GetEntity() const { return m_pEntity; };

	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize(const SComponentInitializer& init) final;
	virtual void ProcessEvent(SEntityEvent& event) final;
	virtual uint64 GetEventMask() const final { return BIT64(ENTITY_EVENT_XFORM)|BIT64(ENTITY_EVENT_ENTERAREA)|BIT64(ENTITY_EVENT_LEAVEAREA); };
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetProxyType() const final { return ENTITY_PROXY_TRIGGER; }
	virtual void         Release() final { delete this; };
	virtual void         Update(SEntityUpdateContext& ctx) final;
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) final {};
	virtual void         GameSerialize(TSerialize ser) final;
	virtual bool         NeedGameSerialize() final;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityTriggerComponent
	//////////////////////////////////////////////////////////////////////////
	virtual void SetTriggerBounds(const AABB& bbox) final { SetAABB(bbox); };
	virtual void GetTriggerBounds(AABB& bbox) final { bbox = m_aabb; };
	virtual void ForwardEventsTo(EntityId id) final { m_forwardingEntity = id; }
	virtual void InvalidateTrigger() final;
	//////////////////////////////////////////////////////////////////////////

	const AABB&              GetAABB() const { return m_aabb; }
	void                     SetAABB(const AABB& aabb);

	CProximityTriggerSystem* GetTriggerSystem() { return m_pEntity->GetCEntitySystem()->GetProximityTriggerSystem(); }

	virtual void             GetMemoryUsage(ICrySizer* pSizer) const final
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	void OnMove(bool invalidateAABB = false);
	void Reset();

private:
	//////////////////////////////////////////////////////////////////////////
	// Private member variables.
	//////////////////////////////////////////////////////////////////////////
	// Host entity.
	CEntity*           m_pEntity;
	AABB               m_aabb;
	SProximityElement* m_pProximityTrigger;
	EntityId           m_forwardingEntity;
};

#endif // __TriggerProxy_h__
