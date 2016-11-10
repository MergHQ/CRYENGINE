// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryCore/Platform/platform_impl.inl>
#include "IPlugin.h"
#include "EquipPack/EquipPackDialog.h"
#include "EquipPack/EquipPackLib.h"

// Force MFC to use this DllMain
#ifdef _USRDLL
extern "C" {
	int __afxForceUSRDLL;
}
#endif

IEditor* g_pEditor = NULL;

//DECLARE_PYTHON_MODULE(gamesdk);

class EditorGameSDK : public IPlugin
{
public:

	void Init()
	{
		RegisterPlugin();

		GetIEditor()->RegisterDeprecatedPropertyEditor(ePropertyEquip, 
			std::function<bool(const string&, string&)>([](const string& old_value, string& new_value)->bool
		{
			CEquipPackDialog dlg;
			dlg.SetCurrEquipPack(old_value.GetString());
			if (dlg.DoModal() == IDOK)
			{
				new_value = dlg.GetCurrEquipPack().GetString();
				return true;
			}
			return false;
		}));

		CEquipPackLib::GetRootEquipPack().LoadLibs(true);
	}

	void Release() override
	{
		UnregisterPlugin();
		delete this;
	}

	void        ShowAbout() override        {}
	const char* GetPluginGUID() override    { return ""; }
	DWORD       GetPluginVersion() override { return 1; }
	const char* GetPluginName() override    { return "EditorGameSDK"; }
	bool        CanExitNow() override       { return true; }
	void        OnEditorNotify(EEditorNotifyEvent aEventId) override {}
};

IEditor* GetIEditor()
{
	return g_pEditor;
}

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return 0;
	}
	EditorGameSDK* pEditorGameSDKPlugin = new EditorGameSDK;
	g_pEditor = pInitParam->pIEditor;
	ModuleInitISystem(g_pEditor->GetSystem(), "EditorGameSDK");
	pEditorGameSDKPlugin->Init();
	return pEditorGameSDKPlugin;
}
