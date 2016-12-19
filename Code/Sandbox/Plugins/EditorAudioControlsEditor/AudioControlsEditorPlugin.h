// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditor.h>
#include <IPlugin.h>
#include "ATLControlsModel.h"
#include "QATLControlsTreeModel.h"
#include <IAudioSystemEditor.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

#include <QStandardItem>

namespace CryAudio
{
struct IObject;
}

class CImplementationManager;

//------------------------------------------------------------------
class CAudioControlsEditorPlugin : public IPlugin, public ISystemEventListener
{
public:
	explicit CAudioControlsEditorPlugin(IEditor* editor);

	void                            Release() override;
	void                            ShowAbout() override                                 {}
	const char*                     GetPluginGUID() override                             { return "{DFA4AFF7-2C70-4B29-B736-GRH00040314}"; }
	DWORD                           GetPluginVersion() override                          { return 1; }
	const char*                     GetPluginName() override                             { return "AudioControlsEditor"; }
	bool                            CanExitNow() override                                { return true; }
	void                            OnEditorNotify(EEditorNotifyEvent aEventId) override {}

	static void                     SaveModels();
	static void                     ReloadModels(bool bReloadImplementation);
	static void                     ReloadScopes();
	static ACE::CATLControlsModel*  GetATLModel();
	static ACE::QATLTreeModel*      GetControlsTree();
	static CImplementationManager*  GetImplementationManger();
	static ACE::IAudioSystemEditor* GetAudioSystemEditorImpl();
	static void                     ExecuteTrigger(const string& sTriggerName);
	static void                     StopTriggerExecution();
	static uint                     GetLoadingErrorMask() { return s_loadingErrorMask; }

private:
	///////////////////////////////////////////////////////////////////////////
	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
	///////////////////////////////////////////////////////////////////////////

	static ACE::CATLControlsModel s_ATLModel;
	static ACE::QATLTreeModel     s_layoutModel;
	static std::set<string>       s_currentFilenames;
	static CryAudio::IObject*     s_pIAudioObject;
	static CryAudio::ControlId    s_audioTriggerId;

	static CImplementationManager s_implementationManager;
	static uint                   s_loadingErrorMask;
};
