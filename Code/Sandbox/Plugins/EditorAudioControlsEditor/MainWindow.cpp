// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MainWindow.h"

#include "AudioControlsEditorPlugin.h"
#include "SystemControlsIcons.h"
#include "PreferencesDialog.h"
#include "SystemAssetsManager.h"
#include "ImplementationManager.h"
#include "SystemControlsWidget.h"
#include "PropertiesWidget.h"
#include "MiddlewareDataWidget.h"
#include "FileMonitor.h"

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryString/CryPath.h>
#include <QtUtil.h>
#include <CryIcon.h>
#include <Controls/QuestionDialog.h>

#include <QAction>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CMainWindow::CMainWindow()
	: m_pAssetsManager(CAudioControlsEditorPlugin::GetAssetsManager())
	, m_pSystemControlsWidget(nullptr)
	, m_pPropertiesWidget(nullptr)
	, m_pMiddlewareDataWidget(nullptr)
	, m_pImplNameLabel(new QLabel(this))
	, m_pToolBar(new QToolBar("ACE Tools", this))
	, m_pMonitorSystem(new CFileMonitorSystem(1000, *m_pAssetsManager, this))
	, m_pMonitorMiddleware(new CFileMonitorMiddleware(1000, *m_pAssetsManager, this))
	, m_isModified(false)
	, m_isReloading(false)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setObjectName(GetEditorName());

	if (m_pAssetsManager->IsLoading())
	{
		// The middleware is being swapped out therefore we must not
		// reload it and must not call signalAboutToLoad and signalLoaded!
		CAudioControlsEditorPlugin::ReloadModels(false);
	}
	else
	{
		CAudioControlsEditorPlugin::SignalAboutToLoad();
		CAudioControlsEditorPlugin::ReloadModels(true);
		CAudioControlsEditorPlugin::SignalLoaded();
	}

	m_isModified = m_pAssetsManager->IsDirty();

	auto const pWindowLayout = new QVBoxLayout(this);
	pWindowLayout->setContentsMargins(0, 0, 0, 0);

	InitMenuBar();

	layout()->removeWidget(m_pMenuBar);

	auto const pTopBox = new QHBoxLayout(this);
	pTopBox->setMargin(0);
	pTopBox->addWidget(m_pMenuBar, 0, Qt::AlignLeft);
	pTopBox->addWidget(m_pImplNameLabel, 0, Qt::AlignRight);

	QWidget* const pTopBar = new QWidget(this);
	pTopBar->setLayout(pTopBox);

	layout()->addWidget(pTopBar);

	InitToolbar(pWindowLayout);
	SetContent(pWindowLayout);
	RegisterWidgets();

	m_pAssetsManager->UpdateAllConnectionStates();
	CheckErrorMask();
	UpdateImplLabel();
	m_pToolBar->setEnabled(g_pEditorImpl != nullptr);

	QObject::connect(m_pMonitorSystem, &CFileMonitorSystem::SignalReloadData, this, &CMainWindow::ReloadSystemData);
	QObject::connect(m_pMonitorMiddleware, &CFileMonitorMiddleware::SignalReloadData, this, &CMainWindow::ReloadMiddlewareData);

	m_pAssetsManager->SignalIsDirty.Connect([&](bool const isDirty)
		{
			m_isModified = isDirty;
			m_pSaveAction->setEnabled(isDirty);
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.Connect(this, &CMainWindow::SaveBeforeImplementationChange);
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.Connect([this]()
		{
			UpdateImplLabel();
			Reload();

			m_pToolBar->setEnabled(g_pEditorImpl != nullptr);
	  }, reinterpret_cast<uintptr_t>(this));

	GetIEditor()->RegisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
CMainWindow::~CMainWindow()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalIsDirty.DisconnectById(reinterpret_cast<uintptr_t>(this));
	GetIEditor()->UnregisterNotifyListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::InitMenuBar()
{
	CEditor::MenuItems const items[] = { CEditor::MenuItems::EditMenu };
	AddToMenu(items, sizeof(items) / sizeof(CEditor::MenuItems));

	CAbstractMenu* const pMenuEdit = GetMenu(MenuItems::EditMenu);
	QAction const* const pPreferencesAction = pMenuEdit->CreateAction(tr("Preferences..."));
	QObject::connect(pPreferencesAction, &QAction::triggered, this, &CMainWindow::OnPreferencesDialog);
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::InitToolbar(QVBoxLayout* const pWindowLayout)
{
	auto const pToolBarsLayout = new QHBoxLayout(this);
	pToolBarsLayout->setDirection(QBoxLayout::LeftToRight);
	pToolBarsLayout->setSizeConstraint(QLayout::SetMaximumSize);

	m_pSaveAction = m_pToolBar->addAction(CryIcon("icons:General/File_Save.ico"), QString());
	m_pSaveAction->setToolTip(tr("Save all changes"));
	m_pSaveAction->setEnabled(m_isModified);
	QObject::connect(m_pSaveAction, &QAction::triggered, this, &CMainWindow::Save);

	QAction* const pReloadAction = m_pToolBar->addAction(CryIcon("icons:General/Reload.ico"), QString());
	pReloadAction->setToolTip(tr("Reload all ACE and middleware files"));
	QObject::connect(pReloadAction, &QAction::triggered, this, &CMainWindow::Reload);

	QAction* const pRefreshAudioSystemAction = m_pToolBar->addAction(CryIcon("icons:Audio/Refresh_Audio.ico"), QString());
	pRefreshAudioSystemAction->setToolTip(tr("Refresh Audio System"));
	QObject::connect(pRefreshAudioSystemAction, &QAction::triggered, this, &CMainWindow::RefreshAudioSystem);

	pToolBarsLayout->addWidget(m_pToolBar, 0, Qt::AlignLeft);

	pWindowLayout->addLayout(pToolBarsLayout);
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::RegisterWidgets()
{
	EnableDockingSystem();

	RegisterDockableWidget("Audio System Controls", [&]() { return CreateSystemControlsWidget(); }, true, false);
	RegisterDockableWidget("Properties", [&]() { return CreatePropertiesWidget(); }, true, false);
	RegisterDockableWidget("Middleware Data", [&]() { return CreateMiddlewareDataWidget(); }, true, false);
}

//////////////////////////////////////////////////////////////////////////
CSystemControlsWidget* CMainWindow::CreateSystemControlsWidget()
{
	auto pSystemControlsWidget = new CSystemControlsWidget(m_pAssetsManager, this);

	if (m_pSystemControlsWidget == nullptr)
	{
		m_pSystemControlsWidget = pSystemControlsWidget;
	}

	QObject::connect(pSystemControlsWidget, &CSystemControlsWidget::SignalSelectedControlChanged, this, &CMainWindow::SignalSelectedSystemControlChanged);
	QObject::connect(this, &CMainWindow::SignalSelectConnectedSystemControl, pSystemControlsWidget, &CSystemControlsWidget::SelectConnectedSystemControl);
	QObject::connect(pSystemControlsWidget, &QObject::destroyed, this, &CMainWindow::OnSystemControlsWidgetDestruction);

	return pSystemControlsWidget;
}

//////////////////////////////////////////////////////////////////////////
CPropertiesWidget* CMainWindow::CreatePropertiesWidget()
{
	auto pPropertiesWidget = new CPropertiesWidget(m_pAssetsManager, this);

	if (m_pPropertiesWidget == nullptr)
	{
		m_pPropertiesWidget = pPropertiesWidget;
	}

	QObject::connect(this, &CMainWindow::SignalSelectedSystemControlChanged, [&]()
		{
			if (m_pPropertiesWidget != nullptr)
			{
			  m_pPropertiesWidget->OnSetSelectedAssets(GetSelectedSystemAssets(), !m_isReloading);
			}
	  });

	QObject::connect(pPropertiesWidget, &QObject::destroyed, this, &CMainWindow::OnPropertiesWidgetDestruction);
	QObject::connect(pPropertiesWidget, &CPropertiesWidget::SignalSelectConnectedImplItem, this, &CMainWindow::SignalSelectConnectedImplItem);

	m_pPropertiesWidget->OnSetSelectedAssets(GetSelectedSystemAssets(), !m_isReloading);
	return pPropertiesWidget;
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget* CMainWindow::CreateMiddlewareDataWidget()
{
	auto pMiddlewareDataWidget = new CMiddlewareDataWidget(m_pAssetsManager, this);

	if (m_pMiddlewareDataWidget == nullptr)
	{
		m_pMiddlewareDataWidget = pMiddlewareDataWidget;
	}

	QObject::connect(pMiddlewareDataWidget, &QObject::destroyed, this, &CMainWindow::OnMiddlewareDataWidgetDestruction);
	QObject::connect(pMiddlewareDataWidget, &CMiddlewareDataWidget::SignalSelectConnectedSystemControl, this, &CMainWindow::SignalSelectConnectedSystemControl);
	QObject::connect(this, &CMainWindow::SignalSelectConnectedImplItem, pMiddlewareDataWidget, &CMiddlewareDataWidget::SelectConnectedImplItem);

	return pMiddlewareDataWidget;
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnSystemControlsWidgetDestruction(QObject* const pObject)
{
	auto pWidget = static_cast<CSystemControlsWidget*>(pObject);

	if (m_pSystemControlsWidget == pWidget)
	{
		m_pSystemControlsWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnPropertiesWidgetDestruction(QObject* const pObject)
{
	auto pWidget = static_cast<CPropertiesWidget*>(pObject);

	if (m_pPropertiesWidget == pWidget)
	{
		m_pPropertiesWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnMiddlewareDataWidgetDestruction(QObject* const pObject)
{
	auto pWidget = static_cast<CMiddlewareDataWidget*>(pObject);

	if (m_pMiddlewareDataWidget == pWidget)
	{
		m_pMiddlewareDataWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::CreateDefaultLayout(CDockableContainer* pSender)
{
	CRY_ASSERT_MESSAGE(pSender != nullptr, "Dockable container is null pointer.");

	pSender->SpawnWidget("Audio System Controls");
	pSender->SpawnWidget("Properties", QToolWindowAreaReference::VSplitRight);
	QWidget* pWidget = pSender->SpawnWidget("Middleware Data", QToolWindowAreaReference::VSplitRight);
	pSender->SetSplitterSizes(pWidget, { 1, 1, 1 });
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::closeEvent(QCloseEvent* pEvent)
{
	if (TryClose())
	{
		pEvent->accept();
	}
	else
	{
		pEvent->ignore();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::keyPressEvent(QKeyEvent* pEvent)
{
	if ((m_pSystemControlsWidget != nullptr) && (!m_pSystemControlsWidget->IsEditing()))
	{
		if ((pEvent->key() == Qt::Key_S) && (pEvent->modifiers() == Qt::ControlModifier))
		{
			Save();
		}
		else if ((pEvent->key() == Qt::Key_Z) && (pEvent->modifiers() & Qt::ControlModifier))
		{
			if (pEvent->modifiers() & Qt::ShiftModifier)
			{
				// GetIEditor()->GetIUndoManager()->Redo();
			}
			else
			{
				// GetIEditor()->GetIUndoManager()->Undo();
			}
		}
	}

	QWidget::keyPressEvent(pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::UpdateImplLabel()
{
	if (g_pEditorImpl != nullptr)
	{
		m_pImplNameLabel->setText(QtUtil::ToQString(g_pEditorImpl->GetName()));
	}
	else
	{
		m_pImplNameLabel->setText(tr("Warning: No middleware implementation!"));
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::Reload()
{
	if (m_pAssetsManager != nullptr)
	{
		m_pMonitorSystem->Disable();
		m_pMonitorMiddleware->Disable();

		bool shouldReload = true;

		if (m_isModified)
		{
			auto const messageBox = new CQuestionDialog();
			messageBox->SetupQuestion(tr(GetEditorName()), tr("If you reload you will lose all your unsaved changes.\nAre you sure you want to reload?"), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
			shouldReload = (messageBox->Execute() == QDialogButtonBox::Yes);
		}

		if (shouldReload)
		{
			m_isReloading = true;
			QGuiApplication::setOverrideCursor(Qt::WaitCursor);
			BackupTreeViewStates();

			if (m_pAssetsManager->IsLoading())
			{
				// The middleware is being swapped out therefore we must not
				// reload it and must not call signalAboutToLoad and signalLoaded!
				CAudioControlsEditorPlugin::ReloadModels(false);
			}
			else
			{
				CAudioControlsEditorPlugin::SignalAboutToLoad();
				CAudioControlsEditorPlugin::ReloadModels(true);
				CAudioControlsEditorPlugin::SignalLoaded();
			}

			if (m_pMiddlewareDataWidget != nullptr)
			{
				m_pMiddlewareDataWidget->Reset();
			}

			if (m_pSystemControlsWidget != nullptr)
			{
				m_pSystemControlsWidget->Reload();
			}

			if (m_pPropertiesWidget != nullptr)
			{
				m_pPropertiesWidget->OnSetSelectedAssets(GetSelectedSystemAssets(), false);
				m_pPropertiesWidget->Reload();
			}

			m_pAssetsManager->UpdateAllConnectionStates();
			CheckErrorMask();

			RestoreTreeViewStates();
			QGuiApplication::restoreOverrideCursor();
			m_isReloading = false;
		}

		m_pMonitorSystem->Enable();
		m_pMonitorMiddleware->Enable();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::SaveBeforeImplementationChange()
{
	if (m_pAssetsManager != nullptr)
	{
		if (m_isModified)
		{
			auto const messageBox = new CQuestionDialog();
			messageBox->SetupQuestion(tr(GetEditorName()), tr("Middleware implementation changed.\nThere are unsaved changes.\nDo you want to save before reloading?"), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);

			if (messageBox->Execute() == QDialogButtonBox::Yes)
			{
				m_pAssetsManager->ClearDirtyFlags();
				Save();
			}
		}

		m_pAssetsManager->ClearDirtyFlags();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::CheckErrorMask()
{
	EErrorCode const errorCodeMask = CAudioControlsEditorPlugin::GetLoadingErrorMask();

	if ((errorCodeMask& EErrorCode::UnkownPlatform) != 0)
	{
		CQuestionDialog::SWarning(tr(GetEditorName()), tr("Audio Preloads reference an unknown platform.\nSaving will permanently erase this data."));
	}
	else if ((errorCodeMask& EErrorCode::NonMatchedActivityRadius) != 0)
	{
		if (g_pEditorImpl != nullptr)
		{
			QString const middlewareName = g_pEditorImpl->GetName();
			CQuestionDialog::SWarning(tr(GetEditorName()), tr("The attenuation of some controls has changed in your ") + middlewareName + tr(" project.\n\nActivity radius of triggers will be updated next time you save."));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::Save()
{
	if (m_pAssetsManager != nullptr)
	{
		QGuiApplication::setOverrideCursor(Qt::WaitCursor);
		m_pMonitorSystem->Disable();
		CAudioControlsEditorPlugin::SaveModels();
		UpdateAudioSystemData();
		m_pMonitorSystem->EnableDelayed();
		QGuiApplication::restoreOverrideCursor();

		// if preloads have been modified, ask the user if s/he wants to refresh the audio system
		if (m_pAssetsManager->IsTypeDirty(ESystemItemType::Preload))
		{
			auto const messageBox = new CQuestionDialog();
			messageBox->SetupQuestion(tr(GetEditorName()), tr("Preload requests have been modified. \n\nFor the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio. Do you want to do this now?. \n\nYou can always refresh manually at a later time through the Audio menu."), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);

			if (messageBox->Execute() == QDialogButtonBox::Yes)
			{
				RefreshAudioSystem();
			}
		}

		m_pAssetsManager->ClearDirtyFlags();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::UpdateAudioSystemData()
{
	string levelPath = CryAudio::s_szLevelsFolderName;
	levelPath += CRY_NATIVE_PATH_SEPSTR;
	levelPath += GetIEditor()->GetLevelName();
	gEnv->pAudioSystem->ReloadControlsData(gEnv->pAudioSystem->GetConfigPath(), levelPath.c_str());
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::RefreshAudioSystem()
{
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	char const* szLevelName = GetIEditor()->GetLevelName();

	if (_stricmp(szLevelName, "Untitled") == 0)
	{
		// Rather pass nullptr to indicate that no level is loaded!
		szLevelName = nullptr;
	}

	CryAudio::SRequestUserData const data(CryAudio::ERequestFlags::ExecuteBlocking);
	gEnv->pAudioSystem->Refresh(szLevelName, data);
	QGuiApplication::restoreOverrideCursor();
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
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
void CMainWindow::ReloadSystemData()
{
	if (m_pAssetsManager != nullptr)
	{
		m_pMonitorSystem->Disable();

		bool shouldReload = true;
		char const* messageText;

		if (m_isModified)
		{
			messageText = "External changes have been made to audio controls files.\nIf you reload you will lose all your unsaved changes.\nAre you sure you want to reload?";
		}
		else
		{
			messageText = "External changes have been made to audio controls files.\nDo you want to reload?";
		}

		auto const messageBox = new CQuestionDialog();
		messageBox->SetupQuestion(tr(GetEditorName()), tr(messageText), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
		shouldReload = (messageBox->Execute() == QDialogButtonBox::Yes);

		if (shouldReload)
		{
			m_pAssetsManager->ClearDirtyFlags();
			Reload();
		}
		else
		{
			m_pMonitorSystem->Enable();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::ReloadMiddlewareData()
{
	if (m_pAssetsManager != nullptr)
	{
		if (m_isModified)
		{
			m_pMonitorMiddleware->Disable();

			char const* messageText = "External changes have been made to middleware data.\n\nWarning: If middleware data gets refreshed without saving audio control files, unsaved connection changes will get lost!\n\nDo you want to save before refreshing middleware data?";
			auto const messageBox = new CQuestionDialog();
			messageBox->SetupQuestion(tr(GetEditorName()), tr(messageText), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::Yes);

			if (messageBox->Execute() == QDialogButtonBox::Yes)
			{
				Save();
			}

			m_pMonitorMiddleware->Enable();
		}
	}

	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	BackupTreeViewStates();

	if (g_pEditorImpl != nullptr)
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_COMMENT, "[Audio Controls Editor] Reloading audio implementation data");
		g_pEditorImpl->Reload();
	}

	m_pAssetsManager->ClearAllConnections();
	m_pAssetsManager->ReloadAllConnections();
	m_pAssetsManager->UpdateAllConnectionStates();

	if (m_pMiddlewareDataWidget != nullptr)
	{
		m_pMiddlewareDataWidget->Reset();
	}

	if (m_pPropertiesWidget != nullptr)
	{
		m_pPropertiesWidget->Reload();
	}

	RestoreTreeViewStates();
	QGuiApplication::restoreOverrideCursor();
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::BackupTreeViewStates()
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
void CMainWindow::RestoreTreeViewStates()
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
void CMainWindow::OnPreferencesDialog()
{
	auto const pPreferencesDialog = new CPreferencesDialog(this);

	QObject::connect(pPreferencesDialog, &CPreferencesDialog::SignalImplementationSettingsAboutToChange, [&]()
		{
			BackupTreeViewStates();
	  });

	QObject::connect(pPreferencesDialog, &CPreferencesDialog::SignalImplementationSettingsChanged, [&]()
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
std::vector<CSystemAsset*> CMainWindow::GetSelectedSystemAssets()
{
	std::vector<CSystemAsset*> assets;

	if (m_pSystemControlsWidget != nullptr)
	{
		assets = m_pSystemControlsWidget->GetSelectedAssets();
	}

	return assets;
}

//////////////////////////////////////////////////////////////////////////
bool CMainWindow::TryClose()
{
	bool canClose = true;

	if (m_isModified)
	{
		if (!GetIEditor()->IsMainFrameClosing())
		{
			QString const title = tr("Closing %1").arg(GetEditorName());
			auto const button = CQuestionDialog::SQuestion(title, tr("There are unsaved changes.\nDo you want to save your changes?"), QDialogButtonBox::Save | QDialogButtonBox::Discard | QDialogButtonBox::Cancel, QDialogButtonBox::Cancel);

			switch (button)
			{
			case QDialogButtonBox::Save:
				Save();
				break;
			case QDialogButtonBox::Discard:
				CAudioControlsEditorPlugin::SignalAboutToLoad();
				CAudioControlsEditorPlugin::ReloadModels(false);
				CAudioControlsEditorPlugin::SignalLoaded();
				break;
			case QDialogButtonBox::Cancel: // Intentional fall-through.
			default:
				canClose = false;
				break;
			}
		}
	}

	return canClose;
}

//////////////////////////////////////////////////////////////////////////
bool CMainWindow::CanQuit(std::vector<string>& unsavedChanges)
{
	bool canQuit = true;

	if ((m_pAssetsManager != nullptr) && m_isModified)
	{
		for (char const* const modifiedLibrary : m_pAssetsManager->GetModifiedLibraries())
		{
			string const reason = QtUtil::ToString(tr("'%1' has unsaved modifications.").arg(modifiedLibrary));
			unsavedChanges.emplace_back(reason);
		}

		canQuit = false;
	}

	return canQuit;
}
} // namespace ACE
