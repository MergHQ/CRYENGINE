// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "QToolWindowTabBar.h"

QToolWindowTabBar::QToolWindowTabBar(QWidget* parent)
	: QTabBar(parent)
#if QT_VERSION <= 0x050000
	, bAutoHide(false)
#endif
{
}

QToolWindowTabBar::~QToolWindowTabBar()
{

}