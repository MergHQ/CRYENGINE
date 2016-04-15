// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ImplementationManager.h"
#include <CrySystem/IConsole.h>
#include <IAudioSystemEditor.h>
#include <IEditor.h>
#include "ATLControlsModel.h"
#include "AudioControlsEditorPlugin.h"

using namespace ACE;
const string g_sImplementationCVarName = "s_AudioImplName";

typedef void (WINAPI *         PGNSI)();
typedef IAudioSystemEditor* (* TPfnGetAudioInterface)(ISystem*);

CImplementationManager::CImplementationManager()
	: ms_hMiddlewarePlugin(nullptr)
	, ms_pAudioSystemImpl(nullptr)
{
}

CImplementationManager::~CImplementationManager()
{
	if (ms_hMiddlewarePlugin)
	{
		FreeLibrary(ms_hMiddlewarePlugin);
		ms_hMiddlewarePlugin = nullptr;
	}

	ms_pAudioSystemImpl = nullptr;
}

bool CImplementationManager::LoadImplementation()
{
	CATLControlsModel* pATLModel = CAudioControlsEditorPlugin::GetATLModel();
	ICVar* pCVar = gEnv->pConsole->GetCVar(g_sImplementationCVarName);
	if (pCVar && pATLModel)
	{
		pATLModel->ClearAllConnections();
		if (ms_hMiddlewarePlugin)
		{
			// Need to flush the undo/redo queue to make sure we're not keeping data from
			// previous implementation there
			GetIEditor()->FlushUndo(false);

			FreeLibrary(ms_hMiddlewarePlugin);
			ms_hMiddlewarePlugin = nullptr;
			ms_pAudioSystemImpl = nullptr;
		}

		char szExecutableDirPath[_MAX_PATH];
		CryGetExecutableFolder(sizeof(szExecutableDirPath), szExecutableDirPath);
#ifdef SANDBOX_QT
		cry_sprintf(szExecutableDirPath, "%sEditorPlugins\\Editor%s.dll", szExecutableDirPath, pCVar->GetString());
#else
		cry_sprintf(szExecutableDirPath, "%sEditorPluginsLegacy\\Editor%s.dll", szExecutableDirPath, pCVar->GetString());
#endif

		ms_hMiddlewarePlugin = LoadLibraryA(szExecutableDirPath);
		if (!ms_hMiddlewarePlugin)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "[Audio Controls Editor] Couldn't load the middleware specific editor dll.");
			ImplementationChanged();
			return false;
		}

		TPfnGetAudioInterface pfnAudioInterface = (TPfnGetAudioInterface) GetProcAddress(ms_hMiddlewarePlugin, "GetAudioInterface");
		if (!pfnAudioInterface)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "[Audio Controls Editor] Couldn't get middleware interface from loaded dll.");
			FreeLibrary(ms_hMiddlewarePlugin);
			ImplementationChanged();
			return false;
		}

		IEditor* pEditor = GetIEditor();
		if (pEditor)
		{
			ms_pAudioSystemImpl = pfnAudioInterface(pEditor->GetSystem());
			if (ms_pAudioSystemImpl)
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_COMMENT, "[Audio Controls Editor] Reloading audio implementation data");
				ms_pAudioSystemImpl->Reload();
				pATLModel->ReloadAllConnections();
				pATLModel->ClearDirtyFlags();
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "[Audio Controls Editor] Data from middleware is empty.");
			}
		}
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "[Audio Controls Editor] CVar %s not defined. Needed to derive the Editor plugin name.", g_sImplementationCVarName);
		ImplementationChanged();
		return false;
	}
	ImplementationChanged();
	return true;
}

void CImplementationManager::Release()
{
	if (ms_hMiddlewarePlugin)
	{
		FreeLibrary(ms_hMiddlewarePlugin);
	}
}

ACE::IAudioSystemEditor* CImplementationManager::GetImplementation()
{
	return ms_pAudioSystemImpl;
}
