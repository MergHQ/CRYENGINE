// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryCore/Platform/platform_impl.inl>
#include "IEditorClassFactory.h"
#include "ICommandManager.h"
#include "IPlugin.h"
#include "IResourceSelectorHost.h"
#include "AnimationCompressionManager.h"
#include "CharacterTool/CharacterToolForm.h"
#include "QtViewPane.h"

// just for CGFContent:
#include <CryRenderer/VertexFormats.h>
#include <CryMath/Cry_Geo.h>
#include <CryCore/TypeInfo_impl.h>
#include <CryCore/Common_TypeInfo.h>
#include <Cry3DEngine/IIndexedMesh_info.h>
#include <Cry3DEngine/CGF/CGFContent_info.h>
// ^^^

#include "Serialization.h"

#include "CharacterTool/CharacterToolForm.h"
#include "CharacterTool/CharacterToolSystem.h"

static IEditor* g_pEditor;

void Log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	g_pEditor->GetSystem()->GetILog()->LogV(ILog::eAlways, format, args);
	va_end(args);
}

IEditor* GetIEditor()
{
	return g_pEditor;
}

CharacterTool::System* g_pCharacterToolSystem;

// ---------------------------------------------------------------------------

class CEditorAnimationPlugin : public IPlugin
{
public:
	CEditorAnimationPlugin(IEditor* pEditor)
	{
	}

	void Init()
	{
		RegisterPlugin();
		RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());

		g_pCharacterToolSystem = new CharacterTool::System();
		g_pCharacterToolSystem->Initialize();

		const ICVar* pUseImgCafCVar = gEnv->pConsole->GetCVar("ca_UseIMG_CAF");
		const bool useImgCafSet = (pUseImgCafCVar && pUseImgCafCVar->GetIVal());
		if (useImgCafSet)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "[EditorAnimation] Animation compilation disabled: 'ca_UseIMG_CAF' should be set to zero at load time for compilation to work.");
		}
		else
		{
			g_pCharacterToolSystem->animationCompressionManager.reset(new CAnimationCompressionManager());
		}
	}

	void Release()
	{
		if (g_pCharacterToolSystem)
		{
			delete g_pCharacterToolSystem;
			g_pCharacterToolSystem = 0;
		}

		UnregisterPlugin();
		delete this;
	}

	// implements IEditorNotifyListener
	void OnEditorNotify(EEditorNotifyEvent event)
	{
		switch (event)
		{
		case eNotify_OnInit:
			break;
		case eNotify_OnSelectionChange:
		case eNotify_OnEndNewScene:
		case eNotify_OnEndSceneOpen:
			break;
		}
	}

	// implements IPlugin
	void        ShowAbout()                               {}
	const char* GetPluginGUID()                           { return 0; }
	DWORD       GetPluginVersion()                        { return 0x01; }
	const char* GetPluginName()                           { return "EditorPhysics"; }
	bool        CanExitNow()                              { return true; }
	void        Serialize(FILE* hFile, bool bIsStoring)   {}
	void        ResetContent()                            {}
	bool        CreateUIElements()                        { return false; }
	bool        ExportDataToGame(const char* pszGamePath) { return false; }
};

//////////////////////////////////////////////////////////////////////////-

extern "C" IMAGE_DOS_HEADER __ImageBase;
HINSTANCE g_hInstance = (HINSTANCE)&__ImageBase;

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	g_pEditor = pInitParam->pIEditor;
	ModuleInitISystem(g_pEditor->GetSystem(), "EditorAnimation");

	CEditorAnimationPlugin* pPlugin = new CEditorAnimationPlugin(pInitParam->pIEditor);

	pPlugin->Init();

	return pPlugin;
}
