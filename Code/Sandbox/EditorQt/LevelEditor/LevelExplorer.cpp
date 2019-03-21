// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>

#include "LevelExplorer.h"
#include "IEditorImpl.h"

#include "LevelEditor/LevelEditorViewport.h"
#include "LevelEditor/LevelExplorerCommandHelper.h"
#include "LevelEditor/LevelLayerModel.h"
#include "LevelEditor/LevelModel.h"
#include "LevelEditor/LevelModelsManager.h"
#include "Objects/Group.h"
#include "Objects/ObjectLayer.h"
#include "Objects/ObjectLayerManager.h"
#include "Objects/ObjectManager.h"
#include "QT/QtMainFrame.h"
#include "CryEdit.h"

#include "VersionControl/VersionControlEventHandler.h"

#include <EditorFramework/Events.h>
#include <Commands/QCommandAction.h>
#include <Controls/DynamicPopupMenu.h>
#include <Controls/QMenuComboBox.h>
#include <Controls/QuestionDialog.h>
#include <FileDialogs/SystemFileDialog.h>
#include <Menu/MenuWidgetBuilders.h>
#include <ProxyModels/AttributeFilterProxyModel.h>
#include <ProxyModels/MergingProxyModel.h>
#include <PathUtils.h>
#include <MenuHelpers.h>
#include <QAdvancedItemDelegate.h>
#include <QAdvancedTreeView.h>
#include <QControls.h>
#include <QFilteringPanel.h>
#include <QSearchBox.h>

#include <CryIcon.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QDir>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QProcess>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QVBoxLayout>

REGISTER_VIEWPANE_FACTORY_AND_MENU(CLevelExplorer, "Level Explorer", "Tools", false, "Level Editor")

namespace Private_LevelExplorer
{
class Delegate : public QAdvancedItemDelegate
{
public:
	Delegate(QAdvancedTreeView* pParent = nullptr) : QAdvancedItemDelegate(pParent) {}

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		if (index.isValid())
		{
			QVariant attributeVar = index.model()->headerData(index.column(), Qt::Horizontal, Attributes::s_getAttributeRole);
			if (attributeVar.isValid())
			{
				CItemModelAttribute* attribute = attributeVar.value<CItemModelAttribute*>();
				if (attribute == &LevelModelsAttributes::s_PlatformAttribute)
				{
					auto combo = new QMenuComboBox(parent);
					combo->SetMultiSelect(true);
					combo->AddItem("PC");
					combo->AddItem("XBox One");
					combo->AddItem("PS4");
					combo->SetText(index.data().toString());
					return combo;
				}
			}
		}

		return QStyledItemDelegate::createEditor(parent, option, index);
	}
};

struct SColorPreset
{
	string name;
	string icon;
	ColorB value;
};

const std::vector<SColorPreset> _colorPresets =
{
	{ "Blue",   "icons:General/colour_blue.ico",   { 77,  155, 214 } },
	{ "Green",  "icons:General/colour_green.ico",  { 125, 209, 77  } },
	{ "Orange", "icons:General/colour_orange.ico", { 214, 155, 77  } },
	{ "Purple", "icons:General/colour_purple.ico", { 144, 92,  235 } },
	{ "Red",    "icons:General/colour_red.ico",    { 210, 86,  86  } },
	{ "Yellow", "icons:General/colour_yellow.ico", { 225, 225, 80  } } };

ColorB AskForColor(const ColorB& color, QWidget* pParent)
{
	QColor qtColor(color.r, color.g, color.b);
	QColor selectedColor = QColorDialog::getColor(qtColor, pParent, "Choose color");
	if (selectedColor.isValid())
	{
		int r;
		int g;
		int b;
		selectedColor.getRgb(&r, &g, &b);
		ColorB finalColor
		{
			static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b) };
		return finalColor;
	}
	return ColorB(0, 0, 0);
}

void AddColorSubMenuForLayers(CAbstractMenu& menu, const std::vector<CObjectLayer*>& layers)
{
	int actionsSection = menu.GetNextEmptySection();

	CAbstractMenu* pColorMenu = menu.CreateMenu(QObject::tr("Color"), actionsSection);

	auto action = pColorMenu->CreateAction("None", actionsSection);
	QObject::connect(action, &QAction::triggered, [&layers]()
		{
			for (CObjectLayer* layer : layers)
			{
			  layer->UseColorOverride(false);
			}
		});

	for (const SColorPreset& presets : _colorPresets)
	{
		action = pColorMenu->CreateAction(QIcon(QtUtil::ToQString(presets.icon)), QtUtil::ToQString(presets.name), actionsSection);
		QObject::connect(action, &QAction::triggered, [&presets, &layers]()
			{
				for (CObjectLayer* layer : layers)
				{
				  layer->SetColor(presets.value, true);
				}
			});
	}

	action = pColorMenu->CreateAction(QIcon("icons:General/colour_other.ico"), "Other...", actionsSection);
	QObject::connect(action, &QAction::triggered, [&layers]()
		{
			ColorB currentColor = layers[0]->GetColor();
			ColorB finalColor = AskForColor(currentColor, nullptr);
			for (CObjectLayer* layer : layers)
			{
			  layer->SetColor(finalColor, true);
			}
		});
}

std::vector<IObjectLayer*> ToIObjectLayers(std::vector<CObjectLayer*> layers)
{
	std::vector<IObjectLayer*> iObjLayers;
	iObjLayers.reserve(layers.size());
	for (auto pLayer : layers)
	{
		iObjLayers.push_back(pLayer);
	}
	return iObjLayers;
}

}

CLevelExplorer::CLevelExplorer(QWidget* pParent)
	: CDockableEditor(pParent)
	, m_pAttributeFilterProxyModel(new QAttributeFilterProxyModel(QAttributeFilterProxyModel::AcceptIfChildMatches, this))
	, m_modelType(Objects)
	, m_syncSelection(false)
	, m_ignoreSelectionEvents(false)
	, m_treeView(nullptr)
{
	auto pCommandManager = GetIEditor()->GetICommandManager();
	m_pShowActiveLayer = pCommandManager->CreateNewAction("level_explorer.show_active_layer_contents");
	m_pShowAllObjects = pCommandManager->CreateNewAction("level_explorer.show_all_objects");
	m_pShowFullHierarchy = pCommandManager->CreateNewAction("level_explorer.show_full_hierarchy");
	m_pShowLayers = pCommandManager->CreateNewAction("level_explorer.show_layers");
	m_pSyncSelection = pCommandManager->CreateNewAction("level_explorer.sync_selection");

	SetModelType(FullHierarchy);

	m_treeView = new QAdvancedTreeView(QAdvancedTreeView::UseItemModelAttribute);
	m_treeView->setModel(m_pAttributeFilterProxyModel);
	m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_treeView->setEditTriggers(QTreeView::EditKeyPressed | QTreeView::SelectedClicked); //Not using double clicked as edit trigger because it is more likely to be used for set active layer/go to object
	m_treeView->setTreePosition((int)eLayerColumns_Name);
	m_treeView->setUniformRowHeights(true);
	m_treeView->setDragEnabled(true);
	m_treeView->setDragDropMode(QAbstractItemView::DragDrop);
	m_treeView->header()->setMinimumSectionSize(10);

	m_pAttributeFilterProxyModel->setFilterKeyColumn((int)eLayerColumns_Name);
	m_treeView->sortByColumn((int)eLayerColumns_Name, Qt::AscendingOrder);

	// Disable expanding on double click since we have other functionality in the level explorer that executes when double clicking.
	// When double clicking an object we want to go to that object. Double clicking a layer will set it as the current layer
	m_treeView->setExpandsOnDoubleClick(false);

	connect(m_treeView, &QTreeView::customContextMenuRequested, this, &CLevelExplorer::OnContextMenu);
	connect(m_treeView, &QAdvancedTreeView::doubleClicked, this, &CLevelExplorer::OnDoubleClick);
	connect(m_treeView, &QTreeView::clicked, this, &CLevelExplorer::OnClick);
	connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CLevelExplorer::OnSelectionChanged);

	auto delegate = new Private_LevelExplorer::Delegate(m_treeView);
	delegate->SetColumnBehavior((int)eLayerColumns_Visible, QAdvancedItemDelegate::BehaviorFlags(QAdvancedItemDelegate::DragCheck | QAdvancedItemDelegate::OverrideCheckIcon));
	delegate->SetColumnBehavior((int)eLayerColumns_Frozen, QAdvancedItemDelegate::BehaviorFlags(QAdvancedItemDelegate::DragCheck | QAdvancedItemDelegate::OverrideCheckIcon));
	delegate->SetColumnBehavior((int)eLayerColumns_Color, QAdvancedItemDelegate::BehaviorFlags(QAdvancedItemDelegate::DragCheck | QAdvancedItemDelegate::OverrideCheckIcon | QAdvancedItemDelegate::OverrideDrawRect));
	m_treeView->setItemDelegate(delegate);

	m_treeView->signalVisibleSectionCountChanged.Connect(this, &CLevelExplorer::OnHeaderSectionCountChanged);

	// trigger a column refresh to set sizes for checkbox-like columns (to avoid displaying the "visible"/"frozen" columns unsized)
	// this occurs when the Level Explorer is first displayed but there's no level loaded, as there's no signal/virtual function to hook to
	// to get an item delegate change (which we could actually trigger from inside the QAdvancedTreeView class, obviating the need for this call)
	m_treeView->TriggerRefreshHeaderColumns();
	// trigger our own custom sizes for columns
	OnHeaderSectionCountChanged();

	m_pShortcutBarLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	m_pShortcutBarLayout->setMargin(0);
	m_pShortcutBarLayout->setSpacing(4);

	m_pShortcutBarLayout->addWidget(CreateToolButton(m_pShowFullHierarchy));
	m_pShortcutBarLayout->addWidget(CreateToolButton(m_pShowLayers));
	m_pShortcutBarLayout->addWidget(CreateToolButton(m_pShowAllObjects));
	m_pShortcutBarLayout->addWidget(CreateToolButton(m_pShowActiveLayer));
	m_pShortcutBarLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	m_pShortcutBarLayout->addWidget(CreateToolButton(m_pSyncSelection));

	m_pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pMainLayout->addWidget(m_treeView);
	m_pMainLayout->addLayout(m_pShortcutBarLayout);
	m_pMainLayout->setMargin(0);
	m_pMainLayout->setSpacing(0);

	m_filterPanel = new QFilteringPanel("LevelExplorer", m_pAttributeFilterProxyModel);
	QWidget* pContent = new QWidget();
	pContent->setLayout(m_pMainLayout);
	m_filterPanel->SetContent(pContent);
	m_filterPanel->GetSearchBox()->SetAutoExpandOnSearch(m_treeView);

	InitMenuBar();

	SetContent(m_filterPanel);

	CObjectManager* pObjManager = static_cast<CObjectManager*>(GetIEditorImpl()->GetObjectManager());
	pObjManager->GetLayersManager()->signalChangeEvent.Connect(this, &CLevelExplorer::OnLayerChange);
	pObjManager->signalObjectsChanged.Connect(this, &CLevelExplorer::OnObjectsChanged);
	pObjManager->signalSelectionChanged.Connect(this, &CLevelExplorer::OnViewportSelectionChanged);

	CLevelModelsManager::GetInstance().signalLayerModelsUpdated.Connect(this, &CLevelExplorer::OnLayerModelsUpdated);

	CLevelModelsManager::GetInstance().signalLayerModelsUpdated.Connect(this, &CLevelExplorer::OnLayerModelsUpdated);

	//Register to any LevelLayerModel reset event, this will be used to stop selection reset
	CLevelModelsManager::GetInstance().signalLayerModelResetBegin.Connect(this, &CLevelExplorer::OnLayerModelResetBegin);
	CLevelModelsManager::GetInstance().signalLayerModelResetEnd.Connect(this, &CLevelExplorer::OnLayerModelResetEnd);

	InstallReleaseMouseFilter(this);
}

CLevelExplorer::~CLevelExplorer()
{
	CObjectManager* pObjManager = static_cast<CObjectManager*>(GetIEditorImpl()->GetObjectManager());
	pObjManager->GetLayersManager()->signalChangeEvent.DisconnectObject(this);
	pObjManager->signalObjectsChanged.DisconnectObject(this);
	pObjManager->signalSelectionChanged.DisconnectObject(this);

	CLevelModelsManager::GetInstance().signalLayerModelsUpdated.DisconnectObject(this);
}

QToolButton* CLevelExplorer::CreateToolButton(QCommandAction* pAction)
{
	QToolButton* pToolButton = new QToolButton(this);
	pToolButton->setDefaultAction(pAction);
	return pToolButton;
}

void CLevelExplorer::InitMenuBar()
{
	//File
	AddToMenu({ CEditor::MenuItems::FileMenu, MenuItems::New, MenuItems::New_Folder });
	CAbstractMenu* const fileMenu = GetMenu(CEditor::MenuItems::FileMenu);

	// Get 'new' action so we can provide additional information relevant to current context
	m_pNewLayer = GetMenuAction(MenuItems::New);
	m_pNewLayer->setText("New Layer");

	int section = fileMenu->GetNextEmptySection();

	fileMenu->CreateCommandAction("general.import", section);

	//Edit
	AddToMenu({ CEditor::MenuItems::EditMenu, CEditor::MenuItems::Delete, CEditor::MenuItems::Find });
	CAbstractMenu* pEditMenu = GetMenu(CEditor::MenuItems::EditMenu);

	// Get 'delete' action so we can change it's state based on selection changes
	m_pDelete = GetMenuAction(CEditor::MenuItems::Delete);

	m_pRename = pEditMenu->CreateCommandAction("general.rename");

	section = pEditMenu->GetNextEmptySection();
	m_pToggleVisibility = pEditMenu->CreateCommandAction("general.toggle_visibility", section);
	m_pIsolateVisibility = pEditMenu->CreateCommandAction("general.isolate_visibility", section);

	CAbstractMenu* pMoreVisibility = pEditMenu->CreateMenu("More", section);
	section = pMoreVisibility->GetNextEmptySection();
	pMoreVisibility->CreateCommandAction("general.hide_all", section);
	pMoreVisibility->CreateCommandAction("general.unhide_all", section);

	section = pMoreVisibility->GetNextEmptySection();
	m_pToggleChildrenVisibility = pMoreVisibility->CreateCommandAction("general.toggle_children_visibility", section);
	pMoreVisibility->CreateCommandAction("general.hide_children", section);
	pMoreVisibility->CreateCommandAction("general.unhide_children", section);

	section = pEditMenu->GetNextEmptySection();
	m_pToggleLocking = pEditMenu->CreateCommandAction("general.toggle_lock", section);
	m_pIsolateLocked = pEditMenu->CreateCommandAction("general.isolate_locked", section);

	CAbstractMenu* pMoreLock = pEditMenu->CreateMenu("More", section);
	section = pMoreLock->GetNextEmptySection();
	pMoreLock->CreateCommandAction("general.lock_all", section);
	pMoreLock->CreateCommandAction("general.unlock_all", section);

	section = pMoreLock->GetNextEmptySection();
	m_pToggleChildrenLocking = pMoreLock->CreateCommandAction("general.toggle_children_locking", section);
	pMoreLock->CreateCommandAction("general.lock_children", section);
	pMoreLock->CreateCommandAction("general.unlock_children", section);

	section = pEditMenu->GetNextEmptySection();
	pEditMenu->CreateCommandAction("path_utils.show_in_file_explorer", section);
	pEditMenu->CreateCommandAction("path_utils.copy_name", section);
	pEditMenu->CreateCommandAction("path_utils.copy_path", section);

	// Layer
	CAbstractMenu* pLayerMenu =  GetRootMenu()->CreateMenu("Layer");
	pLayerMenu->CreateCommandAction("layer.make_active");
	pLayerMenu->CreateCommandAction("layer.lock_read_only_layers");

	CAbstractMenu* pMoreMenu = pLayerMenu->CreateMenu("More");
	pMoreMenu->CreateCommandAction("general.reload");

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Exportable");
	m_pToggleExportable = pMoreMenu->CreateCommandAction("layer.toggle_exportable", section);
	m_pToggleExportableToPak = pMoreMenu->CreateCommandAction("layer.toggle_exportable_to_pak", section);

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Auto-Load");
	m_pToggleAutoLoaded = pMoreMenu->CreateCommandAction("layer.toggle_auto_load", section);

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Physics");
	m_pTogglePhysics = pMoreMenu->CreateCommandAction("layer.toggle_physics", section);

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Platforms");
	m_pTogglePC = pMoreMenu->CreateCommandAction("layer.toggle_pc", section);
	m_pToggleXBoxOne = pMoreMenu->CreateCommandAction("layer.toggle_xbox_one", section);
	m_pTogglePS4 = pMoreMenu->CreateCommandAction("layer.toggle_ps4", section);

	//View
	AddToMenu(CEditor::MenuItems::ViewMenu);
	CAbstractMenu* const menuView = GetMenu(CEditor::MenuItems::ViewMenu);

	section = menuView->GetNextEmptySection();

	menuView->AddAction(m_pShowFullHierarchy, section);
	menuView->AddAction(m_pShowLayers, section);
	menuView->AddAction(m_pShowAllObjects, section);
	menuView->AddAction(m_pShowActiveLayer, section);

	section = menuView->GetNextEmptySection();

	menuView->CreateCommandAction("general.expand_all", section);
	menuView->CreateCommandAction("general.collapse_all", section);

	section = menuView->GetNextEmptySection();

	menuView->AddAction(m_pSyncSelection, section);

	if (m_filterPanel)
	{
		m_filterPanel->CreateMenu(menuView);
	}

	GetRootMenu()->CreateCommandAction("level_explorer.focus_on_active_layer", 0);

	SetActionsEnabled(false);
}

void CLevelExplorer::SetLayout(const QVariantMap& state)
{
	CDockableEditor::SetLayout(state);

	QVariant modelVar = state.value("model");
	if (modelVar.isValid())
	{
		SetModelType((ModelType)modelVar.toInt());
	}

	QVariant syncSelVar = state.value("syncSelection");
	if (syncSelVar.isValid())
	{
		SetSyncSelection(syncSelVar.toBool());
	}

	QVariant filtersStateVar = state.value("filters");
	if (filtersStateVar.isValid() && filtersStateVar.type() == QVariant::Map)
	{
		m_filterPanel->SetState(filtersStateVar.value<QVariantMap>());
	}

	QVariant treeViewStateVar = state.value("treeView");
	if (treeViewStateVar.isValid() && treeViewStateVar.type() == QVariant::Map)
	{
		m_treeView->SetState(treeViewStateVar.value<QVariantMap>());
	}
}

QVariantMap CLevelExplorer::GetLayout() const
{
	QVariantMap state = CDockableEditor::GetLayout();
	state.insert("model", (int)m_modelType);
	state.insert("syncSelection", m_syncSelection);
	state.insert("filters", m_filterPanel->GetState());
	state.insert("treeView", m_treeView->GetState());
	return state;
}

void CLevelExplorer::customEvent(QEvent* pEvent)
{
	using namespace Private_LevelExplorer;

	CDockableEditor::customEvent(pEvent);

	if (pEvent->isAccepted() || pEvent->type() != SandboxEvent::Command)
	{
		return;
	}

	QStringList params = QtUtil::ToQString(static_cast<CommandEvent*>(pEvent)->GetCommand()).split(' ');

	if (params.empty())
		return;

	QString command = params[0];
	params.removeFirst();

	QStringList fullCommand = command.split('.');
	QString module = fullCommand[0];
	command = fullCommand[1];

	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;
	std::vector<CObjectLayer*> layerFolders;

	QModelIndexList selection = m_treeView->selectionModel()->selectedRows();

	LevelModelsUtil::GetObjectsAndLayersForIndexList(selection, objects, layers, layerFolders);

	std::vector<CObjectLayer*> allLayers;
	allLayers.reserve(layers.size() + layerFolders.size());
	allLayers.insert(allLayers.end(), layers.cbegin(), layers.cend());
	allLayers.insert(allLayers.end(), layerFolders.cbegin(), layerFolders.cend());

	if (module == "level_explorer")
	{
		pEvent->setAccepted(true);

		if (command == "focus_on_active_layer")
		{
			FocusActiveLayer();
		}
		else if (command == "show_full_hierarchy")
		{
			SetModelType(FullHierarchy);
		}
		else if (command == "show_layers")
		{
			SetModelType(Layers);
		}
		else if (command == "show_all_objects")
		{
			SetModelType(Objects);
		}
		else if (command == "show_active_layer_contents")
		{
			SetModelType(ActiveLayer);
		}
		else if (command == "sync_selection")
		{
			SetSyncSelection(!m_syncSelection);
		}
		else
		{
			pEvent->setAccepted(false);
		}
	}
	else if (module == "version_control_system")
	{
		VersionControlEventHandler::HandleOnLevelExplorer(command, ToIObjectLayers(layers), ToIObjectLayers(layerFolders));
	}
	else if (module == "layer")
	{
		pEvent->setAccepted(true);

		if (command == "make_active")
		{
			if (!layers.empty())
			{
				GetIEditorImpl()->GetObjectManager()->GetLayersManager()->SetCurrentLayer(layers.back());
			}
		}
		else if (command == "toggle_exportable")
		{
			LevelExplorerCommandHelper::ToggleExportable(layers);
		}
		else if (command == "toggle_exportable_to_pak")
		{
			LevelExplorerCommandHelper::ToggleExportableToPak(layers);
		}
		else if (command == "toggle_auto_load")
		{
			LevelExplorerCommandHelper::ToggleAutoLoad(layers);
		}
		else if (command == "toggle_physics")
		{
			LevelExplorerCommandHelper::TogglePhysics(layers);
		}
		else if (command == "toggle_pc")
		{
			LevelExplorerCommandHelper::TogglePlatformSpecs(layers, eSpecType_PC);
		}
		else if (command == "toggle_xbox_one")
		{
			LevelExplorerCommandHelper::TogglePlatformSpecs(layers, eSpecType_XBoxOne);
		}
		else if (command == "toggle_ps4")
		{
			LevelExplorerCommandHelper::TogglePlatformSpecs(layers, eSpecType_PS4);
		}
		else
		{
			pEvent->setAccepted(false);
		}
	}
	else if (module == "path_utils")
	{
		pEvent->setAccepted(true);

		if (command == "copy_name")
		{
			string clipBoardText;
			for (const CObjectLayer* pLayer : layers)
			{
				clipBoardText.Format("%s%s%s", clipBoardText, PathUtil::GetFile(pLayer->GetLayerFilepath().c_str()), "\n");
			}
			QApplication::clipboard()->setText(clipBoardText.c_str());
		}
		else if (command == "copy_path")
		{
			string clipBoardText;
			for (const CObjectLayer* pLayer : layers)
			{
				clipBoardText.Format("%s%s%s", clipBoardText, pLayer->GetLayerFilepath(), "\n");
			}
			QApplication::clipboard()->setText(clipBoardText.c_str());
		}
		else if (command == "show_in_file_explorer")
		{
			for (const CObjectLayer* pLayer : layers)
			{
				QtUtil::OpenInExplorer(pLayer->GetLayerFilepath().c_str());
			}
		}
		else
		{
			pEvent->setAccepted(false);
		}
	}
	else
	{
		CDockableEditor::customEvent(pEvent);
	}
}

void CLevelExplorer::resizeEvent(QResizeEvent* event)
{
	m_pShortcutBarLayout->setDirection(width() > height() ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
	m_pMainLayout->setDirection(width() > height() ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
}

void CLevelExplorer::OnContextMenu(const QPoint& pos) const
{
	using namespace Private_LevelExplorer;
	CAbstractMenu abstractMenu;

	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;
	std::vector<CObjectLayer*> layerFolders;

	auto selection = m_treeView->selectionModel()->selectedRows();

	LevelModelsUtil::GetObjectsAndLayersForIndexList(selection, objects, layers, layerFolders);
	int section = 0;

	if (IsModelTypeShowingLayers())
	{
		section = abstractMenu.GetNextEmptySection();
		abstractMenu.SetSectionName(section, "Create");
		abstractMenu.AddCommandAction(m_pNewLayer, section);
		abstractMenu.CreateCommandAction("general.new_folder", section);
		abstractMenu.CreateCommandAction("general.import", section);
	}

	if (!selection.isEmpty())
	{
		section = abstractMenu.GetNextEmptySection();
		abstractMenu.SetSectionName(section, "Edit");
		abstractMenu.CreateCommandAction("general.rename", section);
		abstractMenu.CreateCommandAction("general.delete", section);
	}

	const bool hasNormalLayers = layers.size() > 1 || (layers.size() == 1 && layers[0]->GetLayerType() != eObjectLayerType_Terrain);

	if (hasNormalLayers)
	{
		CreateContextMenuForLayers(abstractMenu, layers);
	}

	if (!selection.isEmpty())
	{
		section = abstractMenu.GetNextEmptySection();
		abstractMenu.SetSectionName(section, "Visibility");
		abstractMenu.AddCommandAction(m_pToggleVisibility, section);
		abstractMenu.CreateCommandAction("general.isolate_visibility", section);

		section = abstractMenu.GetNextEmptySection();
		abstractMenu.SetSectionName(section, "Lock");
		abstractMenu.AddCommandAction(m_pToggleLocking, section);
		abstractMenu.CreateCommandAction("general.isolate_locked", section);
	}

	if (layers.size() || layerFolders.size())
	{
		PopulateExistingSectionsForLayers(abstractMenu);

		AddColorSubMenuForLayers(abstractMenu, layers);

		section = abstractMenu.GetNextEmptySection();
		abstractMenu.SetSectionName(section, "Path Utils");
		abstractMenu.CreateCommandAction("path_utils.show_in_file_explorer", section);
		abstractMenu.CreateCommandAction("path_utils.copy_name", section);
		abstractMenu.CreateCommandAction("path_utils.copy_path", section);
	}

	std::vector<IObjectLayer*> iLayers;
	std::vector<IObjectLayer*> iLayerFolders;
	iLayers.reserve(layers.size());
	iLayerFolders.reserve(layerFolders.size());
	std::move(layers.begin(), layers.end(), std::back_inserter(iLayers));
	std::move(layerFolders.begin(), layerFolders.end(), std::back_inserter(iLayerFolders));
	GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->signalContextMenuRequested(abstractMenu, objects, iLayers, iLayerFolders);

	section = abstractMenu.GetNextEmptySection();
	abstractMenu.SetSectionName(section, "View");
	abstractMenu.CreateCommandAction("general.expand_all", section);
	abstractMenu.CreateCommandAction("general.collapse_all", section);

	if (selection.isEmpty())
	{
		section = abstractMenu.GetNextEmptySection();
		abstractMenu.AddAction(m_pShowFullHierarchy, section);
		abstractMenu.AddAction(m_pShowLayers, section);
		abstractMenu.AddAction(m_pShowAllObjects, section);
		abstractMenu.AddAction(m_pShowActiveLayer, section);
		section = abstractMenu.GetNextEmptySection();
		abstractMenu.AddAction(m_pSyncSelection, section);
	}


	QMenu menu;
	abstractMenu.Build(MenuWidgetBuilders::CMenuBuilder(&menu));

	if (objects.size() == 1)//TODO : support menu for multiple selection of objects !
	{
		CDynamicPopupMenu menuVisitor;
		objects[0]->OnContextMenu(&menuVisitor.GetRoot());

		CPopupMenuItem& colorMenu = menuVisitor.GetRoot().Add("Color");
		colorMenu.Add("None", [&objects]()
		{
			objects[0]->UseColorOverride(false);
		});

		for (const SColorPreset& preset : _colorPresets)
		{
			colorMenu.Add(preset.name, preset.icon, [&objects, &preset]()
			{
				objects[0]->ChangeColor(preset.value);
			});
		}

		colorMenu.Add("Other...", "icons:General/colour_other.ico", [&objects, this]()
		{
			ColorB currentColor = objects[0]->GetColor();
			ColorB finalColor = AskForColor(currentColor, nullptr);
			objects[0]->ChangeColor(finalColor);
		});

		if (!menuVisitor.IsEmpty())
		{
			menu.addAction(new QMenuLabelSeparator("Objects"));
			menuVisitor.PopulateQMenu(&menu);
		}
	}

	//executes the menu synchronously so we can keep references to the arrays scoped to this method in lambdas
	menu.exec(m_treeView->mapToGlobal(pos));
}

void CLevelExplorer::PopulateExistingSectionsForLayers(CAbstractMenu& abstractMenu) const
{
	// Visibility
	int section = abstractMenu.FindSectionByName("Visibility");
	CAbstractMenu* pMoreVisibility = abstractMenu.CreateMenu("More", section);
	section = pMoreVisibility->GetNextEmptySection();
	pMoreVisibility->CreateCommandAction("general.hide_all", section);
	pMoreVisibility->CreateCommandAction("general.unhide_all", section);

	section = pMoreVisibility->GetNextEmptySection();
	pMoreVisibility->CreateCommandAction("general.toggle_children_visibility", section);
	pMoreVisibility->CreateCommandAction("general.hide_children", section);
	pMoreVisibility->CreateCommandAction("general.unhide_children", section);

	// Lock
	section = abstractMenu.FindSectionByName("Lock");
	CAbstractMenu* pMoreLock = abstractMenu.CreateMenu("More", section);
	section = pMoreLock->GetNextEmptySection();
	pMoreLock->CreateCommandAction("general.lock_all", section);
	pMoreLock->CreateCommandAction("general.unlock_all", section);
	pMoreLock->CreateCommandAction("layer.lock_read_only_layers", section);

	section = pMoreLock->GetNextEmptySection();
	pMoreLock->CreateCommandAction("general.toggle_children_locking", section);
	pMoreLock->CreateCommandAction("general.lock_children", section);
	pMoreLock->CreateCommandAction("general.unlock_children", section);
}

void CLevelExplorer::CreateContextMenuForLayers(CAbstractMenu& abstractMenu, const std::vector<CObjectLayer*>& layers) const
{
	using namespace Private_LevelExplorer;
	int section = abstractMenu.GetNextEmptySection();
	abstractMenu.SetSectionName(section, "Layer");

	abstractMenu.CreateCommandAction("layer.make_active", section);
	abstractMenu.CreateCommandAction("layer.lock_read_only_layers", section);

	CAbstractMenu* pMoreMenu = abstractMenu.CreateMenu("More");
	pMoreMenu->CreateCommandAction("general.reload");

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Exportable");
	pMoreMenu->AddCommandAction(m_pToggleExportable, section);
	pMoreMenu->AddCommandAction(m_pToggleExportableToPak, section);

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Auto-Load");
	pMoreMenu->AddCommandAction(m_pToggleAutoLoaded, section);

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Physics");
	pMoreMenu->AddCommandAction(m_pTogglePhysics, section);

	section = pMoreMenu->GetNextEmptySection();
	pMoreMenu->SetSectionName(section, "Platforms");
	pMoreMenu->AddCommandAction(m_pTogglePC, section);
	pMoreMenu->AddCommandAction(m_pToggleXBoxOne, section);
	pMoreMenu->AddCommandAction(m_pTogglePS4, section);
}

void CLevelExplorer::OnClick(const QModelIndex& index)
{
	if (index.isValid())
	{
		Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
		switch (modifier)
		{
		case Qt::KeyboardModifier::AltModifier:
			{
				auto type = index.data((int)CLevelModel::Roles::TypeCheckRole);
				if (type.toInt() == ELevelElementType::eLevelElement_Layer)
				{
					CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
					if (pLayer && (pLayer->GetLayerType() == eObjectLayerType_Layer || pLayer->GetLayerType() == eObjectLayerType_Folder))
					{
						if (index.column() == ELayerColumns::eLayerColumns_Visible) // visibility
						{
							OnUnhideAllInLayer(pLayer);
						}
						else if (index.column() == ELayerColumns::eLayerColumns_Frozen) // freezing
						{
							OnUnlockAllInLayer(pLayer);
						}
						m_treeView->update(); // push an update/redraw here
					}
				}
			}
			break;
		case Qt::KeyboardModifier::ControlModifier:
			{
				if (index.column() == ELayerColumns::eLayerColumns_Visible) // visibility
				{
					IsolateVisibility(index);
				}
				else if (index.column() == ELayerColumns::eLayerColumns_Frozen) // freezing
				{
					IsolateLocked(index);
				}
				break;
			}
		default:
			break;
		}
	}
}

void CLevelExplorer::OnDoubleClick(const QModelIndex& index)
{
	if (index.isValid())
	{
		auto type = index.data((int)CLevelModel::Roles::TypeCheckRole);
		switch (type.toInt())
		{
		case ELevelElementType::eLevelElement_Layer:
			{
				CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
				if (pLayer && pLayer->GetLayerType() == eObjectLayerType_Layer)
				{
					GetIEditorImpl()->GetObjectManager()->GetLayersManager()->SetCurrentLayer(pLayer);
				}
			}
			break;
		case ELevelElementType::eLevelElement_Object:
			{
				CBaseObject* pObject = reinterpret_cast<CBaseObject*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
				if (pObject)
				{
					if (index.column() != ELayerColumns::eLayerColumns_Frozen) // freezing
					{
						GetIEditorImpl()->GetObjectManager()->SelectObject(pObject);
						CViewport* vp = GetIEditorImpl()->GetActiveView();
						if (vp)
						{
							AABB aabb;
							pObject->GetBoundBox(aabb);
							vp->CenterOnAABB(&aabb);
						}
					}
				}
			}
			break;
		default:
			break;
		}
	}
}

CObjectLayer* CLevelExplorer::GetParentLayerForIndexList(const QModelIndexList& indexList) const
{
	if (indexList.isEmpty())
		return nullptr;

	QModelIndex lastSelectedItem = indexList.last();
	QVariant type = indexList.last().data(static_cast<int>(CLevelModel::Roles::TypeCheckRole));

	if (type.toInt() != ELevelElementType::eLevelElement_Layer)
		return nullptr;

	CObjectLayer* pLayer = static_cast<CObjectLayer*>(lastSelectedItem.internalPointer());
	if (pLayer->GetLayerType() == eObjectLayerType_Folder)
	{
		return pLayer;
	}

	return nullptr;
}

CObjectLayer* CLevelExplorer::CreateLayer(EObjectLayerType layerType)
{
	CUndo undo(layerType == eObjectLayerType_Layer ? "Create Layer" : "Create Folder");

	CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();
	CObjectLayer* pLayer = pLayerManager->CreateLayer(layerType, GetParentLayerForIndexList(m_treeView->selectionModel()->selectedRows()));

	if (!pLayer)
	{
		undo.Cancel();
		return pLayer;
	}

	EditLayer(pLayer);

	return pLayer;
}

bool CLevelExplorer::OnNew()
{
	return CreateLayer(eObjectLayerType_Layer) != nullptr;
}

bool CLevelExplorer::OnNewFolder()
{
	return CreateLayer(eObjectLayerType_Folder) != nullptr;
}

bool CLevelExplorer::OnImport()
{
	CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();
	CObjectLayer* pTargetLayer = GetParentLayerForIndexList(m_treeView->selectionModel()->selectedRows());

	CSystemFileDialog::OpenParams openParams(CSystemFileDialog::OpenMultipleFiles);
	openParams.title = QObject::tr("Import Layer");
	openParams.extensionFilters << CExtensionFilter(QObject::tr("Layer Files (lyr)"), "lyr");

	CSystemFileDialog dialog(openParams, const_cast<CLevelExplorer*>(this));
	if (!dialog.Execute())
		return false;

	std::vector<QString> files = dialog.GetSelectedFiles();
	if (files.size() > 0)
	{
		for (auto& file : files)
		{
			auto pNewLayer = pLayerManager->ImportLayerFromFile(QtUtil::ToString(file));
			if (pTargetLayer && pNewLayer)
				pTargetLayer->AddChild(pNewLayer);
		}
	}

	return true;
}

bool CLevelExplorer::OnReload()
{
	std::vector<CObjectLayer*> layers;
	LevelModelsUtil::GetObjectLayersForIndexList(m_treeView->selectionModel()->selectedRows(), layers);

	bool isModified = false;
	for (const CObjectLayer* pLayer : layers)
	{
		if (pLayer->IsModified())
		{
			isModified = true;
			break;
		}
	}

	if (isModified)
	{
		QString message = tr("Some of the selected layers have been modified. Your changes will be lost. Reload selected layers anyway?");
		if (QDialogButtonBox::StandardButton::Yes != CQuestionDialog::SQuestion(tr("Reload layers"), message))
		{
			// Even if we didn't import, we technically completed the action. So make sure to return true
			return true;
		}
	}

	for (CObjectLayer* pLayer : layers)
	{
		GetIEditorImpl()->GetObjectManager()->GetLayersManager()->ReloadLayer(pLayer);
	}

	return true;
}

bool CLevelExplorer::OnFind()
{
	m_filterPanel->GetSearchBox()->setFocus();
	return true;
}

bool CLevelExplorer::OnDelete()
{
	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;

	auto selection = m_treeView->selectionModel()->selectedRows();

	objects.reserve(selection.count());
	layers.reserve(selection.count());

	for (auto& index : selection)
	{
		QVariant type = index.data((int)CLevelModel::Roles::TypeCheckRole);
		QVariant internalPtrVar = index.data((int)CLevelModel::Roles::InternalPointerRole);
		switch (type.toInt())
		{
		case ELevelElementType::eLevelElement_Layer:
			{
				layers.push_back(reinterpret_cast<CObjectLayer*>(internalPtrVar.value<intptr_t>()));
			}
			break;
		case ELevelElementType::eLevelElement_Object:
			{
				objects.push_back(reinterpret_cast<CBaseObject*>(internalPtrVar.value<intptr_t>()));
			}
			break;
		default:
			break;
		}
	}

	QString message = tr("Are you sure you want to delete the selected layers and objects ?");
	if (layers.empty() || QDialogButtonBox::StandardButton::Yes == CQuestionDialog::SQuestion(tr("Delete layers"), message))
	{
		//TODO : optimize by deleting layers first and objects second
		GetIEditorImpl()->GetIUndoManager()->Begin();

		if (!objects.empty())
		{
			GetIEditorImpl()->GetObjectManager()->DeleteObjects(objects);
		}

		if (!layers.empty())
		{
			for (auto& layer : layers)
			{
				GetIEditorImpl()->GetObjectManager()->GetLayersManager()->DeleteLayer(layer);
			}
		}

		GetIEditorImpl()->GetIUndoManager()->Accept("Delete Selection");
	}
	return true;
}

bool CLevelExplorer::OnRename()
{
	QModelIndexList selection = m_treeView->selectionModel()->selectedRows();

	if (selection.isEmpty())
		return false;

	// If there's at least one selected item, then rename the current index (last selected)
	QModelIndex index = m_treeView->selectionModel()->currentIndex();
	m_treeView->edit(index.sibling(index.row(), (int)eLayerColumns_Name));

	return true;
}

bool CLevelExplorer::OnLock()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	for (CObjectLayer* pLayer : allLayers)
	{
		pLayer->SetFrozen(true);
	}

	return true;
}

bool CLevelExplorer::OnUnlock()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	for (CObjectLayer* pLayer : allLayers)
	{
		pLayer->SetFrozen(false);
	}

	return true;
}

bool CLevelExplorer::OnToggleLock()
{
	std::vector<CObjectLayer*> allLayers;
	std::vector<CBaseObject*> objects;

	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);
	LevelModelsUtil::GetObjectsForIndexList(m_treeView->selectionModel()->selectedRows(), objects);

	if (allLayers.empty() && objects.empty())
		return false;

	LevelExplorerCommandHelper::ToggleLocked(allLayers, objects);

	return true;
}

bool CLevelExplorer::OnIsolateLocked()
{
	return IsolateLocked(m_treeView->currentIndex());
}

bool CLevelExplorer::OnHide()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	for (CObjectLayer* pLayer : allLayers)
	{
		pLayer->SetVisible(false);
	}

	return true;
}

bool CLevelExplorer::OnUnhide()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	for (CObjectLayer* pLayer : allLayers)
	{
		pLayer->SetVisible(true);
	}

	return true;
}

bool CLevelExplorer::OnToggleHide()
{
	std::vector<CObjectLayer*> allLayers;
	std::vector<CBaseObject*> objects;

	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);
	LevelModelsUtil::GetObjectsForIndexList(m_treeView->selectionModel()->selectedRows(), objects);

	if (allLayers.empty() && objects.empty())
		return false;

	LevelExplorerCommandHelper::ToggleVisibility(allLayers, objects);

	return true;
}

bool CLevelExplorer::OnIsolateVisibility()
{
	return IsolateVisibility(m_treeView->currentIndex());
}

bool CLevelExplorer::OnCollapseAll()
{
	m_treeView->collapseAll();
	return true;
}

bool CLevelExplorer::OnExpandAll()
{
	m_treeView->expandAll();
	return true;
}

bool CLevelExplorer::OnLockChildren()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	if (allLayers.empty())
		return false;

	for (CObjectLayer* pLayer : allLayers)
	{
		OnLockAllInLayer(pLayer);
	}

	return true;
}

bool CLevelExplorer::OnUnlockChildren()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	if (allLayers.empty())
		return false;

	for (CObjectLayer* pLayer : allLayers)
	{
		OnUnlockAllInLayer(pLayer);
	}

	return true;
}

bool CLevelExplorer::OnToggleLockChildren()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	if (allLayers.empty())
		return false;

	// First check if all layers have the same lock state
	bool isFirstObjectLocked = IsFirstChildLocked(allLayers);
	bool hasMultipleState = DoChildrenMatchLockedState(allLayers, isFirstObjectLocked);

	for (CObjectLayer* pLayer : allLayers)
	{
		// If layers don't have the same lock state, then make them all unlocked. If not, then toggle their lock state
		if (hasMultipleState || isFirstObjectLocked)
		{
			OnUnlockAllInLayer(pLayer);
		}
		else
		{
			OnLockAllInLayer(pLayer);
		}
	}

	return true;
}

bool CLevelExplorer::OnHideChildren()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	if (allLayers.empty())
		return false;

	for (CObjectLayer* pLayer : allLayers)
	{
		OnHideAllInLayer(pLayer);
	}

	return true;
}

bool CLevelExplorer::OnUnhideChildren()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	if (allLayers.empty())
		return false;

	for (CObjectLayer* pLayer : allLayers)
	{
		OnUnlockAllInLayer(pLayer);
	}

	return true;
}

bool CLevelExplorer::OnToggleHideChildren()
{
	std::vector<CObjectLayer*> allLayers;
	LevelModelsUtil::GetAllLayersForIndexList(m_treeView->selectionModel()->selectedRows(), allLayers);

	if (allLayers.empty())
		return false;

	// First check if all layers have the same lock state
	bool isFirstObjectHidden = IsFirstChildHidden(allLayers);
	bool hasMultipleState = DoChildrenMatchHiddenState(allLayers, isFirstObjectHidden);

	for (CObjectLayer* pLayer : allLayers)
	{
		// If layers don't have the same lock state, then make them all unlocked. If not, then toggle their lock state
		if (hasMultipleState || isFirstObjectHidden)
		{
			OnUnhideAllInLayer(pLayer);
		}
		else
		{
			OnHideAllInLayer(pLayer);
		}
	}

	return true;
}

bool CLevelExplorer::IsFirstChildLocked(const std::vector<CObjectLayer*>& layers) const
{
	for (const CObjectLayer* pLayer : layers)
	{
		CBaseObjectsArray objects;
		GetIEditorImpl()->GetObjectManager()->GetObjects(objects, pLayer);

		for (const CBaseObject* pObject : objects)
		{
			return pObject->IsFrozen();
		}
	}

	return false;
}

bool CLevelExplorer::DoChildrenMatchLockedState(const std::vector<CObjectLayer*>& layers, bool isLocked) const
{
	for (const CObjectLayer* pLayer : layers)
	{
		CBaseObjectsArray objects;
		GetIEditorImpl()->GetObjectManager()->GetObjects(objects, pLayer);

		for (const CBaseObject* pObject : objects)
		{
			if (isLocked != pObject->IsFrozen())
				return false;
		}

		std::vector<CObjectLayer*> childLayers;
		childLayers.reserve(pLayer->GetChildCount());
		for (int i = 0; i < pLayer->GetChildCount(); i++)
		{
			childLayers.push_back(pLayer->GetChild(i));
		}

		if (!DoChildrenMatchLockedState(childLayers, isLocked))
			return false;
	}

	return true;
}

bool CLevelExplorer::IsFirstChildHidden(const std::vector<CObjectLayer*>& layers) const
{
	for (const CObjectLayer* pLayer : layers)
	{
		CBaseObjectsArray objects;
		GetIEditorImpl()->GetObjectManager()->GetObjects(objects, pLayer);

		for (const CBaseObject* pObject : objects)
		{
			return pObject->IsHidden();
		}
	}

	return false;
}

bool CLevelExplorer::DoChildrenMatchHiddenState(const std::vector<CObjectLayer*>& layers, bool isHidden) const
{
	for (const CObjectLayer* pLayer : layers)
	{
		CBaseObjectsArray objects;
		GetIEditorImpl()->GetObjectManager()->GetObjects(objects, pLayer);

		for (const CBaseObject* pObject : objects)
		{
			if (isHidden != pObject->IsFrozen())
				return false;
		}

		std::vector<CObjectLayer*> childLayers;
		childLayers.reserve(pLayer->GetChildCount());
		for (int i = 0; i < pLayer->GetChildCount(); i++)
		{
			childLayers.push_back(pLayer->GetChild(i));
		}

		if (!DoChildrenMatchHiddenState(childLayers, isHidden))
			return false;
	}

	return true;
}

bool CLevelExplorer::IsolateLocked(const QModelIndex& index)
{
	if (!index.isValid())
		return false;

	auto type = index.data((int)CLevelModel::Roles::TypeCheckRole);

	if (type.toInt() == ELevelElementType::eLevelElement_Layer)
	{
		CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();

		if (!pLayerManager || !pLayer)
			return false;

		pLayerManager->ToggleFreezeAllBut(pLayer);
	}
	if (type.toInt() == ELevelElementType::eLevelElement_Object)
	{
		CBaseObject* pObject = reinterpret_cast<CBaseObject*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		IObjectManager* pObjectManager = GetIEditorImpl()->GetObjectManager();

		if (!pObjectManager || !pObject)
			return false;

		pObjectManager->ToggleFreezeAllBut(pObject);
	}

	return true;
}

bool CLevelExplorer::IsolateVisibility(const QModelIndex& index)
{
	if (!index.isValid())
		return false;

	auto type = index.data((int)CLevelModel::Roles::TypeCheckRole);

	if (type.toInt() == ELevelElementType::eLevelElement_Layer)
	{
		CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();

		if (!pLayerManager || !pLayer)
			return false;

		pLayerManager->ToggleHideAllBut(pLayer);
	}
	if (type.toInt() == ELevelElementType::eLevelElement_Object)
	{
		CBaseObject* pObject = reinterpret_cast<CBaseObject*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		IObjectManager* pObjectManager = GetIEditorImpl()->GetObjectManager();

		if (!pObjectManager || !pObject)
			return false;

		pObjectManager->ToggleHideAllBut(pObject);
	}

	return true;
}

void CLevelExplorer::OnLayerChange(const CLayerChangeEvent& event)
{
	switch (event.m_type)
	{
	case CLayerChangeEvent::LE_SELECT:
		update();
	case CLayerChangeEvent::LE_MODIFY:
	case CLayerChangeEvent::LE_UPDATE_ALL:
		if (m_modelType == ActiveLayer)
		{
			auto layer = GetIEditorImpl()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
			SetSourceModel(CLevelModelsManager::GetInstance().GetLayerModel(layer));
		}
		break;
	default:
		break;
	}
}

void CLevelExplorer::OnObjectsChanged(const std::vector<CBaseObject*>& objects, const CObjectEvent& event)
{
	if (!m_syncSelection || m_ignoreSelectionEvents || m_modelType == Layers)
		return;

	for (const CBaseObject* pObject : objects)
	{
		if (event.m_type == OBJECT_ON_LAYERCHANGE && pObject->IsSelected())
		{
			auto index = FindObjectIndex(pObject);
			if (index.isValid())
			{
				m_treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
			}
		}
	}
}

void CLevelExplorer::OnViewportSelectionChanged(const std::vector<CBaseObject*>& selected, const std::vector<CBaseObject*>& deselected)
{
	CRY_PROFILE_FUNCTION(PROFILE_EDITOR);

	if (!m_syncSelection || m_ignoreSelectionEvents || m_modelType == Layers)
		return;

	m_ignoreSelectionEvents = true;

	QItemSelection deselect;
	QItemSelection select;

	CreateItemSelectionFrom(selected, select);
	CreateItemSelectionFrom(deselected, deselect);

	m_treeView->selectionModel()->select(deselect, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
	m_treeView->selectionModel()->select(select, QItemSelectionModel::Select | QItemSelectionModel::Rows);

	UpdateCurrentIndex();

	m_ignoreSelectionEvents = false;
}

void CLevelExplorer::OnLayerModelsUpdated()
{
	if (m_modelType == ActiveLayer)
	{
		auto layer = GetIEditorImpl()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
		SetSourceModel(CLevelModelsManager::GetInstance().GetLayerModel(layer));
	}
}

void CLevelExplorer::SetModelType(ModelType aModelType)
{
	ModelType lastModelType = m_modelType;
	m_modelType = aModelType;
	QAbstractItemModel* model = nullptr;

	QVariantMap columnStateMap;

	if (m_treeView && m_treeView->header() && (lastModelType != m_modelType))
	{
		columnStateMap = m_treeView->GetState();
	}

	m_pShowActiveLayer->setChecked(false);
	m_pShowAllObjects->setChecked(false);
	m_pShowFullHierarchy->setChecked(false);
	m_pShowLayers->setChecked(false);

	switch (m_modelType)
	{
	case ModelType::Objects:
		model = CLevelModelsManager::GetInstance().GetAllObjectsListModel();
		m_pShowAllObjects->setChecked(true);
		break;
	case ModelType::Layers:
		model = CLevelModelsManager::GetInstance().GetLevelModel();
		m_pShowLayers->setChecked(true);
		break;
	case ModelType::FullHierarchy:
		model = CLevelModelsManager::GetInstance().GetFullHierarchyModel();
		m_pShowFullHierarchy->setChecked(true);
		break;
	case ModelType::ActiveLayer:
		{
			auto layer = GetIEditorImpl()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
			model = CLevelModelsManager::GetInstance().GetLayerModel(layer);
			m_pShowActiveLayer->setChecked(true);
			break;
		}
	}

	SetSourceModel(model);

	if (m_treeView && m_treeView->header() && (lastModelType != m_modelType))
	{
		m_treeView->SetState(columnStateMap);
	}

	if (m_syncSelection)
		SyncSelection();
}

void CLevelExplorer::SetSyncSelection(bool syncSelection)
{
	if (m_syncSelection != syncSelection)
	{
		m_syncSelection = syncSelection;
		m_pSyncSelection->setChecked(m_syncSelection);

		if (m_syncSelection)
			SyncSelection();
	}
}

void CLevelExplorer::SyncSelection()
{
	if (m_modelType == ModelType::Layers)
		return;

	m_ignoreSelectionEvents = true;

	const CSelectionGroup* pSelection = GetIEditorImpl()->GetSelection();
	std::vector<CBaseObject*> selected;
	selected.reserve(pSelection->GetCount());

	for (int i = 0, count = pSelection->GetCount(); i < count; ++i)
	{
		selected.push_back(pSelection->GetObject(i));
	}

	QItemSelection select;
	CreateItemSelectionFrom(selected, select);
	m_treeView->selectionModel()->select(select, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	UpdateCurrentIndex();

	m_ignoreSelectionEvents = false;
}

void CLevelExplorer::UpdateCurrentIndex()
{
	if (m_modelType == ModelType::Layers)
		return;

	// If there's an object selected, update the current index. This is used for expanding to current index and for keyboard
	// traversal of the tree
	const CSelectionGroup* pSelection = GetIEditorImpl()->GetSelection();
	if (pSelection->GetCount())
	{
		QModelIndex index = FindObjectIndex(pSelection->GetObject(pSelection->GetCount() - 1));

		// Clear the current index in case that it hasn't changed, yet we still want to scroll to the currentIndex
		m_treeView->selectionModel()->clearCurrentIndex();
		m_treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Current | QItemSelectionModel::Rows);
	}
}

void CLevelExplorer::CreateItemSelectionFrom(const std::vector<CBaseObject*>& objects, QItemSelection& selection) const
{
	for (const CBaseObject* pObject : objects)
	{
		QModelIndex index = FindObjectIndex(pObject);

		if (index.isValid())
		{
			selection.select(index, index);
		}
	}
}

QModelIndex CLevelExplorer::FindObjectIndex(const CBaseObject* pObject) const
{
	switch (m_modelType)
	{
	case CLevelExplorer::Objects:
	case CLevelExplorer::ActiveLayer:
		// if the current model only lists objects, do the straight unoptimized Qt indexing
		return FindIndexByInternalId(reinterpret_cast<intptr_t>(pObject));

	case CLevelExplorer::Layers:
		// if the current model only lists layers, this surely wouldn't return anything reasonable anyway
		return QModelIndex();

	case CLevelExplorer::FullHierarchy:
	default:
		auto pLayer = pObject->GetLayer();
		if (!pLayer)
		{
			return QModelIndex();
		}
		// This is a big optimization (in the current state of the level explorer models) because of two things:
		// - searching inside each layer is actually a lot faster than searching the whole model, as the actual layer model is invoked and "cached"
		// - layers in the mounting/merging models are ordered breadth-first
		auto layerIndex = FindLayerIndexInModel(reinterpret_cast<CObjectLayer*>(pLayer));
		return FindObjectInHierarchy(layerIndex, pObject);
	}
}

QModelIndex CLevelExplorer::FindLayerIndex(const CObjectLayer* layer) const
{
	// if the current model lists objects (or objects in the active layer), this wouldn't make any sense
	if (m_modelType == Objects || m_modelType == ActiveLayer)
		return QModelIndex();

	return FindLayerIndexInModel(layer);
}

QModelIndex CLevelExplorer::FindLayerIndexInModel(const CObjectLayer* pLayer) const
{
	std::vector<const CObjectLayer*> nestingChain = { pLayer };
	CObjectLayer* pParentLayer = pLayer->GetParent();
	while (pParentLayer)
	{
		nestingChain.push_back(pParentLayer);
		pParentLayer = pParentLayer->GetParent();
	}

	// there is no way to get objects of a layer at the moment, that's why the first pass is through Qt indices
	QModelIndex layerIndex;
	auto pCurrentLayer = nestingChain[nestingChain.size() - 1];
	const auto count = m_pAttributeFilterProxyModel->rowCount(layerIndex);
	for (int i = 0; i < count; i++)
	{
		const auto index = m_pAttributeFilterProxyModel->index(i, 0, layerIndex);
		const QVariant internalPtrVar = index.data((int)CLevelModel::Roles::InternalPointerRole);

		const auto thisLayer = reinterpret_cast<CObjectLayer*>(internalPtrVar.value<intptr_t>());

		if (pCurrentLayer == thisLayer)
		{
			layerIndex = index;
			break;
		}
	}

	// As an optimization we can map the layer index to the source model. By doing so we can guarantee
	// that the layer order in the data is the same than the one in the source model. We can then iterate
	// quickly through the data and find the correct row for child layers rather than go through model->index()
	layerIndex = m_pAttributeFilterProxyModel->mapToSource(layerIndex);
	for (int i = nestingChain.size() - 2; i >= 0; --i)
	{
		auto pChainChild = nestingChain[i];
		for (int childIndex = 0; childIndex < pCurrentLayer->GetChildCount(); ++childIndex)
		{
			auto pChild = pCurrentLayer->GetChild(childIndex);
			if (pChild == pChainChild)
			{
				layerIndex = m_pAttributeFilterProxyModel->sourceModel()->index(childIndex, 0, layerIndex);
				pCurrentLayer = pChainChild;
				break;
			}
		}
	}

	return pCurrentLayer == pLayer ? m_pAttributeFilterProxyModel->mapFromSource(layerIndex) : QModelIndex();
}

QModelIndex CLevelExplorer::FindObjectInHierarchy(const QModelIndex& parentIndex, const CBaseObject* object) const
{
	auto count = m_pAttributeFilterProxyModel->rowCount(parentIndex);
	for (int i = 0; i < count; i++)
	{
		auto index = m_pAttributeFilterProxyModel->index(i, 0, parentIndex);
		QVariant internalPtrVar = index.data((int)CLevelModel::Roles::InternalPointerRole);
		if (object == reinterpret_cast<CBaseObject*>(internalPtrVar.value<intptr_t>()))
			return index;

		// Make sure to go through the whole hierarchy and not just top level items
		index = FindObjectInHierarchy(index, object);
		if (index.isValid())
			return index;
	}
	return QModelIndex();
}

QModelIndex CLevelExplorer::FindIndexByInternalId(intptr_t id) const
{
	auto model = m_treeView->model();
	auto matchingList = model->match(model->index(0, 0, QModelIndex()), (int)CLevelModel::Roles::InternalPointerRole, id, 1, Qt::MatchRecursive | Qt::MatchExactly);
	if (matchingList.size() == 1)
	{
		return matchingList.first();
	}

	return QModelIndex();
}

void CLevelExplorer::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{

	// Optimization for layer changes that really shouldn't involve deselecting and selecting objects, but given the nature of
	// how we build the level explorer using separate item models for each layer, moving an object from one layer to another,
	// means we must remove the object from one model and add to the other. The removal forces the qt model to deselect the index,
	// if sync selection is activated, it'll then trigger a selection change in the editor, which is WRONG in this case.
	IObjectManager* pObjectManager = GetIEditorImpl()->GetObjectManager();
	if (pObjectManager->IsLayerChanging())
		return;

	if (!m_syncSelection || m_ignoreSelectionEvents || m_modelType == Layers)
		return;

	//Propagate selection to object manager
	m_ignoreSelectionEvents = true;

	QModelIndexList unselectObjectIndices;
	std::vector<CBaseObject*> objectsToSelect;
	std::vector<CBaseObject*> objectsToDeselect;

	//Select
	{
		std::vector<CBaseObject*> objects;
		std::vector<CObjectLayer*> layers;
		std::vector<CObjectLayer*> layerFolders;

		LevelModelsUtil::GetObjectsAndLayersForIndexList(LevelModelsUtil::FilterByColumn(selected.indexes()), objects, layers, layerFolders);

		objectsToSelect.reserve(objects.size());

		for (auto pObject : objects)
		{
			if (pObject->IsFrozen())
			{
				unselectObjectIndices.append(FindObjectIndex(pObject));
			}
			else
			{
				objectsToSelect.push_back(pObject);
			}
		}
	}

	//Deselect
	{
		std::vector<CBaseObject*> objects;
		std::vector<CObjectLayer*> layers;
		std::vector<CObjectLayer*> layerFolders;

		LevelModelsUtil::GetObjectsAndLayersForIndexList(LevelModelsUtil::FilterByColumn(deselected.indexes()), objects, layers, layerFolders);

		objectsToDeselect.insert(objectsToDeselect.end(), objects.begin(), objects.end());
	}

	CUndo undo("Select Objects");
	pObjectManager->SelectAndUnselectObjects(objectsToSelect, objectsToDeselect);

	//Prevent selecting frozen objects
	for (auto& index : unselectObjectIndices)
	{
		m_treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
	}

	//validate that selections is synced properly, objects can be selected in the manager but not in the table selection (happens for instance if you search/filter and then select)
	{
		const CSelectionGroup* objManSelection = pObjectManager->GetSelection();
		auto selection = m_treeView->selectionModel()->selectedRows();

		std::vector<CBaseObject*> objects;
		std::vector<CObjectLayer*> layers;
		std::vector<CObjectLayer*> layerFolders;

		LevelModelsUtil::GetObjectsAndLayersForIndexList(selection, objects, layers, layerFolders);
		const int objManSelectionCount = objManSelection->GetCount();

		if (objManSelectionCount != objects.size())
		{
			for (int i = 0; i < objManSelectionCount; i++)
			{
				auto object = objManSelection->GetObject(i);
				if (std::find(objects.begin(), objects.end(), object) == objects.end())
					objectsToDeselect.push_back(object);
			}
			pObjectManager->UnselectObjects(objectsToDeselect);
		}
	}

	UpdateSelectionActionState();

	m_ignoreSelectionEvents = false;
}

void CLevelExplorer::SetActionsEnabled(bool areEnabled)
{
	m_pRename->setEnabled(areEnabled);
	m_pDelete->setEnabled(areEnabled);

	m_pToggleVisibility->setEnabled(areEnabled);
	m_pToggleChildrenVisibility->setEnabled(areEnabled);
	m_pIsolateVisibility->setEnabled(areEnabled);

	m_pToggleLocking->setEnabled(areEnabled);
	m_pToggleChildrenLocking->setEnabled(areEnabled);
	m_pIsolateLocked->setEnabled(areEnabled);
}

void CLevelExplorer::UpdateSelectionActionState()
{
	QModelIndexList selection = m_treeView->selectionModel()->selectedRows();

	SetActionsEnabled(false);

	if (selection.isEmpty())
		return;

	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;
	std::vector<CObjectLayer*> layerFolders;
	LevelModelsUtil::GetObjectsAndLayersForIndexList(m_treeView->selectionModel()->selectedRows(), objects, layers, layerFolders);

	std::vector<CObjectLayer*> allLayers;
	allLayers.reserve(layers.size() + layerFolders.size());
	allLayers.insert(allLayers.end(), layers.cbegin(), layers.cend());
	allLayers.insert(allLayers.end(), layerFolders.cbegin(), layerFolders.cend());
	
	if (!objects.empty() || !allLayers.empty())
	{
		SetActionsEnabled(true);

		m_pToggleVisibility->setChecked(LevelExplorerCommandHelper::AreVisible(allLayers, objects));
		m_pToggleLocking->setChecked(LevelExplorerCommandHelper::AreLocked(allLayers, objects));
	}

	if (!allLayers.empty())
	{
		m_pTogglePC->setChecked(LevelExplorerCommandHelper::HasPlatformSpecs(layers, eSpecType_PC));
		m_pToggleXBoxOne->setChecked(LevelExplorerCommandHelper::HasPlatformSpecs(layers, eSpecType_XBoxOne));
		m_pTogglePS4->setChecked(LevelExplorerCommandHelper::HasPlatformSpecs(layers, eSpecType_PS4));

		m_pToggleExportable->setChecked(LevelExplorerCommandHelper::AreExportable(layers));
		m_pToggleExportableToPak->setChecked(LevelExplorerCommandHelper::AreExportableToPak(layers));
		m_pToggleAutoLoaded->setChecked(LevelExplorerCommandHelper::AreAutoLoaded(layers));
		m_pTogglePhysics->setChecked(LevelExplorerCommandHelper::HavePhysics(layers));
	}
}

void CLevelExplorer::OnLayerModelResetBegin()
{
	m_ignoreSelectionEvents = true;
}

void CLevelExplorer::OnLayerModelResetEnd()
{
	m_ignoreSelectionEvents = false;
}

void CLevelExplorer::SetSourceModel(QAbstractItemModel* model)
{
	auto old = m_pAttributeFilterProxyModel->sourceModel();
	if (old)
	{
		disconnect(old, &QObject::destroyed, this, &CLevelExplorer::OnModelDestroyed);
	}

	if (model)
	{
		connect(model, &QObject::destroyed, this, &CLevelExplorer::OnModelDestroyed);
	}

	m_pAttributeFilterProxyModel->setSourceModel(model);
}

void CLevelExplorer::OnHeaderSectionCountChanged()
{
	// Ensure that color, visible and frozen columns are consistently of these sizes
	// Since the columns are fixed size, if they end up stretching, there's no way
	// to reset them
	m_treeView->header()->resizeSection((int)eLayerColumns_Color, 10);
	m_treeView->header()->resizeSection((int)eLayerColumns_Visible, 25);
	m_treeView->header()->resizeSection((int)eLayerColumns_Frozen, 25);
	m_treeView->header()->resizeSection((int)eLayerColumns_VCS, 25);
}

void CLevelExplorer::OnModelDestroyed()
{
	m_pAttributeFilterProxyModel->setSourceModel(nullptr);
}

void CLevelExplorer::EditLayer(CObjectLayer* pLayer) const
{
	if (!pLayer)
		return;

	//Focus and edit
	auto index = FindLayerIndex(pLayer);
	if (index.isValid())
	{
		m_treeView->expand(index.parent());
		m_treeView->scrollTo(index);
		m_treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
		m_treeView->edit(index.sibling(index.row(), (int)eLayerColumns_Name));
	}
}

void CLevelExplorer::OnDeleteLayers(const std::vector<CObjectLayer*>& layers) const
{
	QString message = tr("Are you sure you want to delete the selected layers?");
	GetIEditorImpl()->GetIUndoManager()->Begin();
	if (QDialogButtonBox::StandardButton::Yes == CQuestionDialog::SQuestion(tr("Delete layers"), message))
	{
		for (auto& layer : layers)
		{
			GetIEditorImpl()->GetObjectManager()->GetLayersManager()->DeleteLayer(layer);
		}
	}
	GetIEditorImpl()->GetIUndoManager()->Accept("Delete Layers");
}

void CLevelExplorer::OnLockAllInLayer(CObjectLayer* layer) const
{
	CBaseObjectsArray objects;
	GetIEditorImpl()->GetObjectManager()->GetObjects(objects, layer);
	for (auto item : objects)
	{
		item->SetFrozen(true);
	}
}

void CLevelExplorer::OnUnlockAllInLayer(CObjectLayer* layer) const
{
	CBaseObjectsArray objects;
	GetIEditorImpl()->GetObjectManager()->GetObjects(objects, layer);
	layer->SetFrozen(false);
	for (auto item : objects)
	{
		item->SetFrozen(false);
	}

	// Do the same for all child layers
	for (int i = 0; i < layer->GetChildCount(); i++)
	{
		OnUnlockAllInLayer(layer->GetChild(i));
	}
}

void CLevelExplorer::OnHideAllInLayer(CObjectLayer* layer) const
{
	CBaseObjectsArray objects;
	GetIEditorImpl()->GetObjectManager()->GetObjects(objects, layer);
	for (auto item : objects)
	{
		item->SetVisible(false);
	}
}

void CLevelExplorer::OnUnhideAllInLayer(CObjectLayer* layer) const
{
	CBaseObjectsArray objects;
	GetIEditorImpl()->GetObjectManager()->GetObjects(objects, layer);
	layer->SetVisible(true);
	for (auto item : objects)
	{
		item->SetVisible(true);
	}

	// Do the same for all child layers
	for (int i = 0; i < layer->GetChildCount(); i++)
	{
		OnUnhideAllInLayer(layer->GetChild(i));
	}
}

bool CLevelExplorer::AllFrozenInLayer(CObjectLayer* layer) const
{
	CBaseObjectsArray objects;
	GetIEditorImpl()->GetObjectManager()->GetObjects(objects, layer);
	for (auto item : objects)
	{
		if (!item->IsFrozen())
			return false;
	}
	return true;
}

bool CLevelExplorer::AllHiddenInLayer(CObjectLayer* layer) const
{
	CBaseObjectsArray objects;
	GetIEditorImpl()->GetObjectManager()->GetObjects(objects, layer);
	for (auto item : objects)
	{
		if (item->IsVisible())
			return false;
	}
	return true;
}

void CLevelExplorer::OnSelectColor(const std::vector<CObjectLayer*>& layers) const
{
	using namespace Private_LevelExplorer;
	ColorB color(255, 255, 255);
	if (layers.size() == 1)
	{
		color = layers[0]->GetColor();
	}
	color = AskForColor(color, const_cast<CLevelExplorer*>(this));
	for (auto& layer : layers)
	{
		layer->SetColor(color, true);
	}
}

void CLevelExplorer::FocusActiveLayer()
{
	auto layer = GetIEditorImpl()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
	if (layer)
	{
		auto index = FindLayerIndex(layer);
		if (index.isValid())
		{
			m_treeView->expand(index.parent());
			m_treeView->scrollTo(index);
			m_treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
		}
	}
}
