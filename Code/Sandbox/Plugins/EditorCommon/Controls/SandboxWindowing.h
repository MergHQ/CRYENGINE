// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include "QToolWindowManager/QCustomWindowFrame.h"
#include "QToolWindowManager/QToolWindowCustomWrapper.h"
#include "QToolWindowManager/QToolWindowManager.h"
#include "QToolWindowManager/QToolWindowArea.h"

struct IPane;
class QHBoxLayout;

//Extra icon keys for the TWM.
#define SANDBOX_WRAPPER_MINIMIZE_ICON "sandboxMinimizeIcon"
#define SANDBOX_WRAPPER_MAXIMIZE_ICON "sandboxMaximizeIcon"
#define SANDBOX_WRAPPER_RESTORE_ICON  "sandboxRestoreIcon"
#define SANDBOX_WRAPPER_CLOSE_ICON    "sandboxWindowCloseIcon"

/*
   If a tool (inherits from CDockableEditor) is docked it will have a TabPane as owner,
   if it's floating it will be owned by a Sandbox wrapper which is basically a titlebar plus the QToolsMenuToolWindowArea
   Tools not using CDockableEditor and IPane are usually legacy mfc tools
 */

/*
   QBaseTabPane and QMFCPaneHost are used to recognize when a widget is :
   1) A qt widget, in this case a specialized QMenu is set based on the IPane GetPaneMenu() function
   2) A LEGACY Mfc widget, in this case a default help menu is set
   This classes are defined here and inherited in other modules, namely Sandbox and MFC.
 */

//This is specialized in EditorQt's ToolTabManager to also store an mfc window handle
class EDITOR_COMMON_API QBaseTabPane : public QFrame
{
	Q_OBJECT;

public:
	string m_title;
	string m_category;
	string m_class;

	bool   m_bViewCreated;

	IPane* m_pane;

	QSize  m_defaultSize;
	QSize  m_minimumSize;
};

//This is specialized MFCToolsPlugin to store an MFC window
class EDITOR_COMMON_API QMFCPaneHost : public QWidget
{
	Q_OBJECT;

public:
	QMFCPaneHost(QWidget* parent, Qt::WindowFlags flags) : QWidget(parent, flags)
	{}
};

// QSplitterHandle class that also lets us know when we have started resizing the layout
class EDITOR_COMMON_API QNotifierSplitterHandle : public QSplitterHandle
{
	Q_OBJECT;
public:
	QNotifierSplitterHandle(Qt::Orientation orientation, QSplitter* parent);
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
};

// QSplitter class that creates QNotifierSplitterHandle
class EDITOR_COMMON_API QNotifierSplitter : public QSizePreservingSplitter
{
	Q_OBJECT;
public:
	QNotifierSplitter(QWidget* parent = 0);

	QSplitterHandle* createHandle();
};

class EDITOR_COMMON_API QSandboxWindow : public QCustomWindowFrame
{
public:
	QSandboxWindow(QToolWindowManager* manager) : QCustomWindowFrame(), m_manager(manager), m_config() {}
	QSandboxWindow(QVariantMap config) : QCustomWindowFrame(), m_manager(nullptr), m_config(config) {}
	virtual void           ensureTitleBar();
	virtual void           keyPressEvent(QKeyEvent* keyEvent) override;
	virtual bool           eventFilter(QObject*, QEvent*) override;
	static QSandboxWindow* wrapWidget(QWidget* w, QToolWindowManager* manager);
private:
	QToolWindowManager* m_manager;
	QVariantMap         m_config;
};

class EDITOR_COMMON_API QSandboxWrapper : public QToolWindowCustomWrapper
{
public:
	QSandboxWrapper(QToolWindowManager* manager) : QToolWindowCustomWrapper(manager) {}
	virtual void ensureTitleBar();
	virtual void keyPressEvent(QKeyEvent* keyEvent) override;
	virtual bool eventFilter(QObject*, QEvent*) override;
};

class EDITOR_COMMON_API QSandboxTitleBar : public QCustomTitleBar
{
	Q_OBJECT;
public:
	QSandboxTitleBar(QWidget* parent, const QVariantMap& config);

public slots:
	void updateWindowStateButtons() override;

private:
	QVariantMap m_config;
};
// Subclassed to be able to add a toolbutton containing the menu for the toolpane
// Depending on the number of tabs the menu button is either shown as a corner widget on the tabbar or in the corner of the tabframe

// Allows access to members through friend definition
class EDITOR_COMMON_API QToolsMenuWindowSingleTabAreaFrame : public QToolWindowSingleTabAreaFrame
{
	Q_OBJECT;
public:
	QToolsMenuWindowSingleTabAreaFrame(QToolWindowManager* manager, QWidget* parent);
	QHBoxLayout* m_pUpperBarLayout;
	virtual void setContents(QWidget* widget) override;

	friend class QToolsMenuToolWindowArea;
};

// Handles the tabbar and decides where the menu button should be displayed
class EDITOR_COMMON_API QToolsMenuToolWindowArea : public QToolWindowArea
{
	Q_OBJECT;
public:
	explicit QToolsMenuToolWindowArea(QToolWindowManager* manager, QWidget* parent = 0);
	//Called when a window is dragger outside of a tabbar or inside one
	void adjustDragVisuals() Q_DECL_OVERRIDE;

protected:
	virtual bool shouldShowSingleTabFrame() Q_DECL_OVERRIDE;
	virtual bool event(QEvent* event) Q_DECL_OVERRIDE;

private:
	//Sent when the selected tabbar tab changes
	//This updates the menu to display in the menu button
	void OnCurrentChanged(int index);

	//If the window is docked there is going to be a QBaseTabPane encapsulating the IPane
	//If we have a floating window the pane will just be a direct child
	//If the tabpane contains an mfc widget the IPane pointer will be null
	std::pair<IPane*, QWidget*> FindIPaneInTabChildren(int tabIndex);

	//Sets the menu for the tab at currentIndex based on the widget type
	//If the widget is a qt widget (aka uses an IPane interface) the IPane menu will be used
	//If the widget is an mfc widget (derives from QMFCPaneHost) a default hel menu will be used
	//Return the widget that should be the window's focus proxy (this is needed for editor commands to work properly)
	QWidget* SetupMenu(int currentIndex);

	// Request to update menu button visibility. If the current panel has a menu it will be visible, otherwise it will
	// remain hidden
	void     UpdateMenuButtonVisibility();

	QMenu*   CreateDefaultMenu();

	QToolButton* m_menuButton;
};