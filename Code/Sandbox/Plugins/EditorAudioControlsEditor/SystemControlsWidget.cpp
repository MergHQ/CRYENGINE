// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemControlsWidget.h"

#include "SystemControlsModel.h"
#include "AudioControlsEditorPlugin.h"
#include "MiddlewareDataWidget.h"
#include "MiddlewareDataModel.h"
#include "AudioAssets.h"
#include "AudioAssetsManager.h"
#include "SystemControlsEditorIcons.h"
#include "AudioTreeView.h"

#include <CryAudio/IAudioSystem.h>
#include <IAudioSystemEditor.h>
#include <IAudioSystemItem.h>
#include <IEditor.h>
#include <QtUtil.h>
#include <EditorStyleHelper.h>
#include <FilePathUtil.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>
#include <CryMath/Cry_Camera.h>
#include <CryIcon.h>
#include <Controls/QuestionDialog.h>
#include <ProxyModels/MountingProxyModel.h>
#include <QSearchBox.h>
#include <IUndoObject.h>

#include <QAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>


namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CSystemControlsWidget::CSystemControlsWidget(CAudioAssetsManager* pAssetsManager)
	: m_pAssetsManager(pAssetsManager)
	, m_pFilterProxyModel(new CSystemControlsFilterProxyModel(this))
{
	m_pAssetsModel = new CSystemControlsModel(m_pAssetsManager);

	auto const libCount = m_pAssetsManager->GetLibraryCount();
	m_libraryModels.resize(libCount);

	for (int i = 0; i < libCount; ++i)
	{
		m_libraryModels[i] = new CAudioLibraryModel(m_pAssetsManager, m_pAssetsManager->GetLibrary(i));
	}

	m_pMountingProxyModel = new CMountingProxyModel(WrapMemberFunction(this, &CSystemControlsWidget::CreateLibraryModelFromIndex));
	m_pMountingProxyModel->SetHeaderDataCallbacks(1, &GetHeaderData);
	m_pMountingProxyModel->SetSourceModel(m_pAssetsModel);

	m_pFilterProxyModel->setSourceModel(m_pMountingProxyModel);
	m_pFilterProxyModel->setDynamicSortFilter(true);
	
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QVBoxLayout* pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);

	QSplitter* pSplitter = new QSplitter();
	pSplitter->setOrientation(Qt::Vertical);
	pSplitter->setChildrenCollapsible(false);

	InitFilterWidgets(pMainLayout);
	pSplitter->addWidget(m_pFilterWidget);

	m_pTreeView = new CAudioTreeView();
	m_pTreeView->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
	m_pTreeView->setDragEnabled(true);
	m_pTreeView->setDragDropMode(QAbstractItemView::DragDrop);
	m_pTreeView->setDefaultDropAction(Qt::MoveAction);
	m_pTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pTreeView->setModel(m_pFilterProxyModel);
	m_pTreeView->sortByColumn(0, Qt::AscendingOrder);
	m_pTreeView->installEventFilter(this);
	pSplitter->addWidget(m_pTreeView);
	
	pSplitter->setStretchFactor(0, 0);
	pSplitter->setStretchFactor(1, 1);
	pMainLayout->addWidget(pSplitter);

	QObject::connect(m_pTreeView, &CAudioTreeView::customContextMenuRequested, this, &CSystemControlsWidget::OnContextMenu);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSystemControlsWidget::UpdateCreateButtons);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSystemControlsWidget::SelectedControlChanged);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, m_pTreeView, &CAudioTreeView::OnSelectionChanged);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CSystemControlsWidget::StopControlExecution);
	
	QObject::connect(m_pMountingProxyModel, &CMountingProxyModel::rowsInserted, this, &CSystemControlsWidget::SelectNewAsset);
	
	m_pAssetsManager->signalItemAboutToBeAdded.Connect([&](CAudioAsset* pItem)
	{
		ResetFilters();
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAboutToBeAdded.Connect([&]()
	{
		ResetFilters();
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAboutToBeRemoved.Connect([&](CAudioLibrary* pLibrary)
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
void CSystemControlsWidget::InitAddControlWidget(QHBoxLayout* pLayout)
{
	QPushButton* pAddButton = new QPushButton(tr("Add"));
	pAddButton->setToolTip(tr("Add new library, folder or control"));

	QMenu* pAddButtonMenu = new QMenu();

	pAddButtonMenu->addAction(GetItemTypeIcon(EItemType::Library), tr("Library"), [&]()
	{
		m_bCreatedFromMenu = true;
		m_pAssetsManager->CreateLibrary(Utils::GenerateUniqueLibraryName("new_library", *m_pAssetsManager));
	});

	pAddButtonMenu->addSeparator();

	m_pCreateFolderAction = new QAction(GetItemTypeIcon(EItemType::Folder), tr("Folder"), pAddButtonMenu);
	QObject::connect(m_pCreateFolderAction, &QAction::triggered, [&]() { CreateFolder(GetSelectedAsset()); });
	m_pCreateFolderAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateFolderAction);

	m_pCreateTriggerAction = new QAction(GetItemTypeIcon(EItemType::Trigger), tr("Trigger"), pAddButtonMenu);
	QObject::connect(m_pCreateTriggerAction, &QAction::triggered, [&]() { CreateControl("new_trigger", EItemType::Trigger, GetSelectedAsset()); });
	m_pCreateTriggerAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateTriggerAction);

	m_pCreateParameterAction = new QAction(GetItemTypeIcon(EItemType::Parameter), tr("Parameter"), pAddButtonMenu);
	QObject::connect(m_pCreateParameterAction, &QAction::triggered, [&]() { CreateControl("new_parameter", EItemType::Parameter, GetSelectedAsset()); });
	m_pCreateParameterAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateParameterAction);

	m_pCreateSwitchAction = new QAction(GetItemTypeIcon(EItemType::Switch), tr("Switch"), pAddButtonMenu);
	QObject::connect(m_pCreateSwitchAction, &QAction::triggered, [&]() { CreateControl("new_switch", EItemType::Switch, GetSelectedAsset()); });
	m_pCreateSwitchAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateSwitchAction);

	m_pCreateStateAction = new QAction(GetItemTypeIcon(EItemType::State), tr("State"), pAddButtonMenu);
	QObject::connect(m_pCreateStateAction, &QAction::triggered, [&]() { CreateControl("new_state", EItemType::State, GetSelectedAsset()); });
	m_pCreateStateAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateStateAction);

	m_pCreateEnvironmentAction = new QAction(GetItemTypeIcon(EItemType::Environment), tr("Environment"), pAddButtonMenu);
	QObject::connect(m_pCreateEnvironmentAction, &QAction::triggered, [&]() { CreateControl("new_environment", EItemType::Environment, GetSelectedAsset()); });
	m_pCreateEnvironmentAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateEnvironmentAction);

	m_pCreatePreloadAction = new QAction(GetItemTypeIcon(EItemType::Preload), tr("Preload"), pAddButtonMenu);
	QObject::connect(m_pCreatePreloadAction, &QAction::triggered, [&]() { CreateControl("new_preload", EItemType::Preload, GetSelectedAsset()); });
	m_pCreatePreloadAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreatePreloadAction);

	pAddButton->setMenu(pAddButtonMenu);
	pLayout->addWidget(pAddButton);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::InitFilterWidgets(QVBoxLayout* pMainLayout)
{
	QHBoxLayout* pFilterLayout = new QHBoxLayout();

	m_pSearchBox = new QSearchBox();
	pFilterLayout->addWidget(m_pSearchBox);

	m_pFilterButton = new QToolButton();
	m_pFilterButton->setIcon(CryIcon("icons:General/Filter.ico"));
	m_pFilterButton->setToolTip(tr("Show Control Type Filters"));
	m_pFilterButton->setCheckable(true);
	m_pFilterButton->setMaximumSize(QSize(20, 20));
	pFilterLayout->addWidget(m_pFilterButton);

	InitAddControlWidget(pFilterLayout);

	m_pFilterWidget = new QWidget();
	InitTypeFilters();

	QObject::connect(m_pFilterButton, &QToolButton::toggled, [&](bool const bChecked)
	{
		m_pFilterWidget->setVisible(bChecked);

		if (bChecked)
		{
			m_pFilterButton->setToolTip(tr("Hide Control Type Filters"));
		}
		else
		{
			m_pFilterButton->setToolTip(tr("Show Control Type Filters"));
		}
	});


	QObject::connect(m_pSearchBox, &QSearchBox::textChanged, [&](QString const& filter)
	{
		if (m_filter != filter)
		{
			if (m_filter.isEmpty() && !filter.isEmpty())
			{
				BackupTreeViewStates();
				StartTextFiltering();
				m_pTreeView->expandAll();
			}
			else if (!m_filter.isEmpty() && filter.isEmpty())
			{
				m_pFilterProxyModel->setFilterFixedString(filter);
				m_pTreeView->collapseAll();
				RestoreTreeViewStates();
				StopTextFiltering();
			}
			else if (!m_filter.isEmpty() && !filter.isEmpty())
			{
				m_pFilterProxyModel->setFilterFixedString(filter);
				m_pTreeView->expandAll();
			}

			m_filter = filter;
		}

		m_pFilterProxyModel->setFilterFixedString(filter);
	});

	pMainLayout->addLayout(pFilterLayout);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::InitTypeFilters()
{
	QVBoxLayout* pTypeFiltersLayout = new QVBoxLayout();
	pTypeFiltersLayout->setContentsMargins(0, 5, 0, 5);

	QCheckBox* pFilterTriggersCheckbox = new QCheckBox(tr("Triggers"));
	QObject::connect(pFilterTriggersCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(EItemType::Trigger, bShow);
	});

	pFilterTriggersCheckbox->setChecked(true);
	pFilterTriggersCheckbox->setToolTip(tr("Show/hide Triggers"));
	pFilterTriggersCheckbox->setIcon(GetItemTypeIcon(EItemType::Trigger));
	pTypeFiltersLayout->addWidget(pFilterTriggersCheckbox);

	QCheckBox* pFilterParametersCheckbox = new QCheckBox(tr("Parameters"));
	QObject::connect(pFilterParametersCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(EItemType::Parameter, bShow);
	});

	pFilterParametersCheckbox->setChecked(true);
	pFilterParametersCheckbox->setToolTip(tr("Show/hide Parameters"));
	pFilterParametersCheckbox->setIcon(GetItemTypeIcon(EItemType::Parameter));
	pTypeFiltersLayout->addWidget(pFilterParametersCheckbox);

	QCheckBox* pFilterSwitchesCheckbox = new QCheckBox(tr("Switches"));
	QObject::connect(pFilterSwitchesCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(EItemType::Switch, bShow);
	});

	pFilterSwitchesCheckbox->setChecked(true);
	pFilterSwitchesCheckbox->setToolTip(tr("Show/hide Switches"));
	pFilterSwitchesCheckbox->setIcon(GetItemTypeIcon(EItemType::Switch));
	pTypeFiltersLayout->addWidget(pFilterSwitchesCheckbox);

	QCheckBox* pFilterEnvironmentsCheckbox = new QCheckBox(tr("Environments"));
	QObject::connect(pFilterEnvironmentsCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(EItemType::Environment, bShow);
	});

	pFilterEnvironmentsCheckbox->setChecked(true);
	pFilterEnvironmentsCheckbox->setToolTip(tr("Show/hide Environments"));
	pFilterEnvironmentsCheckbox->setIcon(GetItemTypeIcon(EItemType::Environment));
	pTypeFiltersLayout->addWidget(pFilterEnvironmentsCheckbox);

	QCheckBox* pFilterPreloadsCheckbox = new QCheckBox(tr("Preloads"));
	QObject::connect(pFilterPreloadsCheckbox, &QCheckBox::toggled, [&](bool const bShow)
	{
		ShowControlType(EItemType::Preload, bShow);
	});

	pFilterPreloadsCheckbox->setChecked(true);
	pFilterPreloadsCheckbox->setToolTip(tr("Show/hide Preloads"));
	pFilterPreloadsCheckbox->setIcon(GetItemTypeIcon(EItemType::Preload));
	pTypeFiltersLayout->addWidget(pFilterPreloadsCheckbox);

	m_pFilterWidget = QtUtil::MakeScrollable(pTypeFiltersLayout);
	m_pFilterWidget->setHidden(true);
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if ((pEvent->type() == QEvent::KeyRelease) && !m_pTreeView->IsEditing())
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
	QModelIndexList const indexes = m_pTreeView->selectionModel()->selectedIndexes();
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
void CSystemControlsWidget::ShowControlType(EItemType const type, bool const bShow)
{
	m_pFilterProxyModel->EnableControl(bShow, type);

	if (type == EItemType::Switch)
	{
		m_pFilterProxyModel->EnableControl(bShow, EItemType::State);
	}

	ControlTypeFiltered(type, bShow);
}

//////////////////////////////////////////////////////////////////////////
CAudioControl* CSystemControlsWidget::CreateControl(string const& name, EItemType type, CAudioAsset* pParent)
{
	m_bCreatedFromMenu = true;

	if (type != EItemType::State)
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
	return m_pAssetsManager->CreateFolder(Utils::GenerateUniqueName("new_folder", EItemType::Folder, pParent), pParent);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::OnContextMenu(QPoint const& pos)
{
	QMenu* pContextMenu = new QMenu();

	std::vector<CAudioLibrary*> libraries;
	std::vector<CAudioFolder*> folders;
	std::vector<CAudioControl*> controls;

	auto const selection = m_pTreeView->selectionModel()->selectedRows();
	AudioModelUtils::GetAssetsFromIndices(selection, libraries, folders, controls);

	if (!selection.isEmpty())
	{
		if ((controls.size() == 0) && ((libraries.size() == 1) || (folders.size() == 1)))
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
			pAddMenu->addAction(GetItemTypeIcon(EItemType::Folder), tr("Folder"), [&]() { CreateFolder(pParent); });
			// To do: Add parent folder and move selection into.
			pAddMenu->addSeparator();
			pAddMenu->addAction(GetItemTypeIcon(EItemType::Trigger), tr("Trigger"), [&]() { CreateControl("new_trigger", EItemType::Trigger, pParent); });
			pAddMenu->addAction(GetItemTypeIcon(EItemType::Parameter), tr("Parameter"), [&]() { CreateControl("new_parameter", EItemType::Parameter, pParent); });
			pAddMenu->addAction(GetItemTypeIcon(EItemType::Switch), tr("Switch"), [&]() { CreateControl("new_switch", EItemType::Switch, pParent); });
			pAddMenu->addAction(GetItemTypeIcon(EItemType::Environment), tr("Environment"), [&]() { CreateControl("new_environment", EItemType::Environment, pParent); });
			pAddMenu->addAction(GetItemTypeIcon(EItemType::Preload), tr("Preload"), [&]() { CreateControl("new_preload", EItemType::Preload, pParent); });
			pContextMenu->addMenu(pAddMenu);
			pContextMenu->addSeparator();

			if (pParent->GetType() == EItemType::Library)
			{
				//TODO: Take into account files in the "levels" folder
				pContextMenu->addAction(tr("Open Containing Folder"), [&]()
				{
					QtUtil::OpenInExplorer(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), Utils::GetAssetFolder() + pParent->GetName() + ".xml").c_str());
				});

				pContextMenu->addSeparator();
			}
		}

		if (controls.size() == 1)
		{
			CAudioControl* pControl = controls[0];

			switch (pControl->GetType())
			{
			case EItemType::Trigger:
				pContextMenu->addAction(tr("Execute Trigger"), [&]() { ExecuteControl(); });
				pContextMenu->addSeparator();
				break;
			case EItemType::Switch:
				pContextMenu->addAction(GetItemTypeIcon(EItemType::State), tr("Add State"), [&]() { CreateControl("new_state", EItemType::State, pControl); });
				pContextMenu->addSeparator();
				break;
			case EItemType::Preload:
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
				if (pControl->GetType() == EItemType::Preload && pControl->GetScope() == CCrc32::Compute("global") && !pControl->IsAutoLoad())
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

		pContextMenu->addAction(tr("Rename"), [&]() { m_pTreeView->edit(m_pTreeView->currentIndex()); });
		pContextMenu->addAction(tr("Delete"), [&]() { DeleteSelectedControl(); });
		pContextMenu->addSeparator();
		pContextMenu->addAction(tr("Expand Selection"), [&]() { m_pTreeView->ExpandSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addAction(tr("Collapse Selection"), [&]() { m_pTreeView->CollapseSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addSeparator();
	}
	else
	{
		pContextMenu->addAction(GetItemTypeIcon(EItemType::Library), tr("Add Library"), [&]()
		{
			m_bCreatedFromMenu = true;
			m_pAssetsManager->CreateLibrary(Utils::GenerateUniqueLibraryName("new_library", *m_pAssetsManager));
		});

		pContextMenu->addSeparator();
	}
	
	pContextMenu->addAction(tr("Expand All"), [&]() { m_pTreeView->expandAll(); });
	pContextMenu->addAction(tr("Collapse All"), [&]() { m_pTreeView->collapseAll(); });

	pContextMenu->exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::DeleteSelectedControl()
{
	auto const selection = m_pTreeView->selectionModel()->selectedRows();
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
			text = "Are you sure you want to delete the selected controls and folders?";
		}

		CQuestionDialog* messageBox = new CQuestionDialog();
		messageBox->SetupQuestion("Audio Controls Editor", text);

		if (messageBox->Execute() == QDialogButtonBox::Yes)
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
	CAudioAsset* pAsset = AudioModelUtils::GetAssetFromIndex(m_pTreeView->currentIndex());

	if (pAsset && pAsset->GetType() == EItemType::Trigger)
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
	QModelIndex const& index = m_pTreeView->currentIndex();

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
		QModelIndex const& assetIndex = m_pFilterProxyModel->mapFromSource(m_pMountingProxyModel->index(row, 0, parent));
		m_pTreeView->setCurrentIndex(assetIndex);
		m_pTreeView->edit(assetIndex);
		m_bCreatedFromMenu = false;
	}
	else if (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading())
	{
		QModelIndex const& parentIndex = m_pFilterProxyModel->mapFromSource(parent);
		m_pTreeView->expand(parentIndex);
		m_pTreeView->setCurrentIndex(parentIndex);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::UpdateCreateButtons()
{
	auto const selection = m_pTreeView->selectionModel()->selectedRows();

	if (!selection.isEmpty())
	{
		QModelIndex const index = m_pTreeView->currentIndex();

		if (index.isValid())
		{
			CAudioAsset* const pAsset = AudioModelUtils::GetAssetFromIndex(index);

			if (pAsset != nullptr)
			{
				EItemType const itemType = pAsset->GetType();
				bool const bLibraryOrFolder = (itemType == EItemType::Library) || (itemType == EItemType::Folder);

				m_pCreateFolderAction->setVisible(bLibraryOrFolder);
				m_pCreateTriggerAction->setVisible(bLibraryOrFolder);
				m_pCreateParameterAction->setVisible(bLibraryOrFolder);
				m_pCreateSwitchAction->setVisible(bLibraryOrFolder);
				m_pCreateStateAction->setVisible(itemType == EItemType::Switch);
				m_pCreateEnvironmentAction->setVisible(bLibraryOrFolder);
				m_pCreatePreloadAction->setVisible(bLibraryOrFolder);
				return;
			}
		}
	}

	m_pCreateFolderAction->setVisible(false);
	m_pCreateTriggerAction->setVisible(false);
	m_pCreateParameterAction->setVisible(false);
	m_pCreateSwitchAction->setVisible(false);
	m_pCreateStateAction->setVisible(false);
	m_pCreateEnvironmentAction->setVisible(false);
	m_pCreatePreloadAction->setVisible(false);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::ResetFilters()
{
	for (auto pCheckBox : m_pFilterWidget->findChildren<QCheckBox*>())
	{
		pCheckBox->setChecked(true);
	}

	m_pSearchBox->clear();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::BackupTreeViewStates()
{
	m_pTreeView->BackupExpanded();
	m_pTreeView->BackupSelection();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::RestoreTreeViewStates()
{
	m_pTreeView->RestoreExpanded();
	m_pTreeView->RestoreSelection();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsWidget::IsEditing() const
{
	return m_pTreeView->IsEditing();
}
} // namespace ACE
