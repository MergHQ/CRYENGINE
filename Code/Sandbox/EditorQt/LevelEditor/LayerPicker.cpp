// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include "LayerPicker.h"

#include "LevelModelsManager.h"
#include "Objects/ObjectLayer.h"
#include "Objects/ObjectLayerManager.h"
#include "Objects/SelectionGroup.h"

#include "ProxyModels/DeepFilterProxyModel.h"
#include "QAdvancedTreeView.h"
#include "QSearchBox.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "IResourceSelectorHost.h"
#include "IEditorImpl.h"

//////////////////////////////////////////////////////////////////////////
// Register for yasli

namespace
{
SResourceSelectionResult ShowDialog(const SResourceSelectorContext& context, const char* szPreviousValue)
{
	CLayerPicker layerPicker;
	CObjectLayer* previousValue = GetIEditorImpl()->GetObjectManager()->GetLayersManager()->FindLayerByFullName(szPreviousValue);
	layerPicker.SetSelectedLayer(previousValue);

	bool accepted = layerPicker.exec() == QDialog::Accepted;
	SResourceSelectionResult result{ accepted, szPreviousValue };

	if (accepted)
	{
		result.selectedResource = layerPicker.GetSelectedLayer()->GetFullName().GetBuffer();
	}

	return result;
}

void SwitchLayer()
{
	const CSelectionGroup* pSelection = GetIEditorImpl()->GetSelection();
	auto selectionCount = pSelection->GetCount();

	if (selectionCount < 1)
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Please select an object to switch layer");
		return;
	}

	// Set selected layer in picker to last selected object's layer
	CBaseObject* pObject = pSelection->GetObject(selectionCount - 1);
	CLayerPicker layerPicker;
	layerPicker.SetSelectedLayer(static_cast<CObjectLayer*>(pObject->GetLayer()));

	if (layerPicker.exec() == QDialog::Accepted)
	{
		CObjectLayer* pLayer = layerPicker.GetSelectedLayer();
		if (!pLayer)
			return;

		CUndo moveObjects(string().Format("Move Objects to Layer %s", pLayer->GetName()));

		// We just care about setting the layer in the top level objects as children will inherit the parent's layer
		pSelection->FilterParents();
		auto filteredCount = pSelection->GetFilteredCount();
		for (auto i = 0; i < filteredCount; ++i)
		{
			pSelection->GetFilteredObject(i)->SetLayer(pLayer);
		}
	}
}

REGISTER_RESOURCE_SELECTOR("LevelLayer", ShowDialog, "")
REGISTER_EDITOR_AND_SCRIPT_COMMAND(SwitchLayer, object, switch_layer,
                                   CCommandDescription("Open Layer dialog to allow switching layers"))
REGISTER_EDITOR_UI_COMMAND_DESC(object, switch_layer, "Switch Layer", "Ctrl+L", "", false)
}

//////////////////////////////////////////////////////////////////////////

CLayerPicker::CLayerPicker()
	: CEditorDialog("LayerPicker")
{
	SetTitle("Select Layer");
	auto model = CLevelModelsManager::GetInstance().GetLevelModel();

	QDeepFilterProxyModel::BehaviorFlags behavior = QDeepFilterProxyModel::AcceptIfChildMatches | QDeepFilterProxyModel::AcceptIfParentMatches;
	QDeepFilterProxyModel* proxy = new QDeepFilterProxyModel(behavior);
	proxy->setSourceModel(model);
	proxy->setFilterKeyColumn((int)eLayerColumns_Name);

	auto searchBox = new QSearchBox();
	searchBox->SetModel(proxy);
	searchBox->EnableContinuousSearch(true);

	m_treeView = new QAdvancedTreeView();
	m_treeView->setModel(proxy);
	m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_treeView->setDragDropMode(QAbstractItemView::NoDragDrop);
	m_treeView->setTreePosition((int)eLayerColumns_Name);
	m_treeView->setSortingEnabled(true);
	m_treeView->sortByColumn((int)eLayerColumns_Name, Qt::AscendingOrder);

	connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CLayerPicker::OnSelectionChanged);
	connect(m_treeView, &QTreeView::activated, this, &CLayerPicker::SelectLayerIndex);

	searchBox->SetAutoExpandOnSearch(m_treeView);

	const int count = proxy->columnCount(QModelIndex());
	if (count > 1)
	{
		for (int i = 0; i < count; i++)
		{
			m_treeView->SetColumnVisible(i, false);
		}

		m_treeView->SetColumnVisible((int)eLayerColumns_Name, true);
	}

	//TODO : make use of the standard button dialog bar
	m_pOkButton = new QPushButton(this);
	m_pOkButton->setText(tr("Ok"));
	m_pOkButton->setDefault(true);
	connect(m_pOkButton, &QPushButton::clicked, this, &CLayerPicker::OnOk);

	QPushButton* cancelButton = new QPushButton(this);
	cancelButton->setText(tr("Cancel"));
	connect(cancelButton, &QPushButton::clicked, [this]()
	{
		reject();
	});

	auto buttonsLayout = new QHBoxLayout();
	buttonsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
	buttonsLayout->addWidget(m_pOkButton);
	buttonsLayout->addWidget(cancelButton);

	auto mainLayout = new QVBoxLayout();
	mainLayout->addWidget(searchBox);
	mainLayout->addWidget(m_treeView);
	mainLayout->addLayout(buttonsLayout);

	setLayout(mainLayout);
}

void CLayerPicker::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QModelIndex index = m_treeView->currentIndex();
	if (!index.isValid())
		return;

	QVariant layerVar = index.data((int)CLevelModel::Roles::InternalPointerRole);
	if (layerVar.isValid())
	{
		CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(layerVar.value<intptr_t>());
		if (!pLayer)
			return;

		if (pLayer->GetLayerType() == eObjectLayerType_Terrain)
		{
			m_pOkButton->setEnabled(false);
		}
		else
		{
			m_pOkButton->setEnabled(true);
		}
	}
}

void CLayerPicker::SetSelectedLayer(CObjectLayer* layer)
{
	auto model = m_treeView->model();
	auto matchingList = model->match(model->index(0, 0, QModelIndex()), (int)CLevelModel::Roles::InternalPointerRole, reinterpret_cast<intptr_t>(layer), 1, Qt::MatchRecursive | Qt::MatchExactly);
	if (matchingList.size() == 1)
	{
		m_treeView->expand(matchingList.first().parent());
		m_treeView->scrollTo(matchingList.first());
		m_treeView->selectionModel()->setCurrentIndex(matchingList.first(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	}
}

void CLayerPicker::OnOk()
{
	QModelIndex selected = m_treeView->selectionModel()->currentIndex();
	SelectLayerIndex(selected);
}

void CLayerPicker::SelectLayerIndex(const QModelIndex& index)
{
	if (index.isValid())
	{
		QVariant layerVar = index.data((int)CLevelModel::Roles::InternalPointerRole);
		if (layerVar.isValid())
		{
			CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(layerVar.value<intptr_t>());
			if (pLayer != nullptr && pLayer->GetLayerType() == eObjectLayerType_Layer)
			{
				m_selectedLayer = pLayer;
				accept();
			}
		}
	}
}
