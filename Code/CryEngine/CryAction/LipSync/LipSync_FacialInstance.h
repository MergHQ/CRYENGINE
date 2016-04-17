// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   LipSync_FacialInstance.h
//  Version:     v1.00
//  Created:     2014-08-29 by Christian Werle.
//  Description: Automatic start of facial animation when a sound is being played back.
//               Legacy version that uses CryAnimation's FacialInstance.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

class CLipSyncProvider_FacialInstance : public ILipSyncProvider
{
public:
	explicit CLipSyncProvider_FacialInstance(EntityId entityId);

	// ILipSyncProvider
	virtual void RequestLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) override;
	virtual void StartLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) override;
	virtual void PauseLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) override;
	virtual void UnpauseLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) override;
	virtual void StopLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) override;
	virtual void UpdateLipSync(IEntityAudioProxy* pProxy, const AudioControlId audioTriggerId, const ELipSyncMethod lipSyncMethod) override;
	// ~ILipSyncProvider

	void FullSerialize(TSerialize ser);
	void GetEntityPoolSignature(TSerialize signature);

private:
	void LipSyncWithSound(const AudioControlId audioTriggerId, bool bStop = false);
	EntityId m_entityId;
};
DECLARE_SHARED_POINTERS(CLipSyncProvider_FacialInstance);

class CLipSync_FacialInstance : public CGameObjectExtensionHelper<CLipSync_FacialInstance, IGameObjectExtension>
{
public:
	// IGameObjectExtension
	virtual void                 GetMemoryUsage(ICrySizer* pSizer) const override;
	virtual bool                 Init(IGameObject* pGameObject) override;
	virtual void                 PostInit(IGameObject* pGameObject) override;
	virtual void                 InitClient(int channelId) override;
	virtual void                 PostInitClient(int channelId) override;
	virtual bool                 ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
	virtual void                 PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
	virtual bool                 GetEntityPoolSignature(TSerialize signature) override;
	virtual void                 Release() override;
	virtual void                 FullSerialize(TSerialize ser) override;
	virtual bool                 NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override;
	virtual void                 PostSerialize() override;
	virtual void                 SerializeSpawnInfo(TSerialize ser) override;
	virtual ISerializableInfoPtr GetSpawnInfo() override;
	virtual void                 Update(SEntityUpdateContext& ctx, int updateSlot) override;
	virtual void                 HandleEvent(const SGameObjectEvent& event) override;
	virtual void                 ProcessEvent(SEntityEvent& event) override;
	virtual void                 SetChannelId(uint16 id) override;
	virtual void                 SetAuthority(bool auth) override;
	virtual void                 PostUpdate(float frameTime) override;
	virtual void                 PostRemoteSpawn() override;
	// ~IGameObjectExtension

private:
	void InjectLipSyncProvider();

private:
	CLipSyncProvider_FacialInstancePtr m_pLipSyncProvider;
};
