#pragma once

#include <CryRenderer/IRenderer.h>
#include <CrySystem/XML/IXml.h>
#include <CryMath/Cry_Math.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <IGameObject.h>

// Helper struct to avoid having to implement all IGameObjectExtension functions every time
struct ISimpleExtension : public IGameObjectExtension
{
	//IGameObjectExtension
	virtual bool                   Init(IGameObject* pGameObject) override                                                  { SetGameObject(pGameObject); return pGameObject->BindToNetwork(); }
	virtual void                   PostInit(IGameObject* pGameObject) override                                              {}
	virtual void                   HandleEvent(const SGameObjectEvent& event) override                                      {}
	virtual void                   ProcessEvent(SEntityEvent& event) override                                               {}
	virtual void                   InitClient(int channelId) override                                                       {}
	virtual void                   PostInitClient(int channelId) override                                                   {}
	virtual bool                   ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override     { return true; }
	virtual void                   PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override {}
	virtual bool                   GetEntityPoolSignature(TSerialize signature) override                                    { return false; }
	virtual void                   Release() override                                                                       { delete this; }
	virtual void                   FullSerialize(TSerialize ser) override                                                   {}
	virtual bool                   NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override  { return true; }
	virtual void                   PostSerialize() override                                                                 {}
	virtual void                   SerializeSpawnInfo(TSerialize ser) override                                              {}
	virtual ISerializableInfoPtr   GetSpawnInfo() override                                                                  { return nullptr; }
	virtual void                   Update(SEntityUpdateContext& ctx, int updateSlot) override                               {}
	virtual void                   SetChannelId(uint16 id) override                                                         {}
	virtual void                   SetAuthority(bool auth) override                                                         {}
	virtual void                   PostUpdate(float frameTime) override                                                     {}
	virtual void                   PostRemoteSpawn() override                                                               {}
	virtual void                   GetMemoryUsage(ICrySizer* pSizer) const override                                         {}
	virtual ComponentEventPriority GetEventPriority(const int eventID) const override                                       { return EEntityEventPriority_GameObject; }
	virtual const void*            GetRMIBase() const override                                                              { return nullptr; }
	//~IGameObjectExtension

	virtual ~ISimpleExtension() {}

	// Helper for loading geometry or characters
	void LoadMesh(int slot, const char* path)
	{
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

template<class T>
struct CObjectCreator : public IGameObjectExtensionCreatorBase
{
	IGameObjectExtensionPtr Create()
	{
		return ComponentCreate_DeleteWithRelease<T>();
	}

	void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
	{
		*ppRMI = nullptr;
		*nCount = 0;
	}
};

template<class T>
static void RegisterEntityWithDefaultComponent(const char* szName, const char* editorPath = "", const char* editorIcon = "")
{
	IEntityClassRegistry::SEntityClassDesc clsDesc;
	clsDesc.sName = szName;

	clsDesc.editorClassInfo.sCategory = editorPath;
	clsDesc.editorClassInfo.sIcon = editorIcon;

	static CObjectCreator<T> _creator;

	gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(szName, &_creator, &clsDesc);
}

struct IEntityRegistrator
{
	IEntityRegistrator()
	{
		if (g_pFirst == nullptr)
		{
			g_pFirst = this;
			g_pLast = this;
		}
		else
		{
			g_pLast->m_pNext = this;
			g_pLast = g_pLast->m_pNext;
		}
	}

	virtual void Register() = 0;

public:
	IEntityRegistrator*        m_pNext;

	static IEntityRegistrator* g_pFirst;
	static IEntityRegistrator* g_pLast;
};