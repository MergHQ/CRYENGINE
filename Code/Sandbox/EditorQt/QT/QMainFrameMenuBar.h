// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>

class QMenuBar;
class QStandardItemModel;

class QMainFrameMenuBar : public QWidget
{
	Q_OBJECT
public:
	QMainFrameMenuBar(QMenuBar* pMenuBar = 0, QWidget* pParent = 0);

	QMenuBar*           GetMenuBar() const { return m_pMenuBar; }

protected:
	virtual void        paintEvent(QPaintEvent* pEvent) override;

protected:
	QMenuBar* m_pMenuBar;
};
