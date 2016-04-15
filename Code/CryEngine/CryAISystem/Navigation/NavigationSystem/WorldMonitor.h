// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __WorldMonitor_h__
#define __WorldMonitor_h__

#pragma once

class WorldMonitor
{
public:
	typedef Functor1<const AABB&> Callback;

	WorldMonitor();
	WorldMonitor(const Callback& callback);

	void Start();
	void Stop();

	// - fires all AABB changes that were queued asynchronously
	// - should be called on a regular basis (i. e. typically on each frame)
	void FlushPendingAABBChanges();

protected:

	bool IsEnabled() const;

	Callback m_callback;

	bool     m_enabled;

private:
	// physics event handlers
	static int StateChangeHandler(const EventPhys* pPhysEvent);
	static int EntityRemovedHandler(const EventPhys* pPhysEvent);
	static int EntityRemovedHandlerAsync(const EventPhys* pPhysEvent);

	// - common code for EntityRemovedHandler() and EntityRemovedHandlerAsync()
	// - inspects the type of physics entity in given event, returns true if we're interested in this kind of entity and fills given AABB
	static bool ShallEventPhysEntityDeletedBeHandled(const EventPhys* pPhysEvent, AABB& outAabb);

	CryMT::vector<AABB> m_queuedAABBChanges;    // changes from the physical world that have been queued asynchronously; will be fired by FlushPendingAABBChanges()
};

#endif
