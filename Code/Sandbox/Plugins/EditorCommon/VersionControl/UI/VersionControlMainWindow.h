// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include <EditorCommonAPI.h>
#include <EditorFramework/Editor.h>
#include <QWidget>

class CVersionControlWorkspaceOverviewTab;
class CVersionControlHistoryTab;
class CVersionControlSettingsTab;
class QTabWidget;

//! Main widget for version control system.
class EDITOR_COMMON_API CVersonControlMainView : public QWidget
{
	Q_OBJECT
public:
	CVersonControlMainView(QWidget* pParent = nullptr);

private:
	CVersionControlWorkspaceOverviewTab* m_pChangelistsTab{ nullptr };
	CVersionControlHistoryTab*           m_pHistoryTab{ nullptr };
	CVersionControlSettingsTab*          m_pSettingsTab{ nullptr };
	QTabWidget*                          m_pTabWidget{ nullptr };
};

//! Main dockable window for version control system.
class EDITOR_COMMON_API CVersionControlMainWindow : public CDockableEditor
{
	Q_OBJECT
public:
	CVersionControlMainWindow(QWidget* pParent = nullptr);

	CVersonControlMainView* GetView()                      { return m_pView; }

	virtual const char*     GetEditorName() const override { return "Version Control System"; }

private:
	CVersonControlMainView* m_pView{ nullptr };
};