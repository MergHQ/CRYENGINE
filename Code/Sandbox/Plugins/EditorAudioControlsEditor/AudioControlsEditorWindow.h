// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QMainWindow>
#include <IEditor.h>
#include "AudioAssetsManager.h"
#include <QFileSystemWatcher>
#include <QtViewPane.h>
#include <qobjectdefs.h>

class QSplitter;
class QAction;

namespace ACE
{
class CAudioAssetsManager;
class CAudioAssetsExplorer;
class CInspectorPanel;
class CAudioSystemPanel;
class CAudioControl;
class CFileMonitorSystem;
class CFileMonitorMiddleware;

class CAudioControlsEditorWindow final : public CDockableWindow, public IEditorNotifyListener
{
	Q_OBJECT

public:

	CAudioControlsEditorWindow();
	virtual ~CAudioControlsEditorWindow() override;

	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
	// ~IEditorNotifyListener

	//////////////////////////////////////////////////////////
	// CDockableWindow implementation
	virtual const char*                       GetPaneTitle() const override        { return "Audio Controls Editor"; };
	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	//////////////////////////////////////////////////////////

	void ReloadSystemData();
	void ReloadMiddlewareData();

protected:

	virtual void keyPressEvent(QKeyEvent* pEvent) override;
	virtual void closeEvent(QCloseEvent* pEvent) override;

private:

	void Reload();
	void Save();
	void SaveBeforeImplementationChange();
	void FilterControlType(EItemType type, bool bShow);
	void CheckErrorMask();
	void UpdateAudioSystemData();
	void BackupTreeViewStates();
	void RestoreTreeViewStates();

	CAudioAssetsManager*                                m_pAssetsManager;
	CAudioAssetsExplorer*                               m_pExplorer;
	CInspectorPanel*                                    m_pInspectorPanel;
	CAudioSystemPanel*                                  m_pAudioSystemPanel;
	std::unique_ptr<CFileMonitorSystem>                 m_pMonitorSystem;
	std::unique_ptr<CFileMonitorMiddleware>             m_pMonitorMiddleware;
	bool                                                m_allowedTypes[EItemType::eItemType_NumTypes];

	QSplitter* m_pSplitter;
	QAction*   m_pSaveAction;
};
} // namespace ACE
