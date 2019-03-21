// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "Inspector.h"

#include <QApplication>
#include <QLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QToolBar>
#include <QToolButton>
#include <QCloseEvent>
#include "QScrollableBox.h"

#include "CryIcon.h"
#include "QtViewPane.h"
#include "EditorFramework/Editor.h"
#include "EditorFramework/Events.h"
#include "EditorFramework/BroadcastManager.h"
#include <CrySystem/Profilers/FrameProfiler/FrameProfiler.h>

CInspector::CInspector(QWidget* pParent)
	: CDockableWidget(pParent)
	, m_pOwnedWidget(nullptr)
{
	CRY_ASSERT(pParent);
	Init();
	Connect(CBroadcastManager::Get(pParent));
}

CInspector::CInspector(CEditor* pParent)
	: m_pOwnedWidget(nullptr)
{
	CRY_ASSERT(pParent);
	Init();
	Connect(&pParent->GetBroadcastManager());
}

CInspector::CInspector(CBroadcastManager* pBroadcastManager)
	: m_pOwnedWidget(nullptr)
{
	Init();
	Connect(pBroadcastManager);
}

void CInspector::Init()
{
	m_pLockButton = new QToolButton();
	m_pTitleLabel = new QLabel();
	QFont font = m_pTitleLabel->font();
	font.setBold(true);
	m_pTitleLabel->setFont(font);

	QWidget* pSpacer = new QWidget();
	pSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	CInspectorHeaderWidget* pHeader = new CInspectorHeaderWidget();
	QHBoxLayout* pToolbarLayout = new QHBoxLayout(pHeader);
	pToolbarLayout->addWidget(m_pTitleLabel);
	pToolbarLayout->addWidget(pSpacer);
	pToolbarLayout->addWidget(m_pLockButton);

	QVBoxLayout* pLayout = new QVBoxLayout();
	pLayout->setContentsMargins(1, 1, 1, 1);
	pLayout->addWidget(pHeader);
	setLayout(pLayout);

	m_pWidgetLayout = new QVBoxLayout();
	pLayout->addLayout(m_pWidgetLayout);

	Unlock();

	connect(m_pLockButton, &QPushButton::clicked, this, &CInspector::ToggleLock);
}

CInspector::~CInspector()
{
	Disconnect();
	Clear();
}

void CInspector::SetLockable(bool bLockable)
{
	if (!bLockable)
	{
		if (IsLocked())
		{
			Unlock();
		}

		m_pLockButton->setVisible(false);
	}
	else
	{
		m_pLockButton->setVisible(true);
	}
}

void CInspector::OnPopulate(PopulateInspectorEvent& event)
{
	CRY_PROFILE_FUNCTION(PROFILE_EDITOR);

	// Skip adding if we locked to the previous selection
	if (IsLocked())
	{
		return;
	}

	Clear();

	setUpdatesEnabled(false);

	// Trigger creation of widgets handled by whoever invoked the populate event
	event.GetCallback()(*this);

	// Set the final title
	m_pTitleLabel->setText(event.GetTitle());

	setUpdatesEnabled(true);
}

void CInspector::AddWidget(QWidget* pWidget)
{
	CRY_PROFILE_FUNCTION(PROFILE_EDITOR);
	//this inspector supports only one widget at time, clear the previous widget if we add a new one
	Clear();
	m_pWidgetLayout->addWidget(pWidget);
	m_pOwnedWidget = pWidget;
}

void CInspector::closeEvent(QCloseEvent* event)
{
	Disconnect();
	event->setAccepted(true);
}

void CInspector::Clear()
{
	CRY_PROFILE_FUNCTION(PROFILE_EDITOR);

	if (m_pOwnedWidget)
	{
		m_pOwnedWidget->deleteLater();
	}

	m_pOwnedWidget = nullptr;

	return;
}

void CInspector::Connect(CBroadcastManager* pBroadcastManager)
{
	CRY_ASSERT(pBroadcastManager);
	m_pBroadcastManager = pBroadcastManager;
	m_pBroadcastManager->Connect(BroadcastEvent::PopulateInspector, this, &CInspector::OnPopulate);
}

void CInspector::Disconnect()
{
	if (m_pBroadcastManager)
	{
		m_pBroadcastManager->DisconnectObject(this);
		m_pBroadcastManager = nullptr;
	}
}

void CInspector::Lock()
{
	m_isLocked = true;
	m_pLockButton->setToolTip("Unlock Inspector");
	m_pLockButton->setIcon(CryIcon("icons:general_lock_true.ico"));
}

void CInspector::Unlock()
{
	m_isLocked = false;
	m_pLockButton->setToolTip("Lock Inspector");
	m_pLockButton->setIcon(CryIcon("icons:general_lock_false.ico"));
}
