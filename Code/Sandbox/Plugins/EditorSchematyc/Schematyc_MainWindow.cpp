// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_MainWindow.h"

#include "Schematyc_PropertiesWidget.h"
#include "Schematyc_EnvBrowserWidget.h"
#include "Schematyc_LogWidget.h"
#include "Schematyc_PreviewWidget.h"
#include "Schematyc_ScriptBrowserWidget.h"
#include "Schematyc_ScriptGraphView.h"
#include "Schematyc_SourceControlManager.h"

#include "Schematyc_ObjectModel.h"
#include "Schematyc_ComponentsWidget.h"
#include "Schematyc_ObjectStructureWidget.h"
#include "Schematyc_GraphViewWidget.h"
#include "Schematyc_GraphViewModel.h"
#include "Schematyc_VariablesWidget.h"

#include <Schematyc/Compiler/Schematyc_ICompiler.h>
#include <Schematyc/SerializationUtils/Schematyc_SerializationToString.h>

#include <CrySystem/File/ICryPak.h>
#include <Serialization/Qt.h>
#include <CryIcon.h>
#include <EditorFramework/Inspector.h>

#include <QAction>
#include <QMenu>
#include <QToolbar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QSplitter>
#include <QApplication>

namespace CrySchematycEditor {

static const char* g_szSaveIcon = "editor/icons/save.png";
static const char* g_szSettingsFolder = "schematyc";
static const char* g_szSettingsFile = "settings.json";

inline Schematyc::CStackString GetSettingsFolder()
{
	Schematyc::CStackString settingsFolder = GetIEditor()->GetUserFolder();
	settingsFolder.append(g_szSettingsFolder);
	return settingsFolder;
}

inline Schematyc::CStackString GetSettingsFileName()
{
	Schematyc::CStackString settingsFileName = GetSettingsFolder();
	settingsFileName.append("\\");
	settingsFileName.append(g_szSettingsFile);
	return settingsFileName;
}

inline void FormatDetailHeader(Schematyc::CStackString& detailHeader, const Schematyc::IScriptElement* pScriptElement)
{
	detailHeader = "Detail";
	if (pScriptElement)
	{
		detailHeader.append(" - ");
		detailHeader.append(pScriptElement->GetName());
		detailHeader.append(" [");
		{
			Schematyc::CStackString elementType;
			Schematyc::SerializationUtils::ToString(elementType, pScriptElement->GetElementType());
			detailHeader.append(elementType.c_str());
		}
		detailHeader.append("]");
	}
}

inline void FormatDetailHeader(Schematyc::CStackString& detailHeader, const Schematyc::CScriptGraphViewNodePtr& pGraphViewNode)
{
	detailHeader = "Detail";
	if (pGraphViewNode)
	{
		detailHeader.append(" - ");
		detailHeader.append(pGraphViewNode->GetName());
	}
}

REGISTER_VIEWPANE_FACTORY(CMainWindow, "Schematyc", "Tools", true)

CMainWindow::CMainWindow()
	: m_pSourceControlManager(new Schematyc::CSourceControlManager())
	, m_pCompileMenuAction(nullptr)
	, m_pRefreshEnvironmentMenuAction(nullptr)
	, m_pObjectModel(nullptr)
	, m_pGraph(nullptr)
	, m_pGraphView(nullptr)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setObjectName(GetEditorName());

	QVBoxLayout* pWindowLayout = new QVBoxLayout();
	pWindowLayout->setContentsMargins(1, 1, 1, 1);
	SetContent(pWindowLayout);

	InitMenu();
	InitToolbar(pWindowLayout);

	QSplitter* pEditorWidgetsSplitter = new QSplitter(Qt::Horizontal);
	pEditorWidgetsSplitter->setContentsMargins(0, 0, 0, 0);
	pWindowLayout->addWidget(pEditorWidgetsSplitter);

	// TEMP
	QWidget* pOldEditor = InitOldEditor();
	QWidget* pNewEditor = InitNewEditor();

	QTabWidget* pOldNewTab = new QTabWidget();
	pOldNewTab->addTab(pOldEditor, "Old");
	pOldNewTab->addTab(pNewEditor, "New");

	pEditorWidgetsSplitter->addWidget(pOldNewTab);
	// ~TEMP

	m_pInspector = new CInspector(this);
	pEditorWidgetsSplitter->addWidget(m_pInspector);
	pEditorWidgetsSplitter->setSizes(QList<int> { 1600, 300 });

	ConfigureLogs();
	GetIEditor()->RegisterNotifyListener(this);

	//LoadSettings();
}

CMainWindow::~CMainWindow()
{
	SaveSettings();

	GetIEditor()->UnregisterNotifyListener(this);
}

QWidget* CMainWindow::InitNewEditor()
{
	QSplitter* pContent = new QSplitter();
	pContent->setContentsMargins(0, 0, 0, 0);

	// Browser widgets
	QWidget* pLeftLayoutWidget = new QWidget();
	pContent->addWidget(pLeftLayoutWidget);
	QVBoxLayout* pLeftLayout = new QVBoxLayout();
	pLeftLayoutWidget->setLayout(pLeftLayout);

	m_pComponents = new CComponentsWidget();
	pLeftLayout->addWidget(m_pComponents);

	m_pGlobalVariables = new CVariablesWidget("Object Variables");
	pLeftLayout->addWidget(m_pGlobalVariables);

	m_pObjectStructure = new CObjectStructureWidget();
	pLeftLayout->addWidget(m_pObjectStructure);

	QObject::connect(m_pObjectStructure, &CObjectStructureWidget::SignalEntrySelected, this, &CMainWindow::OnObjectStructureSelectionChanged);

	m_pLocalVariables = new CVariablesWidget("State Variables");
	pLeftLayout->addWidget(m_pLocalVariables);
	m_pLocalVariables->ConnectToBroadcast(&GetBroadcastManager());

	return pContent;
}

QWidget* CMainWindow::InitOldEditor()
{
	QWidget* pContent = new QWidget();

	QVBoxLayout* pWindowLayout = new QVBoxLayout();
	pWindowLayout->setContentsMargins(1, 1, 1, 1);
	pContent->setLayout(pWindowLayout);

	QSplitter* pVContentLayout = new QSplitter(Qt::Vertical);
	pVContentLayout->setContentsMargins(0, 0, 0, 0);

	QSplitter* pHContentLayout = new QSplitter(Qt::Horizontal);
	pHContentLayout->setContentsMargins(0, 0, 0, 0);
	pVContentLayout->addWidget(pHContentLayout);

	// Logs
	QTabWidget* pLogTabs = new QTabWidget();
	pLogTabs->setTabPosition(QTabWidget::South);
	pVContentLayout->addWidget(pLogTabs);

	m_pLog = new Schematyc::CLogWidget(this);
	m_pLog->InitLayout();
	pLogTabs->addTab(m_pLog, "Log");
	m_pCompilerLog = new Schematyc::CLogWidget(this);
	m_pCompilerLog->InitLayout();
	pLogTabs->addTab(m_pCompilerLog, "Compiler Log");

	// Script Browsers
	m_pScriptBrowser = new Schematyc::CScriptBrowserWidget(this, *m_pSourceControlManager);
	m_pScriptBrowser->InitLayout();
	pHContentLayout->addWidget(m_pScriptBrowser);

	// View
	QTabWidget* pViewTabs = new QTabWidget();
	pViewTabs->setTabPosition(QTabWidget::South);
	pHContentLayout->addWidget(pViewTabs);

	m_pGraphView = new CNodeGraphView();
	pViewTabs->addTab(m_pGraphView, "Graph");

	m_pPreview = new Schematyc::CPreviewWidget(this);
	m_pPreview->InitLayout();
	pViewTabs->addTab(m_pPreview, "Preview");

	// View Old
	m_pGraph = new Schematyc::CScriptGraphWidget(this, AfxGetMainWnd());
	m_pGraph->InitLayout();
	pViewTabs->addTab(m_pGraph, "Old Graph");

	pVContentLayout->setSizes(QList<int> { 850, 200 });
	pHContentLayout->setSizes(QList<int> { 300, 1300 });

	pWindowLayout->addWidget(pVContentLayout);

	m_pScriptBrowser->GetSelectionSignalSlots().Connect(Schematyc::Delegate::Make(*this, &CMainWindow::OnScriptBrowserSelection), m_connectionScope);
	m_pGraph->GetSelectionSignalSlots().Connect(Schematyc::Delegate::Make(*this, &CMainWindow::OnGraphSelection), m_connectionScope);

	return pContent;
}

void CMainWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	switch (event)
	{
	case eNotify_OnIdleUpdate:
		{
			// TODO: To be removed.
			m_pSourceControlManager->Update();

			m_pPreview->Update();
			break;
		}
	}
}

void CMainWindow::Serialize(Serialization::IArchive& archive)
{
	// TODO: What is going to happen with this?
	int stateVersion = 1;
	QByteArray state;
	if (archive.isOutput())
	{
		//state = QMainWindow::saveState(stateVersion);
	}
	archive(Schematyc::SerializationUtils::ArchiveBlock(state, "state"), "mainWindow");
	if (archive.isInput() && !state.isEmpty())
	{
		//QMainWindow::restoreState(state, stateVersion);
	}

	archive(*m_pScriptBrowser, "scriptBrowser");
	//archive(*m_pDetail, "detail");
	//archive(*m_pGraph, "graph");
	//archive(*m_pLog, "log");
	//archive(*m_pPreview, "preview");
}

void CMainWindow::Show(const Schematyc::SGUID& elementGUID, const Schematyc::SGUID& detailGUID)
{
	m_pScriptBrowser->SelectItem(elementGUID);   // #SchematycTODO : What do we do with detail guid?
}

void CMainWindow::InitMenu()
{
	const CEditor::MenuItems items[] = {
		CEditor::MenuItems::FileMenu, CEditor::MenuItems::New,   CEditor::MenuItems::Save,
		CEditor::MenuItems::EditMenu, CEditor::MenuItems::Undo,  CEditor::MenuItems::Redo,
		CEditor::MenuItems::Copy,     CEditor::MenuItems::Paste, CEditor::MenuItems::Delete
	};
	AddToMenu(items, sizeof(items) / sizeof(CEditor::MenuItems));

	{
		QMenu* pFileMenu = GetMenu("File");

		// TODO: Add entry to preference page so these entries can be hidden..
		{
			pFileMenu->addSeparator();

			// TODO: Compile icon!
			m_pCompileMenuAction = pFileMenu->addAction(CryIcon("icons:General/Reload.ico"), tr("Compile"));
			QObject::connect(m_pCompileMenuAction, &QAction::triggered, this, &CMainWindow::OnCompile);
			m_pCompileMenuAction->setEnabled(false);

			m_pRefreshEnvironmentMenuAction = pFileMenu->addAction(CryIcon("icons:General/Reload.ico"), tr("Refresh Environment"));
			connect(m_pRefreshEnvironmentMenuAction, &QAction::triggered, this, &CMainWindow::OnRefreshEnv);
			//m_pRefreshEnvironmentMenuAction->setEnabled(false);
		}
	}

	QMenu* pViewMenu = GetMenu("View");
	QObject::connect(pViewMenu, &QMenu::aboutToShow, [pViewMenu, this]()
		{
			pViewMenu->clear();
	  });
}

void CMainWindow::InitToolbar(QVBoxLayout* pWindowLayout)
{
#define ADD_BUTTON(slot, name, shortcut, icon)                        \
  {                                                                   \
    QAction* pAction = pToolBar->addAction(CryIcon(icon), QString()); \
    pAction->setToolTip(name);                                        \
    if (shortcut)                                                     \
      pAction->setShortcut(tr(shortcut));                             \
    connect(pAction, &QAction::triggered, this, &slot);               \
  }

	QHBoxLayout* pToolBarsLayout = new QHBoxLayout();
	pToolBarsLayout->setDirection(QBoxLayout::LeftToRight);
	pToolBarsLayout->setSizeConstraint(QLayout::SetMaximumSize);

	{
		QToolBar* pToolBar = new QToolBar("Library");

		ADD_BUTTON(CMainWindow::OnSave, "Save Object", "Ctrl+S", "icons:General/File_Save.ico");

		pToolBarsLayout->addWidget(pToolBar, 0, Qt::AlignLeft);
	}

	pWindowLayout->addLayout(pToolBarsLayout);
}

bool CMainWindow::OnNew()
{
	// TODO: Not yet implemented in graph view.
	return true;
}

bool CMainWindow::OnSave()
{
	// TODO: This should just save the current opened object later.

	// TODO: Move settings to into a preference page.
	GetSchematycFramework().GetSettingsManager().SaveAllSettings();
	// ~TODO
	GetSchematycFramework().GetScriptRegistry().Save();

	return true;
}

bool CMainWindow::OnUndo()
{
	// TODO: Not yet implemented in graph view.
	return true;
}

bool CMainWindow::OnRedo()
{
	// TODO: Not yet implemented in graph view.
	return true;
}

bool CMainWindow::OnCopy()
{
	// TODO: Not yet implemented in graph view.
	return true;
}

bool CMainWindow::OnCut()
{
	// TODO: Not yet implemented in graph view.
	return true;
}

bool CMainWindow::OnPaste()
{
	// TODO: Not yet implemented in graph view.
	return true;
}

bool CMainWindow::OnDelete()
{
	// TODO: Not yet implemented in graph view.
	if (QApplication::focusWidget() == m_pScriptBrowser)
	{
		m_pScriptBrowser->OnRemoveItem();
	}

	return true;
}

void CMainWindow::SetLayout(const QVariantMap& state)
{
	CEditor::SetLayout(state);
}

QVariantMap CMainWindow::GetLayout() const
{
	QVariantMap state = CEditor::GetLayout();
	return state;
}

void CMainWindow::OnCompile()
{
	GetSchematycFramework().GetCompiler().CompileAll();
}

void CMainWindow::OnRefreshEnv()
{
	GetSchematycFramework().RefreshEnv();
}

void CMainWindow::ConfigureLogs()
{
	Schematyc::ILog& log = GetSchematycFramework().GetLog();

	{
		Schematyc::SLogSettings settings;
		settings.streams.push_back(log.GetStreamName(Schematyc::g_defaultLogStreamId));
		settings.streams.push_back(log.GetStreamName(Schematyc::g_coreLogStreamId));
		settings.streams.push_back(log.GetStreamName(Schematyc::g_editorLogStreamId));
		settings.streams.push_back(log.GetStreamName(Schematyc::g_envLogStreamId));
		m_pLog->ApplySettings(settings);
	}

	{
		Schematyc::SLogSettings settings;
		settings.streams.push_back(log.GetStreamName(Schematyc::g_compilerLogStreamId));
		m_pCompilerLog->ApplySettings(settings);
	}

	m_pLog->ShowSettings(true);
}

void CMainWindow::LoadSettings()
{
	const Schematyc::CStackString settingsFileName = GetSettingsFileName();
	Serialization::LoadJsonFile(*this, settingsFileName.c_str());
}

void CMainWindow::SaveSettings()
{
	{
		const Schematyc::CStackString szSettingsFolder = GetSettingsFolder();
		gEnv->pCryPak->MakeDir(szSettingsFolder.c_str());
	}
	{
		const Schematyc::CStackString settingsFileName = GetSettingsFileName();
		Serialization::SaveJsonFile(settingsFileName.c_str(), *this);
	}
}

void CMainWindow::OnScriptBrowserSelection(const Schematyc::SScriptBrowserSelection& selection)
{
	if (selection.pScriptElement)
	{
		if (CBroadcastManager* pBroadcastManager = CBroadcastManager::Get(this))
		{
			IDetailItem* pDetailItem = new CScriptElementDetailItem(selection.pScriptElement);
			CPropertiesWidget* pPropertiesWidget = new CPropertiesWidget(*pDetailItem);
			QObject::connect(pPropertiesWidget, &CPropertiesWidget::SignalPropertyChanged, this, &CMainWindow::OnDetailModification);

			auto populateInspector = [pPropertiesWidget](const PopulateInspectorEvent&)
			{
				return pPropertiesWidget;
			};
			PopulateInspectorEvent populateEvent(populateInspector, "Properties");
			pBroadcastManager->Broadcast(populateEvent);
		}

		Schematyc::IScriptGraph* pScriptGraph = selection.pScriptElement->GetExtensions().QueryExtension<Schematyc::IScriptGraph>();
		if (pScriptGraph)
		{
			m_pGraphView->SetModel(new CNodeGraphViewModel(*pScriptGraph));
			m_pGraph->LoadGraph(pScriptGraph);
		}
		else
		{
			CNodeGraphViewModel* pModel = static_cast<CNodeGraphViewModel*>(m_pGraphView->GetModel());
			m_pGraphView->SetModel(nullptr);
			delete pModel;

			m_pGraph->LoadGraph(nullptr);
		}

		if (selection.pScriptElement->GetElementType() == Schematyc::EScriptElementType::Class)
		{
			Schematyc::IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(selection.pScriptElement->GetGUID());
			if (pScriptView)
			{
				// TODO: Temp solution.
				if (m_pObjectModel)
				{
					m_pComponents->SetModel(nullptr);
					m_pObjectStructure->SetModel(nullptr);
					m_pGlobalVariables->SetModel(nullptr);
					delete m_pObjectModel;
				}
				// ~TODO

				m_pObjectModel = new CObjectModel(*pScriptView);
				m_pComponents->SetModel(m_pObjectModel);
				m_pObjectStructure->SetModel(m_pObjectModel);
				m_pGlobalVariables->SetModel(m_pObjectModel);
			}
		}

		// Find parent class and attach to preview.
		const Schematyc::IScriptElement* pScriptClass = selection.pScriptElement;
		for (; pScriptClass && (pScriptClass->GetElementType() != Schematyc::EScriptElementType::Class); pScriptClass = pScriptClass->GetParent())
		{
		}
		if (pScriptClass)
		{
			m_pPreview->SetClass(static_cast<const Schematyc::IScriptClass*>(pScriptClass));
		}
	}
	else
	{
		CNodeGraphViewModel* pModel = static_cast<CNodeGraphViewModel*>(m_pGraphView->GetModel());
		m_pGraphView->SetModel(nullptr);
		delete pModel;
		m_pGraph->LoadGraph(nullptr);
		m_pPreview->SetClass(nullptr);
	}
}

void CMainWindow::OnDetailModification(IDetailItem* pDetailItem)
{
	//CNodeGraphViewModel* pModel = static_cast<CNodeGraphViewModel*>(m_pGraphView->GetModel());
	//m_pGraphView->SetModel(nullptr);
	//delete pModel;

	m_pGraph->Refresh();

	m_pPreview->Reset(); // #SchematycTODO : Also need to reset when selected class changes.
}

void CMainWindow::OnGraphSelection(const Schematyc::SScriptGraphViewSelection& selection)
{
	if (selection.pNode)
	{
		if (CBroadcastManager* pBroadcastManager = CBroadcastManager::Get(this))
		{
			IDetailItem* pDetailItem = new CScriptGraphViewNodeDetailItem(selection.pNode);
			CPropertiesWidget* pPropertiesWidget = new CPropertiesWidget(*pDetailItem);
			QObject::connect(pPropertiesWidget, &CPropertiesWidget::SignalPropertyChanged, this, &CMainWindow::OnDetailModification);

			auto populateInspector = [pPropertiesWidget](const PopulateInspectorEvent&)
			{
				return pPropertiesWidget;
			};
			PopulateInspectorEvent populateEvent(populateInspector, "Properties");
			pBroadcastManager->Broadcast(populateEvent);
		}
	}
}

void CMainWindow::OnObjectStructureSelectionChanged(CAbstractObjectStructureModelItem& item)
{
	if (item.GetType() == eObjectItemType_Graph)
	{
		CGraphItem& graphItem = static_cast<CGraphItem&>(item);
		m_pGraphView->SetModel(static_cast<CryGraphEditor::CNodeGraphViewModel*>(graphItem.GetGraphModel()));
	}
	else
	{
		m_pGraphView->SetModel(nullptr);
	}
}

} // Schematyc
