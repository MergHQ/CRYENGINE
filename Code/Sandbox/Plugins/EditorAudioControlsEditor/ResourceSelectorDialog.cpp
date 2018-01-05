// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ResourceSelectorDialog.h"

#include "ResourceSelectorModel.h"
#include "AudioControlsEditorPlugin.h"
#include "SystemAssetsManager.h"
#include "TreeView.h"

#include <QtUtil.h>
#include <QSearchBox.h>
#include <ProxyModels/MountingProxyModel.h>
#include <ProxyModels/DeepFilterProxyModel.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QMenu>
#include <QModelIndex>
#include <QKeyEvent>

namespace ACE
{
string CResourceSelectorDialog::s_previousControlName = "";
ESystemItemType CResourceSelectorDialog::s_previousControlType = ESystemItemType::Invalid;

//////////////////////////////////////////////////////////////////////////
CResourceSelectorDialog::CResourceSelectorDialog(ESystemItemType const type, Scope const scope, QWidget* const pParent)
	: CEditorDialog("AudioControlsResourceSelectorDialog", pParent)
	, m_type(type)
	, m_scope(scope)
	, m_pAssetsManager(CAudioControlsEditorPlugin::GetAssetsManager())
	, m_pFilterProxyModel(new CResourceFilterProxyModel(m_type, m_scope, this))
	, m_pSourceModel(new CResourceSourceModel(m_pAssetsManager, this))
	, m_pTreeView(new CTreeView(this, QAdvancedTreeView::Behavior::None))
	, m_pSearchBox(new QSearchBox(this))
	, m_pDialogButtons(new QDialogButtonBox(this))
{
	setWindowTitle("Select Audio System Control");

	m_pMountingProxyModel = new CMountingProxyModel(WrapMemberFunction(this, &CResourceSelectorDialog::CreateLibraryModelFromIndex));
	m_pMountingProxyModel->SetHeaderDataCallbacks(1, &ResourceModelUtils::GetHeaderData);
	m_pMountingProxyModel->SetSourceModel(m_pSourceModel);

	m_pFilterProxyModel->setSourceModel(m_pMountingProxyModel);

	m_pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pTreeView->setDragEnabled(false);
	m_pTreeView->setDragDropMode(QAbstractItemView::NoDragDrop);
	m_pTreeView->setDefaultDropAction(Qt::IgnoreAction);
	m_pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pTreeView->setUniformRowHeights(true);
	m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pTreeView->setModel(m_pFilterProxyModel);
	m_pTreeView->sortByColumn(0, Qt::AscendingOrder);
	m_pTreeView->installEventFilter(this);

	m_pSearchBox->SetModel(m_pFilterProxyModel);
	m_pSearchBox->SetAutoExpandOnSearch(m_pTreeView);
	m_pSearchBox->EnableContinuousSearch(true);

	m_pDialogButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_pDialogButtons->button(QDialogButtonBox::Ok)->setEnabled(false);

	QVBoxLayout* const pWindowLayout = new QVBoxLayout(this);
	setLayout(pWindowLayout);
	pWindowLayout->addWidget(m_pSearchBox);
	pWindowLayout->addWidget(m_pTreeView);
	pWindowLayout->addWidget(m_pDialogButtons);

	QObject::connect(m_pTreeView, &CTreeView::customContextMenuRequested, this, &CResourceSelectorDialog::OnContextMenu);
	QObject::connect(m_pTreeView, &CTreeView::doubleClicked, this, &CResourceSelectorDialog::OnItemDoubleClicked);
	QObject::connect(m_pTreeView->header(), &QHeaderView::sortIndicatorChanged, [&]() { m_pTreeView->scrollTo(m_pTreeView->currentIndex()); });
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CResourceSelectorDialog::OnUpdateSelectedControl);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CResourceSelectorDialog::OnStopTrigger);
	QObject::connect(m_pDialogButtons, &QDialogButtonBox::accepted, this, &CResourceSelectorDialog::accept);
	QObject::connect(m_pDialogButtons, &QDialogButtonBox::rejected, this, &CResourceSelectorDialog::reject);

	m_pSearchBox->signalOnFiltered.Connect([&]()
	{
		m_pTreeView->scrollTo(m_pTreeView->currentIndex());
	}, reinterpret_cast<uintptr_t>(this));

	if (s_previousControlType != type)
	{
		s_previousControlName.clear();
		s_previousControlType = type;
	}

	OnUpdateSelectedControl();
}

//////////////////////////////////////////////////////////////////////////
CResourceSelectorDialog::~CResourceSelectorDialog()
{
	m_pSearchBox->signalOnFiltered.DisconnectById(reinterpret_cast<uintptr_t>(this));

	OnStopTrigger();
	DeleteModels();
}

//////////////////////////////////////////////////////////////////////////
QSize CResourceSelectorDialog::sizeHint() const
{
	return QSize(400, 900);
}

//////////////////////////////////////////////////////////////////////////
QAbstractItemModel* CResourceSelectorDialog::CreateLibraryModelFromIndex(QModelIndex const& sourceIndex)
{
	QAbstractItemModel* pLibraryModel = nullptr;

	if (sourceIndex.model() == m_pSourceModel)
	{
		size_t const numLibraries = m_libraryModels.size();
		size_t const row = static_cast<size_t>(sourceIndex.row());

		if (row >= numLibraries)
		{
			m_libraryModels.resize(row + 1);

			for (size_t i = numLibraries; i < row + 1; ++i)
			{
				m_libraryModels[i] = new CResourceLibraryModel(m_pAssetsManager, m_pAssetsManager->GetLibrary(i), this);
			}
		}

		pLibraryModel = m_libraryModels[row];
	}

	return pLibraryModel;
}

//////////////////////////////////////////////////////////////////////////
char const* CResourceSelectorDialog::ChooseItem(char const* szCurrentValue)
{
	char const* szControlName = szCurrentValue;

	if (std::strcmp(szCurrentValue, "") != 0)
	{
		s_previousControlName = szCurrentValue;
	}

	// Automatically select the previous control
	if ((m_pFilterProxyModel != nullptr) && !s_previousControlName.empty())
	{
		QModelIndex const& index = FindItem(s_previousControlName);

		if (index.isValid())
		{
			m_pTreeView->setCurrentIndex(index);
		}
	}

	if (exec() == QDialog::Accepted)
	{
		szControlName = s_previousControlName.c_str();
	}

	return szControlName;
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::OnUpdateSelectedControl()
{
	if (m_pAssetsManager != nullptr)
	{
		QModelIndex const& index = m_pTreeView->currentIndex();

		if (index.isValid())
		{
			CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, 0);
			m_selectionIsValid = ((pAsset != nullptr) && (pAsset->GetType() == m_type));

			if (m_selectionIsValid)
			{
				s_previousControlName = pAsset->GetName();
			}

			m_pDialogButtons->button(QDialogButtonBox::Ok)->setEnabled(m_selectionIsValid);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CResourceSelectorDialog::FindItem(string const& sControlName)
{
	QModelIndex modelIndex = QModelIndex();

	QModelIndexList const indexes = m_pFilterProxyModel->match(m_pFilterProxyModel->index(0, 0, QModelIndex()), Qt::DisplayRole, QtUtil::ToQString(sControlName), -1, Qt::MatchRecursive);
	
	if (!indexes.empty())
	{
		CSystemAsset* const pAsset = SystemModelUtils::GetAssetFromIndex(indexes[0], 0);

		if (pAsset != nullptr)
		{
			CSystemControl const* const pControl = static_cast<CSystemControl*>(pAsset);

			if (pControl != nullptr)
			{
				Scope const scope = pControl->GetScope();

				if (scope == Utils::GetGlobalScope() || scope == m_scope)
				{
					modelIndex = indexes[0];
				}
			}
		}
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSelectorDialog::eventFilter(QObject* pObject, QEvent* pEvent)
{
	bool isEvent = false;

	if ((pEvent->type() == QEvent::KeyRelease) && m_selectionIsValid)
	{
		QKeyEvent const* const pKeyEvent = static_cast<QKeyEvent*>(pEvent);

		if ((pKeyEvent != nullptr) && (pKeyEvent->key() == Qt::Key_Space))
		{
			QModelIndex const& index = m_pTreeView->currentIndex();

			if (index.isValid())
			{
				CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, 0);

				if ((pAsset != nullptr) && (pAsset->GetType() == ESystemItemType::Trigger))
				{
					CAudioControlsEditorPlugin::ExecuteTrigger(pAsset->GetName());
					isEvent = true;
				}
			}
		}
	}

	if (!isEvent)
	{
		isEvent = QWidget::eventFilter(pObject, pEvent);
	}

	return isEvent;
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::OnStopTrigger()
{
	CAudioControlsEditorPlugin::StopTriggerExecution();
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::OnItemDoubleClicked(QModelIndex const& modelIndex)
{
	if (m_selectionIsValid)
	{
		CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(modelIndex, 0);

		if ((pAsset != nullptr) && (pAsset->GetType() == m_type))
		{
			s_previousControlName = pAsset->GetName();
			accept();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::OnContextMenu(QPoint const& pos)
{
	QMenu* const pContextMenu = new QMenu();
	QModelIndex const& index = m_pTreeView->currentIndex();

	if (index.isValid())
	{
		CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, 0);

		if ((pAsset != nullptr) && (pAsset->GetType() == ESystemItemType::Trigger))
		{
			pContextMenu->addAction(tr("Execute Trigger"), [&]()
			{
				CAudioControlsEditorPlugin::ExecuteTrigger(pAsset->GetName());
			});

			pContextMenu->addSeparator();
			
		}

		pContextMenu->addAction(tr("Expand Selection"), [&]() { m_pTreeView->ExpandSelection(QModelIndexList{ index }); });
		pContextMenu->addAction(tr("Collapse Selection"), [&]() { m_pTreeView->CollapseSelection(QModelIndexList{ index }); });
		pContextMenu->addSeparator();
	}

	pContextMenu->addAction(tr("Expand All"), [&]() { m_pTreeView->expandAll(); });
	pContextMenu->addAction(tr("Collapse All"), [&]() { m_pTreeView->collapseAll(); });

	pContextMenu->exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::DeleteModels()
{
	m_pSourceModel->DisconnectSignals();
	m_pSourceModel->deleteLater();

	for (auto const pModel : m_libraryModels)
	{
		pModel->DisconnectSignals();
		pModel->deleteLater();
	}

	m_libraryModels.clear();
}
} // namespace ACE
