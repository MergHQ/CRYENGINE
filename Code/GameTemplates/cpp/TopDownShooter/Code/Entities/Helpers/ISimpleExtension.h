#pragma once

#include <IGameObject.h>

// Helper struct to avoid having to implement all IGameObjectExtension functions every time
struct ISimpleExtension : public IGameObjectExtension
{
	//IGameObjectExtension
	virtual bool Init(IGameObject* pGameObject) override {SetGameObject(pGameObject); return pGameObject->BindToNetwork();}
	virtual void PostInit(IGameObject* pGameObject) override {}
	virtual void HandleEvent(const SGameObjectEvent& event) override {}
	virtual void ProcessEvent(SEntityEvent& event) override {}
	virtual void InitClient(int channelId) override {}
	virtual void PostInitClient(int channelId) override {}
	virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override {return true;}
	virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override {}
	virtual bool GetEntityPoolSignature(TSerialize signature) override {return false;}
	virtual void Release() override {delete this;}
	virtual void FullSerialize(TSerialize ser) override {}
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override {return true;}
	virtual void PostSerialize() override {}
	virtual void SerializeSpawnInfo(TSerialize ser) override {}
	virtual ISerializableInfoPtr GetSpawnInfo() override {return nullptr;}
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) override {}
	virtual void SetChannelId(uint16 id) override {}
	virtual void SetAuthority(bool auth) override {}
	virtual void PostUpdate(float frameTime) override {}
	virtual void PostRemoteSpawn() override {}
	virtual void GetMemoryUsage(ICrySizer* pSizer) const override {}
	virtual ComponentEventPriority GetEventPriority(const int eventID) const override {return EEntityEventPriority_GameObject;}
	//~IGameObjectExtension

	virtual ~ISimpleExtension() {}

	// Helper for loading geometry or characters
	void LoadMesh(int slot, const char *path)
	{
		// Load the rifle geometry into the first slot
		if (IsCharacterFile(path))
		{
			GetEntity()->LoadCharacter(slot, path);
		}
		else
		{
			GetEntity()->LoadGeometry(slot, path);
		}
	}
};