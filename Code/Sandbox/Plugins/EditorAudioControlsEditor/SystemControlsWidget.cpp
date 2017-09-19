// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemControlsWidget.h"

#include "AudioControlsEditorPlugin.h"
#include "MiddlewareDataWidget.h"
#include "MiddlewareDataModel.h"
#include "AudioAssets.h"
#include "AudioAssetsManager.h"
#include "SystemControlsEditorIcons.h"
#include "AdvancedTreeView.h"

#include <IEditor.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>
#include <CryMath/Cry_Camera.h>
#include <CryIcon.h>
#include <CryAudio/IAudioSystem.h>
#include <IAudioSystemEditor.h>
#include <IAudioSystemItem.h>
#include <QtUtil.h>
#include <EditorStyleHelper.h>
#include <IUndoObject.h>
#include <Controls/QuestionDialog.h>
#include <FilePathUtil.h>
#include <ProxyModels/MountingProxyModel.h>
#include <QSearchBox.h>

#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QKeyEvent>
#include <QAction>
#include <QSplitter>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>


namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CSystemControlsWidget::CSystemControlsWidget(CAudioAssetsManager* pAssetsManager)
	: m_pAssetsManager(pAssetsManager)
	, m_pProxyModel(new CSystemControlsFilterProxyModel(this))
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QVBoxLayout* pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);

	m_pSearchBox = new QSearchBox();
	m_pSearchBox->setToolTip(tr("Show only controls with this name"));
	
	QToolButton* pFilterButton = new QToolButton();
	pFilterButton->setIcon(CryIcon("icons:General/Filter.ico"));
	pFilterButton->setToolTip(tr("Filters"));
	pFilterButton->setCheckable(true);
	pFilterButton->setMaximumSize(QSize(20, 20));
	
	QPushButton* pAddButton = new QPushButton();
	pAddButton->setToolTip(tr("Add new library, folder or control"));
	pAddButton->setText(tr("Add"));

	QHBoxLayout* pHorizontalLayout = new QHBoxLayout();
	pHorizontalLayout->addWidget(m_pSearchBox);
	pHorizontalLayout->addWidget(pFilterButton);
	pHorizontalLayout->addWidget(pAddButton);

	m_pFilterWidget = new QWidget();
	InitFilterWidget();

	m_pControlsTree = new CAdvancedTreeView();
	m_pControlsTree->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
	m_pControlsTree->setDragEnabled(true);
	m_pControlsTree->setDragDropMode(QAbstractItemView::DragDrop);
	m_pControlsTree->setDefaultDropAction(Qt::MoveAction);
	m_pControlsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_pControlsTree->setSortingEnabled(true);
	m_pControlsTree->sortByColumn(0, Qt::AscendingOrder);
	m_pControlsTree->header()->setVisible(false);
	m_pControlsTree->installEventFilter(this);
	m_pControlsTree->viewport()->installEventFilter(this);

	QSplitter* pSplitter = new QSplitter();
	pSplitter->setOrientation(Qt::Vertical);
	pSplitter->setChildrenCollapsible(false);
	pSplitter->addWidget(m_pFilterWidget);
	pSplitter->addWidget(m_pControlsTree);
	pSplitter->setStretchFactor(0, 0);
	pSplitter->setStretchFactor(1, 1);

	pMainLayout->addLayout(pHorizontalLayout);
	pMainLayout->addWidget(pSplitter);

	QObject::connect(pFilterButton, &QToolButton::toggled, m_pFilterWidget, &QWidget::setVisible);
	QObject::connect(m_pSearchBox, &QSearchBox::textChanged, [&](QString const& filter)
	{
		if (m_filter != filter)
		{
			if (m_filter.isEmpty() && !filter.isEmpty())
			{
				BackupTreeViewStates();
				StartTextFiltering();
				m_pControlsTree->expandAll();
			}
			else if (!m_filter.isEmpty() && filter.isEmpty())
			{
				m_pProxyModel->setFilterFixedString(filter);
				m_pControlsTree->collapseAll();
				RestoreTreeViewStates();
				StopTextFiltering();
			}
			else if (!m_filter.isEmpty() && !filter.isEmpty())
			{
				m_pProxyModel->setFilterFixedString(filter);
				m_pControlsTree->expandAll();
			}

			m_filter = filter;
		}

		m_pProxyModel->setFilterFixedString(filter);
	});

	// ************ Context Menu ************
	QMenu* pContextMenu = new QMenu(this);
	pContextMenu->addAction(GetItemTypeIcon(eItemType_Library), tr("Library"), [this]()
	{
		m_bCreatedFromMenu = true;
		m_pAssetsManager->CreateLibrary(Utils::GenerateUniqueLibraryName("new_library", *m_pAssetsManager));
	});

	pContextMenu->addAction(GetItemTypeIcon(eItemType_Folder), tr("Folder"), [&]() { CreateFolder(GetSelectedAsset()); });
	pContextMenu->addAction(GetItemTypeIcon(eItemType_Trigger), tr("Trigger"), [&]() { CreateControl("new_trigger", eItemType_Trigger, GetSelectedAsset()); });
	pContextMenu->addAction(GetItemTypeIcon(eItemType_Parameter), tr("Parameter"), [&]() { CreateControl("new_parameter", eItemType_Parameter, GetSelectedAsset()); });
	pContextMenu->addAction(GetItemTypeIcon(eItemType_Switch), tr("Switch"), [&]() { CreateControl("new_switch", eItemType_Switch, GetSelectedAsset()); });
	pContextMenu->addAction(GetItemTypeIcon(eItemType_Environment), tr("Environment"), [&]() { CreateControl("new_environment", eItemType_Environment, GetSelectedAsset()); });
	pContextMenu->addAction(GetItemTypeIcon(eItemType_Preload), tr("Preload"), [&]() { CreateControl("new_preload", eItemType_Preload, GetSelectedAsset()); });
	pContextMenu->addSeparator();
	pAddButton->setMenu(pContextMenu);

	m_pControlsTree->setContextMenuPolicy(Qt::CustomContextMenu);
	QObject::connect(m_pControlsTree, &CAdvancedTreeView::customContextMenuRequested, this, &CSystemControlsWidget::OnContextMenu);
	// *********************************

	m_pAssetsManager->signalItemAboutToBeAdded.Connect([&](CAudioAsset* pItem)
	{
		if (!m_bReloading)
		{
			ResetFilters();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAboutToBeAdded.Connect([&]()
	{
		if (!m_bReloading)
		{
			ResetFilters();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsModel = new CSystemControlsModel(m_pAssetsManager);

	auto const count = m_pAssetsManager->GetLibraryCount();
	m_libraryModels.resize(count);

	for (int i = 0; i < count; ++i)
	{
		m_libraryModels[i] = new CAudioLibraryModel(m_pAssetsManager, m_pAssetsManager->GetLibrary(i));
	}

	m_pMountedModel = new CMountingProxyModel(WrapMemberFunction(this, &CSystemControlsWidget::CreateLibraryModelFromIndex));
	m_pMountedModel->SetHeaderDataCallbacks(1, &GetHeaderData);
	m_pMountedModel->SetSourceModel(m_pAssetsModel);

	m_pProxyModel->setSourceModel(m_pMountedModel);
	m_pProxyModel->setDynamicSortFilter(true);
	m_pControlsTree->setModel(m_pProxyModel);

	QObject::connect(m_pControlsTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSystemControlsWidget::SelectedControlChanged);
	QObject::connect(m_pControlsTree->selectionModel(), &QItemSelectionModel::selectionChanged, m_pControlsTree, &CAdvancedTreeView::OnSelectionChanged);
	QObject::connect(m_pControlsTree->selectionModel(), &QItemSelectionModel::currentChanged, this, &CSystemControlsWidget::StopControlExecution);

	QObject::connect(m_pMountedModel, &CMountingProxyModel::rowsInserted, this, &CSystemControlsWidget::SelectNewAsset);

	m_pAssetsManager->signalLibraryAboutToBeRemoved.Connect([&](CAudioLibrary* pLibrary)
	{
		if (!m_bReloading)
		{
			int const libCount = m_pAssetsManager->GetLibraryCount();

			for (int i = 0; i < libCount; ++i)
			{
				if (m_pAssetsManager->GetLibrary(i) == pLibrary)
				{
					m_libraryModels[i]->deleteLater();
					m_libraryModels.erase(m_libraryModels.begin() + i);
					break;
				}
			}
		}
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
CSystemControlsWidget::~CSystemControlsWidget()
{
	m_pAssetsManager->signalLibraryAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));

	StopControlExecution();
	delete m_pAssetsModel;
	int const libCount = m_libraryModels.size();

	for (int i = 0; i < libCount; ++i)
	{
		m_libraryModels[i]->DisconnectFromSystem();
		delete m_libraryModels[i];
	}

	m_libraryModels.clear();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if ((pEvent->type() == QEvent::KeyRelease) && !m_pControlsTree->IsEditing())
	{
		QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);

		if (pKeyEvent)
		{
			if (pKeyEvent->key() == Qt::Key_Delete)
			{
				DeleteSelectedControl();
			}
			else if (pKeyEvent->key() == Qt::Key_Space)
			{
				ExecuteControl();
			}
		}
	}
	return QWidget::eventFilter(pObject, pEvent);
}

//////////////////////////////////////////////////////////////////////////
std::vector<CAudioControl*> CSystemControlsWidget::GetSelectedControls()
{
	QModelIndexList indexes = m_pControlsTree->selectionModel()->selectedIndexes();
	std::vector<CAudioLibrary*> libraries;
	std::vector<CAudioFolder*> folders;
	std::vector<CAudioControl*> controls;
	AudioModelUtils::GetAssetsFromIndices(indexes, libraries, folders, controls);
	return controls;
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::Reload()
{
	ResetFilters();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::ShowControlType(EItemType type, bool bShow)
{
	m_pProxyModel->EnableControl(bShow, type);

	if (type == eItemType_Switch)
	{
		m_pProxyModel->EnableControl(bShow, eItemType_State);
	}

	ControlTypeFiltered(type, bShow);
}

//////////////////////////////////////////////////////////////////////////
CAudioControl* CSystemControlsWidget::CreateControl(string const& name, EItemType type, CAudioAsset* pParent)
{
	m_bCreatedFromMenu = true;

	if (type != eItemType_State)
	{
		return m_pAssetsManager->CreateControl(Utils::GenerateUniqueControlName(name, type, *m_pAssetsManager), type, pParent);
	}
	else
	{
		return m_pAssetsManager->CreateControl(Utils::GenerateUniqueName(name, type, pParent), type, pParent);
	}
}

//////////////////////////////////////////////////////////////////////////
CAudioAsset* CSystemControlsWidget::CreateFolder(CAudioAsset* pParent)
{
	m_bCreatedFromMenu = true;
	return m_pAssetsManager->CreateFolder(Utils::GenerateUniqueName("new_folder", EItemType::eItemType_Folder, pParent), pParent);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::OnContextMenu(QPoint const& pos)
{
	QMenu* pContextMenu = new QMenu();
	std::vector<CAudioLibrary*> libraries;
	std::vector<CAudioFolder*> folders;
	std::vector<CAudioControl*> controls;
	auto const selection = m_pControlsTree->selectionModel()->selectedRows();
	AudioModelUtils::GetAssetsFromIndices(selection, libraries, folders, controls);

	if (libraries.size() == 1 || folders.size() == 1)
	{
		CAudioAsset* pParent = nullptr;

		if (libraries.empty())
		{
			pParent = static_cast<CAudioAsset*>(folders[0]);
		}
		else
		{
			pParent = static_cast<CAudioAsset*>(libraries[0]);
		}

		QMenu* pAddMenu = new QMenu(tr("Add"));
		pAddMenu->addAction(GetItemTypeIcon(eItemType_Folder), tr("Folder"), [&]() { CreateFolder(pParent); });
		pAddMenu->addSeparator();
		pAddMenu->addAction(GetItemTypeIcon(eItemType_Trigger), tr("Trigger"), [&]() { CreateControl("new_trigger", eItemType_Trigger, pParent); });
		pAddMenu->addAction(GetItemTypeIcon(eItemType_Parameter), tr("Parameter"), [&]() { CreateControl("new_parameter", eItemType_Parameter, pParent); });
		pAddMenu->addAction(GetItemTypeIcon(eItemType_Switch), tr("Switch"), [&]() { CreateControl("new_switch", eItemType_Switch, pParent); });
		pAddMenu->addAction(GetItemTypeIcon(eItemType_Environment), tr("Environment"), [&]() { CreateControl("new_environment", eItemType_Environment, pParent); });
		pAddMenu->addAction(GetItemTypeIcon(eItemType_Preload), tr("Preload"), [&]() { CreateControl("new_preload", eItemType_Preload, pParent); });
		pContextMenu->addMenu(pAddMenu);
		pContextMenu->addSeparator();

		if (pParent->GetType() == EItemType::eItemType_Library)
		{
			//TODO: Take into account files in the "levels" folder
			pContextMenu->addAction(tr("Open Containing Folder"), [&]()
				{
					QtUtil::OpenInExplorer(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), Utils::GetAssetFolder() + pParent->GetName() + ".xml").c_str());
				});
		}
	}

	if (controls.size() == 1)
	{
		CAudioControl* pControl = controls[0];

		switch (pControl->GetType())
		{
		case EItemType::eItemType_Trigger:
			pContextMenu->addAction(tr("Execute Trigger"), [&]() { ExecuteControl(); });
			pContextMenu->addSeparator();
			break;
		case EItemType::eItemType_Switch:
			pContextMenu->addAction(GetItemTypeIcon(eItemType_State), tr("Add State"), [&]() { CreateControl("new_state", eItemType_State, pControl); });
			pContextMenu->addSeparator();
			break;
		case EItemType::eItemType_Preload:
			if (pControl->GetScope() == CCrc32::Compute("global") && !pControl->IsAutoLoad())
			{
				pContextMenu->addAction(tr("Load Global Preload Request"), [&]() { gEnv->pAudioSystem->PreloadSingleRequest(CryAudio::StringToId(pControl->GetName()), false); });
				pContextMenu->addAction(tr("Unload Global Preload Request"), [&]() { gEnv->pAudioSystem->UnloadSingleRequest(CryAudio::StringToId(pControl->GetName())); });
				pContextMenu->addSeparator();
			}
			break;
		}
	}
	else
	{
		bool bOnlyGlobalPreloads = false;

		for (auto const pControl : controls)
		{
			if (pControl->GetType() == EItemType::eItemType_Preload && pControl->GetScope() == CCrc32::Compute("global") && !pControl->IsAutoLoad())
			{
				bOnlyGlobalPreloads = true;
			}
			else
			{
				bOnlyGlobalPreloads = false;
				break;
			}
		}

		if (bOnlyGlobalPreloads)
		{
			pContextMenu->addAction(tr("Load Global Preload Requests"), [&]()
			{
				for (auto const pControl : controls)
				{
					gEnv->pAudioSystem->PreloadSingleRequest(CryAudio::StringToId(pControl->GetName()), false);
				}
			});

			pContextMenu->addAction(tr("Unload Global Preload Requests"), [&]()
			{
				for (auto const pControl : controls)
				{
					gEnv->pAudioSystem->UnloadSingleRequest(CryAudio::StringToId(pControl->GetName()));
				}
			});

			pContextMenu->addSeparator();
		}
	}

	pContextMenu->addAction(tr("Rename"), [&]() { m_pControlsTree->edit(m_pControlsTree->currentIndex()); });
	pContextMenu->addAction(tr("Delete"), [&]() { DeleteSelectedControl(); });
	pContextMenu->addSeparator();

	if (!selection.isEmpty())
	{
		pContextMenu->addAction(tr("Expand Selection"), [&]() { m_pControlsTree->ExpandSelection(m_pControlsTree->GetSelectedIndexes()); });
		pContextMenu->addAction(tr("Collapse Selection"), [&]() { m_pControlsTree->CollapseSelection(m_pControlsTree->GetSelectedIndexes()); });
		pContextMenu->addSeparator();
	}
	
	pContextMenu->addAction(tr("Expand All"), [&]() { m_pControlsTree->expandAll(); });
	pContextMenu->addAction(tr("Collapse All"), [&]() { m_pControlsTree->collapseAll(); });

	pContextMenu->exec(m_pControlsTree->mapToGlobal(pos));
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::DeleteSelectedControl()
{
	auto const selection = m_pControlsTree->selectionModel()->selectedRows();
	int const size = selection.length();

	if (size > 0)
	{
		QString text;

		if (size == 1)
		{
			text = R"(Are you sure you want to delete ")" + selection[0].data(Qt::DisplayRole).toString() + R"("?.)";
		}
		else
		{
			text = "Are you sure you want to delete the selected controls and folders?.";
		}

		CQuestionDialog messageBox;
		messageBox.SetupQuestion("Audio Controls Editor", text);

		if (messageBox.Execute() == QDialogButtonBox::Yes)
		{
			CUndo undo("Audio Control Removed");

			std::vector<CAudioAsset*> selectedItems;

			for (auto index : selection)
			{
				if (index.isValid())
				{
					selectedItems.push_back(AudioModelUtils::GetAssetFromIndex(index));
				}
			}

			std::vector<CAudioAsset*> itemsToDelete;
			Utils::SelectTopLevelAncestors(selectedItems, itemsToDelete);

			for (auto pItem : itemsToDelete)
			{
				m_pAssetsManager->DeleteItem(pItem);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::ExecuteControl()
{
	CAudioAsset* pAsset = AudioModelUtils::GetAssetFromIndex(m_pControlsTree->currentIndex());

	if (pAsset && pAsset->GetType() == eItemType_Trigger)
	{
		CAudioControlsEditorPlugin::ExecuteTrigger(pAsset->GetName());
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::StopControlExecution()
{
	CAudioControlsEditorPlugin::StopTriggerExecution();
}

//////////////////////////////////////////////////////////////////////////
QAbstractItemModel* CSystemControlsWidget::CreateLibraryModelFromIndex(QModelIndex const& sourceIndex)
{
	if (sourceIndex.model() != m_pAssetsModel)
	{
		return nullptr;
	}

	size_t const numLibraries = m_libraryModels.size();
	size_t const row = static_cast<size_t>(sourceIndex.row());

	if (row >= numLibraries)
	{
		m_libraryModels.resize(row + 1);

		for (size_t i = numLibraries; i < row + 1; ++i)
		{
			m_libraryModels[i] = new CAudioLibraryModel(m_pAssetsManager, m_pAssetsManager->GetLibrary(i));
		}
	}

	return m_libraryModels[row];
}

//////////////////////////////////////////////////////////////////////////
CAudioAsset* CSystemControlsWidget::GetSelectedAsset() const
{
	QModelIndex const& index = m_pControlsTree->currentIndex();

	if (index.isValid())
	{
		return AudioModelUtils::GetAssetFromIndex(index);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::SelectNewAsset(QModelIndex const& parent, int const row)
{
	if (m_bCreatedFromMenu)
	{
		QModelIndex const& assetIndex = m_pProxyModel->mapFromSource(m_pMountedModel->index(row, 0, parent));
		m_pControlsTree->setCurrentIndex(assetIndex);
		m_pControlsTree->edit(assetIndex);
		m_bCreatedFromMenu = false;
	}
	else if (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading())
	{
		QModelIndex const& parentIndex = m_pProxyModel->mapFromSource(parent);
		m_pControlsTree->expand(parentIndex);
		m_pControlsTree->setCurrentIndex(parentIndex);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::InitFilterWidget()
{
	m_pFiltersLayout = new QVBoxLayout();
	m_pFiltersLayout->setContentsMargins(0, 5, 0, 5);

	QCheckBox* pFilterTriggersCheckbox = new QCheckBox(tr("Triggers"));
	QObject::connect(pFilterTriggersCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(eItemType_Trigger, bShow);
	});

	pFilterTriggersCheckbox->setChecked(true);
	pFilterTriggersCheckbox->setToolTip(tr("Show/hide Triggers"));
	pFilterTriggersCheckbox->setIcon(GetItemTypeIcon(eItemType_Trigger));
	m_pFiltersLayout->addWidget(pFilterTriggersCheckbox);

	QCheckBox* pFilterParametersCheckbox = new QCheckBox(tr("Parameters"));
	QObject::connect(pFilterParametersCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(eItemType_Parameter, bShow);
	});

	pFilterParametersCheckbox->setChecked(true);
	pFilterParametersCheckbox->setToolTip(tr("Show/hide Parameters"));
	pFilterParametersCheckbox->setIcon(GetItemTypeIcon(eItemType_Parameter));
	m_pFiltersLayout->addWidget(pFilterParametersCheckbox);

	QCheckBox* pFilterSwitchesCheckbox = new QCheckBox(tr("Switches"));
	QObject::connect(pFilterSwitchesCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(eItemType_Switch, bShow);
	});

	pFilterSwitchesCheckbox->setChecked(true);
	pFilterSwitchesCheckbox->setToolTip(tr("Show/hide Switches"));
	pFilterSwitchesCheckbox->setIcon(GetItemTypeIcon(eItemType_Switch));
	m_pFiltersLayout->addWidget(pFilterSwitchesCheckbox);

	QCheckBox* pFilterEnvironmentsCheckbox = new QCheckBox(tr("Environments"));
	QObject::connect(pFilterEnvironmentsCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(eItemType_Environment, bShow);
	});

	pFilterEnvironmentsCheckbox->setChecked(true);
	pFilterEnvironmentsCheckbox->setToolTip(tr("Show/hide Environments"));
	pFilterEnvironmentsCheckbox->setIcon(GetItemTypeIcon(eItemType_Environment));
	m_pFiltersLayout->addWidget(pFilterEnvironmentsCheckbox);

	QCheckBox* pFilterPreloadsCheckbox = new QCheckBox(tr("Preloads"));
	QObject::connect(pFilterPreloadsCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(eItemType_Preload, bShow);
	});

	pFilterPreloadsCheckbox->setChecked(true);
	pFilterPreloadsCheckbox->setToolTip(tr("Show/hide Preloads"));
	pFilterPreloadsCheckbox->setIcon(GetItemTypeIcon(eItemType_Preload));
	m_pFiltersLayout->addWidget(pFilterPreloadsCheckbox);

	m_pFilterWidget = QtUtil::MakeScrollable(m_pFiltersLayout);
	m_pFilterWidget->setHidden(true);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::ResetFilters()
{
	int const filtersCount = m_pFiltersLayout->count();

	for (int i = 0; i < filtersCount; ++i)
	{
		QCheckBox* pCheckBox = static_cast<QCheckBox*>(m_pFiltersLayout->itemAt(i)->widget());

		if (pCheckBox != nullptr)
		{
			pCheckBox->setChecked(true);
		}
	}

	m_pSearchBox->clear();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::BackupTreeViewStates()
{
	m_pControlsTree->BackupExpanded();
	m_pControlsTree->BackupSelection();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::RestoreTreeViewStates()
{
	m_pControlsTree->RestoreExpanded();
	m_pControlsTree->RestoreSelection();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsWidget::IsEditing() const
{
	return m_pControlsTree->IsEditing();
}
} // namespace ACE
