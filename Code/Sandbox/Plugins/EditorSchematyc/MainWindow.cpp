// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MainWindow.h"

#include "PropertiesWidget.h"
#include "EnvBrowserWidget.h"
#include "LogWidget.h"
#include "PreviewWidget.h"
#include "ScriptBrowserWidget.h"

#include "ObjectModel.h"
#include "ComponentsWidget.h"
#include "ObjectStructureWidget.h"
#include "GraphViewWidget.h"
#include "GraphViewModel.h"
#include "VariablesWidget.h"

#include <Schematyc/Compiler/ICompiler.h>
#include <Schematyc/SerializationUtils/SerializationToString.h>
#include <Schematyc/Script/Elements/IScriptComponentInstance.h>

#include <CrySystem/File/ICryPak.h>
#include <Serialization/Qt.h>
#include <CryIcon.h>
#include <EditorFramework/Inspector.h>
#include <Controls/QuestionDialog.h>

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

namespace CVars
{
static int sc_EditorAdvanced = 0;

namespace Private
{
static uint32 g_referenceCount = 0;
}   // Private

inline void Register()
{
	if (Private::g_referenceCount++ == 0)
	{
		REGISTER_CVAR(sc_EditorAdvanced, sc_EditorAdvanced, VF_INVISIBLE, "Schematyc - Show/hide advanced options in editor");
	}
}

inline void Unregister()
{
	if (--Private::g_referenceCount == 0)
	{
		gEnv->pConsole->UnregisterVariable("sc_EditorAdvanced");
	}
}
} //CVars

static const char* g_szSaveIcon = "icons:General/File_Save.ico";
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
			Schematyc::SerializationUtils::ToString(elementType, pScriptElement->GetType());
			detailHeader.append(elementType.c_str());
		}
		detailHeader.append("]");
	}
}

REGISTER_VIEWPANE_FACTORY(CMainWindow, "Schematyc", "Tools", true)

CMainWindow::CMainWindow()
	: m_pCompileAllMenuAction(nullptr)
	, m_pRefreshEnvironmentMenuAction(nullptr)
	, m_pGraphView(nullptr)
	, m_pLog(nullptr)
	, m_pCompilerLog(nullptr)
	, m_pPreview(nullptr)
{
	CVars::Register();

	setAttribute(Qt::WA_DeleteOnClose);
	setObjectName(GetEditorName());

	QVBoxLayout* pWindowLayout = new QVBoxLayout();
	pWindowLayout->setContentsMargins(0, 0, 0, 0);
	SetContent(pWindowLayout);

	InitMenu();
	InitToolbar(pWindowLayout);

	QSplitter* pVMainContentLayout = new QSplitter(Qt::Vertical);
	QSplitter* pHMainContentLayout = new QSplitter(Qt::Horizontal);
	pHMainContentLayout->setContentsMargins(0, 0, 0, 0);
	pVMainContentLayout->addWidget(pHMainContentLayout);

	//////////////////////////////////////////////////////////////////////////

	// Script Browsers
	m_pScriptBrowser = new Schematyc::CScriptBrowserWidget(this);
	m_pScriptBrowser->InitLayout();
	pHMainContentLayout->addWidget(m_pScriptBrowser);

	// View
	QTabWidget* pViewTabs = new QTabWidget();
	pViewTabs->setTabPosition(QTabWidget::South);
	pHMainContentLayout->addWidget(pViewTabs);

	m_pGraphView = new CNodeGraphView();
	pViewTabs->addTab(m_pGraphView, "Graph");

	if (CreatePreview())
		pViewTabs->addTab(m_pPreview, "Preview");

	// Inspector
	m_pInspector = new CInspector(this);
	pHMainContentLayout->addWidget(m_pInspector);

	pWindowLayout->addWidget(pVMainContentLayout);

	// Logs
	ConfigureLogs();

	QTabWidget* pLogTabs = new QTabWidget();
	pLogTabs->setTabPosition(QTabWidget::South);

	if (CreateLog())
		pLogTabs->addTab(m_pLog, "Log");
	if (CVars::sc_EditorAdvanced)
	{
		if (CreateCompilerLog())
			pLogTabs->addTab(m_pCompilerLog, "Compiler Log");
	}

	pVMainContentLayout->addWidget(pLogTabs);

	pVMainContentLayout->setSizes(QList<int> { 850, 200 });
	pHMainContentLayout->setSizes(QList<int> { 300, 1300, 300 });

	m_pScriptBrowser->GetSelectionSignalSlots().Connect(SCHEMATYC_MEMBER_DELEGATE(&CMainWindow::OnScriptBrowserSelection, *this), m_connectionScope);

	GetIEditor()->RegisterNotifyListener(this);

	//LoadSettings();
}

CMainWindow::~CMainWindow()
{
	if (m_pScriptBrowser->HasUnsavedScriptElements())
	{
		CQuestionDialog saveDialog;
		saveDialog.SetupQuestion("Schematyc", "Save changes?");
		if (saveDialog.exec())
		{
			OnSave();
		}
	}

	SaveSettings();

	GetIEditor()->UnregisterNotifyListener(this);

	CVars::Unregister();
}

void CMainWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	switch (event)
	{
	case eNotify_OnIdleUpdate:
		{
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
		CEditor::MenuItems::FileMenu,                                                       /*CEditor::MenuItems::New,*/ CEditor::MenuItems::Save,
		CEditor::MenuItems::EditMenu,                                                       /*CEditor::MenuItems::Undo,  CEditor::MenuItems::Redo,*/
		CEditor::MenuItems::Copy,                                                           CEditor::MenuItems::Paste, CEditor::MenuItems::Delete,
		CEditor::MenuItems::ViewMenu
	};
	AddToMenu(items, sizeof(items) / sizeof(CEditor::MenuItems));

	if (CVars::sc_EditorAdvanced) // TODO: Add entry to preference page so these entries can be hidden..
	{
		CAbstractMenu* const pAdvancedMenu = GetMenu()->CreateMenu(tr("Advanced"));
		{
			m_pCompileAllMenuAction = pAdvancedMenu->CreateAction(tr("Compile All"));
			QObject::connect(m_pCompileAllMenuAction, &QAction::triggered, this, &CMainWindow::OnCompileAll);

			m_pRefreshEnvironmentMenuAction = pAdvancedMenu->CreateAction(tr("Refresh Environment"));
			QObject::connect(m_pRefreshEnvironmentMenuAction, &QAction::triggered, this, &CMainWindow::OnRefreshEnv);
		}
	}

	CAbstractMenu* const pViewMenu = GetMenu(MenuItems::ViewMenu);
	pViewMenu->signalAboutToShow.Connect([pViewMenu, this]()
	{
		pViewMenu->Clear();
	});
}

void CMainWindow::InitToolbar(QVBoxLayout* pWindowLayout)
{
	QHBoxLayout* pToolBarsLayout = new QHBoxLayout();
	pToolBarsLayout->setDirection(QBoxLayout::LeftToRight);
	pToolBarsLayout->setSizeConstraint(QLayout::SetMaximumSize);

	{
		QToolBar* pToolBar = new QToolBar("Schematyc Tools");

		{
			QAction* pAction = pToolBar->addAction(CryIcon("icons:General/File_Save.ico"), QString());
			pAction->setToolTip("Saves all changes.");
			pAction->setShortcut(QObject::tr("Ctrl+S"));
			QObject::connect(pAction, &QAction::triggered, this, &CMainWindow::OnSave);
		}

		{
			QAction* pToogleScope = pToolBar->addAction(CryIcon("icons:schematyc/toolbar_toggle_scope.ico"), QString());
			pToogleScope->setToolTip("Toggles scope in the browser to/from current object.");
			pToogleScope->setCheckable(true);
			QObject::connect(pToogleScope, &QAction::triggered, [this, pToogleScope](bool isChecked)
				{
					static bool ignoreChanges = false;
					if (!ignoreChanges && m_pScriptBrowser)
					{
					  if (!m_pScriptBrowser->SetScope(isChecked))
					  {
					    ignoreChanges = true;
					    pToogleScope->setChecked(false);
					    ignoreChanges = false;
					  }
					}
			  });
		}

		{
			m_pClearLogToolbarAction = pToolBar->addAction(CryIcon("icons:schematyc/toolbar_clear_log.ico"), QString());
			m_pClearLogToolbarAction->setToolTip("Clears log output.");
			m_pClearLogToolbarAction->setEnabled(false);
			QObject::connect(m_pClearLogToolbarAction, &QAction::triggered, this, &CMainWindow::ClearLog);
		}

		if (CVars::sc_EditorAdvanced)
		{
			m_pClearCompilerLogToolbarAction = pToolBar->addAction(CryIcon("icons:schematyc/toolbar_clear_compiler_log.ico"), QString());
			m_pClearCompilerLogToolbarAction->setToolTip("Clears compiler log output.");
			m_pClearCompilerLogToolbarAction->setEnabled(false);
			QObject::connect(m_pClearCompilerLogToolbarAction, &QAction::triggered, this, &CMainWindow::ClearCompilerLog);
		}

		{
			m_pShowLogSettingsToolbarAction = pToolBar->addAction(CryIcon("icons:schematyc/toolbar_log_settings.ico"), QString());
			m_pShowLogSettingsToolbarAction->setToolTip("Shows log settings in properties panel.");
			m_pShowLogSettingsToolbarAction->setEnabled(false);
			QObject::connect(m_pShowLogSettingsToolbarAction, &QAction::triggered, this, &CMainWindow::ShowLogSettings);
		}

		{
			m_pShowPreviewSettingsToolbarAction = pToolBar->addAction(CryIcon("icons:schematyc/toolbar_preview_settings.ico"), QString());
			m_pShowPreviewSettingsToolbarAction->setToolTip("Shows preview settings in properties panel.");
			m_pShowPreviewSettingsToolbarAction->setEnabled(false);
			QObject::connect(m_pShowPreviewSettingsToolbarAction, &QAction::triggered, this, &CMainWindow::ShowPreviewSettings);
		}

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
	gEnv->pSchematyc->GetSettingsManager().SaveAllSettings();
	// ~TODO
	gEnv->pSchematyc->GetScriptRegistry().Save();

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
	// TODO: Not yet implemented in script browser.
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

void CMainWindow::OnCompileAll()
{
	gEnv->pSchematyc->GetCompiler().CompileAll();
}

void CMainWindow::OnRefreshEnv()
{
	gEnv->pSchematyc->RefreshEnv();
}

void CMainWindow::ConfigureLogs()
{
	Schematyc::ILog& log = gEnv->pSchematyc->GetLog();

	// Log
	m_logSettings.streams.push_back(log.GetStreamName(Schematyc::LogStreamId::Default));
	m_logSettings.streams.push_back(log.GetStreamName(Schematyc::LogStreamId::Core));
	m_logSettings.streams.push_back(log.GetStreamName(Schematyc::LogStreamId::Editor));
	m_logSettings.streams.push_back(log.GetStreamName(Schematyc::LogStreamId::Env));

	// Compiler Log
	m_compilerLogSettings.streams.push_back(log.GetStreamName(Schematyc::LogStreamId::Compiler));
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
		Schematyc::IScriptGraph* pScriptGraph = selection.pScriptElement->GetExtensions().QueryExtension<Schematyc::IScriptGraph>();
		if (pScriptGraph)
		{
			if (m_pGraphView)
			{
				if (CNodeGraphViewModel* pModel = static_cast<CNodeGraphViewModel*>(m_pGraphView->GetModel()))
				{
					const QPoint pos = m_pGraphView->GetPosition();
					pModel->GetScriptGraph().SetPos(Vec2(pos.x(), pos.y()));
				}

				m_pGraphView->SetModel(new CNodeGraphViewModel(*pScriptGraph));

				const Vec2 pos = pScriptGraph->GetPos();
				m_pGraphView->SetPosition(QPoint(pos.x, pos.y));
			}
		}
		else
		{
			if (m_pGraphView)
			{
				CNodeGraphViewModel* pModel = static_cast<CNodeGraphViewModel*>(m_pGraphView->GetModel());
				if (pModel)
				{
					const QPoint pos = m_pGraphView->GetPosition();
					pModel->GetScriptGraph().SetPos(Vec2(pos.x(), pos.y()));
				}
				m_pGraphView->SetModel(nullptr);
				delete pModel;
			}
		}

		// Update properties
		if (CBroadcastManager* pBroadcastManager = CBroadcastManager::Get(this))
		{
			IDetailItem* pDetailItem = new CScriptElementDetailItem(selection.pScriptElement);
			CPropertiesWidget* pPropertiesWidget = new CPropertiesWidget(*pDetailItem, m_pPreview);

			auto populateInspector = [pPropertiesWidget](const PopulateInspectorEvent&)
			{
				return pPropertiesWidget;
			};
			PopulateInspectorEvent populateEvent(populateInspector, "Properties");
			pBroadcastManager->Broadcast(populateEvent);
		}

		// Attach to preview.
		const Schematyc::IScriptElement* pScriptClass = selection.pScriptElement;
		for (; pScriptClass && (pScriptClass->GetType() != Schematyc::EScriptElementType::Class); pScriptClass = pScriptClass->GetParent())
		{
		}
		if (pScriptClass)
		{
			m_pPreview->SetClass(static_cast<const Schematyc::IScriptClass*>(pScriptClass));
		}
		if (selection.pScriptElement->GetType() == Schematyc::EScriptElementType::ComponentInstance)
		{
			m_pPreview->SetComponentInstance(static_cast<const Schematyc::IScriptComponentInstance*>(selection.pScriptElement));
		}
		else
		{
			m_pPreview->SetComponentInstance(nullptr);
		}
	}
	else
	{
		if (m_pGraphView)
		{
			CNodeGraphViewModel* pModel = static_cast<CNodeGraphViewModel*>(m_pGraphView->GetModel());
			if (pModel)
			{
				const QPoint pos = m_pGraphView->GetPosition();
				pModel->GetScriptGraph().SetPos(Vec2(pos.x(), pos.y()));
			}
			m_pGraphView->SetModel(nullptr);
			delete pModel;
		}

		m_pPreview->SetClass(nullptr);
		m_pPreview->SetComponentInstance(nullptr);
	}
}

void CMainWindow::ClearLog()
{
	if (m_pLog)
	{
		m_pLog->Clear();
	}
}

void CMainWindow::ClearCompilerLog()
{
	if (m_pCompilerLog)
	{
		m_pCompilerLog->Clear();
	}
}

void CMainWindow::ShowLogSettings()
{
	CRY_ASSERT(m_pLog);
	if (m_pLog)
	{
		if (CBroadcastManager* pBroadcastManager = CBroadcastManager::Get(this))
		{
			auto populateInspector = [this](const PopulateInspectorEvent&)
			{
				return new Schematyc::CLogSettingsWidget(m_logSettings);
			};
			PopulateInspectorEvent populateEvent(populateInspector, "Log Settings");
			pBroadcastManager->Broadcast(populateEvent);
		}
	}
}

void CMainWindow::ShowPreviewSettings()
{
	CRY_ASSERT(m_pPreview);
	if (m_pPreview)
	{
		if (CBroadcastManager* pBroadcastManager = CBroadcastManager::Get(this))
		{
			auto populateInspector = [this](const PopulateInspectorEvent&)
			{
				return new Schematyc::CPreviewSettingsWidget(*m_pPreview);
			};
			PopulateInspectorEvent populateEvent(populateInspector, "Preview Settings");
			pBroadcastManager->Broadcast(populateEvent);
		}
	}
}

Schematyc::CLogWidget* CMainWindow::CreateLog()
{
	if (m_pLog == nullptr)
	{
		m_pLog = new Schematyc::CLogWidget(m_logSettings);
		m_pLog->InitLayout();

		m_pClearLogToolbarAction->setEnabled(true);
		m_pShowLogSettingsToolbarAction->setEnabled(true);

		return m_pLog;
	}
	return nullptr;
}

Schematyc::CLogWidget* CMainWindow::CreateCompilerLog()
{
	if (m_pCompilerLog == nullptr)
	{
		m_pCompilerLog = new Schematyc::CLogWidget(m_compilerLogSettings);
		m_pCompilerLog->InitLayout();

		m_pClearCompilerLogToolbarAction->setEnabled(true);

		return m_pCompilerLog;
	}
	return nullptr;
}

Schematyc::CPreviewWidget* CMainWindow::CreatePreview()
{
	if (m_pPreview == nullptr)
	{
		m_pPreview = new Schematyc::CPreviewWidget(this);
		m_pPreview->InitLayout();

		m_pShowPreviewSettingsToolbarAction->setEnabled(true);

		return m_pPreview;
	}
	return nullptr;
}

} // Schematyc
