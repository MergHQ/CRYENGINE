// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MainWindow.h"

#include "AudioControlsEditorPlugin.h"
#include "AssetsManager.h"
#include "ImplementationManager.h"
#include "PreferencesDialog.h"
#include "SystemControlsWidget.h"
#include "PropertiesWidget.h"
#include "MiddlewareDataWidget.h"
#include "FileMonitorMiddleware.h"
#include "FileMonitorSystem.h"
#include "Common/IImpl.h"
#include "Common/ModelUtils.h"

#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <QtUtil.h>
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
void OnBeforeReload()
{
	if (g_pSystemControlsWidget != nullptr)
	{
		g_pSystemControlsWidget->OnBeforeReload();
	}

	if (g_pIImpl != nullptr)
	{
		g_pIImpl->OnBeforeReload();
	}

	if (g_pPropertiesWidget != nullptr)
	{
		g_pPropertiesWidget->OnBeforeReload();
	}
}

//////////////////////////////////////////////////////////////////////////
void OnAfterReload()
{
	if (g_pSystemControlsWidget != nullptr)
	{
		g_pSystemControlsWidget->OnAfterReload();
	}

	if (g_pIImpl != nullptr)
	{
		g_pIImpl->OnAfterReload();
	}

	if (g_pPropertiesWidget != nullptr)
	{
		g_pPropertiesWidget->OnAfterReload();
	}
}

//////////////////////////////////////////////////////////////////////////
CMainWindow::CMainWindow()
	: m_pImplNameLabel(new QLabel(this))
	, m_pToolBar(new QToolBar("ACE Tools", this))
	, m_pMonitorSystem(new CFileMonitorSystem(1000, this))
	, m_isModified(false)
	, m_isReloading(false)
{
	g_pMainWindow = this;
	g_pFileMonitorMiddleware = new CFileMonitorMiddleware(500, this);

	setAttribute(Qt::WA_DeleteOnClose);
	setObjectName(GetEditorName());

	ModelUtils::InitIcons();

	if (g_assetsManager.IsLoading())
	{
		// The middleware is being swapped out therefore we must not
		// reload it and must not call signals!
		CAudioControlsEditorPlugin::ReloadData(EReloadFlags::ReloadSystemControls);
	}
	else
	{
		CAudioControlsEditorPlugin::ReloadData(EReloadFlags::ReloadSystemControls | EReloadFlags::ReloadImplData | EReloadFlags::SendSignals);
	}

	m_isModified = g_assetsManager.IsDirty();

	auto const pWindowLayout = new QVBoxLayout();
	pWindowLayout->setContentsMargins(0, 0, 0, 0);

	InitMenuBar();

	InitToolbar(pWindowLayout);
	SetContent(pWindowLayout);
	RegisterWidgets();

	if (g_pIImpl != nullptr)
	{
		g_assetsManager.UpdateAllConnectionStates();
		CheckErrorMask();
	}

	UpdateImplLabel();
	m_pToolBar->setEnabled(g_pIImpl != nullptr);

	QObject::connect(m_pMonitorSystem, &CFileMonitorSystem::SignalReloadData, this, &CMainWindow::ReloadSystemData);
	QObject::connect(g_pFileMonitorMiddleware, &CFileMonitorMiddleware::SignalReloadData, this, &CMainWindow::ReloadMiddlewareData);

	g_assetsManager.SignalIsDirty.Connect([this](bool const isDirty)
		{
			m_isModified = isDirty;
			m_pSaveAction->setEnabled(isDirty);
		}, reinterpret_cast<uintptr_t>(this));

	g_implementationManager.SignalOnBeforeImplementationChange.Connect(this, &CMainWindow::SaveBeforeImplementationChange);
	g_implementationManager.SignalOnAfterImplementationChange.Connect([this]()
		{
			UpdateImplLabel();
			Reload(true);

			m_pToolBar->setEnabled(g_pIImpl != nullptr);
		}, reinterpret_cast<uintptr_t>(this));

	GetIEditor()->RegisterNotifyListener(this);
	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this, "CAudioControlsEditorMainWindow");
}

//////////////////////////////////////////////////////////////////////////
CMainWindow::~CMainWindow()
{
	g_implementationManager.SignalOnBeforeImplementationChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_implementationManager.SignalOnAfterImplementationChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalIsDirty.DisconnectById(reinterpret_cast<uintptr_t>(this));
	GetIEditor()->UnregisterNotifyListener(this);
	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
	g_pMainWindow = nullptr;
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
	auto const pToolBarsLayout = new QHBoxLayout();
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
	pToolBarsLayout->addWidget(m_pImplNameLabel, 0, Qt::AlignRight);

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
	auto pSystemControlsWidget = new CSystemControlsWidget(this);

	if (g_pSystemControlsWidget == nullptr)
	{
		g_pSystemControlsWidget = pSystemControlsWidget;
	}

	QObject::connect(pSystemControlsWidget, &QObject::destroyed, this, &CMainWindow::OnSystemControlsWidgetDestruction);

	return pSystemControlsWidget;
}

//////////////////////////////////////////////////////////////////////////
CPropertiesWidget* CMainWindow::CreatePropertiesWidget()
{
	auto pPropertiesWidget = new CPropertiesWidget(this);

	if (g_pPropertiesWidget == nullptr)
	{
		g_pPropertiesWidget = pPropertiesWidget;
	}

	QObject::connect(pPropertiesWidget, &QObject::destroyed, this, &CMainWindow::OnPropertiesWidgetDestruction);

	g_pPropertiesWidget->OnSetSelectedAssets(GetSelectedAssets(), !m_isReloading);
	return pPropertiesWidget;
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget* CMainWindow::CreateMiddlewareDataWidget()
{
	auto pMiddlewareDataWidget = new CMiddlewareDataWidget(this);

	if (g_pMiddlewareDataWidget == nullptr)
	{
		g_pMiddlewareDataWidget = pMiddlewareDataWidget;
	}

	QObject::connect(pMiddlewareDataWidget, &QObject::destroyed, this, &CMainWindow::OnMiddlewareDataWidgetDestruction);

	return pMiddlewareDataWidget;
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnSystemControlsWidgetDestruction(QObject* const pObject)
{
	auto pWidget = static_cast<CSystemControlsWidget*>(pObject);

	if (g_pSystemControlsWidget == pWidget)
	{
		g_pSystemControlsWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnPropertiesWidgetDestruction(QObject* const pObject)
{
	auto pWidget = static_cast<CPropertiesWidget*>(pObject);

	if (g_pPropertiesWidget == pWidget)
	{
		g_pPropertiesWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnMiddlewareDataWidgetDestruction(QObject* const pObject)
{
	auto pWidget = static_cast<CMiddlewareDataWidget*>(pObject);

	if (g_pMiddlewareDataWidget == pWidget)
	{
		g_pMiddlewareDataWidget = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::CreateDefaultLayout(CDockableContainer* pSender)
{
	CRY_ASSERT_MESSAGE(pSender != nullptr, "Dockable container is null pointer during %s", __FUNCTION__);

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
	if ((g_pSystemControlsWidget != nullptr) && (!g_pSystemControlsWidget->IsEditing()))
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
	if (g_pIImpl != nullptr)
	{
		m_pImplNameLabel->setText(QtUtil::ToQString(g_implInfo.name.c_str()));
	}
	else
	{
		m_pImplNameLabel->setText(tr("Warning: No middleware implementation!"));
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::Reload(bool const hasImplChanged /*= false*/)
{
	m_pMonitorSystem->Disable();
	g_pFileMonitorMiddleware->Disable();

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

		if (!hasImplChanged)
		{
			OnBeforeReload();
		}

		if (g_assetsManager.IsLoading())
		{
			// The middleware is being swapped out therefore we must not
			// reload it and must not call signals!
			CAudioControlsEditorPlugin::ReloadData(EReloadFlags::ReloadSystemControls);
		}
		else
		{
			CAudioControlsEditorPlugin::ReloadData(EReloadFlags::ReloadSystemControls | EReloadFlags::ReloadImplData | EReloadFlags::SendSignals);
		}

		if (g_pSystemControlsWidget != nullptr)
		{
			g_pSystemControlsWidget->Reset();
		}

		if (!hasImplChanged)
		{
			if (g_pPropertiesWidget != nullptr)
			{
				g_pPropertiesWidget->OnSetSelectedAssets(GetSelectedAssets(), false);
				g_pPropertiesWidget->Reset();
			}
		}

		if (g_pIImpl != nullptr)
		{
			g_assetsManager.UpdateAllConnectionStates();
			CheckErrorMask();
		}

		if (!hasImplChanged)
		{
			OnAfterReload();
		}

		QGuiApplication::restoreOverrideCursor();
		m_isReloading = false;
	}

	m_pMonitorSystem->Enable();
	g_pFileMonitorMiddleware->Enable();
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::SaveBeforeImplementationChange()
{
	if (m_isModified)
	{
		auto const messageBox = new CQuestionDialog();
		messageBox->SetupQuestion(tr(GetEditorName()), tr("Middleware implementation changed.\nThere are unsaved changes.\nDo you want to save before reloading?"), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);

		if (messageBox->Execute() == QDialogButtonBox::Yes)
		{
			g_assetsManager.ClearDirtyFlags();
			Save();
		}
	}

	g_assetsManager.ClearDirtyFlags();
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::CheckErrorMask()
{
	EErrorCode const errorCodeMask = CAudioControlsEditorPlugin::GetLoadingErrorMask();

	if ((errorCodeMask& EErrorCode::UnkownPlatform) != 0)
	{
		CQuestionDialog::SWarning(tr(GetEditorName()), tr("Audio Preloads reference an unknown platform.\nSaving will permanently erase this data."));
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::Save()
{
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	m_pMonitorSystem->Disable();
	CAudioControlsEditorPlugin::SaveData();
	UpdateAudioSystemData();
	m_pMonitorSystem->EnableDelayed();
	QGuiApplication::restoreOverrideCursor();

	// if preloads have been modified, ask the user if s/he wants to refresh the audio system
	if (g_assetsManager.IsTypeDirty(EAssetType::Preload))
	{
		auto const messageBox = new CQuestionDialog();
		messageBox->SetupQuestion(tr(GetEditorName()), tr("Preload requests have been modified. \n\nFor the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio. Do you want to do this now?. \n\nYou can always refresh manually at a later time through the Audio menu."), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);

		if (messageBox->Execute() == QDialogButtonBox::Yes)
		{
			RefreshAudioSystem();
		}
	}

	g_assetsManager.ClearDirtyFlags();
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::UpdateAudioSystemData()
{
	string levelPath = CryAudio::g_szLevelsFolderName;
	levelPath += "/";
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

	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_REFRESH, reinterpret_cast<UINT_PTR>(szLevelName), 0);
	QGuiApplication::restoreOverrideCursor();
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	if (event == eNotify_OnEndSceneSave)
	{
		CAudioControlsEditorPlugin::ReloadData(EReloadFlags::ReloadScopes);

		if (g_pPropertiesWidget != nullptr)
		{
			g_pPropertiesWidget->Reset();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_AUDIO_LANGUAGE_CHANGED:
		ReloadMiddlewareData();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::ReloadSystemData()
{
	m_pMonitorSystem->Disable();
	bool shouldReload = true;

	if (m_isModified)
	{
		auto const messageBox = new CQuestionDialog();
		char const* const szMessageText = "External changes have been made to audio controls files.\nIf you reload you will lose all your unsaved changes.\nAre you sure you want to reload?";
		messageBox->SetupQuestion(tr(GetEditorName()), tr(szMessageText), QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No);
		shouldReload = (messageBox->Execute() == QDialogButtonBox::Yes);
	}

	if (shouldReload)
	{
		g_assetsManager.ClearDirtyFlags();
		Reload();
	}
	else
	{
		m_pMonitorSystem->Enable();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::ReloadMiddlewareData()
{
	g_pFileMonitorMiddleware->Disable();
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	OnBeforeReload();

	CAudioControlsEditorPlugin::ReloadData(EReloadFlags::ReloadImplData | EReloadFlags::BackupConnections);

	if (g_pPropertiesWidget != nullptr)
	{
		g_pPropertiesWidget->Reset();
	}

	OnAfterReload();
	QGuiApplication::restoreOverrideCursor();
	g_pFileMonitorMiddleware->Enable();
}

//////////////////////////////////////////////////////////////////////////
void CMainWindow::OnPreferencesDialog()
{
	auto const pPreferencesDialog = new CPreferencesDialog(this);

	QObject::connect(pPreferencesDialog, &CPreferencesDialog::SignalOnBeforeImplementationSettingsChange, [&]()
		{
			OnBeforeReload();
		});

	QObject::connect(pPreferencesDialog, &CPreferencesDialog::SignalOnAfterImplementationSettingsChanged, [&]()
		{
			if (g_pPropertiesWidget != nullptr)
			{
			  g_pPropertiesWidget->Reset();
			}

			OnAfterReload();
			g_pFileMonitorMiddleware->Enable();
		});

	pPreferencesDialog->exec();
}

//////////////////////////////////////////////////////////////////////////
Assets CMainWindow::GetSelectedAssets()
{
	Assets assets;

	if (g_pSystemControlsWidget != nullptr)
	{
		assets = g_pSystemControlsWidget->GetSelectedAssets();
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
				CAudioControlsEditorPlugin::ReloadData(EReloadFlags::ReloadSystemControls | EReloadFlags::SendSignals);
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

	if (m_isModified)
	{
		for (char const* const modifiedLibrary : g_assetsManager.GetModifiedLibraries())
		{
			string const reason = QtUtil::ToString(tr("'%1' has unsaved modifications.").arg(modifiedLibrary));
			unsavedChanges.emplace_back(reason);
		}

		canQuit = false;
	}

	return canQuit;
}
} // namespace ACE
