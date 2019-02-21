// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioControlsEditorPlugin.h"

#include "Common.h"
#include "MainWindow.h"
#include "AudioControlsLoader.h"
#include "FileWriter.h"
#include "FileLoader.h"
#include "ImplementationManager.h"
#include "AssetIcons.h"
#include "Common/IImpl.h"

#include <CryMath/Cry_Camera.h>
#include <CryCore/Platform/platform_impl.inl>
#include <IUndoManager.h>
#include <QtViewPane.h>
#include <ConfigurationManager.h>

REGISTER_PLUGIN(ACE::CAudioControlsEditorPlugin);

namespace ACE
{
CAssetsManager g_assetsManager;
CImplementationManager g_implementationManager;
FileNames CAudioControlsEditorPlugin::s_currentFilenames;
CryAudio::ControlId CAudioControlsEditorPlugin::s_audioTriggerId = CryAudio::InvalidControlId;
EErrorCode CAudioControlsEditorPlugin::s_loadingErrorMask;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalOnBeforeLoad;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalOnAfterLoad;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalOnBeforeSave;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalOnAfterSave;

REGISTER_VIEWPANE_FACTORY(CMainWindow, "Audio Controls Editor", "Tools", true)

//////////////////////////////////////////////////////////////////////////
void InitPlatforms()
{
	g_platforms.clear();
	std::vector<dll_string> const& platforms = GetIEditor()->GetConfigurationManager()->GetPlatformNames();

	for (auto const& platform : platforms)
	{
		g_platforms.push_back(platform.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorPlugin::CAudioControlsEditorPlugin()
{
	InitPlatforms();
	InitAssetIcons();

	g_assetsManager.Initialize();
	g_implementationManager.LoadImplementation();

	ReloadData(EReloadFlags::ReloadSystemControls | EReloadFlags::SendSignals);

	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this, "CAudioControlsEditorPlugin");
}

//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorPlugin::~CAudioControlsEditorPlugin()
{
	g_implementationManager.Release();
	StopTriggerExecution();
	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::SaveData()
{
	SignalOnBeforeSave();

	if (g_pIImpl != nullptr)
	{
		CFileWriter writer(s_currentFilenames);
		writer.WriteAll();
	}

	s_loadingErrorMask = EErrorCode::None;
	SignalOnAfterSave();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::ReloadData(EReloadFlags const flags)
{
	if ((flags& EReloadFlags::SendSignals) != 0)
	{
		SignalOnBeforeLoad();
	}

	if ((flags& EReloadFlags::ReloadSystemControls) != 0)
	{
		GetIEditor()->GetIUndoManager()->Suspend();

		g_assetsManager.UpdateConfigFolderPath();
		g_assetsManager.Clear();

		if (g_pIImpl != nullptr)
		{
			if ((flags& EReloadFlags::ReloadImplData) != 0)
			{
				ReloadImplData(flags);
			}

			CFileLoader loader;
			loader.CreateInternalControls();

			// CAudioControlsLoader is deprecated and only used for backwards compatibility. It will be removed before March 2019.
			CAudioControlsLoader loaderForBackwardsCompatibility;
			loaderForBackwardsCompatibility.LoadAll(true);

			loader.LoadAll();
			s_currentFilenames = loader.GetLoadedFilenamesList();
			s_loadingErrorMask = loader.GetErrorCodeMask();

			loaderForBackwardsCompatibility.LoadAll(false);
			auto const& fileNames = loaderForBackwardsCompatibility.GetLoadedFilenamesList();

			for (auto const& name : fileNames)
			{
				s_currentFilenames.emplace(name);
			}
		}

		GetIEditor()->GetIUndoManager()->Resume();
	}
	else if ((flags& EReloadFlags::ReloadImplData) != 0)
	{
		ReloadImplData(flags);
	}

	if ((flags& EReloadFlags::ReloadScopes) != 0)
	{
		g_assetsManager.ClearScopes();

		CFileLoader loader;
		loader.LoadScopes();

		// CAudioControlsLoader is deprecated and only used for backwards compatibility. It will be removed before March 2019.
		CAudioControlsLoader loaderForBackwardsCompatibility;
		loaderForBackwardsCompatibility.LoadScopes();
	}

	if ((flags& EReloadFlags::SendSignals) != 0)
	{
		SignalOnAfterLoad();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::ReloadImplData(EReloadFlags const flags)
{
	if (g_pIImpl != nullptr)
	{
		if ((flags& EReloadFlags::BackupConnections) != 0)
		{
			g_assetsManager.BackupAndClearAllConnections();
		}

		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_COMMENT, "[Audio Controls Editor] Reloading audio implementation data");
		g_pIImpl->Reload(g_implInfo);

		if ((flags& EReloadFlags::BackupConnections) != 0)
		{
			g_assetsManager.ReloadAllConnections();
		}
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "[Audio Controls Editor] Data from middleware is empty.");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::ExecuteTrigger(string const& triggerName)
{
	if (!triggerName.empty())
	{
		StopTriggerExecution();
		s_audioTriggerId = CryAudio::StringToId(triggerName.c_str());
		gEnv->pAudioSystem->ExecutePreviewTrigger(s_audioTriggerId);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::ExecuteTriggerEx(string const& triggerName, XmlNodeRef const pNode)
{
	if (pNode != nullptr)
	{
		StopTriggerExecution();
		s_audioTriggerId = CryAudio::StringToId(triggerName.c_str());
		gEnv->pAudioSystem->ExecutePreviewTriggerEx(pNode);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::StopTriggerExecution()
{
	if (s_audioTriggerId != CryAudio::InvalidControlId)
	{
		gEnv->pAudioSystem->StopPreviewTrigger();
		s_audioTriggerId = CryAudio::InvalidControlId;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_AUDIO_IMPLEMENTATION_LOADED:
		{
			g_implementationManager.LoadImplementation();
			break;
		}
	default:
		{
			break;
		}
	}
}
} // namespace ACE
