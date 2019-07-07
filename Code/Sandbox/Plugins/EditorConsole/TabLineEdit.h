// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QLineEdit>

//subclassed QLineEdit that allows capturing of tab
class QTabLineEdit : public QLineEdit
{
	Q_OBJECT;

public:
	QTabLineEdit(QWidget* pWidget) : QLineEdit(pWidget) {}
	bool event(QEvent* pEvent);

signals:
	void tabPressed();
};
