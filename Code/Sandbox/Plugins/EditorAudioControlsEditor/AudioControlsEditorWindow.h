// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QMainWindow>
#include <IEditor.h>
#include "ATLControlsModel.h"
#include <QFileSystemWatcher>
#include <QtViewPane.h>
#include <qobjectdefs.h>
#include <FileSystem/FileSystem_SubTreeMonitor.h>
#include <FileSystem/FileSystem_Enumerator.h>
#include <FileSystem/FileSystem_FileFilter.h>

namespace ACE
{
class CATLControlsModel;
class CATLControlsPanel;
class CInspectorPanel;
class CAudioSystemPanel;
class CATLControl;

class CAudioControlsEditorWindow : public CDockableWindow, public IEditorNotifyListener
{
	Q_OBJECT
public:
	CAudioControlsEditorWindow();
	~CAudioControlsEditorWindow();
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

	//////////////////////////////////////////////////////////
	// CDockableWindow implementation
	virtual const char*                       GetPaneTitle() const override        { return "Audio Controls Editor"; };
	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	//////////////////////////////////////////////////////////

	void ReloadMiddlewareData();

private:
	void Reload();
	void Save();
	void UpdateFilterFromSelection();
	void FilterControlType(EACEControlType type, bool bShow);
	void Update();

protected:
	void keyPressEvent(QKeyEvent* pEvent);
	void closeEvent(QCloseEvent* pEvent);

private:
	void UpdateAudioSystemData();
	void StartWatchingFolder(const QString& folderPath);

	CATLControlsModel*                                  m_pATLModel;
	CATLControlsPanel*                                  m_pATLControlsPanel;
	CInspectorPanel*                                    m_pInspectorPanel;
	CAudioSystemPanel*                                  m_pAudioSystemPanel;
	FileSystem::SubTreeMonitorPtr                       m_pMonitor;
	std::vector<FileSystem::CEnumerator::MonitorHandle> m_watchingHandles;
	FileSystem::SFileFilter                             m_filter;
	bool m_allowedTypes[EACEControlType::eACEControlType_NumTypes];

};
}
