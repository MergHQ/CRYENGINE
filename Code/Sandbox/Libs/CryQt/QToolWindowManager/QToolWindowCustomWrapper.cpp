// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "QToolWindowCustomWrapper.h"
#include "QToolWindowManager.h"
#include "IToolWindowArea.h"
#include "QToolWindowTaskbarHandler.h"

#include <QGridLayout>
#include <QSpacerItem>
#include <QCloseEvent>
#include <QWindowStateChangeEvent>
#include <QToolButton>
#include <QApplication>
#include <QHoverEvent>
#include <QTime>
#include <QStyle>
#include <QDesktopWidget>

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#ifdef UNICODE
#define _UNICODE
#endif

#include <tchar.h>

#if QT_VERSION >= 0x050000
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
#else
#define qt_pixmapFromWinHICON(hIcon)QPixmap::fromWinHICON(hIcon)
#endif
#endif

QToolWindowCustomTitleBar::QToolWindowCustomTitleBar(QToolWindowCustomWrapper* parent)
	: QCustomTitleBar(parent)
{
}

QToolWindowCustomWrapper* QToolWindowCustomWrapper::wrapWidget(QWidget* w, QVariantMap config)
{
	return new QToolWindowCustomWrapper(nullptr, w, config);
}

QToolWindowCustomWrapper::QToolWindowCustomWrapper(QToolWindowManager* manager, QWidget* wrappedWidget, QVariantMap config)
	: QCustomWindowFrame()
	, m_manager(manager)	
{
	m_manager->installEventFilter(this);
	setStyleSheet(m_manager->styleSheet());

	if (manager->config().value(QTWM_WRAPPERS_ARE_CHILDREN, false).toBool())
	{
		setParent(manager);
	}

	setContents(wrappedWidget);
	AddToTaskbar();
}

QToolWindowCustomWrapper::~QToolWindowCustomWrapper()
{
	m_manager->removeWrapper(this);
	RemoveFromTaskbar();
}

bool QToolWindowCustomWrapper::event(QEvent* e)
{
	if (e->type() == QEvent::Show || e->type() == QEvent::Hide)
	{
		return QFrame::event(e); // Don't want QCustomWindowFrame custom handling of show and hide.
	}
	else if (e->type() == QEvent::Polish)
	{
		ensureTitleBar();
	}
	return QCustomWindowFrame::event(e);
}

void QToolWindowCustomWrapper::closeEvent(QCloseEvent* event)
{
	if (m_contents)
	{
		QList<QWidget*> toolWindows;

		for (QWidget* toolWindow : m_manager->toolWindows())
		{
			if (toolWindow->window() == this)
			{
				toolWindows << toolWindow;
			}
		}
		if (!m_manager->releaseToolWindows(toolWindows, true))
		{
			event->ignore();
		}
	}
}

bool QToolWindowCustomWrapper::eventFilter(QObject* o, QEvent* e)
{
	if (o == m_manager)
	{
		if (e->type() == QEvent::StyleChange && m_manager->styleSheet() != styleSheet())
		{
			setStyleSheet(m_manager->styleSheet());
		}
	}

	if (o == m_contents)
	{
		switch (e->type())
		{
			// Don't intercept these messages in parent class.
		case QEvent::Close:
		case QEvent::HideToParent:
		case QEvent::ShowToParent:
			return false;
		}
	}

	return QCustomWindowFrame::eventFilter(o, e);
}

void QToolWindowCustomWrapper::AddToTaskbar()
{
#ifdef QTWM_CREATE_TASKBAR_THUMBNAILS
	if (m_manager->config().value(QTWM_WRAPPER_REGISTER_IN_TASKBAR, true).toBool())
	{
		QToolWindowTaskbarHandler::Register(this);
	}
#endif
}

void QToolWindowCustomWrapper::RemoveFromTaskbar()
{
#ifdef QTWM_CREATE_TASKBAR_THUMBNAILS
	if (m_manager->config().value(QTWM_WRAPPER_REGISTER_IN_TASKBAR, true).toBool())
	{
		QToolWindowTaskbarHandler::Unregister(this);
	}
#endif
}

Qt::WindowFlags QToolWindowCustomWrapper::calcFrameWindowFlags()
{
	Qt::WindowFlags flags = QCustomWindowFrame::calcFrameWindowFlags();

	if (parentWidget())
	{
		// Disable minimize button on children, since they won't show up on the task bar, so can't be restored.
		flags = flags & ~Qt::WindowMinimizeButtonHint;
	}
	return flags;
}

#if QT_VERSION >= 0x050000
bool QToolWindowCustomWrapper::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	if (!m_titleBar)
		return false;

#if defined(WIN32) || defined(WIN64)
	MSG* msg = reinterpret_cast<MSG*>(message);
	if (winEvent(msg, result))
		return true;
#endif
	return QCustomWindowFrame::nativeEvent(eventType, message, result);
}
#endif

#if defined(WIN32) || defined(WIN64)
bool QToolWindowCustomWrapper::winEvent(MSG *msg, long *result)
{
	switch (msg->message)
	{
	case WM_MOVING:
		if (m_manager)
		{
			if (m_manager->draggedWrapper() == this)
			{
				m_manager->updateDragPosition();
			}
			else
			{
				m_manager->startDrag(this);
			}
		}
		return false;
		break;
	case WM_EXITSIZEMOVE:
		if (m_manager && m_manager->draggedWrapper() == this)
		{
			m_manager->finishWrapperDrag();
		}
	}
#if QT_VERSION < 0x050000
	return QCustomWindowFrame::winEvent(msg, result);
#else
	return false;
#endif
}
#endif

QRect QToolWindowCustomWrapper::getWrapperFrameSize()
{
	QRect wrapperSize;
	wrapperSize.setTopLeft(-(m_contents->mapToGlobal(QPoint(0, 0)) - mapToGlobal(QPoint(0, 0))));
	wrapperSize.setBottomRight(rect().bottomRight() - m_contents->rect().bottomRight() + wrapperSize.topLeft());
	return wrapperSize;
}

void QToolWindowCustomWrapper::startDrag()
{
	m_titleBar->onBeginDrag();
}
