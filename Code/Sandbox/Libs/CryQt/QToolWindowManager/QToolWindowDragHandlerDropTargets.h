// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "QToolWindowManagerCommon.h"
#include "IToolWindowDragHandler.h"

#include <QMap>

class QToolWindowDropTarget : public QWidget
{
public:
	QToolWindowDropTarget(QString imagePath, QToolWindowManager::AreaReference areaReference);
	virtual ~QToolWindowDropTarget();
	QToolWindowManager::AreaReference target() { return m_areaReference; }

protected:
	virtual void paintEvent(QPaintEvent *paintEvent) Q_DECL_OVERRIDE;

private:
	QPixmap* m_pixmap;
	QToolWindowManager::AreaReference m_areaReference;
};

class QTOOLWINDOWMANAGER_EXPORT QToolWindowDragHandlerDropTargets :
	public IToolWindowDragHandler
{
public:
	QToolWindowDragHandlerDropTargets(QToolWindowManager* manager);
	~QToolWindowDragHandlerDropTargets();
	void startDrag() Q_DECL_OVERRIDE;
	void switchedArea(IToolWindowArea* lastArea, IToolWindowArea* newArea) Q_DECL_OVERRIDE;
	QToolWindowManager::AreaTarget getTargetFromPosition(IToolWindowArea* area) const Q_DECL_OVERRIDE;
	bool isHandlerWidget(QWidget* widget) const Q_DECL_OVERRIDE;
	QToolWindowManager::AreaTarget finishDrag(QList<QWidget*> toolWindows, IToolWindowArea* source, IToolWindowArea* destination) Q_DECL_OVERRIDE;
	IToolWindowDragHandler::SplitSizes getSplitSizes(int originalSize) const Q_DECL_OVERRIDE;

private:
	QList<QToolWindowDropTarget*> m_targets;
	void hideTargets();
	void showTargets(IToolWindowArea* area);
};

