// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MiddlewareDataWidget.h"

#include "AudioControlsEditorPlugin.h"
#include "MiddlewareDataModel.h"
#include "ImplementationManager.h"
#include "SystemAssetsManager.h"
#include "SystemControlsIcons.h"
#include "TreeView.h"

#include <IEditorImpl.h>
#include <ImplItem.h>
#include <QFilteringPanel.h>
#include <QSearchBox.h>
#include <QtUtil.h>
#include <ProxyModels/AttributeFilterProxyModel.h>

#include <QHeaderView>
#include <QMenu>
#include <QVBoxLayout>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget::CMiddlewareDataWidget(CSystemAssetsManager* const pAssetsManager, QWidget* const pParent)
	: QWidget(pParent)
	, m_pAssetsManager(pAssetsManager)
	, m_pAttributeFilterProxyModel(new QAttributeFilterProxyModel(QAttributeFilterProxyModel::AcceptIfChildMatches, this))
	, m_pMiddlewareDataModel(new CMiddlewareDataModel(this))
	, m_pTreeView(new CTreeView(this))
	, m_nameColumn(static_cast<int>(CMiddlewareDataModel::EColumns::Name))
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	m_pAttributeFilterProxyModel->setSourceModel(m_pMiddlewareDataModel);
	m_pAttributeFilterProxyModel->setFilterKeyColumn(m_nameColumn);

	m_pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pTreeView->setDragEnabled(true);
	m_pTreeView->setDragDropMode(QAbstractItemView::DragOnly);
	m_pTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_pTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pTreeView->setTreePosition(m_nameColumn);
	m_pTreeView->setUniformRowHeights(true);
	m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pTreeView->setModel(m_pAttributeFilterProxyModel);
	m_pTreeView->sortByColumn(m_nameColumn, Qt::AscendingOrder);
	m_pTreeView->header()->setMinimumSectionSize(25);
	m_pTreeView->header()->setSectionResizeMode(static_cast<int>(CMiddlewareDataModel::EColumns::Notification), QHeaderView::ResizeToContents);
	m_pTreeView->SetNameColumn(m_nameColumn);
	m_pTreeView->SetNameRole(static_cast<int>(CMiddlewareDataModel::ERoles::Name));
	m_pTreeView->TriggerRefreshHeaderColumns();

	m_pFilteringPanel = new QFilteringPanel("ACEMiddlewareData", m_pAttributeFilterProxyModel);
	m_pFilteringPanel->SetContent(m_pTreeView);
	m_pFilteringPanel->GetSearchBox()->SetAutoExpandOnSearch(m_pTreeView);

	QVBoxLayout* const pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);
	pMainLayout->addWidget(m_pFilteringPanel);

	QObject::connect(m_pTreeView, &CTreeView::customContextMenuRequested, this, &CMiddlewareDataWidget::OnContextMenu);

	m_pAssetsManager->signalConnectionAdded.Connect([&]()
	{
		if (!m_pAssetsManager->IsLoading())
		{
			m_pAttributeFilterProxyModel->invalidate();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalConnectionRemoved.Connect([&]()
	{
		if (!m_pAssetsManager->IsLoading())
		{
			m_pAttributeFilterProxyModel->invalidate();
		}
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataWidget::~CMiddlewareDataWidget()
{
	m_pAssetsManager->signalConnectionAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalConnectionRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));

	m_pMiddlewareDataModel->DisconnectSignals();
	m_pMiddlewareDataModel->deleteLater();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::OnContextMenu(QPoint const& pos)
{
	QMenu* const pContextMenu = new QMenu(this);
	auto const& selection = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);

	if (!selection.isEmpty())
	{
		if ((selection.count() == 1) && (m_pAssetsManager != nullptr))
		{
			IEditorImpl const* const pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();

			if (pEditorImpl != nullptr)
			{
				CID const itemId = selection[0].data(static_cast<int>(CMiddlewareDataModel::ERoles::Id)).toInt();
				CImplItem const* const pImplControl = pEditorImpl->GetControl(itemId);

				if ((pImplControl != nullptr) && pImplControl->IsConnected())
				{
					QMenu* const pConnectionsMenu = new QMenu(pContextMenu);
					auto const controls = m_pAssetsManager->GetControls();
					int count = 0;

					for (auto const pControl : controls)
					{
						if (pControl->GetConnection(pImplControl) != nullptr)
						{
							pConnectionsMenu->addAction(GetItemTypeIcon(pControl->GetType()), tr(pControl->GetName()), [=]() { SignalSelectConnectedSystemControl(*pControl); });
							++count;
						}
					}

					if (count > 0)
					{
						pConnectionsMenu->setTitle(tr("Connections (" + ToString(count) + ")"));
						pContextMenu->addMenu(pConnectionsMenu);
						pContextMenu->addSeparator();
					}
				}
			}
		}

		pContextMenu->addAction(tr("Expand Selection"), [&]() { m_pTreeView->ExpandSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addAction(tr("Collapse Selection"), [&]() { m_pTreeView->CollapseSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addSeparator();
	}

	pContextMenu->addAction(tr("Expand All"), [&]() { m_pTreeView->expandAll(); });
	pContextMenu->addAction(tr("Collapse All"), [&]() { m_pTreeView->collapseAll(); });

	pContextMenu->exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::Reset()
{
	m_pMiddlewareDataModel->Reset();
	m_pAttributeFilterProxyModel->invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::BackupTreeViewStates()
{
	m_pTreeView->BackupExpanded();
	m_pTreeView->BackupSelection();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataWidget::RestoreTreeViewStates()
{
	m_pTreeView->RestoreExpanded();
	m_pTreeView->RestoreSelection();
}
} // namespace ACE
