// Copyright 2001-2019 Crytek GmbH. All rights reserved.

#include "StdAfx.h"

#include <CryCore/Platform/platform_impl.inl>

#include <IPlugin.h>
#include <IEditorClassFactory.h>

#include <CryNetwork/ISimpleHttpServer.h>
#include <IEditor.h>

#include "Module/CryLinkService.h"

// Force MFC to use this DllMain
#ifdef _USRDLL
extern "C" { int __afxForceUSRDLL; }
#endif

IEditor* g_pEditor;
IEditor* GetIEditor() { return g_pEditor; }

class CCryLinkEditorPlugin : public IPlugin
{
	enum { version = 1 };

public:
	CCryLinkEditorPlugin(CryLinkService::ICryLinkService &cryLinkService)
		: m_cryLinkService(cryLinkService)
	{}
	~CCryLinkEditorPlugin()
	{}

	bool Init()
	{
		if(IEditor* pEditor = GetIEditor())
		{
			if(ISimpleHttpServer* pHttpServer = pEditor->GetSystem()->GetINetwork()->GetSimpleHttpServerSingleton())
			{
				m_cryLinkService.Start(*pHttpServer);
				return true;
			}
		}

		return false;
	}

	// IPlugin
	virtual void Release() override
	{
		delete this;
	}

	virtual void ShowAbout() override {}
	virtual const char* GetPluginGUID() override { return "{87A5FA84-36F7-4EFD-80F1-5F6D6FEFB089}"; }
	virtual DWORD GetPluginVersion() override { return static_cast<DWORD>(version); }
	virtual const char* GetPluginName() override { return "CryLink"; }
	virtual bool CanExitNow() override { return true; }
	virtual void OnEditorNotify(EEditorNotifyEvent eventId) override 
	{
		switch(eventId)
		{
		case EEditorNotifyEvent::eNotify_OnBeginGameMode:
			{
				m_cryLinkService.Pause(true);
			}
			break;

		case EEditorNotifyEvent::eNotify_OnEndGameMode:
			{
				m_cryLinkService.Pause(false);
			}
			break;
		case EEditorNotifyEvent::eNotify_OnIdleUpdate:
			{
				m_cryLinkService.Update();
			}
			break;

		default:
			return;
		}
	}
	// ~IPlugin

private:
	CryLinkService::ICryLinkService &m_cryLinkService;
};

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if(pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return NULL;
	}

	g_pEditor = pInitParam->pIEditor;
	if(ISystem *pSystem = pInitParam->pIEditor->GetSystem())
	{
		ModuleInitISystem(pSystem, "CryLink");

		if(CryLinkService::IFramework* pFramework = gEnv->pGameFramework->QueryExtension<CryLinkService::IFramework>())
		{
			CCryLinkEditorPlugin* pPlugin = new CCryLinkEditorPlugin(pFramework->GetService());
			if(pPlugin->Init())
			{
				return pPlugin;
			}
			else
			{
				pPlugin->Release();
			}
		}
	}
	return nullptr;
}

HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hinstDLL;
	}

	return TRUE;
}
