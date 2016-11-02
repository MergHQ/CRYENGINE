// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "QToolWindowDragHandlerDropTargets.h"
#include <QPainter>

#define DROPTARGET_PADDING 4

QToolWindowDropTarget::QToolWindowDropTarget(QString imagePath, QToolWindowManager::AreaReference areaReference)
	: QWidget(nullptr)
	, m_pixmap(new QPixmap())
	, m_areaReference(areaReference)
{
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_ShowWithoutActivating);

	m_pixmap->load(imagePath);
	setFixedSize(m_pixmap->size());
}

QToolWindowDropTarget::~QToolWindowDropTarget()
{
	if (m_pixmap)
		delete m_pixmap;
}

void QToolWindowDropTarget::paintEvent(QPaintEvent *paintEvent)
{
	QPainter painter(this);
	painter.drawPixmap(0, 0, *m_pixmap);
}

QToolWindowDragHandlerDropTargets::QToolWindowDragHandlerDropTargets(QToolWindowManager* manager)
{
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_COMBINE, ":/QtDockLibrary/gfx/base_window.png").toString(), QToolWindowManager::AreaReference::Combine));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_TOP, ":/QtDockLibrary/gfx/dock_top.png").toString(), QToolWindowManager::AreaReference::Top));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_BOTTOM, ":/QtDockLibrary/gfx/dock_bottom.png").toString(), QToolWindowManager::AreaReference::Bottom));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_LEFT, ":/QtDockLibrary/gfx/dock_left.png").toString(), QToolWindowManager::AreaReference::Left));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_RIGHT, ":/QtDockLibrary/gfx/dock_right.png").toString(), QToolWindowManager::AreaReference::Right));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_SPLIT_LEFT, ":/QtDockLibrary/gfx/vsplit_left.png").toString(), QToolWindowManager::AreaReference::VSplitLeft));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_SPLIT_RIGHT, ":/QtDockLibrary/gfx/vsplit_right.png").toString(), QToolWindowManager::AreaReference::VSplitRight));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_SPLIT_TOP, ":/QtDockLibrary/gfx/hsplit_top.png").toString(), QToolWindowManager::AreaReference::HSplitTop));
	m_targets.append(new QToolWindowDropTarget(manager->config().value(QTWM_DROPTARGET_SPLIT_BOTTOM, ":/QtDockLibrary/gfx/hsplit_bottom.png").toString(), QToolWindowManager::AreaReference::HSplitBottom));

	hideTargets();
}

QToolWindowDragHandlerDropTargets::~QToolWindowDragHandlerDropTargets()
{
	for (QToolWindowDropTarget* target : m_targets)
	{
		delete target;
	}
}

void QToolWindowDragHandlerDropTargets::startDrag()
{
}

void QToolWindowDragHandlerDropTargets::switchedArea(IToolWindowArea* lastArea, IToolWindowArea* newArea)
{
	if (newArea)
	{
		showTargets(newArea);
	}
	else
	{
		hideTargets();
	}
}

QToolWindowManager::AreaTarget QToolWindowDragHandlerDropTargets::getTargetFromPosition(IToolWindowArea* area) const
{
	QToolWindowManager::AreaTarget result(area, QToolWindowManager::AreaReference::Floating, -1);
	if (area)
	{
		QWidget* widgetUnderMouse = qApp->widgetAt(QCursor::pos());
		if (isHandlerWidget(widgetUnderMouse))
		{
			result.reference = static_cast<QToolWindowDropTarget*>(widgetUnderMouse)->target();
		}

		if (area && result.reference == QToolWindowManager::AreaReference::Floating)
		{
			QPoint& tabPos = area->mapTabBarFromGlobal(QCursor::pos());
			if (area->tabBarRect().contains(tabPos))
			{
				result.reference = QToolWindowManager::AreaReference::Combine;
				result.index = area->tabBarAt(tabPos);
			}
		}
	}
	return result;

}

bool QToolWindowDragHandlerDropTargets::isHandlerWidget(QWidget* widget) const
{
	return m_targets.contains(static_cast<QToolWindowDropTarget*>(widget));
}

QToolWindowManager::AreaTarget QToolWindowDragHandlerDropTargets::finishDrag(QList<QWidget*> toolWindows, IToolWindowArea* source, IToolWindowArea* destination)
{
	QToolWindowManager::AreaTarget result(destination, QToolWindowManager::AreaReference::Floating, -1);
	if (destination)
	{
		QWidget* widgetUnderMouse = qApp->widgetAt(QCursor::pos());
		if (isHandlerWidget(widgetUnderMouse))
		{
			result.reference = static_cast<QToolWindowDropTarget*>(widgetUnderMouse)->target();
		}

		if (result.reference == QToolWindowManager::AreaReference::Floating)
		{
			QPoint& tabPos = destination->mapTabBarFromGlobal(QCursor::pos());
			if (destination->tabBarRect().contains(tabPos))
			{
				result.reference = QToolWindowManager::AreaReference::Combine;
				result.index = destination->tabBarAt(tabPos);
				if (destination == source && result.index > destination->indexOf(toolWindows[0]))
				{
					result.index--;
				}
			}
		}
	}
	hideTargets();
	return result;
}

IToolWindowDragHandler::SplitSizes QToolWindowDragHandlerDropTargets::getSplitSizes(int originalSize) const
{
	IToolWindowDragHandler::SplitSizes result;
	result.oldSize = originalSize / 2;
	result.newSize = originalSize - result.oldSize;
	return result;
}

void QToolWindowDragHandlerDropTargets::hideTargets()
{
	for (QToolWindowDropTarget* target : m_targets)
	{
		target->hide();
	}
}

void QToolWindowDragHandlerDropTargets::showTargets(IToolWindowArea* area)
{
	QPoint areaCenter = area->mapToGlobal(QPoint(area->geometry().width() / 2, area->geometry().height() / 2));

	QWidget* wrapper = findClosestParent<IToolWindowWrapper*>(area->getWidget())->getContents();
	QPoint wrapperCenter = wrapper->mapToGlobal(QPoint(wrapper->geometry().width() / 2, wrapper->geometry().height() / 2));

	for (QToolWindowDropTarget* target : m_targets)
	{
		QPoint newPos;

		// Position drop targets
		switch (target->target())
		{
		case QToolWindowManager::AreaReference::Combine:
			newPos = areaCenter;
			break;
		case QToolWindowManager::AreaReference::Top:
			newPos = wrapperCenter - QPoint(0, (wrapper->geometry().height() / 2) - target->height() - DROPTARGET_PADDING);
			break;
		case QToolWindowManager::AreaReference::Bottom:
			newPos = wrapperCenter + QPoint(0, (wrapper->geometry().height() / 2) - target->height() - DROPTARGET_PADDING);
			break;
		case QToolWindowManager::AreaReference::Left:
			newPos = wrapperCenter - QPoint((wrapper->geometry().width() / 2) - target->width() - DROPTARGET_PADDING, 0);
			break;
		case QToolWindowManager::AreaReference::Right:
			newPos = wrapperCenter + QPoint((wrapper->geometry().width() / 2) - target->width() - DROPTARGET_PADDING, 0);
			break;
		case QToolWindowManager::AreaReference::HSplitTop:
			newPos = areaCenter - QPoint(0, target->height() + DROPTARGET_PADDING);
			break;
		case QToolWindowManager::AreaReference::HSplitBottom:
			newPos = areaCenter + QPoint(0, target->height() + DROPTARGET_PADDING);
			break;
		case QToolWindowManager::AreaReference::VSplitLeft:
			newPos = areaCenter - QPoint(target->width() + DROPTARGET_PADDING, 0);
			break;
		case QToolWindowManager::AreaReference::VSplitRight:
			newPos = areaCenter + QPoint(target->width() + DROPTARGET_PADDING, 0);
			break;

		default:
			newPos = target->pos();
			break;
		}

		newPos -= QPoint(target->geometry().width() / 2, target->geometry().height() / 2);
		target->move(newPos);
		target->show();
		target->raise();
	}
}