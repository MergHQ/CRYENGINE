// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "QToolWindowDragHandlerNinePatch.h"

#include <QStyle>

void QToolWindowDragHandlerNinePatch::startDrag()
{
	
}

void QToolWindowDragHandlerNinePatch::switchedArea(IToolWindowArea* lastArea, IToolWindowArea* newArea)
{
	
}

QToolWindowManager::AreaTarget QToolWindowDragHandlerNinePatch::getTargetFromPosition(IToolWindowArea* area) const
{
	QToolWindowManager::AreaTarget result;
	result.area = area;
	result.index = -1;
	result.reference = QToolWindowManager::AreaReference::Floating;
	if (!area)
		return result;

	QRect areaRect = area->rect();
	QRect centerRect = QRect(areaRect.x() + areaRect.width() / 3, areaRect.y() + areaRect.height() / 3, areaRect.width() / 3, areaRect.height() / 3);
	QPoint pos = area->mapFromGlobal(QCursor::pos());
	QPoint tabPos = area->mapTabBarFromGlobal(QCursor::pos());

	if (area->tabBarRect().contains(tabPos))
	{				
		result.reference = QToolWindowManager::AreaReference::Combine;
		result.index = area->tabBarAt(tabPos);
	}
	else
	{
		if (pos.x() < centerRect.x()) // left region
			result.reference = QToolWindowManager::AreaReference::VSplitLeft;
		else if (pos.x() > centerRect.x() + centerRect.width()) // right region
			result.reference = QToolWindowManager::AreaReference::VSplitRight;
		else if (pos.y() < centerRect.y()) // top region
			result.reference = QToolWindowManager::AreaReference::HSplitTop;
		else if (pos.y() > centerRect.y() + centerRect.height()) // bottom region
			result.reference = QToolWindowManager::AreaReference::HSplitBottom;
	}
	
	return result;
}

bool QToolWindowDragHandlerNinePatch::isHandlerWidget(QWidget* widget) const
{
	return false;
}

QToolWindowManager::AreaTarget QToolWindowDragHandlerNinePatch::finishDrag(QList<QWidget*> toolWindows, IToolWindowArea* source, IToolWindowArea* destination)
{
	return getTargetFromPosition(destination);
}

IToolWindowDragHandler::SplitSizes QToolWindowDragHandlerNinePatch::getSplitSizes(int originalSize) const
{
	IToolWindowDragHandler::SplitSizes result;
	result.oldSize = originalSize * 2 / 3;
	result.newSize = originalSize - result.oldSize;
	return result;
}

QRect tabSeparatorRect(QRect tabRect, int splitterWidth)
{
	return QRect(tabRect.x() - splitterWidth / 2, tabRect.y(), splitterWidth, tabRect.height());
}

QRect QToolWindowDragHandlerNinePatch::getRectFromCursorPos(QWidget* previewArea, IToolWindowArea* area) const
{
	int splitterWidth = area->getWidget()->style()->pixelMetric(QStyle::PM_SplitterWidth, 0, area->getWidget());
	QRect tabEndRect = area->tabRect(area->count() - 1);
	tabEndRect.setX(tabEndRect.x() + tabEndRect.width());
	QToolWindowManager::AreaTarget target = getTargetFromPosition(area);
	if (target.reference == QToolWindowManager::AreaReference::Combine)
	{
		if (target.index == -1)
			return tabSeparatorRect(tabEndRect, splitterWidth);
		else
			return tabSeparatorRect(area->tabRect(target.index), splitterWidth);
	}
	return IToolWindowDragHandler::getRectFromCursorPos(previewArea, area);
}
