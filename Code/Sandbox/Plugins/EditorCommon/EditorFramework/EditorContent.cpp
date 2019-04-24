// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "EditorContent.h"

// EditorCommon
#include "Editor.h"
#include "ToolBar/ToolBarArea.h"
#include "ToolBar/ToolBarAreaManager.h"
#include "ToolBar/ToolBarCustomizeDialog.h"
#include "ToolBar/ToolBarService.h"
#include "Menu/AbstractMenu.h"

// Qt
#include <QHboxLayout>
#include <QPainter>
#include <QStyleOption>
#include <QToolBar>
#include <QVBoxLayout>

CEditorContent::CEditorContent(CEditor* pEditor)
	: m_pEditor(pEditor)
{
	m_pToolBarAreaManager = new CToolBarAreaManager(m_pEditor);
	m_pMainLayout = new QVBoxLayout();
	m_pContentLayout = new QHBoxLayout();

	m_pMainLayout->setMargin(0);
	m_pMainLayout->setSpacing(0);
	m_pContentLayout->setMargin(0);
	m_pContentLayout->setSpacing(0);

	// Temp content
	m_pContent = new QWidget(); // place-holder content
	m_pContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_pEditor->signalAdaptiveLayoutChanged.Connect(this, &CEditorContent::OnAdaptiveLayoutChanged);

	setLayout(m_pMainLayout);
}

void CEditorContent::Initialize()
{
	m_pToolBarAreaManager->Initialize();

	m_pMainLayout->addWidget(m_pToolBarAreaManager->GetWidget(CToolBarAreaManager::Area::Top));
	m_pMainLayout->addLayout(m_pContentLayout);
	m_pMainLayout->addWidget(m_pToolBarAreaManager->GetWidget(CToolBarAreaManager::Area::Bottom));

	m_pContentLayout->addWidget(m_pToolBarAreaManager->GetWidget(CToolBarAreaManager::Area::Left));
	m_pContentLayout->addWidget(m_pContent);
	m_pContentLayout->addWidget(m_pToolBarAreaManager->GetWidget(CToolBarAreaManager::Area::Right));
}

void CEditorContent::SetContent(QWidget* pContent)
{
	pContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QLayoutItem* pItem = m_pContentLayout->replaceWidget(m_pContent, pContent);
	m_pContent->deleteLater();
	delete pItem;

	m_pContent = pContent;
}

void CEditorContent::SetContent(QLayout* pContentLayout)
{
	QWidget* pContent = new QWidget();
	pContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	pContent->setLayout(pContentLayout);
	pContentLayout->setMargin(0);
	pContentLayout->setSpacing(0);

	QLayoutItem* pItem = m_pContentLayout->replaceWidget(m_pContent, pContent);

	m_pContent->deleteLater();
	delete pItem;

	m_pContent = pContent;
}

bool CEditorContent::CustomizeToolBar()
{
	if (m_pEditor->findChild<CToolBarCustomizeDialog*>())
		return true;

	CToolBarCustomizeDialog* pToolBarCustomizeDialog = new CToolBarCustomizeDialog(m_pEditor);

	pToolBarCustomizeDialog->signalToolBarAdded.Connect(m_pToolBarAreaManager, &CToolBarAreaManager::OnNewToolBar);
	pToolBarCustomizeDialog->signalToolBarModified.Connect(m_pToolBarAreaManager, &CToolBarAreaManager::OnToolBarModified);
	pToolBarCustomizeDialog->signalToolBarRenamed.Connect(m_pToolBarAreaManager, &CToolBarAreaManager::OnToolBarRenamed);
	pToolBarCustomizeDialog->signalToolBarRemoved.Connect(m_pToolBarAreaManager, &CToolBarAreaManager::OnToolBarDeleted);

	pToolBarCustomizeDialog->connect(pToolBarCustomizeDialog, &QWidget::destroyed, [this, pToolBarCustomizeDialog]()
	{
		pToolBarCustomizeDialog->signalToolBarRemoved.DisconnectObject(m_pToolBarAreaManager);
		pToolBarCustomizeDialog->signalToolBarModified.DisconnectObject(m_pToolBarAreaManager);
		pToolBarCustomizeDialog->signalToolBarAdded.DisconnectObject(m_pToolBarAreaManager);
	});

	pToolBarCustomizeDialog->show();

	return true;
}

bool CEditorContent::ToggleToolBarLock()
{
	return m_pToolBarAreaManager->ToggleLock();
}

bool CEditorContent::AddExpandingSpacer()
{
	return m_pToolBarAreaManager->AddExpandingSpacer();
}

bool CEditorContent::AddFixedSpacer()
{
	return m_pToolBarAreaManager->AddFixedSpacer();
}

QVariantMap CEditorContent::GetState() const
{
	return m_pToolBarAreaManager->GetState();
}

void CEditorContent::OnAdaptiveLayoutChanged(Qt::Orientation orientation)
{
	const bool isDefaultOrientation = m_pEditor->GetOrientation() == m_pEditor->GetDefaultOrientation();
	m_pMainLayout->setDirection(isDefaultOrientation ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
	m_pContentLayout->setDirection(isDefaultOrientation ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
}

void CEditorContent::SetState(const QVariantMap& state)
{
	if (state.empty())
		return;

	m_pToolBarAreaManager->SetState(state);
}

void CEditorContent::paintEvent(QPaintEvent* pEvent)
{
	QStyleOption styleOption;
	styleOption.init(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);
}
