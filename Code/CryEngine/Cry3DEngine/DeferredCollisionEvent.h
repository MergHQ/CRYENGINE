// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryPhysics/IDeferredCollisionEvent.h>

// Implementation class for the DeferredPhysicsEvent Manager
class CDeferredPhysicsEventManager : public IDeferredPhysicsEventManager, public Cry3DEngineBase
{
public:
	virtual ~CDeferredPhysicsEventManager() {}

	virtual void                   DispatchDeferredEvent(IDeferredPhysicsEvent* pEvent);
	virtual int                    HandleEvent(const EventPhys* pEvent, IDeferredPhysicsEventManager::CreateEventFunc, IDeferredPhysicsEvent::DeferredEventType);

	virtual void                   RegisterDeferredEvent(IDeferredPhysicsEvent* pDeferredEvent);
	virtual void                   UnRegisterDeferredEvent(IDeferredPhysicsEvent* pDeferredEvent);

	virtual void                   ClearDeferredEvents();

	virtual void                   Update();

	virtual IDeferredPhysicsEvent* GetLastCollisionEventForEntity(IPhysicalEntity* pPhysEnt);

private:
	std::vector<IDeferredPhysicsEvent*> m_activeDeferredEvents; // list of all active deferred events, used for cleanup and statistics
	bool                                m_bEntitySystemReset;   // means all entity ptrs in events are stale
};
