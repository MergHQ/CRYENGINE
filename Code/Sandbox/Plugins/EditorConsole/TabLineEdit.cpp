// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TabLineEdit.h"
#include <QEvent>
#include <QKeyEvent>

bool QTabLineEdit::event(QEvent* pEvent)
{
	if (pEvent->type() == QEvent::KeyPress)
	{
		QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
		if (pKeyEvent->key() == Qt::Key_Tab)
		{
			tabPressed();
			return true;
		}
	}
	return QLineEdit::event(pEvent);
}
