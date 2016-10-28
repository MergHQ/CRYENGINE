// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#define VC_EXTRALEAN
#include <afxwin.h>
#include <afxext.h>

#include <CryCore/Platform/platform.h>
#include <CryCore/Platform/platform_impl.inl>

#include <IEditor.h>
#include "IPlugin.h"
#include "IEditorClassFactory.h"

#include "../EditorCommon/QtViewPane.h"

#include "Editor/MainEditorWindow.h"

// Force MFC to use this DllMain
// Fixes mfcs110d.lib(dllmodul.obj) : error LNK2005: DllMain already defined
#ifdef _USRDLL
extern "C" {
	int __afxForceUSRDLL;
}
#endif

IEditor* g_pEditor;
IEditor* GetIEditor() { return g_pEditor; }

#define UQS_EDITOR_NAME "UQS Query History Inspector"

REGISTER_VIEWPANE_FACTORY(CMainEditorWindow, UQS_EDITOR_NAME, "Game", true)

class CUqsEditorPlugin : public IPlugin
{
	enum
	{
		Version = 1,
	};

public:
	CUqsEditorPlugin()
	{
		RegisterPlugin();
	}

	~CUqsEditorPlugin()
	{
		UnregisterPlugin();
	}

	bool Init(IEditor* pEditor)
	{
		return true;
	}

	// IPlugin

	virtual void Release() override
	{
		delete this;
	}

	virtual void        ShowAbout() override                                 {}
	virtual const char* GetPluginGUID() override                             { return "{37D8BA31-2926-4F48-9C39-AF3E8B04B52B}"; }
	virtual DWORD       GetPluginVersion() override                          { return DWORD(Version); }
	virtual const char* GetPluginName() override                             { return UQS_EDITOR_NAME; }
	virtual bool        CanExitNow() override                                { return true; }
	virtual void        OnEditorNotify(EEditorNotifyEvent aEventId) override {}

	// ~IPlugin
};

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	g_pEditor = pInitParam->pIEditor;

	ISystem* pSystem = pInitParam->pIEditor->GetSystem();
	ModuleInitISystem(pSystem, "UQS Query History Inspector");

	CUqsEditorPlugin* pPlugin = new CUqsEditorPlugin();
	if (pPlugin->Init(g_pEditor))
	{
		return pPlugin;
	}
	else
	{
		pPlugin->Release();
		return nullptr;
	}
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
