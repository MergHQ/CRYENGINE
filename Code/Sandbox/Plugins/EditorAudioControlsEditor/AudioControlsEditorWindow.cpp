// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioControlsEditorWindow.h"
#include "AudioControlsEditorPlugin.h"
#include "SystemControlsEditorIcons.h"
#include "PreferencesDialog.h"
#include "AudioAssetsManager.h"
#include <CryAudio/IAudioSystem.h>
#include "AudioControlsEditorUndo.h"
#include "SystemControlsWidget.h"
#include "PropertiesWidget.h"
#include "MiddlewareDataWidget.h"
#include "AdvancedTreeView.h"
#include "FileMonitor.h"
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryString/CryPath.h>
#include "ImplementationManager.h"
#include "QtUtil.h"
#include <CryIcon.h>
#include "Controls/QuestionDialog.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QAction>
#include <QToolBar>
#include <QKeyEvent>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorWindow::CAudioControlsEditorWindow()
	: m_pAssetsManager(nullptr)
	, m_pSystemControlsWidget(nullptr)
	, m_pPropertiesWidget(nullptr)
	, m_pMiddlewareDataWidget(nullptr)
{
	memset(m_allowedTypes, true, sizeof(m_allowedTypes));
	m_pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();
	
	setAttribute(Qt::WA_DeleteOnClose);
	setObjectName(GetEditorName());

	QVBoxLayout* pWindowLayout = new QVBoxLayout();
	pWindowLayout->setContentsMargins(0, 0, 0, 0);
	SetContent(pWindowLayout);

	InitMenu();
	InitToolbar(pWindowLayout);
	RegisterWidgets();

	if (m_pAssetsManager->IsLoading())
	{
		// The middleware is being swapped out therefore we must not reload it and must not call signalAboutToLoad and signalLoaded!
		CAudioControlsEditorPlugin::ReloadModels(false);
	}
	else
	{
		CAudioControlsEditorPlugin::signalAboutToLoad();
		CAudioControlsEditorPlugin::ReloadModels(true);
		CAudioControlsEditorPlugin::signalLoaded();
	}

	m_pAssetsManager->UpdateAllConnectionStates();
	CheckErrorMask();

	m_pMonitorSystem = std::make_unique<ACE::CFileMonitorSystem>(this, 500);
	m_pMonitorMiddleware = std::make_unique<ACE::CFileMonitorMiddleware>(this, 500);

	CAudioControlsEditorPlugin::signalAboutToSave.Connect([&]()
	{
		m_pMonitorSystem->Disable();
	}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::signalSaved.Connect([&]()
	{
		m_pMonitorSystem->EnableDelayed();
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalIsDirty.Connect([&](bool bDirty)
	{
		m_pSaveAction->setEnabled(bDirty);
	}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.Connect(this, &CAudioControlsEditorWindow::SaveBeforeImplementationChange);
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect(this, &CAudioControlsEditorWindow::Reload);

	GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CAudioControlsEditorWindow::~CAudioControlsEditorWindow()
{
	CAudioControlsEditorPlugin::signalAboutToSave.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::signalSaved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalIsDirty.DisconnectById(reinterpret_cast<uintptr_t>(this));
	GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::keyPressEvent(QKeyEvent* pEvent)
{
	if (!m_pSystemControlsWidget->IsEditing())
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
		messageBox.SetupQuestion(tr(GetEditorName()), tr("There are unsaved changes.\nDo you want to save your changes?"), QDialogButtonBox::Save | QDialogButtonBox::Discard | QDialogButtonBox::Cancel, QDialogButtonBox::Save);
		
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
void CAudioControlsEditorWindow::InitMenu()
{
	CEditor::MenuItems const items[] = { CEditor::MenuItems::EditMenu };
	AddToMenu(items, sizeof(items) / sizeof(CEditor::MenuItems));

	CAbstractMenu* const pMenuEdit = GetMenu(MenuItems::EditMenu);
	auto pPreferencesAction = pMenuEdit->CreateAction(tr("Preferences..."));
	QObject::connect(pPreferencesAction, &QAction::triggered, this, &CAudioControlsEditorWindow::OnPreferencesDialog);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::InitToolbar(QVBoxLayout* pWindowLayout)
{
	QHBoxLayout* pToolBarsLayout = new QHBoxLayout();
	pToolBarsLayout->setDirection(QBoxLayout::LeftToRight);
	pToolBarsLayout->setSizeConstraint(QLayout::SetMaximumSize);

	{
		QToolBar* pToolBar = new QToolBar("ACE Tools");

		{
			m_pSaveAction = pToolBar->addAction(CryIcon("icons:General/File_Save.ico"), QString());
			m_pSaveAction->setToolTip(tr("Save all changes."));
			m_pSaveAction->setEnabled(false);
			QObject::connect(m_pSaveAction, &QAction::triggered, this, &CAudioControlsEditorWindow::Save);
		}

		{
			QAction* pReloadAction = pToolBar->addAction(CryIcon("icons:General/Reload.ico"), QString());
			pReloadAction->setToolTip(tr("Reload all ACE and middleware files."));
			QObject::connect(pReloadAction, &QAction::triggered, this, &CAudioControlsEditorWindow::Reload);
		}

		pToolBarsLayout->addWidget(pToolBar, 0, Qt::AlignLeft);
	}

	pWindowLayout->addLayout(pToolBarsLayout);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::RegisterWidgets()
{
	EnableDockingSystem();

	// Audio System Controls
	auto createAudioSystemControlsWidget = [this]()
	{
		CSystemControlsWidget* const pWidget = CreateSystemControlsWidget();

		if (pWidget)
		{
			pWidget->setWindowTitle("Audio System Controls");
		}

		return pWidget;
	};
	RegisterWidget("Audio System Controls", createAudioSystemControlsWidget, true, false);

	// Properties
	auto createPropertiesWidget = [this]()
	{
		CPropertiesWidget* const pWidget = CreatePropertiesWidget();

		if (pWidget)
		{
			pWidget->setWindowTitle("Properties");
		}

		return pWidget;
	};
	RegisterWidget("Properties", createPropertiesWidget, true, false);

	// Middleware Data
	auto createMiddlewareDataWidget = [this]()
	{
		CMiddlewareDataWidget* const pWidget = CreateMiddlewareDataWidget();

		if (pWidget)
		{
			pWidget->setWindowTitle("Middleware Data");
		}

		return pWidget;
	};
	RegisterWidget("Middleware Data", createMiddlewareDataWidget, true, false);
}

//////////////////////////////////////////////////////////////////////////
CSystemControlsWidget* CAudioControlsEditorWindow::CreateSystemControlsWidget()
{
	CSystemControlsWidget* pSystemControlsWidget = new CSystemControlsWidget(m_pAssetsManager);

	if (m_pSystemControlsWidget == nullptr)
	{
		m_pSystemControlsWidget = pSystemControlsWidget;
	}

	QObject::connect(pSystemControlsWidget, &CSystemControlsWidget::SelectedControlChanged, this, &CAudioControlsEditorWindow::OnSelectedSystemControlChanged);
	QObject::connect(pSystemControlsWidget, &CSystemControlsWidget::ControlTypeFiltered, this, &CAudioControlsEditorWindow::FilterControlType);
	QObject::connect(pSystemControlsWidget, &CSystemControlsWidget::StartTextFiltering, this, &CAudioControlsEditorWindow::OnStartTextFiltering);
	QObject::connect(pSystemControlsWidget, &CSystemControlsWidget::StopTextFiltering, this, &CAudioControlsEditorWindow::OnStopTextFiltering);
	QObject::connect(pSystemControlsWidget, &QObject::destroyed, this, &CAudioControlsEditorWindow::OnSystemControlsWidgetDestruction);
	return pSystemControlsWidget;
}

//////////////////////////////////////////////////////////////////////////
CPropertiesWidget* CAudioControlsEditorWindow::CreatePropertiesWidget()
{
	CPropertiesWidget* pPropertiesWidget = new CPropertiesWidget(m_pAssetsManager);

	if (m_pPropertiesWidget == nullptr)
	{
		m_pPropertiesWidget = pPropertiesWidget;
	}

	QObject::connect(this, &CAudioControlsEditorWindow::OnSelectedSystemControlChanged, [&]()
	{
		if (m_pPropertiesWidget != nullptr)
		{
			m_pPropertiesWidget->SetSelectedControls(GetSelectedSystemControls());
		}
	});
	
	QObject::connect(this, &CAudioControlsEditorWindow::OnStartTextFiltering, pPropertiesWidget, &CPropertiesWidget::BackupTreeViewStates);
	QObject::connect(this, &CAudioControlsEditorWindow::OnStopTextFiltering, pPropertiesWidget, &CPropertiesWidget::RestoreTreeViewStates);
	QObject::connect(pPropertiesWidget, &QObject::destroyed, this, &CAudioControlsEditorWindow::OnPropertiesWidgetDestruction);
	return pPropertiesWidget;
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget* CAudioControlsEditorWindow::CreateMiddlewareDataWidget()
{
	CMiddlewareDataWidget* pMiddlewareDataWidget = new CMiddlewareDataWidget();

	if (m_pMiddlewareDataWidget == nullptr)
	{
		m_pMiddlewareDataWidget = pMiddlewareDataWidget;
	}

	QObject::connect(pMiddlewareDataWidget, &QObject::destroyed, this, &CAudioControlsEditorWindow::OnMiddlewareDataWidgetDestruction);
	return pMiddlewareDataWidget;
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::OnSystemControlsWidgetDestruction(QObject* pObject)
{
	CSystemControlsWidget* pWidget = static_cast<CSystemControlsWidget*>(pObject);

	if (m_pSystemControlsWidget == pWidget)
	{
		m_pSystemControlsWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::OnPropertiesWidgetDestruction(QObject* pObject)
{
	CPropertiesWidget* pWidget = static_cast<CPropertiesWidget*>(pObject);

	if (m_pPropertiesWidget == pWidget)
	{
		m_pPropertiesWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::OnMiddlewareDataWidgetDestruction(QObject* pObject)
{
	CMiddlewareDataWidget* pWidget = static_cast<CMiddlewareDataWidget*>(pObject);

	if (m_pMiddlewareDataWidget == pWidget)
	{
		m_pMiddlewareDataWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::CreateDefaultLayout(CDockableContainer* pSender)
{
	CRY_ASSERT(pSender);

	pSender->SpawnWidget("Audio System Controls");
	pSender->SpawnWidget("Properties", QToolWindowAreaReference::VSplitRight);
	pSender->SpawnWidget("Middleware Data", QToolWindowAreaReference::VSplitRight);
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
			messageBox.SetupQuestion(tr(GetEditorName()), tr("If you reload you will lose all your unsaved changes.\nAre you sure you want to reload?"), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
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

			if (m_pPropertiesWidget != nullptr)
			{
				m_pPropertiesWidget->Reload();
			}

			if (m_pMiddlewareDataWidget != nullptr)
			{
				m_pMiddlewareDataWidget->Reset();
			}

			CheckErrorMask();

			RestoreTreeViewStates();

			m_pMonitorSystem->Enable();
			m_pMonitorMiddleware->Enable();
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
			messageBox.SetupQuestion(tr(GetEditorName()), tr("Middleware implementation changed.\nThere are unsaved changes.\nDo you want to save before reloading?"), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
			
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
		CQuestionDialog::SWarning(tr(GetEditorName()), tr("Audio Preloads reference an unknown platform.\nSaving will permanently erase this data."));
	}
	else if ((errorCodeMask & EErrorCode::eErrorCode_NonMatchedActivityRadius) != 0)
	{
		IAudioSystemEditor const* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

		if (pAudioSystemImpl != nullptr)
		{
			QString const middlewareName = pAudioSystemImpl->GetName();
			CQuestionDialog::SWarning(tr(GetEditorName()), tr("The attenuation of some controls has changed in your ") + middlewareName + tr(" project.\n\nTriggers with their activity radius linked to the attenuation will be updated next time you save."));
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

		messageBox.SetupQuestion(tr(GetEditorName()), tr("Preload requests have been modified. \n\nFor the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio. Do you want to do this now?. \n\nYou can always refresh manually at a later time through the Audio menu."), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);

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

		if (m_pPropertiesWidget != nullptr)
		{
			m_pPropertiesWidget->Reload();
		}
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

	if (m_pMiddlewareDataWidget != nullptr)
	{
		m_pMiddlewareDataWidget->SetAllowedControls(type, bShow);
	}
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
		messageBox.SetupQuestion(tr(GetEditorName()), tr(messageText), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
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
			messageBox.SetupQuestion(tr(GetEditorName()), tr(messageText), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::Yes);

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

	if (m_pPropertiesWidget != nullptr)
	{
		m_pPropertiesWidget->Reload();
	}
	
	if (m_pMiddlewareDataWidget != nullptr)
	{
		m_pMiddlewareDataWidget->Reset();
	}

	RestoreTreeViewStates();
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::BackupTreeViewStates()
{
	if (m_pSystemControlsWidget != nullptr)
	{
		m_pSystemControlsWidget->BackupTreeViewStates();
	}

	if (m_pMiddlewareDataWidget != nullptr)
	{
		m_pMiddlewareDataWidget->BackupTreeViewStates();
	}

	if (m_pPropertiesWidget != nullptr)
	{
		m_pPropertiesWidget->BackupTreeViewStates();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::RestoreTreeViewStates()
{
	if (m_pSystemControlsWidget != nullptr)
	{
		m_pSystemControlsWidget->RestoreTreeViewStates();
	}

	if (m_pMiddlewareDataWidget != nullptr)
	{
		m_pMiddlewareDataWidget->RestoreTreeViewStates();
	}

	if (m_pPropertiesWidget != nullptr)
	{
		m_pPropertiesWidget->RestoreTreeViewStates();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsEditorWindow::OnPreferencesDialog()
{
	CPreferencesDialog* pPreferencesDialog = new CPreferencesDialog(this);

	QObject::connect(pPreferencesDialog, &CPreferencesDialog::ImplementationSettingsAboutToChange, [&]()
	{
		BackupTreeViewStates();
	});

	QObject::connect(pPreferencesDialog, &CPreferencesDialog::ImplementationSettingsChanged, [&]()
	{
		if (m_pMiddlewareDataWidget != nullptr)
		{
			m_pMiddlewareDataWidget->Reset();
		}

		RestoreTreeViewStates();
		m_pMonitorMiddleware->Enable();
	});

	pPreferencesDialog->exec();
}

//////////////////////////////////////////////////////////////////////////
std::vector<CAudioControl*> CAudioControlsEditorWindow::GetSelectedSystemControls()
{
	std::vector<CAudioControl*> controls;

	if (m_pSystemControlsWidget != nullptr)
	{
		controls = m_pSystemControlsWidget->GetSelectedControls();
	}

	return controls;
}
} // namespace ACE
