// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioControlsEditorPlugin.h"

#include "MainWindow.h"
#include "AudioControlsLoader.h"
#include "FileWriter.h"
#include "FileLoader.h"
#include "ImplementationManager.h"

#include <IEditorImpl.h>
#include <CryAudio/IAudioSystem.h>
#include <CryAudio/IObject.h>
#include <CryMath/Cry_Camera.h>
#include <CryCore/Platform/platform_impl.inl>
#include <IUndoManager.h>
#include <QtViewPane.h>
#include <IResourceSelectorHost.h>

REGISTER_PLUGIN(ACE::CAudioControlsEditorPlugin);

namespace ACE
{
CSystemAssetsManager CAudioControlsEditorPlugin::s_assetsManager;
std::set<string> CAudioControlsEditorPlugin::s_currentFilenames;
CryAudio::IObject* CAudioControlsEditorPlugin::s_pIAudioObject = nullptr;
CryAudio::ControlId CAudioControlsEditorPlugin::s_audioTriggerId = CryAudio::InvalidControlId;
CImplementationManager CAudioControlsEditorPlugin::s_implementationManager;
EErrorCode CAudioControlsEditorPlugin::s_loadingErrorMask;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalAboutToLoad;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalLoaded;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalAboutToSave;
CCrySignal<void()> CAudioControlsEditorPlugin::SignalSaved;

REGISTER_VIEWPANE_FACTORY(CMainWindow, "Audio Controls Editor", "Tools", true)

//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorPlugin::CAudioControlsEditorPlugin()
{
	CryAudio::SCreateObjectData const objectData("Audio trigger preview", CryAudio::EOcclusionType::Ignore);
	s_pIAudioObject = gEnv->pAudioSystem->CreateObject(objectData);

	s_assetsManager.Initialize();
	s_implementationManager.LoadImplementation();

	SignalAboutToLoad();
	ReloadModels(false);
	SignalLoaded();

	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this, "CAudioControlsEditorPlugin");
}

//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorPlugin::~CAudioControlsEditorPlugin()
{
	s_implementationManager.Release();

	if (s_pIAudioObject != nullptr)
	{
		StopTriggerExecution();
		gEnv->pAudioSystem->ReleaseObject(s_pIAudioObject);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::SaveModels()
{
	SignalAboutToSave();

	if (g_pEditorImpl != nullptr)
	{
		CFileWriter writer(s_assetsManager, s_currentFilenames);
		writer.WriteAll();
	}

	s_loadingErrorMask = EErrorCode::NoError;
	SignalSaved();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::ReloadModels(bool const reloadImplementation)
{
	// Do not call signalAboutToLoad and signalLoaded in here!
	GetIEditor()->GetIUndoManager()->Suspend();

	s_assetsManager.UpdateFolderPaths();
	s_assetsManager.Clear();

	if (g_pEditorImpl != nullptr)
	{
		if (reloadImplementation)
		{
			g_pEditorImpl->Reload();
		}

		CFileLoader loader(s_assetsManager);
		loader.CreateInternalControls();

		// CAudioControlsLoader is deprecated and only used for backwards compatibility. It will be removed before March 2019.
		CAudioControlsLoader loaderForBackwardsCompatibility(&s_assetsManager);
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

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::ReloadScopes()
{
	s_assetsManager.ClearScopes();

	CFileLoader loader(s_assetsManager);
	loader.LoadScopes();

	// CAudioControlsLoader is deprecated and only used for backwards compatibility. It will be removed before March 2019.
	CAudioControlsLoader loaderForBackwardsCompatibility(&s_assetsManager);
	loaderForBackwardsCompatibility.LoadScopes();
}

//////////////////////////////////////////////////////////////////////////
CSystemAssetsManager* CAudioControlsEditorPlugin::GetAssetsManager()
{
	return &s_assetsManager;
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::ExecuteTrigger(string const& sTriggerName)
{
	if (!sTriggerName.empty() && (s_pIAudioObject != nullptr))
	{
		StopTriggerExecution();
		CCamera const& camera = GetIEditor()->GetSystem()->GetViewCamera();
		Matrix34 const& cameraMatrix = camera.GetMatrix();
		s_pIAudioObject->SetTransformation(cameraMatrix);
		s_audioTriggerId = CryAudio::StringToId(sTriggerName.c_str());
		s_pIAudioObject->ExecuteTrigger(s_audioTriggerId);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::StopTriggerExecution()
{
	if (s_pIAudioObject && (s_audioTriggerId != CryAudio::InvalidControlId))
	{
		s_pIAudioObject->StopTrigger(s_audioTriggerId);
		s_audioTriggerId = CryAudio::InvalidControlId;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorPlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_AUDIO_IMPLEMENTATION_LOADED:
		s_implementationManager.LoadImplementation();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
CImplementationManager* CAudioControlsEditorPlugin::GetImplementationManger()
{
	return &s_implementationManager;
}
} // namespace ACE
