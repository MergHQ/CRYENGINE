// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlMainWindow.h"
#include "VersionControlHistoryTab.h"
#include "VersionControlSettingsTab.h"
#include "VersionControlWorkspaceOverviewTab.h"

#include <QBoxLayout>
#include <QTabWidget>

REGISTER_VIEWPANE_FACTORY(CVersionControlMainWindow, "Version Control System", "", true);

CVersonControlMainView::CVersonControlMainView(QWidget* pParent)
	: QWidget(pParent)
{
	QVBoxLayout* pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing(0);
	pMainLayout->setMargin(0);

	m_pTabWidget = new QTabWidget(this);

	m_pChangelistsTab = new CVersionControlWorkspaceOverviewTab(m_pTabWidget);
	m_pHistoryTab = new CVersionControlHistoryTab(m_pTabWidget);
	m_pSettingsTab = new CVersionControlSettingsTab(m_pTabWidget);

	m_pTabWidget->addTab(m_pChangelistsTab, tr("Pending Changes"));
	m_pTabWidget->addTab(m_pHistoryTab, tr("History"));
	m_pTabWidget->addTab(m_pSettingsTab, tr("Settings"));

	pMainLayout->addWidget(m_pTabWidget);

	setLayout(pMainLayout);
}

CVersionControlMainWindow::CVersionControlMainWindow(QWidget* pParent /*= nullptr*/)
	: CDockableEditor(pParent)
{
	CVersonControlMainView* pView = new CVersonControlMainView(this);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_DeleteOnClose);
	SetContent(pView);
}
