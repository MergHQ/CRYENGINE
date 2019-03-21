// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// CryCommon
#include <CrySandbox/CrySignal.h>

// Qt
#include <QWidget>

//EditorCommon
class CAbstractMenu;
class CEditor;

// Qt
class QHBoxLayout;
class QToolBar;

// In charge of creating and managing tool bar areas for all editors. It also serves as mediation between editors
// and editor toolbar service
class CEditorToolBarArea : public QWidget
{
	Q_OBJECT
public:
	CEditorToolBarArea(CEditor* pParent);

	void Initialize();
	bool CustomizeToolBar();
	void SetMenu(CAbstractMenu* pMenu);

protected:
	void paintEvent(QPaintEvent* pEvent) override;

private:
	void      FillMenu(CAbstractMenu* pMenu);
	void      ShowContextMenu();

	void      AddToolBar(QToolBar* pToolBar);

	void      OnToolBarAdded(QToolBar* pToolBar);
	void      OnToolBarModified(QToolBar* pToolBar);
	void      OnToolBarRemoved(const char* szToolBarName);

	QToolBar* FindToolBarByName(const char* szToolBarName);

	CCrySignal<void()>     signalCustomizeToolBar;

	std::vector<QToolBar*> m_toolbars;
	CEditor*               m_pEditor;
	QHBoxLayout*           m_pLayout;
};
