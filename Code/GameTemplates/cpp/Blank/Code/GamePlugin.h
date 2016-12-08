#pragma once

#include <CrySystem/ICryPlugin.h>

#include <CryGame/IGameFramework.h>

#include <CryEntitySystem/IEntityClass.h>

class CGamePlugin 
	: public ICryPlugin
	, public ISystemEventListener
{
public:
	CRYINTERFACE_SIMPLE(ICryPlugin)
	CRYGENERATE_SINGLETONCLASS(CGamePlugin, "Game_Blank", 0xF01244B0A4E74DC6, 0x91E10ED18906FE7C)

	virtual ~CGamePlugin();
	
	//! Retrieve name of plugin.
	virtual const char* GetName() const override { return "BlankGamePlugin"; }

	//! Retrieve category for the plugin.
	virtual const char* GetCategory() const override { return "Game"; }

	//! This is called to initialize the new plugin.
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;

	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {}

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

public:
	template<class T>
	struct CObjectCreator : public IGameObjectExtensionCreatorBase
	{
		IGameObjectExtension* Create(IEntity* pEntity)
		{
			return pEntity->CreateComponentClass<T>();
		}

		void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
		{
			T::GetGameObjectExtensionRMIData(ppRMI, nCount);
		}
	};

	template<class T>
	static void RegisterEntityWithDefaultComponent(const char *name)
	{
		IEntityClassRegistry::SEntityClassDesc clsDesc;
		clsDesc.sName = name;

		static CObjectCreator<T> _creator;

		gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(name, &_creator, &clsDesc);
		T::SetExtensionId(gEnv->pGameFramework->GetIGameObjectSystem()->GetID(name));
	}

	template<class T>
	static void RegisterEntityComponent(const char *name)
	{
		static CObjectCreator<T> _creator;

		gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(name, &_creator, nullptr);
		T::SetExtensionId(gEnv->pGameFramework->GetIGameObjectSystem()->GetID(name));
	}
};

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
	virtual void Unregister() = 0;

public:
	IEntityRegistrator *m_pNext;

	static IEntityRegistrator *g_pFirst;
	static IEntityRegistrator *g_pLast;
};