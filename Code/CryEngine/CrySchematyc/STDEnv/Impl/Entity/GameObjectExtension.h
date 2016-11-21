// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameObject.h>

#include "Entity/EntityObject.h"

namespace Schematyc
{
class CGameObjectExtension : public INetworkObject, public CGameObjectExtensionHelper<CGameObjectExtension, IGameObjectExtension>
{
public:

	// INetworkObject
	virtual bool   RegisterSerializeCallback(int32 aspects, const NetworkSerializeCallback& callback, CConnectionScope& connectionScope) override;
	virtual void   MarkAspectsDirty(int32 aspects) override;
	virtual uint16 GetChannelId() const override;
	virtual bool   ServerAuthority() const override;
	virtual bool   ClientAuthority() const override;
	virtual bool   LocalAuthority() const override;
	// ~INetworkObject

	// IGameObjectExtension
	virtual bool                 Init(IGameObject* pGameObject) override;
	virtual void                 InitClient(int channelId) override;
	virtual void                 PostInit(IGameObject* pGameObject) override;
	virtual void                 PostInitClient(int channelId) override;
	virtual bool                 ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& spawnParams) override;
	virtual void                 PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& spawnParams) override;
	virtual bool                 GetEntityPoolSignature(TSerialize signature) override;
	virtual void                 Release() override;
	virtual void                 FullSerialize(TSerialize serialize) override;
	virtual bool                 NetSerialize(TSerialize serialize, EEntityAspects aspects, uint8 profile, int flags) override;
	virtual void                 PostSerialize() override;
	virtual void                 SerializeSpawnInfo(TSerialize serialize) override;
	virtual ISerializableInfoPtr GetSpawnInfo() override;
	virtual void                 Update(SEntityUpdateContext& context, int updateSlot) override;
	virtual void                 PostUpdate(float frameTime) override;
	virtual void                 PostRemoteSpawn() override;
	virtual void                 ProcessEvent(SEntityEvent& event) override;
	virtual void                 HandleEvent(const SGameObjectEvent& event) override;
	virtual void                 SetChannelId(uint16 channelId) override;
	virtual void                 SetAuthority(bool localAuthority) override;
	virtual void                 GetMemoryUsage(ICrySizer* pSizer) const override;
	// ~IGameObjectExtension

public:

	static const char* ms_szExtensionName;

private:

	CEntityObject m_entityObject;
};
} // Schematyc
