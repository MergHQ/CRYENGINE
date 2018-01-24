// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemControlsWidget.h"

#include "SystemControlsModel.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "SystemAssetsManager.h"
#include "SystemControlsIcons.h"
#include "TreeView.h"

#include <CryAudio/IAudioSystem.h>
#include <QtUtil.h>
#include <FilePathUtil.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>
#include <CryMath/Cry_Camera.h>
#include <Controls/QuestionDialog.h>
#include <QSearchBox.h>
#include <QFilteringPanel.h>
#include <ProxyModels/MountingProxyModel.h>

#include <QAction>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CSystemControlsWidget::CSystemControlsWidget(CSystemAssetsManager* const pAssetsManager, QWidget* const pParent)
	: QWidget(pParent)
	, m_pAssetsManager(pAssetsManager)
	, m_pSystemFilterProxyModel(new CSystemFilterProxyModel(this))
	, m_pSourceModel(new CSystemSourceModel(m_pAssetsManager, this))
	, m_pTreeView(new CTreeView(this))
	, m_nameColumn(static_cast<int>(SystemModelUtils::EColumns::Name))
	, m_isReloading(false)
	, m_isCreatedFromMenu(false)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	m_pMountingProxyModel = new CMountingProxyModel(WrapMemberFunction(this, &CSystemControlsWidget::CreateLibraryModelFromIndex));
	m_pMountingProxyModel->SetHeaderDataCallbacks(static_cast<int>(SystemModelUtils::EColumns::Count), &SystemModelUtils::GetHeaderData, Attributes::s_getAttributeRole);
	m_pMountingProxyModel->SetSourceModel(m_pSourceModel);

	m_pSystemFilterProxyModel->setSourceModel(m_pMountingProxyModel);
	m_pSystemFilterProxyModel->setFilterKeyColumn(m_nameColumn);

	m_pTreeView->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
	m_pTreeView->setDragEnabled(true);
	m_pTreeView->setDragDropMode(QAbstractItemView::DragDrop);
	m_pTreeView->setDefaultDropAction(Qt::MoveAction);
	m_pTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_pTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_pTreeView->setTreePosition(m_nameColumn);
	m_pTreeView->setUniformRowHeights(true);
	m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pTreeView->setModel(m_pSystemFilterProxyModel);
	m_pTreeView->sortByColumn(m_nameColumn, Qt::AscendingOrder);
	m_pTreeView->viewport()->installEventFilter(this);
	m_pTreeView->installEventFilter(this);
	m_pTreeView->header()->setMinimumSectionSize(25);
	m_pTreeView->header()->setSectionResizeMode(static_cast<int>(SystemModelUtils::EColumns::Notification), QHeaderView::ResizeToContents);
	m_pTreeView->SetNameColumn(m_nameColumn);
	m_pTreeView->SetNameRole(static_cast<int>(SystemModelUtils::ERoles::Name));
	m_pTreeView->TriggerRefreshHeaderColumns();

	m_pFilteringPanel = new QFilteringPanel("ACESystemControls", m_pSystemFilterProxyModel);
	m_pFilteringPanel->SetContent(m_pTreeView);
	m_pFilteringPanel->GetSearchBox()->SetAutoExpandOnSearch(m_pTreeView);

	QVBoxLayout* const pMainLayout = new QVBoxLayout(this);
	pMainLayout->setContentsMargins(0, 0, 0, 0);
	InitAddControlWidget(pMainLayout);
	pMainLayout->addWidget(m_pFilteringPanel);

	IEditorImpl const* const pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();

	if (pEditorImpl == nullptr)
	{
		setEnabled(false);
	}

	QObject::connect(m_pTreeView, &CTreeView::customContextMenuRequested, this, &CSystemControlsWidget::OnContextMenu);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSystemControlsWidget::SignalSelectedControlChanged);
	QObject::connect(m_pTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CSystemControlsWidget::StopControlExecution);
	QObject::connect(m_pMountingProxyModel, &CMountingProxyModel::rowsInserted, this, &CSystemControlsWidget::SelectNewAsset);

	m_pAssetsManager->SignalLibraryAboutToBeRemoved.Connect([&](CSystemLibrary* const pLibrary)
		{
			int const libCount = m_pAssetsManager->GetLibraryCount();

			for (int i = 0; i < libCount; ++i)
			{
			  if (m_pAssetsManager->GetLibrary(i) == pLibrary)
			  {
			    m_libraryModels[i]->DisconnectSignals();
			    m_libraryModels[i]->deleteLater();
			    m_libraryModels.erase(m_libraryModels.begin() + i);
			    break;
			  }
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalAssetRenamed.Connect([&]()
		{
			if (!m_pAssetsManager->IsLoading())
			{
			  m_pSystemFilterProxyModel->invalidate();
			  m_pTreeView->scrollTo(m_pTreeView->currentIndex());
			}
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::SignalAboutToLoad.Connect([&]()
		{
			StopControlExecution();
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.Connect([&]()
		{
			StopControlExecution();
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.Connect([&]()
		{
			IEditorImpl const* const pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();
			setEnabled(pEditorImpl != nullptr);
	  }, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
CSystemControlsWidget::~CSystemControlsWidget()
{
	m_pAssetsManager->SignalLibraryAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalAssetRenamed.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::SignalAboutToLoad.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));

	StopControlExecution();
	DeleteModels();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::InitAddControlWidget(QVBoxLayout* const pLayout)
{
	QPushButton* const pAddButton = new QPushButton(tr("Add"), this);
	pAddButton->setToolTip(tr("Add new library, folder or control"));

	QMenu* const pAddButtonMenu = new QMenu(pAddButton);
	QObject::connect(pAddButtonMenu, &QMenu::aboutToShow, this, &CSystemControlsWidget::OnUpdateCreateButtons);

	pAddButtonMenu->addAction(GetItemTypeIcon(ESystemItemType::Library), tr("Library"), [&]()
		{
			m_isCreatedFromMenu = true;
			m_pAssetsManager->CreateLibrary(Utils::GenerateUniqueLibraryName("new_library", *m_pAssetsManager));
	  });

	pAddButtonMenu->addSeparator();

	m_pCreateParentFolderAction = new QAction(GetItemTypeIcon(ESystemItemType::Folder), tr("Parent Folder"), pAddButtonMenu);
	QObject::connect(m_pCreateParentFolderAction, &QAction::triggered, [&]() { CreateParentFolder(); });
	m_pCreateParentFolderAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateParentFolderAction);

	m_pCreateFolderAction = new QAction(GetItemTypeIcon(ESystemItemType::Folder), tr("Folder"), pAddButtonMenu);
	QObject::connect(m_pCreateFolderAction, &QAction::triggered, [&]() { CreateFolder(GetSelectedAsset()); });
	m_pCreateFolderAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateFolderAction);

	m_pCreateTriggerAction = new QAction(GetItemTypeIcon(ESystemItemType::Trigger), tr("Trigger"), pAddButtonMenu);
	QObject::connect(m_pCreateTriggerAction, &QAction::triggered, [&]() { CreateControl("new_trigger", ESystemItemType::Trigger, GetSelectedAsset()); });
	m_pCreateTriggerAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateTriggerAction);

	m_pCreateParameterAction = new QAction(GetItemTypeIcon(ESystemItemType::Parameter), tr("Parameter"), pAddButtonMenu);
	QObject::connect(m_pCreateParameterAction, &QAction::triggered, [&]() { CreateControl("new_parameter", ESystemItemType::Parameter, GetSelectedAsset()); });
	m_pCreateParameterAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateParameterAction);

	m_pCreateSwitchAction = new QAction(GetItemTypeIcon(ESystemItemType::Switch), tr("Switch"), pAddButtonMenu);
	QObject::connect(m_pCreateSwitchAction, &QAction::triggered, [&]() { CreateControl("new_switch", ESystemItemType::Switch, GetSelectedAsset()); });
	m_pCreateSwitchAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateSwitchAction);

	m_pCreateStateAction = new QAction(GetItemTypeIcon(ESystemItemType::State), tr("State"), pAddButtonMenu);
	QObject::connect(m_pCreateStateAction, &QAction::triggered, [&]() { CreateControl("new_state", ESystemItemType::State, GetSelectedAsset()); });
	m_pCreateStateAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateStateAction);

	m_pCreateEnvironmentAction = new QAction(GetItemTypeIcon(ESystemItemType::Environment), tr("Environment"), pAddButtonMenu);
	QObject::connect(m_pCreateEnvironmentAction, &QAction::triggered, [&]() { CreateControl("new_environment", ESystemItemType::Environment, GetSelectedAsset()); });
	m_pCreateEnvironmentAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreateEnvironmentAction);

	m_pCreatePreloadAction = new QAction(GetItemTypeIcon(ESystemItemType::Preload), tr("Preload"), pAddButtonMenu);
	QObject::connect(m_pCreatePreloadAction, &QAction::triggered, [&]() { CreateControl("new_preload", ESystemItemType::Preload, GetSelectedAsset()); });
	m_pCreatePreloadAction->setVisible(false);
	pAddButtonMenu->addAction(m_pCreatePreloadAction);

	pAddButton->setMenu(pAddButtonMenu);
	pLayout->addWidget(pAddButton);
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if ((pEvent->type() == QEvent::KeyRelease) && !m_pTreeView->IsEditing())
	{
		QKeyEvent const* const pKeyEvent = static_cast<QKeyEvent*>(pEvent);

		if (pKeyEvent != nullptr)
		{
			if (pKeyEvent->key() == Qt::Key_Delete)
			{
				OnDeleteSelectedControl();
			}
			else if (pKeyEvent->key() == Qt::Key_Space)
			{
				ExecuteControl();
			}
		}
	}
	else if ((pEvent->type() == QEvent::MouseButtonDblClick) && !m_pTreeView->IsEditing())
	{
		ExecuteControl();
	}
	else if (pEvent->type() == QEvent::Drop)
	{
		m_pTreeView->selectionModel()->clearSelection();
		m_pTreeView->selectionModel()->clearCurrentIndex();
	}

	return QWidget::eventFilter(pObject, pEvent);
}

//////////////////////////////////////////////////////////////////////////
std::vector<CSystemAsset*> CSystemControlsWidget::GetSelectedAssets() const
{
	QModelIndexList const& indexes = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);
	std::vector<CSystemAsset*> assets;
	SystemModelUtils::GetAssetsFromIndexesCombined(indexes, assets);
	return assets;
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::Reload()
{
	ClearFilters();
	m_pSystemFilterProxyModel->invalidate();
}

//////////////////////////////////////////////////////////////////////////
CSystemControl* CSystemControlsWidget::CreateControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent)
{
	CSystemControl* pControl = nullptr;
	m_isCreatedFromMenu = true;

	if (type != ESystemItemType::State)
	{
		pControl = m_pAssetsManager->CreateControl(Utils::GenerateUniqueControlName(name, type, *m_pAssetsManager), type, pParent);
	}
	else
	{
		pControl = m_pAssetsManager->CreateControl(Utils::GenerateUniqueName(name, type, pParent), type, pParent);
	}

	return pControl;
}

//////////////////////////////////////////////////////////////////////////
CSystemAsset* CSystemControlsWidget::CreateFolder(CSystemAsset* const pParent)
{
	m_isCreatedFromMenu = true;
	return m_pAssetsManager->CreateFolder(Utils::GenerateUniqueName("new_folder", ESystemItemType::Folder, pParent), pParent);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::CreateParentFolder()
{
	std::vector<CSystemLibrary*> libraries;
	std::vector<CSystemFolder*> folders;
	std::vector<CSystemControl*> controls;
	std::vector<CSystemAsset*> assetsToMove;

	auto const& selection = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);
	SystemModelUtils::GetAssetsFromIndexesSeparated(selection, libraries, folders, controls);

	for (auto const pFolder : folders)
	{
		assetsToMove.emplace_back(static_cast<CSystemAsset*>(pFolder));
	}

	for (auto const pControl : controls)
	{
		assetsToMove.emplace_back(static_cast<CSystemAsset*>(pControl));
	}

	auto const pParent = assetsToMove[0]->GetParent();
	auto const pNewFolder = CreateFolder(pParent);
	m_pAssetsManager->MoveItems(pNewFolder, assetsToMove);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::OnContextMenu(QPoint const& pos)
{
	QMenu* const pContextMenu = new QMenu(this);
	QMenu* const pAddMenu = new QMenu(tr("Add"), pContextMenu);
	auto const& selection = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);

	pContextMenu->addMenu(pAddMenu);
	pContextMenu->addSeparator();

	pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Library), tr("Library"), [&]()
		{
			m_isCreatedFromMenu = true;
			m_pAssetsManager->CreateLibrary(Utils::GenerateUniqueLibraryName("new_library", *m_pAssetsManager));
	  });

	pAddMenu->addSeparator();

	if (!selection.isEmpty())
	{
		std::vector<CSystemLibrary*> libraries;
		std::vector<CSystemFolder*> folders;
		std::vector<CSystemControl*> controls;

		SystemModelUtils::GetAssetsFromIndexesSeparated(selection, libraries, folders, controls);

		if (!IsDefaultControlSelected())
		{
			if (IsParentFolderAllowed())
			{
				pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Folder), tr("Parent Folder"), [&]() { CreateParentFolder(); });
			}

			if (selection.size() == 1)
			{
				QModelIndex const& index = m_pTreeView->currentIndex();

				if (index.isValid())
				{
					CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, m_nameColumn);

					if (pAsset != nullptr)
					{
						ESystemItemType const assetType = pAsset->GetType();

						if ((assetType == ESystemItemType::Library) || (assetType == ESystemItemType::Folder))
						{
							CSystemAsset* pParent = nullptr;

							if (assetType == ESystemItemType::Folder)
							{
								pParent = static_cast<CSystemAsset*>(folders[0]);
							}
							else
							{
								pParent = static_cast<CSystemAsset*>(libraries[0]);
							}

							pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Folder), tr("Folder"), [&]() { CreateFolder(pParent); });
							pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Trigger), tr("Trigger"), [&]() { CreateControl("new_trigger", ESystemItemType::Trigger, pParent); });
							pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Parameter), tr("Parameter"), [&]() { CreateControl("new_parameter", ESystemItemType::Parameter, pParent); });
							pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Switch), tr("Switch"), [&]() { CreateControl("new_switch", ESystemItemType::Switch, pParent); });
							pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Environment), tr("Environment"), [&]() { CreateControl("new_environment", ESystemItemType::Environment, pParent); });
							pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::Preload), tr("Preload"), [&]() { CreateControl("new_preload", ESystemItemType::Preload, pParent); });

							if (pParent->GetType() == ESystemItemType::Library)
							{
								pContextMenu->addAction(tr("Open Containing Folder"), [&]()
									{
										QtUtil::OpenInExplorer(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), m_pAssetsManager->GetConfigFolderPath() + pParent->GetName() + ".xml").c_str());
								  });

								pContextMenu->addSeparator();
							}
						}
					}
				}

				if (controls.size() == 1)
				{
					CSystemControl* const pControl = controls[0];
					ESystemItemType const controlType = pControl->GetType();

					if (controlType == ESystemItemType::Trigger)
					{
						QAction* const pTriggerAction = new QAction(tr("Execute Trigger"), pContextMenu);
						QObject::connect(pTriggerAction, &QAction::triggered, [&]() { ExecuteControl(); });
						pContextMenu->insertSeparator(pContextMenu->actions().at(0));
						pContextMenu->insertAction(pContextMenu->actions().at(0), pTriggerAction);
					}
					else if (controlType == ESystemItemType::Switch)
					{
						pAddMenu->addAction(GetItemTypeIcon(ESystemItemType::State), tr("State"), [&]() { CreateControl("new_state", ESystemItemType::State, pControl); });
					}
					else if (controlType == ESystemItemType::Preload)
					{
						if (pControl->GetScope() == CryAudio::StringToId("global") && !pControl->IsAutoLoad())
						{
							QAction* const pLoadAction = new QAction(tr("Load Global Preload Request"), pContextMenu);
							QAction* const pUnloadAction = new QAction(tr("Unload Global Preload Request"), pContextMenu);
							QObject::connect(pLoadAction, &QAction::triggered, [&]() { gEnv->pAudioSystem->PreloadSingleRequest(CryAudio::StringToId(pControl->GetName()), false); });
							QObject::connect(pUnloadAction, &QAction::triggered, [&]() { gEnv->pAudioSystem->UnloadSingleRequest(CryAudio::StringToId(pControl->GetName())); });
							pContextMenu->insertSeparator(pContextMenu->actions().at(0));
							pContextMenu->insertAction(pContextMenu->actions().at(0), pUnloadAction);
							pContextMenu->insertAction(pContextMenu->actions().at(0), pLoadAction);
						}
					}
				}
			}
			else if (controls.size() > 1)
			{
				bool hasOnlyGlobalPreloads = false;

				for (auto const pControl : controls)
				{
					if ((pControl->GetType() == ESystemItemType::Preload) && (pControl->GetScope() == CryAudio::StringToId("global")) && !pControl->IsAutoLoad())
					{
						hasOnlyGlobalPreloads = true;
					}
					else
					{
						hasOnlyGlobalPreloads = false;
						break;
					}
				}

				if (hasOnlyGlobalPreloads)
				{

					QAction* const pLoadAction = new QAction(tr("Load Global Preload Requests"), pContextMenu);
					QAction* const pUnloadAction = new QAction(tr("Unload Global Preload Requests"), pContextMenu);
					QObject::connect(pLoadAction, &QAction::triggered, [&]()
						{
							for (auto const pControl : controls)
							{
							  gEnv->pAudioSystem->PreloadSingleRequest(CryAudio::StringToId(pControl->GetName()), false);
							}
					  });

					QObject::connect(pUnloadAction, &QAction::triggered, [&]()
						{
							for (auto const pControl : controls)
							{
							  gEnv->pAudioSystem->UnloadSingleRequest(CryAudio::StringToId(pControl->GetName()));
							}
					  });

					pContextMenu->insertSeparator(pContextMenu->actions().at(0));
					pContextMenu->insertAction(pContextMenu->actions().at(0), pUnloadAction);
					pContextMenu->insertAction(pContextMenu->actions().at(0), pLoadAction);
				}
			}

			if (selection.size() == 1)
			{
				pContextMenu->addAction(tr("Rename"), [&]()
					{
						QModelIndex const& nameColumnIndex = m_pTreeView->currentIndex().sibling(m_pTreeView->currentIndex().row(), m_nameColumn);
						m_pTreeView->edit(nameColumnIndex);
				  });
			}
		}

		pContextMenu->addAction(tr("Delete"), [&]() { OnDeleteSelectedControl(); });
		pContextMenu->addSeparator();
		pContextMenu->addAction(tr("Expand Selection"), [&]() { m_pTreeView->ExpandSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addAction(tr("Collapse Selection"), [&]() { m_pTreeView->CollapseSelection(m_pTreeView->GetSelectedIndexes()); });
		pContextMenu->addSeparator();
	}

	pContextMenu->addAction(tr("Expand All"), [&]() { m_pTreeView->expandAll(); });
	pContextMenu->addAction(tr("Collapse All"), [&]() { m_pTreeView->collapseAll(); });

	pContextMenu->exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::OnDeleteSelectedControl()
{
	auto const& selection = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);
	int const size = selection.length();

	if (size > 0)
	{
		QString text;

		if (size == 1)
		{
			text = tr(R"(Are you sure you want to delete ")") + selection[0].data(Qt::DisplayRole).toString() + R"("?.)";
		}
		else
		{
			text = tr("Are you sure you want to delete the selected controls and folders?");
		}

		CQuestionDialog* const messageBox = new CQuestionDialog();
		messageBox->SetupQuestion("Audio Controls Editor", text);

		if (messageBox->Execute() == QDialogButtonBox::Yes)
		{
			std::vector<CSystemAsset*> selectedItems;

			for (auto const& index : selection)
			{
				if (index.isValid())
				{
					selectedItems.emplace_back(SystemModelUtils::GetAssetFromIndex(index, m_nameColumn));
				}
			}

			std::vector<CSystemAsset*> itemsToDelete;
			Utils::SelectTopLevelAncestors(selectedItems, itemsToDelete);
			std::vector<string> defaultControlNames;

			for (auto const pItem : itemsToDelete)
			{
				if (!pItem->HasDefaultControlChildren(defaultControlNames))
				{
					m_pAssetsManager->DeleteItem(pItem);
				}
			}

			if (!defaultControlNames.empty())
			{
				QString defaultControlText;

				if (defaultControlNames.size() > 1)
				{
					defaultControlText = tr("Default controls could not get deleted:");
				}
				else
				{
					defaultControlText = tr("Default control could not get deleted:");
				}

				for (auto const& name : defaultControlNames)
				{
					defaultControlText.append("\n\"" + QtUtil::ToQString(name) + "\"");
				}

				CQuestionDialog* const defaultControlsMessageBox = new CQuestionDialog();
				defaultControlsMessageBox->SetupQuestion("Audio Controls Editor", defaultControlText, QDialogButtonBox::Ok, QDialogButtonBox::Ok);
				defaultControlsMessageBox->Execute();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::ExecuteControl()
{
	CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(m_pTreeView->currentIndex(), m_nameColumn);

	if ((pAsset != nullptr) && (pAsset->GetType() == ESystemItemType::Trigger))
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
	QAbstractItemModel* model = nullptr;

	if (sourceIndex.model() == m_pSourceModel)
	{
		size_t const numLibraries = m_libraryModels.size();
		size_t const row = static_cast<size_t>(sourceIndex.row());

		if (row >= numLibraries)
		{
			m_libraryModels.resize(row + 1);

			for (size_t i = numLibraries; i < row + 1; ++i)
			{
				m_libraryModels[i] = new CSystemLibraryModel(m_pAssetsManager, m_pAssetsManager->GetLibrary(i), this);
			}
		}

		model = m_libraryModels[row];
	}

	return model;
}

//////////////////////////////////////////////////////////////////////////
CSystemAsset* CSystemControlsWidget::GetSelectedAsset() const
{
	CSystemAsset* pAsset = nullptr;
	QModelIndex const& index = m_pTreeView->currentIndex();

	if (index.isValid())
	{
		pAsset = SystemModelUtils::GetAssetFromIndex(index, m_nameColumn);
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::SelectNewAsset(QModelIndex const& parent, int const row)
{
	if (!CAudioControlsEditorPlugin::GetAssetsManager()->IsLoading())
	{
		ClearFilters();
		QModelIndex const& assetIndex = m_pSystemFilterProxyModel->mapFromSource(m_pMountingProxyModel->index(row, m_nameColumn, parent));

		if (m_isCreatedFromMenu)
		{
			m_pTreeView->setCurrentIndex(assetIndex);
			m_pTreeView->edit(assetIndex);
			m_isCreatedFromMenu = false;
		}
		else
		{
			QModelIndex const& parentIndex = m_pSystemFilterProxyModel->mapFromSource(parent);
			m_pTreeView->expand(parentIndex);
			m_pTreeView->selectionModel()->select(assetIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

			if (m_pTreeView->selectionModel()->selectedRows(m_nameColumn).size() == 1)
			{
				m_pTreeView->setCurrentIndex(assetIndex);
			}
			else
			{
				m_pTreeView->scrollTo(assetIndex);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::OnUpdateCreateButtons()
{
	bool isActionVisible = false;

	if (IsDefaultControlSelected())
	{
		m_pCreateParentFolderAction->setVisible(false);
	}
	else
	{
		m_pCreateParentFolderAction->setVisible(IsParentFolderAllowed());
		auto const& selection = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);

		if (selection.size() == 1)
		{
			QModelIndex const& index = m_pTreeView->currentIndex();

			if (index.isValid())
			{
				CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, m_nameColumn);

				if (pAsset != nullptr)
				{
					ESystemItemType const itemType = pAsset->GetType();
					bool const isLibraryOrFolder = (itemType == ESystemItemType::Library) || (itemType == ESystemItemType::Folder);

					m_pCreateFolderAction->setVisible(isLibraryOrFolder);
					m_pCreateTriggerAction->setVisible(isLibraryOrFolder);
					m_pCreateParameterAction->setVisible(isLibraryOrFolder);
					m_pCreateSwitchAction->setVisible(isLibraryOrFolder);
					m_pCreateStateAction->setVisible(itemType == ESystemItemType::Switch);
					m_pCreateEnvironmentAction->setVisible(isLibraryOrFolder);
					m_pCreatePreloadAction->setVisible(isLibraryOrFolder);
					isActionVisible = true;
				}
			}
		}
	}

	if (!isActionVisible)
	{
		m_pCreateFolderAction->setVisible(false);
		m_pCreateTriggerAction->setVisible(false);
		m_pCreateParameterAction->setVisible(false);
		m_pCreateSwitchAction->setVisible(false);
		m_pCreateStateAction->setVisible(false);
		m_pCreateEnvironmentAction->setVisible(false);
		m_pCreatePreloadAction->setVisible(false);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsWidget::IsParentFolderAllowed() const
{
	bool isAllowed = false;
	auto const& selection = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);

	if (!selection.isEmpty())
	{
		std::vector<CSystemLibrary*> libraries;
		std::vector<CSystemFolder*> folders;
		std::vector<CSystemControl*> controls;

		SystemModelUtils::GetAssetsFromIndexesSeparated(selection, libraries, folders, controls);

		if (libraries.empty() && (!folders.empty() || !controls.empty()))
		{
			isAllowed = true;
			CSystemAsset const* pParent;

			if (!folders.empty())
			{
				pParent = folders[0]->GetParent();

				for (auto const pFolder : folders)
				{
					if (pFolder->GetParent() != pParent)
					{
						isAllowed = false;
						break;
					}
				}
			}
			else
			{
				pParent = controls[0]->GetParent();
			}

			if (isAllowed)
			{
				for (auto const pControl : controls)
				{
					if ((pControl->GetParent() != pParent) || (pControl->GetType() == ESystemItemType::State))
					{
						isAllowed = false;
						break;
					}
				}
			}
		}
	}

	return isAllowed;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsWidget::IsDefaultControlSelected() const
{
	bool isDefaultControlSelected = false;
	auto const& selection = m_pTreeView->selectionModel()->selectedRows(m_nameColumn);

	for (auto const& index : selection)
	{
		if (index.isValid())
		{
			CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, m_nameColumn);

			if ((pAsset != nullptr) && pAsset->IsDefaultControl())
			{
				isDefaultControlSelected = true;
				break;
			}
		}
	}

	return isDefaultControlSelected;
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

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::SelectConnectedSystemControl(CSystemControl& systemControl, CID const itemId)
{
	ClearFilters();
	auto const& matches = m_pSystemFilterProxyModel->match(m_pSystemFilterProxyModel->index(0, 0, QModelIndex()), static_cast<int>(SystemModelUtils::ERoles::Id), systemControl.GetId(), 1, Qt::MatchRecursive);

	if (!matches.isEmpty())
	{
		std::vector<CID> selectedConnection {
			itemId
		};
		systemControl.SetSelectedConnections(selectedConnection);
		m_pTreeView->setFocus();
		m_pTreeView->selectionModel()->setCurrentIndex(matches.first(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::ClearFilters()
{
	m_pFilteringPanel->GetSearchBox()->clear();
	m_pFilteringPanel->Clear();
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsWidget::DeleteModels()
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
