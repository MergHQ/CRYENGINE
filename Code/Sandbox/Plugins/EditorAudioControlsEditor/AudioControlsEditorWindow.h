// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <EditorFramework/Editor.h>
#include <IEditor.h>
#include "AudioAssetsManager.h"
#include <QFileSystemWatcher>
#include <QtViewPane.h>
#include <qobjectdefs.h>

class QAction;
class QVBoxLayout;

namespace ACE
{
class CAudioAssetsManager;
class CSystemControlsWidget;
class CPropertiesWidget;
class CMiddlewareDataWidget;
class CAudioControl;
class CFileMonitorSystem;
class CFileMonitorMiddleware;

class CAudioControlsEditorWindow final : public CDockableEditor, public IEditorNotifyListener
{
	Q_OBJECT

public:

	CAudioControlsEditorWindow();
	virtual ~CAudioControlsEditorWindow() override;

	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
	// ~IEditorNotifyListener

	// CDockableEditor
	virtual const char* GetEditorName() const override { return "Audio Controls Editor"; }
	// ~CDockableEditor

	// IPane
	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	// ~IPane

	void ReloadSystemData();
	void ReloadMiddlewareData();

protected:

	// QWidget
	virtual void keyPressEvent(QKeyEvent* pEvent) override;
	virtual void closeEvent(QCloseEvent* pEvent) override;
	// ~QWidget

	// CEditor
	virtual void CreateDefaultLayout(CDockableContainer* pSender) override;
	// ~CEditor

protected slots:

	void OnSystemControlsWidgetDestruction(QObject* pObject);
	void OnPropertiesWidgetDestruction(QObject* pObject);
	void OnMiddlewareDataWidgetDestruction(QObject* pObject);

signals:

	void OnSelectedSystemControlChanged();
	void OnStartTextFiltering();
	void OnStopTextFiltering();

private slots:

	void OnPreferencesDialog();

private:

	void InitMenu();
	void InitToolbar(QVBoxLayout* pWindowLayout);
	void RegisterWidgets();
	void Reload();
	void Save();
	void SaveBeforeImplementationChange();
	void FilterControlType(EItemType type, bool bShow);
	void CheckErrorMask();
	void UpdateAudioSystemData();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

	std::vector<CAudioControl*> GetSelectedSystemControls();

	CSystemControlsWidget* CreateSystemControlsWidget();
	CPropertiesWidget*     CreatePropertiesWidget();
	CMiddlewareDataWidget* CreateMiddlewareDataWidget();

	CAudioAssetsManager*                    m_pAssetsManager;
	CSystemControlsWidget*                  m_pSystemControlsWidget;
	CPropertiesWidget*                      m_pPropertiesWidget;
	CMiddlewareDataWidget*                  m_pMiddlewareDataWidget;
	QAction*                                m_pSaveAction;
	std::unique_ptr<CFileMonitorSystem>     m_pMonitorSystem;
	std::unique_ptr<CFileMonitorMiddleware> m_pMonitorMiddleware;
	bool                                    m_allowedTypes[EItemType::eItemType_NumTypes];
};
} // namespace ACE
