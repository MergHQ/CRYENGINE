// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created          : 13/05/2014 by Jean Geffroy
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MANNEQUIN_OBJECT_H__
#define __MANNEQUIN_OBJECT_H__

#include <IGameObject.h>
#include "IAnimationGraph.h"

class CMannequinObject
	: public CGameObjectExtensionHelper<CMannequinObject, IGameObjectExtension>
{
public:
	CMannequinObject();
	virtual ~CMannequinObject();

	// IGameObjectExtension
	virtual bool                 Init(IGameObject* pGameObject);
	virtual void                 InitClient(int channelId)                                                       {}
	virtual void                 PostInit(IGameObject* pGameObject);
	virtual void                 PostInitClient(int channelId)                                                   {}
	virtual bool                 ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params)     { return false; }
	virtual void                 PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) {}
	virtual bool                 GetEntityPoolSignature(TSerialize signature)                                    { return false; }
	virtual void                 Release()                                                                       { delete this; }
	virtual void                 FullSerialize(TSerialize ser)                                                   {}
	virtual bool                 NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)   { return true; }
	virtual void                 PostSerialize()                                                                 {}
	virtual void                 SerializeSpawnInfo(TSerialize ser)                                              {}
	virtual ISerializableInfoPtr GetSpawnInfo()                                                                  { return 0; }
	virtual void                 Update(SEntityUpdateContext& ctx, int updateSlot)                               {}
	virtual void                 PostUpdate(float frameTime)                                                     {}
	virtual void                 PostRemoteSpawn()                                                               {}
	virtual void                 HandleEvent(const SGameObjectEvent& event)                                      {}
	virtual void                 ProcessEvent(SEntityEvent& event);
	virtual void                 SetChannelId(uint16 id)                                                         {}
	virtual void                 SetAuthority(bool auth)                                                         {}
	virtual void                 GetMemoryUsage(ICrySizer* s) const                                              { s->Add(*this); }
	// ~IGameObjectExtension

protected:
	void Reset();
	void OnScriptEvent(const char* eventName);

private:
	IAnimatedCharacter* m_pAnimatedCharacter;
};

#endif
