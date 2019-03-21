// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "EditorToolBarArea.h"

#include "Editor.h"
#include "EditorToolBarService.h"
#include "Events.h"
#include "Menu/AbstractMenu.h"
#include "Menu/MenuWidgetBuilders.h"
#include "QtUtil.h"
#include "ToolBarCustomizeDialog.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>
#include <QToolBar>

CEditorToolBarArea::CEditorToolBarArea(CEditor* pParent)
	: QWidget(pParent)
	, m_pEditor(pParent)
{
	connect(this, &QWidget::customContextMenuRequested, this, &CEditorToolBarArea::ShowContextMenu);
	m_pLayout = new QHBoxLayout();
	m_pLayout->setSpacing(0);
	m_pLayout->setMargin(0);

	// Add a spacer item to make sure that all toolbars are aligned to the left for now
	m_pLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

	setLayout(m_pLayout);
	setContextMenuPolicy(Qt::CustomContextMenu);
}

void CEditorToolBarArea::Initialize()
{
	m_toolbars = GetIEditor()->GetEditorToolBarService()->LoadToolBars(m_pEditor);
	for (QToolBar* pToolBar : m_toolbars)
	{
		AddToolBar(pToolBar);

		connect(pToolBar, &QToolBar::destroyed, [this](QObject* pObject)
		{
			QString toolBarName = pObject->objectName();
			toolBarName = "";
		});
	}
}

void CEditorToolBarArea::AddToolBar(QToolBar* pToolBar)
{
	// Make sure that toolbars are added previous to the spacer
	m_pLayout->insertWidget(m_pLayout->count() - 1, pToolBar);
}

void CEditorToolBarArea::SetMenu(CAbstractMenu* pToolBarMenu)
{
	// Make sure a menu with this name doesn't already exist, if so clear it
	pToolBarMenu->signalAboutToShow.Connect(this, &CEditorToolBarArea::FillMenu);
}

void CEditorToolBarArea::FillMenu(CAbstractMenu* pMenu)
{
	pMenu->Clear();

	pMenu->CreateCommandAction("editor.customize_toolbar");
	int sec = pMenu->GetNextEmptySection();
	pMenu->SetSectionName(sec, "Toolbars");

	for (QToolBar* pToolBar : m_toolbars)
	{
		// Toolbar name is packed as part of the window title (this is used by QMainWindow to display toolbar names in context menu)
		QAction* pToggleVisibilityAction = new QAction(pToolBar->windowTitle());
		pToggleVisibilityAction->setCheckable(true);
		pToggleVisibilityAction->setChecked(pToolBar->isVisible());
		connect(pToggleVisibilityAction, &QAction::triggered, [pToolBar](bool isChecked)
		{
			pToolBar->setVisible(isChecked);
		});

		pMenu->AddAction(pToggleVisibilityAction, sec);
	}
}

void CEditorToolBarArea::ShowContextMenu()
{
	CAbstractMenu menuDesc;
	FillMenu(&menuDesc);

	QMenu menu;
	menuDesc.Build(MenuWidgetBuilders::CMenuBuilder(&menu));
	menu.exec(QCursor::pos());
}

void CEditorToolBarArea::OnToolBarAdded(QToolBar* pToolBar)
{
	m_toolbars.push_back(pToolBar);
	AddToolBar(pToolBar);
}

void CEditorToolBarArea::OnToolBarModified(QToolBar* pToolBar)
{
	string toolBarName = QtUtil::ToString(pToolBar->objectName());
	QToolBar* pOldToolBar = FindToolBarByName(toolBarName.c_str());

	if (pOldToolBar)
	{
		m_pLayout->replaceWidget(pOldToolBar, pToolBar);
		pOldToolBar->setVisible(false);
		pToolBar->setVisible(true);

		OnToolBarRemoved(toolBarName.c_str());
		OnToolBarAdded(pToolBar);

		connect(pToolBar, &QToolBar::destroyed, [this](QObject* pObject)
		{
			QString toolBarName = pObject->objectName();
			toolBarName = "";
		});
	}
}

void CEditorToolBarArea::OnToolBarRemoved(const char* szToolBarName)
{
	const QString toolBarName(szToolBarName);

	auto ite = std::find_if(m_toolbars.begin(),
	                        m_toolbars.end(),
	                        [&toolBarName](QToolBar* pToolBar) { return pToolBar->objectName() == toolBarName; });

	if (ite == m_toolbars.end())
		return;

	(*ite)->deleteLater();
	m_toolbars.erase(ite);
}

QToolBar* CEditorToolBarArea::FindToolBarByName(const char* szToolBarName)
{
	const QString toolBarName(szToolBarName);

	auto ite = std::find_if(m_toolbars.begin(),
	                        m_toolbars.end(),
	                        [&toolBarName](QToolBar* pToolBar) { return pToolBar->objectName() == toolBarName; });

	if (ite == m_toolbars.end())
		return nullptr;

	return *ite;
}

bool CEditorToolBarArea::CustomizeToolBar()
{
	if (m_pEditor->findChild<CToolBarCustomizeDialog*>())
		return true;

	CToolBarCustomizeDialog* pToolBarCustomizeDialog = new CToolBarCustomizeDialog(m_pEditor);

	pToolBarCustomizeDialog->signalToolBarAdded.Connect(this, &CEditorToolBarArea::OnToolBarAdded);
	pToolBarCustomizeDialog->signalToolBarModified.Connect(this, &CEditorToolBarArea::OnToolBarModified);
	pToolBarCustomizeDialog->signalToolBarRemoved.Connect(this, &CEditorToolBarArea::OnToolBarRemoved);

	connect(pToolBarCustomizeDialog, &QWidget::destroyed, [this, pToolBarCustomizeDialog]()
	{
		pToolBarCustomizeDialog->signalToolBarRemoved.DisconnectObject(this);
		pToolBarCustomizeDialog->signalToolBarModified.DisconnectObject(this);
		pToolBarCustomizeDialog->signalToolBarAdded.DisconnectObject(this);
	});

	pToolBarCustomizeDialog->show();

	return true;
}

void CEditorToolBarArea::paintEvent(QPaintEvent* pEvent)
{
	QStyleOption styleOption;
	styleOption.init(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);
}
