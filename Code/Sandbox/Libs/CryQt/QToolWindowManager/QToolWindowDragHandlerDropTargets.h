// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "QToolWindowManagerCommon.h"
#include "IToolWindowDragHandler.h"

#include <QMap>

class QTOOLWINDOWMANAGER_EXPORT QToolWindowDropTarget : public QWidget
{
public:
	QToolWindowDropTarget(QString imagePath, QToolWindowAreaReference areaReference);
	virtual ~QToolWindowDropTarget();
	QToolWindowAreaReference target() { return m_areaReference; }

protected:
	virtual void paintEvent(QPaintEvent* paintEvent) override;

private:
	QPixmap*                 m_pixmap;
	QToolWindowAreaReference m_areaReference;
};

class QTOOLWINDOWMANAGER_EXPORT QToolWindowDragHandlerDropTargets : public IToolWindowDragHandler
{
public:
	QToolWindowDragHandlerDropTargets(QToolWindowManager* manager);
	~QToolWindowDragHandlerDropTargets();
	virtual void                               startDrag() override;
	virtual void                               switchedArea(IToolWindowArea* lastArea, IToolWindowArea* newArea) override;
	virtual QToolWindowAreaTarget              getTargetFromPosition(IToolWindowArea* area) const override;
	virtual bool                               isHandlerWidget(QWidget* widget) const override;
	virtual QToolWindowAreaTarget              finishDrag(QList<QWidget*> toolWindows, IToolWindowArea* source, IToolWindowArea* destination) override;
	virtual IToolWindowDragHandler::SplitSizes getSplitSizes(int originalSize) const override;

private:
	QList<QToolWindowDropTarget*> m_targets;
	void hideTargets();
	void showTargets(IToolWindowArea* area);
};
