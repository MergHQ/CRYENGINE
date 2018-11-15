// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "OpenProjectPanel.h"
#include "ProjectManagement/Model/ProjectsModel.h"
#include "ProjectManagement/UI/SelectProjectDialog.h"

#include <QAdvancedTreeView.h>
#include <QSearchBox.h>
#include <QThumbnailView.h>

#include <QButtonGroup>
#include <QHeaderView>

COpenProjectPanel::COpenProjectPanel(CSelectProjectDialog* pParent, bool runOnSandboxInit)
	: QWidget(pParent)
	, m_pParent(pParent)
	, m_pModel(new CProjectsModel(this, pParent->GetProjectManager()))
	, m_pSortedModel(new CProjectSortProxyModel(this))
	, m_pMainLayout(new QVBoxLayout(this))
	, m_pTreeView(new QAdvancedTreeView)
	, m_pThumbnailView(new QThumbnailsView(nullptr, false, this))
{
	setLayout(m_pMainLayout);

	m_pSortedModel->setSourceModel(m_pModel);
	m_pSortedModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	CreateSearchPanel();
	CreateViews();
	CreateDialogButtons(runOnSandboxInit);

	SetViewMode(false);
}

void COpenProjectPanel::CreateSearchPanel()
{
	QSearchBox* pSearchBox = new QSearchBox(this);
	pSearchBox->EnableContinuousSearch(true);
	pSearchBox->SetModel(m_pSortedModel);

	QHBoxLayout* pLayout = new QHBoxLayout;
	pLayout->addWidget(pSearchBox);

	m_pMainLayout->addLayout(pLayout);
}

void COpenProjectPanel::CreateViews()
{
	m_pTreeView->setModel(m_pSortedModel);
	m_pTreeView->sortByColumn(CProjectsModel::eColumn_Name, Qt::AscendingOrder);
	m_pTreeView->setRootIsDecorated(false);

	auto idx = m_pModel->IndexFromProject(m_pParent->GetProjectManager().GetLastUsedProject());
	idx = m_pSortedModel->mapFromSource(idx);
	m_pTreeView->selectionModel()->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);

	m_pTreeView->header()->setSectionsMovable(false);
	m_pTreeView->header()->resizeSection(CProjectsModel::eColumn_Name, fontMetrics().width(QStringLiteral("wwwwwwwwwwwwwwwwwwwwwwwwwwwww")));
	m_pTreeView->header()->resizeSection(CProjectsModel::eColumn_LastAccessTime, fontMetrics().width(QStringLiteral("wwwwwwwwwwwwwwwwww")));

	connect(m_pTreeView, &QAbstractItemView::activated, this, &COpenProjectPanel::OpenProject);
	connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &COpenProjectPanel::OnSelectionChanged);

	m_pThumbnailView->SetModel(m_pSortedModel);
	m_pThumbnailView->SetItemSizeBounds({ 96, 96 }, { 96, 96 });
	m_pThumbnailView->SetRootIndex(QModelIndex());

	QAbstractItemView* const pView = m_pThumbnailView->GetInternalView();
	pView->setSelectionMode(QAbstractItemView::SingleSelection);
	pView->setSelectionBehavior(QAbstractItemView::SelectRows);
	pView->setSelectionModel(m_pTreeView->selectionModel());
	pView->update();

	connect(pView, &QAbstractItemView::activated, this, &COpenProjectPanel::OpenProject);

	QHBoxLayout* pLayout = new QHBoxLayout;
	pLayout->addWidget(m_pTreeView);
	pLayout->addWidget(m_pThumbnailView);

	QVBoxLayout* pButtonsLayout = new QVBoxLayout;
	pButtonsLayout->setMargin(0);
	pButtonsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

	QButtonGroup* pViewModeButtons = new QButtonGroup(this);
	CreateListViewButton(pViewModeButtons);
	CreateThumbnailButton(pViewModeButtons);

	const QList<QAbstractButton*> buttons = pViewModeButtons->buttons();
	for (QAbstractButton* pButton : buttons)
	{
		pButtonsLayout->addWidget(pButton);
	}

	pLayout->addLayout(pButtonsLayout);
	m_pMainLayout->addLayout(pLayout);
}

void COpenProjectPanel::CreateListViewButton(QButtonGroup* pGroup)
{
	QToolButton* pButton = new QToolButton;
	connect(pButton, &QToolButton::clicked, this, [this]() { SetViewMode(false); });
	pButton->setIcon(CryIcon("icons:common/general_view_list.ico"));
	pButton->setCheckable(true);
	pButton->setAutoRaise(true);
	pButton->setToolTip("Shows Details");
	pButton->setChecked(true);
	pGroup->addButton(pButton);
}

void COpenProjectPanel::CreateThumbnailButton(QButtonGroup* pGroup)
{
	QToolButton* pButton = new QToolButton;
	connect(pButton, &QToolButton::clicked, this, [this]() { SetViewMode(true); });
	pButton->setIcon(CryIcon("icons:common/general_view_thumbnail.ico"));
	pButton->setCheckable(true);
	pButton->setAutoRaise(true);
	pButton->setToolTip("Shows Thumbnails");
	pButton->setChecked(false);
	pGroup->addButton(pButton);
}

void COpenProjectPanel::CreateDialogButtons(bool runOnSandboxInit)
{
	QPushButton* pQuitBtn = new QPushButton(runOnSandboxInit ? "Quit" : "Cancel", this);
	connect(pQuitBtn, SIGNAL(clicked()), m_pParent, SLOT(reject()));

	m_pOpenProjectBtn = new QPushButton("Open", this);

	m_pOpenProjectBtn->setDefault(true);
	m_pOpenProjectBtn->setDisabled(m_pTreeView->selectionModel()->selection().count() == 0);

	connect(m_pOpenProjectBtn, &QPushButton::clicked, this, &COpenProjectPanel::OnLoadProjectPressed);

	QHBoxLayout* pButtonsLayout = new QHBoxLayout;
	pButtonsLayout->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	pButtonsLayout->addWidget(pQuitBtn);
	pButtonsLayout->addWidget(m_pOpenProjectBtn);

	m_pMainLayout->addLayout(pButtonsLayout);
}

void COpenProjectPanel::SetViewMode(bool thumbnailMode)
{
	m_pTreeView->setVisible(!thumbnailMode);
	m_pThumbnailView->setVisible(thumbnailMode);
}

void COpenProjectPanel::OnLoadProjectPressed()
{
	auto indexes = m_pTreeView->selectionModel()->selectedIndexes();
	if (indexes.empty())
	{
		return;
	}
	OpenProject(indexes.at(0));
}

void COpenProjectPanel::OpenProject(const QModelIndex& index)
{
	if (!index.isValid())
	{
		CRY_ASSERT(false);
		return;
	}

	m_pParent->OpenProject(m_pModel->ProjectFromIndex(m_pSortedModel->mapToSource(index)));
}

void COpenProjectPanel::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	m_pOpenProjectBtn->setEnabled(!selected.empty());
}
