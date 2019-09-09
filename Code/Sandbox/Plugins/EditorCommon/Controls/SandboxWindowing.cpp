// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SandboxWindowing.h"
#include <EditorFramework/Editor.h>
#include <EditorFramework/Events.h>

#include <IEditor.h>
#include <CryIcon.h>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QSizePolicy>
#include <QToolButton>
#include <QtViewPane.h>

namespace Private_SandboxWindowing
{
bool SendMissedShortcutEvent(QKeyEvent* keyEvent)
{
	const auto key = keyEvent->key();
	if (key != Qt::Key_unknown && !(key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta))
	{
		MissedShortcutEvent* event = new MissedShortcutEvent(QKeySequence(key | keyEvent->modifiers()));
		return QApplication::sendEvent(getMainWindow(), event);
	}
	return false;
}
}

QSandboxTitleBar::QSandboxTitleBar(QWidget* parent, const QVariantMap& config) : QCustomTitleBar(parent), m_config(config)
{
	if (m_minimizeButton)
	{
		m_minimizeButton->setIcon(getIcon(m_config, SANDBOX_WRAPPER_MINIMIZE_ICON, CryIcon()));
	}
	if (m_maximizeButton)
	{
		if (parentWidget()->windowState().testFlag(Qt::WindowMaximized))
		{
			m_maximizeButton->setIcon(getIcon(m_config, SANDBOX_WRAPPER_RESTORE_ICON, CryIcon()));
		}
		else
		{
			m_maximizeButton->setIcon(getIcon(m_config, SANDBOX_WRAPPER_MAXIMIZE_ICON, CryIcon()));
		}
	}
	m_closeButton->setIcon(getIcon(m_config, SANDBOX_WRAPPER_CLOSE_ICON, CryIcon()));
}

void QSandboxTitleBar::updateWindowStateButtons()
{
	QCustomTitleBar::updateWindowStateButtons();
	if (parentWidget()->windowState().testFlag(Qt::WindowMaximized))
	{
		m_maximizeButton->setIcon(getIcon(m_config, SANDBOX_WRAPPER_RESTORE_ICON, CryIcon()));
	}
	else
	{
		m_maximizeButton->setIcon(getIcon(m_config, SANDBOX_WRAPPER_MAXIMIZE_ICON, CryIcon()));
	}
}

QSandboxWindow* QSandboxWindow::wrapWidget(QWidget* w, QToolWindowManager* manager)
{
	QSandboxWindow* windowFrame = new QSandboxWindow(manager);
	windowFrame->internalSetContents(w);
	return windowFrame;
}

void QSandboxWindow::ensureTitleBar()
{
	if (!m_titleBar)
	{
		m_titleBar = new QSandboxTitleBar(this, m_manager ? m_manager->config() : m_config);
	}
}

void QSandboxWindow::keyPressEvent(QKeyEvent* keyEvent)
{
	Private_SandboxWindowing::SendMissedShortcutEvent(keyEvent);
}

bool QSandboxWindow::eventFilter(QObject* o, QEvent* e)
{
	if (!m_manager && o == m_contents && e->type() == QEvent::KeyPress)
	{
		// broadcast not consumed shortcuts to sandbox main window
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
		if (Private_SandboxWindowing::SendMissedShortcutEvent(keyEvent))
			return true;
	}

	return QCustomWindowFrame::eventFilter(o, e);
}

void QSandboxWrapper::ensureTitleBar()
{
	if (!m_titleBar)
	{
		m_titleBar = new QSandboxTitleBar(this, m_manager->config());
	}
}

void QSandboxWrapper::keyPressEvent(QKeyEvent* keyEvent)
{
	Private_SandboxWindowing::SendMissedShortcutEvent(keyEvent);
}

bool QSandboxWrapper::eventFilter(QObject* o, QEvent* e)
{
	if (o == m_contents && e->type() == QEvent::KeyPress)
	{
		// broadcast not consumed shortcuts to sandbox main window
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
		if (Private_SandboxWindowing::SendMissedShortcutEvent(keyEvent))
			return true;
	}

	return QToolWindowCustomWrapper::eventFilter(o, e);
}

QNotifierSplitterHandle::QNotifierSplitterHandle(Qt::Orientation orientation, QSplitter* parent) : QSplitterHandle(orientation, parent)
{
}

void QNotifierSplitterHandle::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		// Notify editors that we have begun resizing
		GetIEditor()->Notify(eNotify_OnBeginLayoutResize);
	}

	QSplitterHandle::mousePressEvent(e);
}

void QNotifierSplitterHandle::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		// Notify editors that we have stopped resizing
		GetIEditor()->Notify(eNotify_OnEndLayoutResize);
	}

	QSplitterHandle::mouseReleaseEvent(e);
}

void QNotifierSplitterHandle::mouseMoveEvent(QMouseEvent* e)
{
	//QT 5.6: Screen position is incorrect in HiDPI contexts; so we fix it
	QMouseEvent e2(e->type(), e->localPos(), e->windowPos(), QCursor::pos(), e->button(), e->buttons(), e->modifiers());
	QSplitterHandle::mouseMoveEvent(&e2);
}

QNotifierSplitter::QNotifierSplitter(QWidget* parent /*= 0*/) : QSizePreservingSplitter(parent)
{
	setChildrenCollapsible(false);
}

QSplitterHandle* QNotifierSplitter::createHandle()
{
	return new QNotifierSplitterHandle(orientation(), this);
}

QToolsMenuWindowSingleTabAreaFrame::QToolsMenuWindowSingleTabAreaFrame(QToolWindowManager* manager, QWidget* parent)
	: QToolWindowSingleTabAreaFrame(manager, parent)
{
	m_pUpperBarLayout = new QHBoxLayout();
	m_pLayout->addLayout(m_pUpperBarLayout, 0, 0);
	m_pUpperBarLayout->addWidget(m_pCaption, Qt::AlignLeft);
}

void QToolsMenuWindowSingleTabAreaFrame::setContents(QWidget* widget)
{
	QToolWindowSingleTabAreaFrame::setContents(widget);
}

QToolsMenuToolWindowArea::QToolsMenuToolWindowArea(QToolWindowManager* manager, QWidget* parent)
	: QToolWindowArea(manager, parent)
{
	connect(tabBar(), &QTabBar::currentChanged, this, &QToolsMenuToolWindowArea::OnCurrentChanged);
	setMovable(true);
	m_menuButton = new QToolButton(this);
	//Used in qss to style this widget
	m_menuButton->setObjectName("QuickAccessButton");
	m_menuButton->setPopupMode(QToolButton::InstantPopup);
	m_menuButton->setVisible(false);
	m_menuButton->setIcon(CryIcon("icons:common/general_corner_menu.ico"));
	//Create temporary custom tab frame to be able to access m_layout
	QToolsMenuWindowSingleTabAreaFrame* customTabFrame = new QToolsMenuWindowSingleTabAreaFrame(manager, this);
	m_menuButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	addAction(GetIEditor()->GetICommandManager()->GetAction("general.open_editor_menu"));
	customTabFrame->m_pLayout->addWidget(m_menuButton, 0, 1, Qt::AlignVCenter | Qt::AlignRight);
	//Override the standard frame with our custom version
	m_pTabFrame = customTabFrame;
	//Without this all undocked mfc widgets will be black
	m_pTabFrame->hide();

	m_pTabFrame->installEventFilter(this);
	customTabFrame->m_pCaption->installEventFilter(this);
	tabBar()->installEventFilter(this);
}

void QToolsMenuToolWindowArea::OnCurrentChanged(int index)
{
	if (index < 0 || tabBar()->count() == 1)
	{
		return;
	}

	/*
	   This is needed to make EditorCommands work properly
	   A widget that derives from CEditor must be in focus
	 */
	QWidget* focusTarget = SetupMenu(index);
	m_menuButton->setFocusProxy(focusTarget);
	tabBar()->setFocusProxy(focusTarget);

	QToolsMenuWindowSingleTabAreaFrame* customTabFrame = qobject_cast<QToolsMenuWindowSingleTabAreaFrame*>(m_pTabFrame);

	if (tabBar()->count() > 1)
	{
		setCornerWidget(m_menuButton);
	}
	else if (tabBar()->count() == 1)
	{
		customTabFrame->m_pLayout->addWidget(m_menuButton, 0, 1, Qt::AlignVCenter | Qt::AlignRight);
	}

	UpdateMenuButtonVisibility();
}

void QToolsMenuToolWindowArea::adjustDragVisuals()
{
	int currentIdx = tabBar()->currentIndex();
	if (currentIdx >= 0)
	{
		/*
		   This is needed to make EditorCommands work properly
		   A widget that derives from CEditor must be in focus
		 */
		QWidget* focusTarget = SetupMenu(currentIdx);
		m_menuButton->setFocusProxy(focusTarget);
		tabBar()->setFocusProxy(focusTarget);

		QToolsMenuWindowSingleTabAreaFrame* customTabFrame = qobject_cast<QToolsMenuWindowSingleTabAreaFrame*>(m_pTabFrame);
		bool floatingWrapper = m_manager->isFloatingWrapper(parentWidget());

		if (tabBar()->count() > 1)
		{
			setCornerWidget(m_menuButton);
		}
		else if (tabBar()->count() == 1)
		{
			customTabFrame->m_pLayout->addWidget(m_menuButton, 0, 1, Qt::AlignVCenter | Qt::AlignRight);
		}

		/*
		   If we are floating we can hide the caption because we already
		   have the titlebar in Sandbox wrapper to show the title
		 */
		if (customTabFrame)
		{
			if (floatingWrapper)
			{
				customTabFrame->m_pCaption->hide();
			}
			else
			{
				customTabFrame->m_pCaption->show();
			}
		}

		UpdateMenuButtonVisibility();
	}

	QToolWindowArea::adjustDragVisuals();
}

bool QToolsMenuToolWindowArea::shouldShowSingleTabFrame()
{
	// If we don't allow single tab frames at all
	if (!m_manager->config().value(QTWM_SINGLE_TAB_FRAME, true).toBool())
	{
		return false;
	}
	// If we have one tool window in this area
	if (count() == 1)
	{
		return true;
	}
	return false;
}

bool QToolsMenuToolWindowArea::event(QEvent* event)
{
	if (event->type() == SandboxEvent::Command)
	{
		CommandEvent* commandEvent = static_cast<CommandEvent*>(event);

		//Note: this could be optimized this with a hash map
		//TODO : better system of macros and registration of those commands in EditorCommon (or move all of this in Editor)
		const string& command = commandEvent->GetCommand();
		if (command == "general.open_editor_menu")
		{
			if (m_menuButton->isVisible())
			{
				m_menuButton->showMenu();
			}

			return true;
		}
	}

	return QToolWindowArea::event(event);
}

std::pair<IPane*, QWidget*> QToolsMenuToolWindowArea::FindIPaneInTabChildren(int tabIndex)
{
	std::pair<IPane*, QWidget*> result{ nullptr, nullptr };
	QWidget* pWidgetAtIndex = widget(tabIndex);
	QBaseTabPane* pPaneProxy = qobject_cast<QBaseTabPane*>(pWidgetAtIndex);
	IPane* pPane = qobject_cast<IPane*>(pWidgetAtIndex);
	if (pPaneProxy)
	{
		result.first = pPaneProxy->m_pane;
		result.second = pPaneProxy;
		return result;
	}
	else if (pPane)
	{
		result.first = pPane;
		result.second = pWidgetAtIndex;
		return result;
	}

	for (QObject* object : pWidgetAtIndex->children())
	{
		QBaseTabPane* pPaneProxy = qobject_cast<QBaseTabPane*>(object);
		IPane* pPane = qobject_cast<IPane*>(object);

		result.second = qobject_cast<QWidget*>(object);

		if (pPaneProxy)
		{
			result.second = pPaneProxy;
			result.first = pPaneProxy->m_pane;
			break;
		}
		else if (pPane)
		{
			result.first = pPane;
			break;
		}
	}

	return result;
}

QWidget* QToolsMenuToolWindowArea::SetupMenu(int currentIndex)
{
	std::pair<IPane*, QWidget*> foundParents = FindIPaneInTabChildren(currentIndex);

	QWidget* ownerWidget = foundParents.second;
	IPane* pane = foundParents.first;

	setCornerWidget(0);

	QWidget* focusTarget = ownerWidget;
	QMenu* menuToAttach = nullptr;

	QMFCPaneHost* host = qobject_cast<QMFCPaneHost*>(ownerWidget);

	if (pane) //qt widget
	{
		menuToAttach = pane->GetPaneMenu();
		focusTarget = pane->GetWidget();
	}
	else if (ownerWidget && host) //Undocked MFC widget
	{

		if (host)
		{
			menuToAttach = CreateDefaultMenu();
			focusTarget = host;
		}
	}
	else if (ownerWidget && !host) //Docked MFC widget (QTabBar is the owner widget)
	{
		for (QObject* object : ownerWidget->children())
		{
			QMFCPaneHost* host = qobject_cast<QMFCPaneHost*>(object);

			if (host)
			{
				menuToAttach = CreateDefaultMenu();
				focusTarget = host;
				break;
			}
		}

	}

	m_menuButton->setMenu(menuToAttach);

	return focusTarget;
}

void QToolsMenuToolWindowArea::UpdateMenuButtonVisibility()
{
	if (!m_menuButton)
		return;

	if (m_menuButton->menu())
	{
		m_menuButton->setVisible(true);
	}
	else
	{
		m_menuButton->setVisible(false);
	}
}

QMenu* QToolsMenuToolWindowArea::CreateDefaultMenu()
{
	QMenu* helpMenu = new QMenu();
	QMenu* menuItem = helpMenu->addMenu("Help");
	menuItem->addAction(GetIEditor()->GetICommandManager()->GetAction("general.help"));
	return helpMenu;
}
