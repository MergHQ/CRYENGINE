// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioControlsEditorWindow.h"
#include "AudioControlsEditorPlugin.h"
#include "QAudioControlEditorIcons.h"
#include "QAudioSystemSettingsDialog.h"
#include "AudioAssetsManager.h"
#include <CryAudio/IAudioSystem.h>
#include "AudioControlsEditorUndo.h"
#include "AudioAssetsExplorer.h"
#include "InspectorPanel.h"
#include "AudioSystemPanel.h"
#include "AdvancedTreeView.h"
#include "FileMonitor.h"
#include <DockTitleBarWidget.h>
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryString/CryPath.h>
#include "ImplementationManager.h"
#include "QtUtil.h"
#include <CryIcon.h>
#include "Controls/QuestionDialog.h"

#include <QApplication>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QSplitter>
#include <QKeyEvent>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorWindow::CAudioControlsEditorWindow()
{
	memset(m_allowedTypes, true, sizeof(m_allowedTypes));
	m_pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();
	m_pMonitorSystem = std::make_unique<ACE::CFileMonitorSystem>(this, 500);
	m_pMonitorMiddleware = std::make_unique<ACE::CFileMonitorMiddleware>(this, 500);

	setWindowTitle(tr("Audio Controls Editor"));

	// Tool Bar
	QToolBar* pToolBar = new QToolBar(this);
	pToolBar->setFloatable(false);
	pToolBar->setMovable(false);

	m_pSaveButton = new QToolButton();
	m_pSaveButton->setIcon(CryIcon("icons:General/File_Save.ico"));
	m_pSaveButton->setToolTip(tr("Save All"));
	m_pSaveButton->setEnabled(false);
	connect(m_pSaveButton, &QToolButton::clicked, this, &CAudioControlsEditorWindow::Save);
	m_pAssetsManager->signalIsDirty.Connect([&](bool bDirty)
	{
		m_pSaveButton->setEnabled(bDirty);
	}, reinterpret_cast<uintptr_t>(this));

	QToolButton* pReloadButton = new QToolButton();
	pReloadButton->setIcon(CryIcon("icons:General/Reload.ico"));
	pReloadButton->setToolTip(tr("Reload"));
	connect(pReloadButton, &QToolButton::clicked, this, &CAudioControlsEditorWindow::Reload);

	QWidget* pSpacer = new QWidget();
	pSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QToolButton* pSettingsButton = new QToolButton();
	pSettingsButton->setIcon(CryIcon("://Icons/Options.ico"));
	pSettingsButton->setToolTip(tr("Location settings of audio middleware project"));
	connect(pSettingsButton, &QToolButton::clicked, [&]()
	{
		QAudioSystemSettingsDialog* pSettingsDialog = new QAudioSystemSettingsDialog(this);
		connect(pSettingsDialog, &QAudioSystemSettingsDialog::ImplementationSettingsAboutToChange, [&]()
		{
			BackupTreeViewStates();
		});

		connect(pSettingsDialog, &QAudioSystemSettingsDialog::ImplementationSettingsChanged, [&]()
		{
			m_pAudioSystemPanel->Reset();
			RestoreTreeViewStates();
			m_pMonitorMiddleware->Update();
		});

		pSettingsDialog->exec();
	});

	pToolBar->addWidget(m_pSaveButton);
	pToolBar->addWidget(pReloadButton);
	pToolBar->addWidget(pSpacer);
	pToolBar->addWidget(pSettingsButton);
	addToolBar(pToolBar);

	IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	if (pAudioSystemImpl)
	{
		m_pExplorer = new CAudioAssetsExplorer(m_pAssetsManager);
		m_pInspectorPanel = new CInspectorPanel(m_pAssetsManager);
		m_pAudioSystemPanel = new CAudioSystemPanel();

		connect(m_pExplorer, &CAudioAssetsExplorer::SelectedControlChanged, [&]()
		{
			m_pInspectorPanel->SetSelectedControls(m_pExplorer->GetSelectedControls());
		});

		connect(m_pExplorer, &CAudioAssetsExplorer::ControlTypeFiltered, this, &CAudioControlsEditorWindow::FilterControlType);
		connect(m_pExplorer, &CAudioAssetsExplorer::StartTextFiltering, m_pInspectorPanel, &CInspectorPanel::BackupTreeViewStates);
		connect(m_pExplorer, &CAudioAssetsExplorer::StopTextFiltering, m_pInspectorPanel, &CInspectorPanel::RestoreTreeViewStates);
		CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.Connect(this, &CAudioControlsEditorWindow::SaveBeforeImplementationChange);
		CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect(this, &CAudioControlsEditorWindow::Reload);

		CAudioControlsEditorPlugin::signalAboutToSave.Connect([&]()
		{
			m_pMonitorSystem->Disable();
		}, reinterpret_cast<uintptr_t>(this));

		CAudioControlsEditorPlugin::signalSaved.Connect([&]()
		{
			m_pMonitorSystem->EnableDelayed();
		}, reinterpret_cast<uintptr_t>(this));

		GetIEditor()->RegisterNotifyListener(this);

		m_pSplitter = new QSplitter();
		m_pSplitter->addWidget(m_pExplorer);
		m_pSplitter->addWidget(m_pInspectorPanel);
		m_pSplitter->addWidget(m_pAudioSystemPanel);
		setCentralWidget(m_pSplitter);
	}

	m_pMonitorSystem->Disable();
	m_pMonitorMiddleware->Disable();

	if (m_pAssetsManager->IsLoading())
	{
		// The middleware is being swapped out therefore we must not
		// reload it and must not call signalAboutToLoad and signalLoaded!
		CAudioControlsEditorPlugin::ReloadModels(false);
	}
	else
	{
		CAudioControlsEditorPlugin::signalAboutToLoad();
		CAudioControlsEditorPlugin::ReloadModels(true);
		CAudioControlsEditorPlugin::signalLoaded();
	}

	m_pAssetsManager->UpdateAllConnectionStates();
	m_pInspectorPanel->Reload();
	m_pAudioSystemPanel->Reset();
	CheckErrorMask();
	m_pMonitorSystem->Enable();
	m_pMonitorMiddleware->Update();
}

//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorWindow::~CAudioControlsEditorWindow()
{
	GetIEditor()->UnregisterNotifyListener(this);
	m_pAssetsManager->signalIsDirty.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::signalAboutToSave.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::signalSaved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::keyPressEvent(QKeyEvent* pEvent)
{
	if (!m_pExplorer->IsEditing())
	{
		if ((pEvent->key() == Qt::Key_S) && (pEvent->modifiers() == Qt::ControlModifier))
		{
			Save();
		}
		else if ((pEvent->key() == Qt::Key_Z) && (pEvent->modifiers() & Qt::ControlModifier))
		{
			if (pEvent->modifiers() & Qt::ShiftModifier)
			{
				GetIEditor()->GetIUndoManager()->Redo();
			}
			else
			{
				GetIEditor()->GetIUndoManager()->Undo();
			}
		}
	}

	QWidget::keyPressEvent(pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::closeEvent(QCloseEvent* pEvent)
{
	if (m_pAssetsManager && m_pAssetsManager->IsDirty())
	{
		CQuestionDialog messageBox;
		messageBox.SetupQuestion(tr("Audio Controls Editor"), tr("There are unsaved changes.\nDo you want to save your changes?"), QDialogButtonBox::Save | QDialogButtonBox::Discard | QDialogButtonBox::Cancel, QDialogButtonBox::Save);
		
		switch (messageBox.Execute())
		{
		case QDialogButtonBox::Save:
			QApplication::setOverrideCursor(Qt::WaitCursor);
			Save();
			QApplication::restoreOverrideCursor();
			pEvent->accept();
			break;
		case QDialogButtonBox::Discard:
			{
				ACE::IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
				if (pAudioSystemEditorImpl)
				{
					pAudioSystemEditorImpl->Reload(false);
				}

				CAudioControlsEditorPlugin::signalAboutToLoad();
				CAudioControlsEditorPlugin::ReloadModels(false);
				CAudioControlsEditorPlugin::signalLoaded();

				pEvent->accept();
			}
			break;
		default:
			pEvent->ignore();
			break;
		}
	}
	else
	{
		pEvent->accept();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::Reload()
{
	if (m_pAssetsManager != nullptr)
	{
		bool bReload = true;

		if (m_pAssetsManager->IsDirty())
		{
			CQuestionDialog messageBox;
			messageBox.SetupQuestion(tr("Audio Controls Editor"), tr("If you reload you will lose all your unsaved changes.\nAre you sure you want to reload?"), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
			bReload = (messageBox.Execute() == QDialogButtonBox::Yes);
		}

		if (bReload)
		{
			m_pMonitorSystem->Disable();
			m_pMonitorMiddleware->Disable();

			BackupTreeViewStates();

			if (m_pAssetsManager->IsLoading())
			{
				// The middleware is being swapped out therefore we must not
				// reload it and must not call signalAboutToLoad and signalLoaded!
				CAudioControlsEditorPlugin::ReloadModels(false);
			}
			else
			{
				CAudioControlsEditorPlugin::signalAboutToLoad();
				CAudioControlsEditorPlugin::ReloadModels(true);
				CAudioControlsEditorPlugin::signalLoaded();
			}

			m_pAssetsManager->UpdateAllConnectionStates();
			m_pInspectorPanel->Reload();
			m_pAudioSystemPanel->Reset();
			CheckErrorMask();

			RestoreTreeViewStates();

			m_pMonitorSystem->Enable();
			m_pMonitorMiddleware->Update();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::SaveBeforeImplementationChange()
{
	if (m_pAssetsManager != nullptr)
	{
		if (m_pAssetsManager->IsDirty())
		{
			CQuestionDialog messageBox;
			messageBox.SetupQuestion(tr("Audio Controls Editor"), tr("Middleware implementation changed.\nThere are unsaved changes.\nDo you want to save before reloading?"), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
			
			if (messageBox.Execute() == QDialogButtonBox::Yes)
			{
				m_pAssetsManager->ClearDirtyFlags();
				Save();
			}
		}

		m_pAssetsManager->ClearDirtyFlags();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::CheckErrorMask()
{
	uint const errorCodeMask = CAudioControlsEditorPlugin::GetLoadingErrorMask();

	if ((errorCodeMask & EErrorCode::eErrorCode_UnkownPlatform) != 0)
	{
		CQuestionDialog::SWarning(tr("Audio Controls Editor"), tr("Audio Preloads reference an unknown platform.\nSaving will permanently erase this data."));
	}
	else if ((errorCodeMask & EErrorCode::eErrorCode_NonMatchedActivityRadius) != 0)
	{
		IAudioSystemEditor const* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

		if (pAudioSystemImpl != nullptr)
		{
			QString const middlewareName = pAudioSystemImpl->GetName();
			CQuestionDialog::SWarning(tr("Audio Controls Editor"), tr("The attenuation of some controls has changed in your ") + middlewareName + tr(" project.\n\nTriggers with their activity radius linked to the attenuation will be updated next time you save."));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::Save()
{
	bool bPreloadsChanged = m_pAssetsManager->IsTypeDirty(eItemType_Preload);
	CAudioControlsEditorPlugin::SaveModels();
	UpdateAudioSystemData();

	// if preloads have been modified, ask the user if s/he wants to refresh the audio system
	if (bPreloadsChanged)
	{
		CQuestionDialog messageBox;

		messageBox.SetupQuestion(tr("Audio Controls Editor"), tr("Preload requests have been modified. \n\nFor the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio. Do you want to do this now?. \n\nYou can always refresh manually at a later time through the Audio menu."), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);

		if (messageBox.Execute() == QDialogButtonBox::Yes)
		{
			char const* szLevelName = GetIEditor()->GetLevelName();

			if (_stricmp(szLevelName, "Untitled") == 0)
			{
				// Rather pass nullptr to indicate that no level is loaded!
				szLevelName = nullptr;
			}

			CryAudio::SRequestUserData const data(CryAudio::ERequestFlags::ExecuteBlocking);
			gEnv->pAudioSystem->Refresh(szLevelName, data);
		}
	}

	m_pAssetsManager->ClearDirtyFlags();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::UpdateAudioSystemData()
{
	string levelPath = CRY_NATIVE_PATH_SEPSTR "levels" CRY_NATIVE_PATH_SEPSTR;
	levelPath += GetIEditor()->GetLevelName();
	gEnv->pAudioSystem->ReloadControlsData(gEnv->pAudioSystem->GetConfigPath(), levelPath.c_str());
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	if (event == eNotify_OnEndSceneSave)
	{
		CAudioControlsEditorPlugin::ReloadScopes();
		m_pInspectorPanel->Reload();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::FilterControlType(EItemType type, bool bShow)
{
	m_allowedTypes[type] = bShow;

	if (type == eItemType_Switch)
	{
		// need to keep states and switches filtering in sync as we don't have a separate filtering for states, only for switches
		m_allowedTypes[eItemType_State] = bShow;
	}

	m_pAudioSystemPanel->SetAllowedControls(type, bShow);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::ReloadSystemData()
{
	if (m_pAssetsManager != nullptr)
	{
		bool bReload = true;
		char const* messageText;

		if (m_pAssetsManager->IsDirty())
		{
			messageText = "External changes have been made to audio controls files.\nIf you reload you will lose all your unsaved changes.\nAre you sure you want to reload?";
		}
		else
		{
			messageText = "External changes have been made to audio controls files.\nDo you want to reload?";
		}

		CQuestionDialog messageBox;
		messageBox.SetupQuestion(tr("Audio Controls Editor"), tr(messageText), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
		bReload = (messageBox.Execute() == QDialogButtonBox::Yes);

		if (bReload)
		{
			m_pAssetsManager->ClearDirtyFlags();
			Reload();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::ReloadMiddlewareData()
{
	if (m_pAssetsManager != nullptr)
	{
		if (m_pAssetsManager->IsDirty())
		{
			char const* messageText = "External changes have been made to middleware data.\n\nWarning: If middleware data gets refreshed without saving audio control files, unsaved connection changes will get lost!\n\nDo you want to save before refreshing middleware data?";
			CQuestionDialog messageBox;
			messageBox.SetupQuestion(tr("Audio Controls Editor"), tr(messageText), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::Yes);

			if (messageBox.Execute() == QDialogButtonBox::Yes)
			{
				Save();
			}
		}
	}

	IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	BackupTreeViewStates();

	if (pAudioSystemImpl)
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_COMMENT, "[Audio Controls Editor] Reloading audio implementation data");
		pAudioSystemImpl->Reload();
	}

	m_pAssetsManager->ClearAllConnections();
	m_pAssetsManager->ReloadAllConnections();
	m_pAssetsManager->UpdateAllConnectionStates();
	m_pInspectorPanel->Reload();
	m_pAudioSystemPanel->Reset();

	RestoreTreeViewStates();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::BackupTreeViewStates()
{
	m_pExplorer->BackupTreeViewStates();
	m_pAudioSystemPanel->BackupTreeViewStates();
	m_pInspectorPanel->BackupTreeViewStates();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::RestoreTreeViewStates()
{
	m_pExplorer->RestoreTreeViewStates();
	m_pAudioSystemPanel->RestoreTreeViewStates();
	m_pInspectorPanel->RestoreTreeViewStates();
}
} // namespace ACE
