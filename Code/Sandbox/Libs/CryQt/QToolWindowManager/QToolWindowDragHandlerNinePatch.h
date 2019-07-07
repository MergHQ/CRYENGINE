// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "QToolWindowManagerCommon.h"
#include "IToolWindowDragHandler.h"

class QTOOLWINDOWMANAGER_EXPORT QToolWindowDragHandlerNinePatch : public IToolWindowDragHandler
{
public:
	virtual void                               startDrag() override;
	virtual void                               switchedArea(IToolWindowArea* lastArea, IToolWindowArea* newArea) override;
	virtual QToolWindowAreaTarget              getTargetFromPosition(IToolWindowArea* area) const override;
	virtual bool                               isHandlerWidget(QWidget* widget) const override;
	virtual QToolWindowAreaTarget              finishDrag(QList<QWidget*> toolWindows, IToolWindowArea* source, IToolWindowArea* destination) override;
	virtual IToolWindowDragHandler::SplitSizes getSplitSizes(int originalSize) const override;

	virtual QRect                              getRectFromCursorPos(QWidget* previewArea, IToolWindowArea* area) const override;
};
