// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlMainWindow.h"
#include "VersionControlPendingChangesTab.h"
#include "VersionControlHistoryTab.h"
#include "VersionControlSettingsTab.h"
#include "QtViewPane.h"

REGISTER_VIEWPANE_FACTORY(CVersionControlMainWindow, "Version Control System", "", true);

CVersionControlMainWindow::CVersionControlMainWindow(QWidget* pParent /*= nullptr*/)
	: CDockableEditor(pParent)
{
	auto pTabWidget = new QTabWidget(this);

	m_pChangelistsTab = new CVersionControlPendingChangesTab(pTabWidget);
	m_pHistoryTab = new CVersionControlHistoryTab(pTabWidget);
	m_pSettingsTab = new CVersionControlSettingsTab(pTabWidget);

	pTabWidget->addTab(m_pChangelistsTab, tr("Pending Changes"));
	pTabWidget->addTab(m_pHistoryTab, tr("History"));
	pTabWidget->addTab(m_pSettingsTab, tr("Settings"));

	pTabWidget->tabBar()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setFocusPolicy(Qt::StrongFocus);

	setAttribute(Qt::WA_DeleteOnClose);

	pTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	SetContent(pTabWidget);
}

void CVersionControlMainWindow::SelectAssets(std::vector<CAsset*> assets)
{
	m_pChangelistsTab->SelectAssets(std::move(assets));
}
