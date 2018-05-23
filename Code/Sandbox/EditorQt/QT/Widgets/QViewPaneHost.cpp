// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "QViewPaneHost.h"
#include <QLayout>

QViewPaneHost::QViewPaneHost()
{
	setLayout(new QVBoxLayout);
	setFrameStyle(StyledPanel | Plain);
	layout()->setContentsMargins(0, 0, 0, 0);
}
