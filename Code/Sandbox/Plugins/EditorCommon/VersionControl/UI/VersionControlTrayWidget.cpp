// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlTrayWidget.h"
#include "VersionControlMainWindow.h"
#include "EditorFramework/TrayArea.h"

REGISTER_TRAY_AREA_WIDGET(CVersionControlTrayWidget, 10)

CVersionControlTrayWidget::CVersionControlTrayWidget(QWidget* pParent /* = nullptr */)
{
	connect(this, &QToolButton::clicked, this, &CVersionControlTrayWidget::OnClicked);

	setIcon(CryIcon("icons:VersionControl/icon.ico"));
}

void CVersionControlTrayWidget::OnClicked(bool bChecked)
{
	GetIEditor()->OpenView("Version Control System");
}
