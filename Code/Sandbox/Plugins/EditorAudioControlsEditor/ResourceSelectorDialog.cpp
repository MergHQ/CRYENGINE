// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ResourceSelectorDialog.h"

#include "ResourceSelectorModel.h"
#include "AudioControlsEditorPlugin.h"
#include "AudioAssetsManager.h"

#include <QtUtil.h>
#include <QAdvancedTreeView.h>
#include <QSearchBox.h>
#include <ProxyModels/MountingProxyModel.h>

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
EItemType CResourceSelectorDialog::s_previousControlType = eItemType_Invalid;

//////////////////////////////////////////////////////////////////////////
CResourceSelectorDialog::CResourceSelectorDialog(QWidget* pParent, EItemType const eType)
	: CEditorDialog("AudioControlsResourceSelectorDialog", pParent)
	, m_eType(eType)
{
	setWindowTitle("Select Audio System Control");
	setWindowModality(Qt::ApplicationModal);

	m_pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();
	m_pFilterProxyModel = new CResourceFilterProxyModel();
	m_pAssetsModel = new CResourceControlModel(m_pAssetsManager);

	m_pMountingProxyModel = new CMountingProxyModel(WrapMemberFunction(this, &CResourceSelectorDialog::CreateLibraryModelFromIndex));
	m_pMountingProxyModel->SetHeaderDataCallbacks(1, &GetHeaderData);
	m_pMountingProxyModel->SetSourceModel(m_pAssetsModel);

	m_pFilterProxyModel->setSourceModel(m_pMountingProxyModel);

	QVBoxLayout* pWindowLayout = new QVBoxLayout();
	setLayout(pWindowLayout);

	QSearchBox* pSearchBox = new QSearchBox();
	pWindowLayout->addWidget(pSearchBox);
	
	m_pControlsTree = new QAdvancedTreeView();
	m_pControlsTree->setAutoScroll(true);
	m_pControlsTree->setDragEnabled(false);
	m_pControlsTree->setDragDropMode(QAbstractItemView::NoDragDrop);
	m_pControlsTree->setDefaultDropAction(Qt::IgnoreAction);
	m_pControlsTree->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pControlsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_pControlsTree->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pControlsTree->setModel(m_pFilterProxyModel);
	m_pControlsTree->installEventFilter(this);
	pWindowLayout->addWidget(m_pControlsTree);

	m_pDialogButtons = new QDialogButtonBox();
	m_pDialogButtons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_pDialogButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
	pWindowLayout->addWidget(m_pDialogButtons);

	QObject::connect(pSearchBox, &QSearchBox::textChanged, this, &CResourceSelectorDialog::SetTextFilter);

	QObject::connect(m_pControlsTree, &QAdvancedTreeView::customContextMenuRequested, this, &CResourceSelectorDialog::OnContextMenu);
	QObject::connect(m_pControlsTree, &QAdvancedTreeView::doubleClicked, this, &CResourceSelectorDialog::ItemDoubleClicked);
	QObject::connect(m_pControlsTree->header(), &QHeaderView::sortIndicatorChanged, [&]()
	{
		QModelIndex const& currentIndex = m_pControlsTree->currentIndex();

		if (currentIndex.isValid())
		{
			m_pControlsTree->scrollTo(currentIndex);
		}
	});

	QObject::connect(m_pControlsTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CResourceSelectorDialog::UpdateSelectedControl);
	QObject::connect(m_pControlsTree->selectionModel(), &QItemSelectionModel::currentChanged, this, &CResourceSelectorDialog::StopTrigger);
	
	QObject::connect(m_pDialogButtons, &QDialogButtonBox::accepted, this, &CResourceSelectorDialog::accept);
	QObject::connect(m_pDialogButtons, &QDialogButtonBox::rejected, this, &CResourceSelectorDialog::reject);

	if (s_previousControlType != eType)
	{
		s_previousControlName.clear();
		s_previousControlType = eType;
	}

	ApplyFilter();
	UpdateSelectedControl();
}

//////////////////////////////////////////////////////////////////////////
CResourceSelectorDialog::~CResourceSelectorDialog()
{
	StopTrigger();
}

//////////////////////////////////////////////////////////////////////////
QAbstractItemModel* CResourceSelectorDialog::CreateLibraryModelFromIndex(QModelIndex const& sourceIndex)
{
	if (sourceIndex.model() != m_pAssetsModel)
	{
		return nullptr;
	}

	int const numLibraries = m_libraryModels.size();
	int const row = sourceIndex.row();

	if (row >= m_libraryModels.size())
	{
		m_libraryModels.resize(row + 1);

		for (uint i = numLibraries; i < row + 1; ++i)
		{
			m_libraryModels[i] = new CResourceLibraryModel(m_pAssetsManager, m_pAssetsManager->GetLibrary(i));
		}
	}

	return m_libraryModels[row];
}

//////////////////////////////////////////////////////////////////////////
const char* CResourceSelectorDialog::ChooseItem(const char* szCurrentValue)
{
	if (std::strcmp(szCurrentValue, "") != 0)
	{
		s_previousControlName = szCurrentValue;
	}

	// Automatically select the previous control
	if (m_pFilterProxyModel && !s_previousControlName.empty())
	{
		QModelIndex const& index = FindItem(s_previousControlName);

		if (index.isValid())
		{
			m_pControlsTree->setCurrentIndex(index);
		}
	}

	if (exec() == QDialog::Accepted)
	{
		return s_previousControlName.c_str();
	}

	return szCurrentValue;
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::UpdateSelectedControl()
{
	if (m_pAssetsManager)
	{
		QModelIndexList const indexes = m_pControlsTree->selectionModel()->selectedIndexes();

		if (!indexes.empty())
		{
			QModelIndex const& index = indexes[0];

			if (index.isValid())
			{
				CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(index);

				if (pAsset && pAsset->GetType() == m_eType)
				{
					s_previousControlName = pAsset->GetName();
				}

				m_bSelectionIsValid = (pAsset && (pAsset->GetType() != eItemType_Folder) && (pAsset && pAsset->GetType() != eItemType_Library));
				m_pDialogButtons->button(QDialogButtonBox::Ok)->setEnabled(m_bSelectionIsValid);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::SetTextFilter(QString const& filter)
{
	if (m_sFilter != filter)
	{
		if (m_sFilter.isEmpty() && !filter.isEmpty())
		{
			m_pControlsTree->BackupExpanded();
			m_pControlsTree->expandAll();
		}
		else if (!m_sFilter.isEmpty() && filter.isEmpty())
		{
			m_pControlsTree->collapseAll();
			m_pControlsTree->RestoreExpanded();
		}

		m_sFilter = filter;
		ApplyFilter();

		QModelIndex const& currentIndex = m_pControlsTree->currentIndex();

		if (currentIndex.isValid())
		{
			m_pControlsTree->scrollTo(currentIndex);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::ApplyFilter()
{
	QModelIndex index = m_pFilterProxyModel->index(0, 0);

	for (int i = 0; index.isValid(); ++i)
	{
		ApplyFilter(index);
		index = index.sibling(i, 0);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSelectorDialog::ApplyFilter(QModelIndex const& parent)
{
	if (parent.isValid())
	{
		bool bChildValid = false;
		QModelIndex child = parent.child(0, 0);

		for (int i = 1; child.isValid(); ++i)
		{
			if (ApplyFilter(child))
			{
				bChildValid = true;
			}

			child = parent.child(i, 0);
		}

		// If it has valid children or it is a valid control (not a folder), show it
		CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(parent);

		if (bChildValid || (pAsset && pAsset->GetType() == m_eType && IsValid(parent)))
		{
			m_pControlsTree->setRowHidden(parent.row(), parent.parent(), false);
			return true;
		}
		else
		{
			m_pControlsTree->setRowHidden(parent.row(), parent.parent(), true);

		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSelectorDialog::IsValid(QModelIndex const& index)
{
	QString const& name = index.data(Qt::DisplayRole).toString();

	if (m_sFilter.isEmpty() || name.contains(m_sFilter, Qt::CaseInsensitive))
	{
		CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(index);

		if (pAsset && (pAsset->GetType() == m_eType))
		{
			CAudioControl* const pControl = static_cast<CAudioControl*>(pAsset);

			if (pControl)
			{
				Scope const scope = pControl->GetScope();

				if (scope == Utils::GetGlobalScope() || scope == m_scope)
				{
					return true;
				}

				return false;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QSize CResourceSelectorDialog::sizeHint() const
{
	return QSize(400, 900);
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::SetScope(Scope const scope)
{
	m_scope = scope;
	ApplyFilter();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CResourceSelectorDialog::FindItem(string const& sControlName)
{
	QModelIndexList const indexes = m_pFilterProxyModel->match(m_pFilterProxyModel->index(0, 0, QModelIndex()), Qt::DisplayRole, QtUtil::ToQString(sControlName), -1, Qt::MatchRecursive);
	
	if (!indexes.empty())
	{
		CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(indexes[0]);

		if (pAsset)
		{
			CAudioControl* const pControl = static_cast<CAudioControl*>(pAsset);

			if (pControl)
			{
				Scope const scope = pControl->GetScope();

				if (scope == Utils::GetGlobalScope() || scope == m_scope)
				{
					return indexes[0];
				}
			}
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSelectorDialog::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if ((pEvent->type() == QEvent::KeyRelease) && m_bSelectionIsValid)
	{
		QKeyEvent* const pKeyEvent = static_cast<QKeyEvent*>(pEvent);

		if (pKeyEvent && (pKeyEvent->key() == Qt::Key_Space))
		{
			QModelIndex const& index = m_pControlsTree->currentIndex();

			if (index.isValid())
			{
				CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(index);

				if (pAsset && (pAsset->GetType() == eItemType_Trigger))
				{
					CAudioControlsEditorPlugin::ExecuteTrigger(pAsset->GetName());
				}
			}
		}
	}

	return QWidget::eventFilter(pObject, pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::StopTrigger()
{
	CAudioControlsEditorPlugin::StopTriggerExecution();
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::ItemDoubleClicked(QModelIndex const& modelIndex)
{
	if (m_bSelectionIsValid)
	{
		CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(modelIndex);

		if (pAsset && (pAsset->GetType() == m_eType))
		{
			s_previousControlName = pAsset->GetName();
			accept();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceSelectorDialog::OnContextMenu(QPoint const& pos)
{
	QModelIndex const& index = m_pControlsTree->currentIndex();

	if (index.isValid())
	{
		CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(index);

		if (pAsset && (pAsset->GetType() == eItemType_Trigger))
		{
			QMenu* pContextMenu = new QMenu();

			pContextMenu->addAction(tr("Execute Trigger"), [&]()
			{
				CAudioControlsEditorPlugin::ExecuteTrigger(pAsset->GetName());
			});

			pContextMenu->exec(QCursor::pos());
		}
	}
}
} // namespace ACE
