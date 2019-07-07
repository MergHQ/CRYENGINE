// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlTrayWidget.h"
#include "VersionControlMainWindow.h"
#include "EditorFramework/TrayArea.h"
#include "Controls/QPopupWidget.h"
#include "QtUtil.h"
#include "CryIcon.h"

REGISTER_TRAY_AREA_WIDGET(CVersionControlTrayWidget, 10)

CVersionControlTrayWidget::CVersionControlTrayWidget(QWidget* pParent /* = nullptr */)
	: QToolButton(pParent)
{
	connect(GetIEditor()->GetTrayArea(), &CTrayArea::MainFrameInitialized, [this]()
	{
		m_pPopUpMenu = new QPopupWidget("NotificationCenterPopup", QtUtil::MakeScrollable(new CVersonControlMainView(parentWidget())), QSize(480, 640));
		m_pPopUpMenu->SetFocusShareWidget(this);
	});
	connect(this, &QToolButton::clicked, this, &CVersionControlTrayWidget::OnClicked);

	setIcon(CryIcon("icons:VersionControl/icon.ico"));
}

CVersionControlTrayWidget::~CVersionControlTrayWidget()
{
	delete m_pPopUpMenu;
}

void CVersionControlTrayWidget::OnClicked(bool bChecked)
{
	if (m_pPopUpMenu->isVisible())
	{
		m_pPopUpMenu->hide();
		return;
	}

	m_pPopUpMenu->ShowAt(mapToGlobal(QPoint(width(), height())));
}
