// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>

#include "LevelExplorer.h"

#include "LevelModel.h"
#include "LevelLayerModel.h"
#include "LevelModelsManager.h"
#include "Objects/ObjectLayerManager.h"
#include "Objects/ObjectLayer.h"
#include "Objects/Group.h"
#include "CryEdit.h"

#include "QControls.h"
#include "QAdvancedTreeView.h"
#include "QAdvancedItemDelegate.h"
#include "QSearchBox.h"
#include "QFilteringPanel.h"
#include "Controls/DynamicPopupMenu.h"
#include "QtUtil.h"
#include "ProxyModels/AttributeFilterProxyModel.h"
#include "ProxyModels/MergingProxyModel.h"
#include "Controls/QMenuComboBox.h"
#include "CryIcon.h"
#include "QtUtil.h"
#include "MenuHelpers.h"
#include "FileDialogs/SystemFileDialog.h"
#include "QT/QtMainFrame.h"
#include "FilePathUtil.h"

#include <QAbstractItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QClipboard>
#include <QApplication>
#include <QDir>
#include <QProcess>
#include <QColorDialog>
#include <QToolButton>
#include <QLabel>
#include <QHeaderView>
#include <QLayout>
#include "Controls/QuestionDialog.h"
#include "LevelEditorViewport.h"
#include "Menu/MenuWidgetBuilders.h"

REGISTER_VIEWPANE_FACTORY_AND_MENU(CLevelExplorer, "Level Explorer", "Tools", false, "Level Editor")

CCrySignal<void(CAbstractMenu&, const std::vector<CBaseObject*>& objects, const std::vector<CObjectLayer*>& layers, const std::vector<CObjectLayer*>& folders)> CLevelExplorer::s_signalContextMenuRequested;

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
	{ "Blue",   "icons:General/colour_blue.ico",{ 77,  155, 214 } },
	{ "Green",  "icons:General/colour_green.ico",{ 125, 209, 77 } },
	{ "Orange", "icons:General/colour_orange.ico",{ 214, 155, 77 } },
	{ "Purple", "icons:General/colour_purple.ico",{ 144, 92, 235 } },
	{ "Red",    "icons:General/colour_red.ico",{ 210, 86, 86 } },
	{ "Yellow", "icons:General/colour_yellow.ico",{ 225, 225, 80 } }
};


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
			static_cast<uint8>(r), static_cast<uint8>(g), static_cast<uint8>(b)
		};
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

}

CLevelExplorer::CLevelExplorer(QWidget* pParent)
	: CDockableEditor(pParent)
	, m_pAttributeFilterProxyModel(new QAttributeFilterProxyModel(QAttributeFilterProxyModel::AcceptIfChildMatches, this))
	, m_modelType(Objects)
	, m_syncSelection(false)
	, m_ignoreSelectionEvents(false)
	, m_treeView(nullptr)
{
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
	m_treeView->setDragDropMode(QAbstractItemView::InternalMove);
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

	m_filterPanel = new QFilteringPanel("LevelExplorer", m_pAttributeFilterProxyModel);
	m_filterPanel->SetContent(m_treeView);
	m_filterPanel->GetSearchBox()->SetAutoExpandOnSearch(m_treeView);

	InitMenuBar();

	SetContent(m_filterPanel);

	CObjectManager* pObjManager = static_cast<CObjectManager*>(GetIEditorImpl()->GetObjectManager());
	pObjManager->GetLayersManager()->signalChangeEvent.Connect(this, &CLevelExplorer::OnLayerChange);
	pObjManager->signalObjectsChanged.Connect(this, &CLevelExplorer::OnObjectsChanged);
	pObjManager->signalSelectionChanged.Connect(this, &CLevelExplorer::OnViewportSelectionChanged);

	CLevelModelsManager::GetInstance().signalLayerModelsUpdated.Connect(this, &CLevelExplorer::OnLayerModelsUpdated);
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

void CLevelExplorer::InitMenuBar()
{

	//File
	AddToMenu(CEditor::MenuItems::FileMenu);
	CAbstractMenu* const fileMenu = GetMenu(CEditor::MenuItems::FileMenu);

	//TODO : this could use the default new action to inherit the shortcut (and icon)
	auto action = fileMenu->CreateAction(CryIcon("icons:General/File_New.ico"), tr("New Layer"));
	connect(action, &QAction::triggered, [&]()
	{
		OnNewLayer(nullptr);
	});

	action = fileMenu->CreateAction(CryIcon("icons:General/Folder_Add.ico"), tr("New Folder"));
	connect(action, &QAction::triggered, [&]()
	{
		OnNewFolder(nullptr);
	});

	int sec = fileMenu->GetNextEmptySection();

	action = fileMenu->CreateAction(tr("Import Layer"), sec);
	connect(action, &QAction::triggered, this, [this]()
	{
		OnImportLayer();
	});

	//Edit
	AddToMenu(CEditor::MenuItems::EditMenu);
	CAbstractMenu* editMenu = GetMenu(CEditor::MenuItems::EditMenu);

	sec = editMenu->GetNextEmptySection();

	editMenu->AddCommandAction("layer.hide_all", sec);
	editMenu->AddCommandAction("layer.show_all", sec);

	sec = editMenu->GetNextEmptySection();

	editMenu->AddCommandAction("layer.make_all_uneditable", sec);
	editMenu->AddCommandAction("layer.make_all_editable", sec);

	editMenu->AddCommandAction("layer.make_read_only_layers_uneditable", sec);

	//View
	AddToMenu(CEditor::MenuItems::ViewMenu);
	CAbstractMenu* const menuView = GetMenu(CEditor::MenuItems::ViewMenu);
	menuView->signalAboutToShow.Connect([menuView, this]()
	{
		menuView->Clear();

		int sec = menuView->GetNextEmptySection();

		auto action = menuView->CreateAction(tr("Show All Objects"), sec);
		action->setCheckable(true);
		action->setChecked(m_modelType == Objects);
		connect(action, &QAction::triggered, this, [&]()
		{
			SetModelType(Objects);
		});

		action = menuView->CreateAction(tr("Show Layers"), sec);
		action->setCheckable(true);
		action->setChecked(m_modelType == Layers);
		connect(action, &QAction::triggered, this, [&]()
		{
			SetModelType(Layers);
		});

		action = menuView->CreateAction(tr("Show Full Hierarchy"), sec);
		action->setCheckable(true);
		action->setChecked(m_modelType == FullHierarchy);
		connect(action, &QAction::triggered, this, [&]()
		{
			SetModelType(FullHierarchy);
		});

		action = menuView->CreateAction(tr("Show Active Layer Contents"), sec);
		action->setCheckable(true);
		action->setChecked(m_modelType == ActiveLayer);
		connect(action, &QAction::triggered, this, [&]()
		{
			SetModelType(ActiveLayer);
		});

		sec = menuView->GetNextEmptySection();

		action = menuView->CreateAction(tr("Expand All"), sec);
		connect(action, &QAction::triggered, this, [&]()
		{
			OnExpandAllLayers();
		});

		action = menuView->CreateAction(tr("Collapse All"), sec);
		connect(action, &QAction::triggered, this, [&]()
		{
			OnCollapseAllLayers();
		});

		sec = menuView->GetNextEmptySection();

		action = menuView->CreateAction(tr("Sync Selection"), sec);
		action->setCheckable(true);
		action->setChecked(m_syncSelection);
		connect(action, &QAction::triggered, [&]()
		{
			SetSyncSelection(!m_syncSelection);
		});

		if (m_filterPanel)
		{
		  m_filterPanel->FillMenu(menuView, tr("Apply Filter"));
		}
	});

	GetRootMenu()->AddCommandAction("level.focus_on_active_layer", 0);
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

void CLevelExplorer::customEvent(QEvent* event)
{
	if (event->type() != SandboxEvent::Command)
	{
		CDockableEditor::customEvent(event);
		return;
	}

	CommandEvent* commandEvent = static_cast<CommandEvent*>(event);
	const string& command = commandEvent->GetCommand();

	if (command == "level.isolate_editability")
	{
		QModelIndex index = m_treeView->currentIndex();
		if (index.isValid())
		{
			IsolateEditability(index);
		}
	}
	else if (command == "level.isolate_visibility")
	{
		QModelIndex index = m_treeView->currentIndex();
		if (index.isValid())
		{
			IsolateVisibility(index);
		}
	}
	else if (command == "level.focus_on_active_layer")
	{
		FocusActiveLayer();
	}
	else
	{
		CDockableEditor::customEvent(event);
	}
}

void CLevelExplorer::OnContextMenu(const QPoint& pos) const
{
	using namespace Private_LevelExplorer;
	CAbstractMenu abstractMenu;

	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;
	std::vector<CObjectLayer*> layerFolders;

	auto selection = m_treeView->selectionModel()->selectedRows();

	LevelModelsUtil::ProcessIndexList(selection, objects, layers, layerFolders);

	if (layerFolders.size() == 1)
	{
		CreateContextForSingleFolderLayer(abstractMenu, layerFolders);
	}

	if (layers.size())
	{
		CreateContextMenuForLayers(abstractMenu, layers);
	}

	if (selection.isEmpty())
	{
		if (IsModelTypeShowingLayers())
		{
			int actionsSection = abstractMenu.GetNextEmptySection();
			abstractMenu.SetSectionName(actionsSection, "Actions");

			auto action = abstractMenu.CreateAction(CryIcon("icons:General/File_New.ico"), tr("New Layer"), actionsSection);
			connect(action, &QAction::triggered, [&]() 
			{ 
				OnNewLayer(nullptr); 
			});

			action = abstractMenu.CreateAction(CryIcon("icons:General/Folder_Add.ico"), tr("New Folder"), actionsSection);
			connect(action, &QAction::triggered, [&]() 
			{ 
				OnNewFolder(nullptr); 
			});

			int importSection = abstractMenu.GetNextEmptySection();

			action = abstractMenu.CreateAction(tr("Import Layer"), importSection);
			connect(action, &QAction::triggered, this, [this, layerFolders]() 
			{ 
				OnImportLayer(); 
			});

			int expandSection = abstractMenu.GetNextEmptySection();

			action = abstractMenu.CreateAction(tr("Expand all"), expandSection);
			connect(action, &QAction::triggered, [this]() 
			{ 
				OnExpandAllLayers(); 
			});

			action = abstractMenu.CreateAction(tr("Collapse all"), expandSection);
			connect(action, &QAction::triggered, [this]() 
			{ 
				OnCollapseAllLayers(); 
			});
		}
	}
	else if (selection.count() == 1)
	{
		int actionsSection = abstractMenu.GetNextEmptySection();
		abstractMenu.SetSectionName(actionsSection, "Actions");

		QModelIndex index = selection.first();
		auto action = abstractMenu.CreateAction(tr("Rename"), actionsSection);
		connect(action, &QAction::triggered, [index, this]() 
		{ 
			OnRename(index); 
		});
	}

	if (!layers.empty() || !layerFolders.empty())
	{
		int actionsSection = abstractMenu.FindOrCreateSectionByName("Actions");

		auto action = abstractMenu.CreateAction(tr("Delete"), actionsSection);
		std::vector<CObjectLayer*> allLayers;
		allLayers.reserve(layers.size() + layerFolders.size());
		allLayers.insert(allLayers.end(), layers.cbegin(), layers.cend());
		allLayers.insert(allLayers.end(), layerFolders.cbegin(), layerFolders.cend());
		connect(action, &QAction::triggered, [layers = std::move(allLayers), this]() 
		{ 
			OnDeleteLayers(layers); 
		});
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

	s_signalContextMenuRequested(abstractMenu, objects, layers, layerFolders);

	//executes the menu synchronously so we can keep references to the arrays scoped to this method in lambdas
	menu.exec(m_treeView->mapToGlobal(pos));
}

void CLevelExplorer::CreateContextMenuForLayers(CAbstractMenu &abstractMenu, const std::vector<CObjectLayer*>& layers) const
{
	using namespace Private_LevelExplorer;
	int layersSection = abstractMenu.GetNextEmptySection();
	abstractMenu.SetSectionName(layersSection, "Layers");

	if (layers.size() == 1)
	{
		CObjectLayer* layer = layers[0];
		auto action = abstractMenu.CreateAction(tr("Set As Active Layer"), layersSection);
		connect(action, &QAction::triggered, [layer]()
		{
			GetIEditorImpl()->GetObjectManager()->GetLayersManager()->SetCurrentLayer(layer);
		});
	}

	auto action = abstractMenu.CreateAction(tr("Reload"), layersSection);
	connect(action, &QAction::triggered, [&]() { OnReloadLayers(layers); });

	int importLayerSection = abstractMenu.GetNextEmptySection();

	abstractMenu.AddAction(GetIEditor()->GetICommandManager()->GetAction("layer.make_read_only_layers_uneditable"), importLayerSection);

	if (layers.size() == 1)
	{
		OnContextMenuForSingleLayer(abstractMenu, layers[0]);
	}
	else
	{
		int showFreezeSection = abstractMenu.GetNextEmptySection();

		// Visibility
		abstractMenu.AddAction(GetIEditor()->GetICommandManager()->GetAction("layer.show_all"));
		abstractMenu.AddAction(GetIEditor()->GetICommandManager()->GetAction("layer.hide_all"));

		// Editability
		abstractMenu.AddAction(GetIEditor()->GetICommandManager()->GetAction("layer.make_all_editable"));
		abstractMenu.AddAction(GetIEditor()->GetICommandManager()->GetAction("layer.make_all_uneditable"));
	}

	AddColorSubMenuForLayers(abstractMenu, layers);

	//Another thing if only one layer selected
	if (layers.size() == 1)
	{
		//capture layer by copy or layers by reference
		CObjectLayer* layer = layers[0];

		int miscLayerSection = abstractMenu.GetNextEmptySection();

		action = abstractMenu.CreateAction(tr("Open Containing Folder"), miscLayerSection);
		connect(action, &QAction::triggered, [&]()
		{
			QtUtil::OpenInExplorer((const char*)layer->GetLayerFilepath());
		});

		action = abstractMenu.CreateAction(tr("Copy Filename"), miscLayerSection);
		connect(action, &QAction::triggered, [&]()
		{
			QApplication::clipboard()->setText(PathUtil::GetFile(layer->GetLayerFilepath()).GetString());
		});

		action = abstractMenu.CreateAction(tr("Copy Full Path"), miscLayerSection);
		connect(action, &QAction::triggered, [&]()
		{
			QApplication::clipboard()->setText(layer->GetLayerFilepath().GetString());
		});
	}

	std::vector<string> selectedFilenames;
	for (auto& layer : layers)
	{
		selectedFilenames.push_back(layer->GetLayerFilepath());
	}
}

void CLevelExplorer::CreateContextForSingleFolderLayer(CAbstractMenu &abstractMenu, const std::vector<CObjectLayer*>& layerFolders) const
{
	int folderSection = abstractMenu.GetNextEmptySection();
	abstractMenu.SetSectionName(folderSection, "Folder");

	auto action = abstractMenu.CreateAction(CryIcon("icons:General/File_New.ico"), tr("New Layer"), folderSection);
	connect(action, &QAction::triggered, [&]() { OnNewLayer(layerFolders[0]); });

	action = abstractMenu.CreateAction(CryIcon("icons:General/Folder_Add.ico"), tr("New Folder"), folderSection);
	connect(action, &QAction::triggered, [&]() { OnNewFolder(layerFolders[0]); });

	int emptySection = abstractMenu.GetNextEmptySection();

	action = abstractMenu.CreateAction(tr("Import Layer"), emptySection);
	connect(action, &QAction::triggered, this, [this, layerFolders]() { OnImportLayer(layerFolders[0]); });

	OnContextMenuForSingleLayer(abstractMenu, layerFolders[0]);
}

void CLevelExplorer::OnContextMenuForSingleLayer(CAbstractMenu& menu, CObjectLayer* layer) const
{
	int toggleSection = menu.GetNextEmptySection();
	menu.AddAction(GetIEditor()->GetICommandManager()->GetAction("levisolate_editabilityers"), toggleSection);
	menu.AddAction(GetIEditor()->GetICommandManager()->GetAction("levisolate_visibilityers"), toggleSection);

	int collapseSection = menu.GetNextEmptySection();

	auto action = menu.CreateAction(tr("Expand all"), collapseSection);

	action = menu.CreateAction(tr("Collapse all"), collapseSection);

	int hideFreezeObjectsSection = menu.GetNextEmptySection();

	if (AllFrozenInLayer(layer))
	{
		action = menu.CreateAction(tr("Unfreeze objects in layer"), hideFreezeObjectsSection);
	}
	else
	{
		action = menu.CreateAction(tr("Freeze objects in layer"), hideFreezeObjectsSection);
	}

	if (AllHiddenInLayer(layer))
	{
		action = menu.CreateAction(tr("Unhide objects in layer"), hideFreezeObjectsSection);
	}
	else
	{
		action = menu.CreateAction(tr("Hide objects in layer"), hideFreezeObjectsSection);
	}

	int hideFreezeSection = menu.GetNextEmptySection();

	action = menu.CreateAction(tr("Visible"), hideFreezeSection);
	action->setCheckable(true);
	action->setChecked(layer->IsVisible(false));
	connect(action, &QAction::toggled, [layer](bool bChecked)
	{
		layer->SetVisible(bChecked);
	});

	action = menu.CreateAction(tr("Frozen"), hideFreezeSection);
	action->setCheckable(true);
	action->setChecked(layer->IsFrozen(false));
	connect(action, &QAction::toggled, [layer](bool bChecked)
	{
		layer->SetFrozen(bChecked);
	});

	int advancedSection = menu.GetNextEmptySection();
	auto pAdvancedMenu = menu.CreateMenu(tr("Advanced"), advancedSection);

	int miscSection = pAdvancedMenu->GetNextEmptySection();

	action = pAdvancedMenu->CreateAction(tr("Exportable"), miscSection);
	action->setCheckable(true);
	action->setChecked(layer->IsExportable());
	connect(action, &QAction::toggled, [layer](bool bChecked)
	{
		layer->SetExportable(bChecked);
	});

	action = pAdvancedMenu->CreateAction(tr("Exportable to Pak"), miscSection);
	action->setCheckable(true);
	action->setChecked(layer->IsExporLayerPak());
	connect(action, &QAction::toggled, [layer](bool bChecked)
	{
		layer->SetExportLayerPak(bChecked);
	});

	action = pAdvancedMenu->CreateAction(tr("Loaded in Game"), miscSection);
	action->setCheckable(true);
	action->setChecked(layer->IsDefaultLoaded());
	connect(action, &QAction::toggled, [layer](bool bChecked)
	{
		layer->SetDefaultLoaded(bChecked);
	});

	action = pAdvancedMenu->CreateAction(tr("Has Physics"), miscSection);
	action->setCheckable(true);
	action->setChecked(layer->HasPhysics());
	connect(action, &QAction::toggled, [layer](bool bChecked)
	{
		layer->SetHavePhysics(bChecked);
	});

	// Platforms
	//TODO : this should use enum serialization
	{
		auto pPlatformMenu = pAdvancedMenu->CreateMenu(tr("Platforms"), miscSection);
		action = pPlatformMenu->CreateAction(tr("PC"));
		action->setCheckable(true);
		action->setChecked(layer->GetSpecs() & eSpecType_PC);
		connect(action, &QAction::toggled, [layer](bool bChecked)
		{
			layer->SetSpecs(layer->GetSpecs() ^ eSpecType_PC);
		});

		action = pPlatformMenu->CreateAction(tr("Xbox One"));
		action->setCheckable(true);
		action->setChecked(layer->GetSpecs() & eSpecType_XBoxOne);
		connect(action, &QAction::toggled, [layer](bool bChecked)
		{
			layer->SetSpecs(layer->GetSpecs() ^ eSpecType_XBoxOne);
		});

		action = pPlatformMenu->CreateAction(tr("PS4"));
		action->setCheckable(true);
		action->setChecked(layer->GetSpecs() & eSpecType_PS4);
		connect(action, &QAction::toggled, [&](bool bChecked)
		{
			layer->SetSpecs(layer->GetSpecs() ^ eSpecType_PS4);
		});
	}
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
							OnUnfreezeAllInLayer(pLayer);
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
					IsolateEditability(index);
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
						GetIEditorImpl()->GetObjectManager()->ClearSelection();
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
			objects.push_back(reinterpret_cast<CBaseObject*>(internalPtrVar.value<intptr_t>()));
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

	QItemSelection deselect;
	QItemSelection select;

	CreateItemSelectionFrom(selected, select);
	CreateItemSelectionFrom(deselected, deselect);

	m_treeView->selectionModel()->select(deselect, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
	m_treeView->selectionModel()->select(select, QItemSelectionModel::Select | QItemSelectionModel::Rows);

	UpdateCurrentIndex();
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

	switch (m_modelType)
	{
	case ModelType::Objects:
		model = CLevelModelsManager::GetInstance().GetAllObjectsListModel();
		break;
	case ModelType::Layers:
		model = CLevelModelsManager::GetInstance().GetLevelModel();
		break;
	case ModelType::FullHierarchy:
		model = CLevelModelsManager::GetInstance().GetFullHierarchyModel();
		break;
	case ModelType::ActiveLayer:
		{
			auto layer = GetIEditorImpl()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
			model = CLevelModelsManager::GetInstance().GetLayerModel(layer);
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

		LevelModelsUtil::ProcessIndexList(LevelModelsUtil::FilterByColumn(selected.indexes()), objects, layers, layerFolders);

		objectsToSelect.reserve(objects.size());

		for (auto object : objects)
		{
			if (object->IsFrozen())
			{
				unselectObjectIndices.append(FindObjectIndex(object));
			}
			else
			{
				CBaseObject* pParent = object->GetGroup();
				if (pParent)
				{
					CGroup* pGroup = static_cast<CGroup*>(pParent);
					if (!pGroup->IsOpen())
					{
						unselectObjectIndices.append(FindObjectIndex(object));
					}
				}
				objectsToSelect.push_back(object);
			}
		}
	}

	//Deselect
	{
		std::vector<CBaseObject*> objects;
		std::vector<CObjectLayer*> layers;
		std::vector<CObjectLayer*> layerFolders;

		LevelModelsUtil::ProcessIndexList(LevelModelsUtil::FilterByColumn(deselected.indexes()), objects, layers, layerFolders);

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

		LevelModelsUtil::ProcessIndexList(selection, objects, layers, layerFolders);
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

	m_ignoreSelectionEvents = false;
}

void CLevelExplorer::OnRename(const QModelIndex& index) const
{
	m_treeView->edit(index.sibling(index.row(), (int)eLayerColumns_Name));
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
}

void CLevelExplorer::OnModelDestroyed()
{
	m_pAttributeFilterProxyModel->setSourceModel(nullptr);
}

void CLevelExplorer::OnNewLayer(CObjectLayer* parent) const
{
	CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();
	CRY_ASSERT(pLayerManager);

	CUndo undo("Create Layer");
	auto pLayer = pLayerManager->CreateLayer(eObjectLayerType_Layer, parent);
	EditLayer(pLayer);
	if (!pLayer)
		undo.Cancel();
}

void CLevelExplorer::OnNewFolder(CObjectLayer* parent) const
{
	CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();
	CRY_ASSERT(pLayerManager);

	CUndo undo("Create Folder");
	auto pFolder = pLayerManager->CreateLayer(eObjectLayerType_Folder, parent);
	EditLayer(pFolder);
	if (!pFolder)
		undo.Cancel();
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

void CLevelExplorer::OnReloadLayers(const std::vector<CObjectLayer*>& layers) const
{
	bool modified = false;
	for (auto& layer : layers)
	{
		if (layer->IsModified())
		{
			modified = true;
			break;
		}
	}

	if (modified)
	{
		QString message = tr("Some of the selected layers have been modified. Your changes will be lost. Reload selected layers anyway?");
		if (QDialogButtonBox::StandardButton::Yes != CQuestionDialog::SQuestion(tr("Reload layers"), message))
		{
			return;
		}
	}

	for (auto& layer : layers)
	{
		GetIEditorImpl()->GetObjectManager()->GetLayersManager()->ReloadLayer(layer);
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

void CLevelExplorer::OnImportLayer(CObjectLayer* pTargetLayer) const
{
	CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();
	if (pLayerManager)
	{
		CSystemFileDialog::OpenParams openParams(CSystemFileDialog::OpenMultipleFiles);
		openParams.title = QObject::tr("Import Layer");
		openParams.extensionFilters << CExtensionFilter(QObject::tr("Layer Files (lyr)"), "lyr");

		CSystemFileDialog dialog(openParams, const_cast<CLevelExplorer*>(this));
		if (dialog.Execute())
		{
			auto files = dialog.GetSelectedFiles();
			if (files.size() > 0)
			{
				for (auto& file : files)
				{
					auto pNewLayer = pLayerManager->ImportLayerFromFile(file.toStdString().c_str());
					if (pTargetLayer && pNewLayer)
						pTargetLayer->AddChild(pNewLayer);
				}
			}
		}
	}
}

void CLevelExplorer::IsolateEditability(const QModelIndex& index) const
{
	auto type = index.data((int)CLevelModel::Roles::TypeCheckRole);

	if (type.toInt() == ELevelElementType::eLevelElement_Layer)
	{
		CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();

		if (!pLayerManager || !pLayer)
			return;

		pLayerManager->ToggleFreezeAllBut(pLayer);
	}
	if (type.toInt() == ELevelElementType::eLevelElement_Object)
	{
		CBaseObject* pObject = reinterpret_cast<CBaseObject*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		IObjectManager* pObjectManager = GetIEditorImpl()->GetObjectManager();

		if (!pObjectManager || !pObject)
			return;

		pObjectManager->ToggleFreezeAllBut(pObject);
	}

	// TODO: check if necessary
	//m_treeView->update(); // push an update/redraw here
}

void CLevelExplorer::IsolateVisibility(const QModelIndex& index) const
{
	auto type = index.data((int)CLevelModel::Roles::TypeCheckRole);

	if (type.toInt() == ELevelElementType::eLevelElement_Layer)
	{
		CObjectLayer* pLayer = reinterpret_cast<CObjectLayer*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		CObjectLayerManager* pLayerManager = GetIEditorImpl()->GetObjectManager()->GetLayersManager();

		if (!pLayerManager || !pLayer)
			return;

		pLayerManager->ToggleHideAllBut(pLayer);
	}
	if (type.toInt() == ELevelElementType::eLevelElement_Object)
	{
		CBaseObject* pObject = reinterpret_cast<CBaseObject*>(index.data((int)CLevelLayerModel::Roles::InternalPointerRole).value<intptr_t>());
		IObjectManager* pObjectManager = GetIEditorImpl()->GetObjectManager();

		if (!pObjectManager || !pObject)
			return;

		pObjectManager->ToggleHideAllBut(pObject);
	}

	// TODO: check if necessary
	//m_treeView->update(); // push an update/redraw here
}

void CLevelExplorer::OnFreezeAllInLayer(CObjectLayer* layer) const
{
	CBaseObjectsArray objects;
	GetIEditorImpl()->GetObjectManager()->GetObjects(objects, layer);
	for (auto item : objects)
	{
		item->SetFrozen(true);
	}
}

void CLevelExplorer::OnUnfreezeAllInLayer(CObjectLayer* layer) const
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
		OnUnfreezeAllInLayer(layer->GetChild(i));
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

void CLevelExplorer::OnExpandAllLayers() const
{
	m_treeView->expandAll();
}

void CLevelExplorer::OnCollapseAllLayers() const
{
	m_treeView->collapseAll();
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
