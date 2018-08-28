// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MainWindow.h"

#include "TreePanel.h"

#include <IEditor.h>

#include <CrySerialization/IArchive.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySystem/File/ICryPak.h>
#include <Serialization/Qt.h>

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenuBar>

#define APPLICATION_USER_DIRECTORY                   "/interim_behavior_tree_editor/"
#define APPLICATION_USER_STATE_FILEPATH              APPLICATION_USER_DIRECTORY "editor_state.json"

#define APPLICATION_MAX_FILES_IN_RECENT_FILE_HISTORY (10)

MainWindow::MainWindow()
	: m_treePanel(nullptr)
	, m_pRecentFilesMenu()
	, m_pShowXmlLineNumberMenuAction()
	, m_pShowCommentsMenuAction()
	, m_pEnableCryEngineSignalsMenuAction()
	, m_pEnableGameSDKSignalsMenuAction()
	, m_pEnableDeprecatedSignalsMenuAction()
	, m_recentFileNamesHistory()
{
	// CVar callbacks
	m_modularBehaviorTreeDebugTreeCvarHandle = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTreeCvar)->AddOnChange(std::bind(&MainWindow::OnChanged_ShowVariablesDebugger, this));
	m_modularBehaviorTreeDebugVariablesCvarHandle = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugVariablesCvar)->AddOnChange(std::bind(&MainWindow::OnChanged_ShowVariablesDebugger, this));
	m_modularBehaviorTreeDebugTimestampsCvarHandle = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTimestampsCvar)->AddOnChange(std::bind(&MainWindow::OnChanged_ShowTimestampsDebugger, this));
	m_modularBehaviorTreeDebugEventsCvarHandle = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugEventsCvar)->AddOnChange(std::bind(&MainWindow::OnChanged_ShowEventsDebugger, this));
	m_modularBehaviorTreeDebugLogCvarHandle = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugLogCvar)->AddOnChange(std::bind(&MainWindow::OnChanged_ShowLogDebugger, this));
	m_modularBehaviorTreeStatisticsCvarHandle = gEnv->pConsole->GetCVar(k_modularBehaviorTreeStatisticsCvar)->AddOnChange(std::bind(&MainWindow::OnChanged_ShowStatistics, this));
	m_modularBehaviorTreeDebugExecutionLogCvarHandle = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugExecutionLogCvar)->AddOnChange(std::bind(&MainWindow::OnChanged_ExecutionHistoryLog, this));
	
	// Qt

	setWindowFlags(Qt::Widget);

	QMenuBar* menu = new QMenuBar(this);

	// File menu

	QMenu* fileMenu = menu->addMenu("&File");
	connect(fileMenu->addAction("&New"), &QAction::triggered, this, &MainWindow::OnMenuActionNew);
	fileMenu->addSeparator();
	connect(fileMenu->addAction("&Open"), &QAction::triggered, this, &MainWindow::OnMenuActionOpen);
	connect(fileMenu->addAction("&Save"), &QAction::triggered, this, &MainWindow::OnMenuActionSave);
	connect(fileMenu->addAction("Save &as..."), &QAction::triggered, this, &MainWindow::OnMenuActionSaveToFile);
	fileMenu->addSeparator();
	m_pRecentFilesMenu.reset(fileMenu->addMenu("&Recent Files"));

	// View menu

	QMenu* viewMenu = menu->addMenu("&View");
	m_pShowXmlLineNumberMenuAction.reset(viewMenu->addAction("&Show XML line numbers"));
	m_pShowXmlLineNumberMenuAction->setCheckable(true);
	connect(m_pShowXmlLineNumberMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowXmlLineNumbers);

	m_pShowCommentsMenuAction.reset(viewMenu->addAction("Show &comments"));
	m_pShowCommentsMenuAction->setCheckable(true);
	connect(m_pShowCommentsMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowComments);

	viewMenu->addSeparator();
	QMenu* signalsMenu = viewMenu->addMenu("&Built-in Events");

	m_pEnableCryEngineSignalsMenuAction.reset(signalsMenu->addAction("Enable Cry&Engine events"));
	m_pEnableCryEngineSignalsMenuAction->setCheckable(true);
	connect(m_pEnableCryEngineSignalsMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowCryEngineSignals);
	
	m_pEnableGameSDKSignalsMenuAction.reset(signalsMenu->addAction("Enable &GameSDK events"));
	m_pEnableGameSDKSignalsMenuAction->setCheckable(true);
	connect(m_pEnableGameSDKSignalsMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowGameSDKSignals);

	m_pEnableDeprecatedSignalsMenuAction.reset(signalsMenu->addAction("Enable &Deprecated events"));
	m_pEnableDeprecatedSignalsMenuAction->setCheckable(true);
	connect(m_pEnableDeprecatedSignalsMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowDeprecatedSignals);
	
	// Debug menu
	QMenu* debugMenu = menu->addMenu("&Debug");

	m_pEnableTreeDebuggerMenuAction.reset(debugMenu->addAction("Show &Tree"));
	m_pEnableTreeDebuggerMenuAction->setCheckable(true);
	m_pEnableTreeDebuggerMenuAction->setChecked(GetShowTreeDebuggerCvar());
	connect(m_pEnableTreeDebuggerMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowTreeDebugger);

	m_pEnableVariablesDebuggerMenuAction.reset(debugMenu->addAction("Show &Variables"));
	m_pEnableVariablesDebuggerMenuAction->setCheckable(true);
	m_pEnableVariablesDebuggerMenuAction->setChecked(GetShowVariablesDebuggerCvar());
	connect(m_pEnableVariablesDebuggerMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowVariablesDebugger);

	m_pEnableTimestampsDebuggerMenuAction.reset(debugMenu->addAction("Show &Timestamps"));
	m_pEnableTimestampsDebuggerMenuAction->setCheckable(true);
	m_pEnableTimestampsDebuggerMenuAction->setChecked(GetShowTimestampsDebuggerCvar());
	connect(m_pEnableTimestampsDebuggerMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowTimestampsDebugger);

	m_pEnableEventsDebuggerMenuAction.reset(debugMenu->addAction("Show &Events"));
	m_pEnableEventsDebuggerMenuAction->setCheckable(true);
	m_pEnableEventsDebuggerMenuAction->setChecked(GetShowEventsDebuggerCvar());
	connect(m_pEnableEventsDebuggerMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowEventsDebugger);

	m_pEnableLogDebuggerMenuAction.reset(debugMenu->addAction("Show &Log"));
	m_pEnableLogDebuggerMenuAction->setCheckable(true);
	m_pEnableLogDebuggerMenuAction->setChecked(GetShowLogDebuggerCvar());
	connect(m_pEnableLogDebuggerMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowLogDebugger);

	debugMenu->addSeparator();

	m_pEnableAllDebuggerMenuAction.reset(debugMenu->addAction("Show &All"));
	m_pEnableAllDebuggerMenuAction->setCheckable(true);
	m_pEnableAllDebuggerMenuAction->setChecked(GetShowAllDebugger());
	connect(m_pEnableAllDebuggerMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowAllDebugger);

	debugMenu->addSeparator();

	m_pEnableStatisticsMenuAction.reset(debugMenu->addAction("Show &Statistics"));
	m_pEnableStatisticsMenuAction->setCheckable(true);
	m_pEnableStatisticsMenuAction->setChecked(GetShowStatisticsCvar());
	connect(m_pEnableStatisticsMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionShowStatistics);

	debugMenu->addSeparator();

	// Log menu
	QMenu* executionHistoryLogMenu = debugMenu->addMenu("Execution History");

	m_pEnableExecutionHistoryLogCurrentEntityMenuAction.reset(executionHistoryLogMenu->addAction("Log execution for &current agent"));
	m_pEnableExecutionHistoryLogCurrentEntityMenuAction->setCheckable(true);
	m_pEnableExecutionHistoryLogCurrentEntityMenuAction->setChecked(GetEnableExecutionHistoryLogCvar() == 1);
	connect(m_pEnableExecutionHistoryLogCurrentEntityMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionEnableExecutionHistoryLogForCurrentEntity);

	m_pEnableExecutionHistoryLogAllEntitiesMenuAction.reset(executionHistoryLogMenu->addAction("Log execution for &all agents"));
	m_pEnableExecutionHistoryLogAllEntitiesMenuAction->setCheckable(true);
	m_pEnableExecutionHistoryLogAllEntitiesMenuAction->setChecked(GetEnableExecutionHistoryLogCvar() == 2);
	connect(m_pEnableExecutionHistoryLogAllEntitiesMenuAction.data(), &QAction::triggered, this, &MainWindow::OnMenuActionEnableExecutionHistoryLogForAllEntities);

	setMenuBar(menu);

	GetIEditor()->RegisterNotifyListener(this);

	m_treePanel = new TreePanel	(this);
	setCentralWidget(m_treePanel);

	LoadState();
}

MainWindow::~MainWindow()
{
	GetIEditor()->UnregisterNotifyListener(this);

	// CVar callbacks remove
	gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTreeCvar)->RemoveOnChangeFunctor(m_modularBehaviorTreeDebugTreeCvarHandle);
	gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugVariablesCvar)->RemoveOnChangeFunctor(m_modularBehaviorTreeDebugVariablesCvarHandle);
	gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTimestampsCvar)->RemoveOnChangeFunctor(m_modularBehaviorTreeDebugTimestampsCvarHandle);
	gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugEventsCvar)->RemoveOnChangeFunctor(m_modularBehaviorTreeDebugEventsCvarHandle);
	gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugLogCvar)->RemoveOnChangeFunctor(m_modularBehaviorTreeDebugLogCvarHandle);
	gEnv->pConsole->GetCVar(k_modularBehaviorTreeStatisticsCvar)->RemoveOnChangeFunctor(m_modularBehaviorTreeStatisticsCvarHandle);
	gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugExecutionLogCvar)->RemoveOnChangeFunctor(m_modularBehaviorTreeDebugExecutionLogCvarHandle);
}

const char* MainWindow::GetPaneTitle() const
{
	return "Interim Behavior Tree Editor";
}

void MainWindow::OnEditorNotifyEvent(EEditorNotifyEvent editorNotifyEvent)
{
	if (editorNotifyEvent == eNotify_OnQuit)
	{
		SaveState();
	}
}

void MainWindow::Serialize(Serialization::IArchive& archive)
{
	int windowStateVersion = 1;

	QByteArray windowState;
	if (archive.isOutput())
	{
		windowState = saveState(windowStateVersion);
	}
	archive(windowState, "windowState");
	if (archive.isInput() && !windowState.isEmpty())
	{
		restoreState(windowState, windowStateVersion);
	}

	archive(m_recentFileNamesHistory, "recentFileNamesHistory");
	RebuildRecentFilesMenu();

	bool showXmlLineNumbers = GetShowXmlLineNumbers();
	archive(showXmlLineNumbers, "showXmlLineNumbers");
	SetShowXmlLineNumbers(showXmlLineNumbers);

	bool showComments = GetShowComments();
	archive(showComments, "showComments");
	SetShowComments(showComments);
	
	// Events

	bool enableCryEngineSignals = GetEnableCryEngineSignals();
	archive(enableCryEngineSignals, "enableCryEngineSignals");
	SetEnableCryEngineSignals(enableCryEngineSignals);

	bool enableGameSDKSignals = GetEnableGameSDKSignals();
	archive(enableGameSDKSignals, "enableGameSDKSignals");
	SetEnableGameSDKSignals(enableGameSDKSignals);

	bool enableDeprecatedSignals = GetEnableDeprecatedSignals();
	archive(enableDeprecatedSignals, "enableDeprecatedSignals");
	SetEnableDeprecatedSignals(enableDeprecatedSignals);

	// Debug
	bool showTreeDebugger = GetShowTreeDebuggerCvar();
	archive(showTreeDebugger);

	bool showVariablesDebugger = GetShowVariablesDebuggerCvar();
	archive(showVariablesDebugger);

	bool showTimestampsDebugger = GetShowTimestampsDebuggerCvar();
	archive(showTimestampsDebugger);

	bool showEventsDebugger = GetShowEventsDebuggerCvar();
	archive(showEventsDebugger);

	bool showLogDebugger = GetShowLogDebuggerCvar();
	archive(showLogDebugger);

	bool showAllDebugger = GetShowAllDebugger();
	archive(showAllDebugger);

	bool showStatistics = GetShowStatisticsCvar();
	archive(showStatistics);

	int enableExecutionHistoryLog = GetEnableExecutionHistoryLogCvar();
	bool enableExecutionHistoryLogCurrentEntity = enableExecutionHistoryLog == 1;
	bool enableExecutionHistoryLogAllEntities = enableExecutionHistoryLog == 2;

	archive(enableExecutionHistoryLogCurrentEntity);
	archive(enableExecutionHistoryLogAllEntities);

	if (archive.isOutput())
	{
		SetShowTreeDebugger(showTreeDebugger);
		SetShowVariablesDebugger(showVariablesDebugger);
		SetShowTimestampsDebugger(showTimestampsDebugger);
		SetShowEventsDebugger(showEventsDebugger);
		SetShowLogDebugger(showTimestampsDebugger);
		SetShowAllDebugger(showAllDebugger);
		SetShowStatisticsCvar(showStatistics);

		if (enableExecutionHistoryLogCurrentEntity)
		{
			SetEnableExecutionHistoryLog(1);
		}
		else if (enableExecutionHistoryLogAllEntities)
		{
			SetEnableExecutionHistoryLog(2);
		}
		else
		{
			SetEnableExecutionHistoryLog(0);
		}
	}
}

void MainWindow::PromoteFileInRecentFileHistory(const char* szFileName)
{
	stl::find_and_erase_all(m_recentFileNamesHistory, szFileName);

	std::vector<string> newFileNameHistory;
	newFileNameHistory.reserve(m_recentFileNamesHistory.size() + 1);
	newFileNameHistory.push_back(szFileName);
	newFileNameHistory.insert(newFileNameHistory.end(), m_recentFileNamesHistory.begin(), m_recentFileNamesHistory.end());

	if (newFileNameHistory.size() > APPLICATION_MAX_FILES_IN_RECENT_FILE_HISTORY)
	{
		newFileNameHistory.resize(APPLICATION_MAX_FILES_IN_RECENT_FILE_HISTORY);
	}

	m_recentFileNamesHistory = newFileNameHistory;

	RebuildRecentFilesMenu();
}

void MainWindow::closeEvent(QCloseEvent* closeEvent)
{
	SaveState();

	if (!m_treePanel->HandleCloseEvent())
	{
		closeEvent->ignore();
		return;
	}

	QMainWindow::closeEvent(closeEvent);
}

void MainWindow::OnMenuActionNew()
{
	m_treePanel->OnWindowEvent_NewFile();
}

void MainWindow::OnMenuActionOpen()
{
	m_treePanel->OnWindowEvent_OpenFile();
}

void MainWindow::OnMenuActionSave()
{
	m_treePanel->OnWindowEvent_Save();
}

void MainWindow::OnMenuActionSaveToFile()
{
	m_treePanel->OnWindowEvent_SaveToFile();
}

void MainWindow::OnMenuActionRecentFile()
{
	QObject* pSenderObject = QObject::sender();
	if (pSenderObject == nullptr)
	{
		return;
	}
	QAction* pSenderAction = qobject_cast<QAction*>(pSenderObject);
	if (pSenderAction == nullptr)
	{
		return;
	}

	m_treePanel->OnWindowEvent_OpenRecentFile(pSenderAction->data().toString());
}

void MainWindow::OnMenuActionShowXmlLineNumbers()
{
	m_treePanel->OnWindowEvent_ShowXmlLineNumbers(GetShowXmlLineNumbers());
}

void MainWindow::OnMenuActionShowComments()
{
	m_treePanel->OnWindowEvent_ShowComments(GetShowComments());
}

void MainWindow::OnMenuActionShowCryEngineSignals()
{
	m_treePanel->OnWindowEvent_EnableCryEngineSignals(GetEnableCryEngineSignals());
}

void MainWindow::OnMenuActionShowGameSDKSignals()
{
	m_treePanel->OnWindowEvent_EnableGameSDKSignals(GetEnableGameSDKSignals());
}

void MainWindow::OnMenuActionShowDeprecatedSignals()
{
	m_treePanel->OnWindowEvent_EnableDeprecatedSignals(GetEnableDeprecatedSignals());
}

void MainWindow::OnMenuActionShowTreeDebugger()
{
	SetShowTreeDebugger(!GetShowTreeDebuggerCvar());
}

void MainWindow::OnMenuActionShowVariablesDebugger()
{
	SetShowVariablesDebugger(!GetShowVariablesDebuggerCvar());
}

void MainWindow::OnMenuActionShowTimestampsDebugger()
{
	SetShowTimestampsDebugger(!GetShowTimestampsDebuggerCvar());
}

void MainWindow::OnMenuActionShowEventsDebugger()
{
	SetShowEventsDebugger(!GetShowEventsDebuggerCvar());
}

void MainWindow::OnMenuActionShowLogDebugger()
{
	SetShowLogDebugger(!GetShowLogDebuggerCvar());
}

void MainWindow::OnMenuActionShowAllDebugger()
{
	const bool isEnabled = GetShowAllDebugger();

	if (isEnabled)
	{
		SetShowTreeDebugger(true);
		SetShowVariablesDebugger(true);
		SetShowTimestampsDebugger(true);
		SetShowEventsDebugger(true);
		SetShowLogDebugger(true);
	}
	else if (GetShowTreeDebuggerCvar()
		&& GetShowVariablesDebuggerCvar()
		&& GetShowTimestampsDebuggerCvar()
		&& GetShowEventsDebuggerCvar()
		&& GetShowLogDebuggerCvar())
	{
		SetShowTreeDebugger(false);
		SetShowVariablesDebugger(false);
		SetShowTimestampsDebugger(false);
		SetShowEventsDebugger(false);
		SetShowLogDebugger(false);
	}
}

void MainWindow::OnMenuActionShowStatistics()
{
	SetShowStatisticsCvar(!GetShowStatisticsCvar());
}

void MainWindow::OnMenuActionEnableExecutionHistoryLogForCurrentEntity()
{
	const int previousValue = GetEnableExecutionHistoryLogCvar();

	if (previousValue == 0 || previousValue == 2)
	{
		SetEnableExecutionHistoryLog(1);
	}
	else if (previousValue == 1)
	{
		SetEnableExecutionHistoryLog(0);
	}
}

void MainWindow::OnMenuActionEnableExecutionHistoryLogForAllEntities()
{
	const int previousValue = GetEnableExecutionHistoryLogCvar();

	if (previousValue == 0 || previousValue == 1)
	{
		SetEnableExecutionHistoryLog(2);
	}
	else if (previousValue == 2)
	{
		SetEnableExecutionHistoryLog(0);
	}
}

void MainWindow::LoadState()
{
	const stack_string loadFilePath = stack_string().Format("%s%s", GetIEditor()->GetUserFolder(), APPLICATION_USER_STATE_FILEPATH).c_str();

	Serialization::LoadJsonFile(*this, loadFilePath.c_str());
}

void MainWindow::SaveState()
{
	const stack_string saveFilePath = stack_string().Format("%s%s", GetIEditor()->GetUserFolder(), APPLICATION_USER_STATE_FILEPATH).c_str();

	// Create directory first in case it does not exist.
	QDir().mkdir(stack_string().Format("%s%s", GetIEditor()->GetUserFolder(), APPLICATION_USER_DIRECTORY).c_str());

	Serialization::SaveJsonFile(saveFilePath.c_str(), *this);
}

void MainWindow::RebuildRecentFilesMenu()
{
	if (!m_pRecentFilesMenu)
	{
		return;
	}

	m_pRecentFilesMenu->clear();
	for (auto fileName : m_recentFileNamesHistory)
	{
		QAction* pAction = m_pRecentFilesMenu->addAction(fileName.c_str());
		pAction->setData(QVariant(fileName.c_str()));

		connect(pAction, &QAction::triggered, this, &MainWindow::OnMenuActionRecentFile);
	}
}

bool MainWindow::GetShowXmlLineNumbers() const
{
	if (!m_pShowXmlLineNumberMenuAction)
	{
		return false;
	}

	return m_pShowXmlLineNumberMenuAction->isChecked();
}

void MainWindow::SetShowXmlLineNumbers(const bool showFlag)
{
	if (!m_pShowXmlLineNumberMenuAction)
	{
		return;
	}

	m_pShowXmlLineNumberMenuAction->setChecked(showFlag);

	m_treePanel->ShowXmlLineNumbers(showFlag);
}

bool MainWindow::GetShowComments() const
{
	if (!m_pShowCommentsMenuAction)
	{
		return false;
	}

	return m_pShowCommentsMenuAction->isChecked();
}

void MainWindow::SetShowComments(const bool showFlag)
{
	if (!m_pShowCommentsMenuAction)
	{
		return;
	}

	m_pShowCommentsMenuAction->setChecked(showFlag);

	m_treePanel->ShowComments(showFlag);
}

bool MainWindow::GetEnableCryEngineSignals() const
{
	if (!m_pEnableCryEngineSignalsMenuAction)
	{
		return false;
	}

	return m_pEnableCryEngineSignalsMenuAction->isChecked();
}

void MainWindow::SetEnableCryEngineSignals(const bool enable)
{
	if (!m_pEnableCryEngineSignalsMenuAction)
	{
		return;
	}

	m_pEnableCryEngineSignalsMenuAction->setChecked(enable);

	m_treePanel->EnableCryEngineSignals(enable);
}

bool MainWindow::GetEnableGameSDKSignals() const
{
	if (!m_pEnableGameSDKSignalsMenuAction)
	{
		return false;
	}

	return m_pEnableGameSDKSignalsMenuAction->isChecked();
}

void MainWindow::SetEnableGameSDKSignals(const bool enable)
{
	if (!m_pEnableGameSDKSignalsMenuAction)
	{
		return;
	}

	m_pEnableGameSDKSignalsMenuAction->setChecked(enable);

	m_treePanel->EnableGameSDKSignals(enable);
}

bool MainWindow::GetEnableDeprecatedSignals() const
{
	if (!m_pEnableDeprecatedSignalsMenuAction)
	{
		return false;
	}

	return m_pEnableDeprecatedSignalsMenuAction->isChecked();
}

void MainWindow::SetEnableDeprecatedSignals(const bool enableFlag)
{
	if (!m_pEnableDeprecatedSignalsMenuAction)
	{
		return;
	}

	m_pEnableDeprecatedSignalsMenuAction->setChecked(enableFlag);

	m_treePanel->EnableDeprecatedSignals(enableFlag);
}

bool MainWindow::GetShowTreeDebuggerCvar() const
{
	if (!m_pEnableTreeDebuggerMenuAction)
	{
		return false;
	}

	const ICVar* pDebugTree = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTreeCvar);
	return pDebugTree->GetIVal();
}

void MainWindow::SetShowTreeDebugger(const bool enableFlag)
{
	if (!m_pEnableTreeDebuggerMenuAction)
	{
		return;
	}

	m_pEnableTreeDebuggerMenuAction->setChecked(enableFlag);

	ICVar* pDebugTree = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTreeCvar);
	pDebugTree->Set(enableFlag);

	if (!enableFlag)
	{
		SetShowAllDebugger(false);
	}
}

void MainWindow::OnChanged_ShowTreeDebugger()
{
	const ICVar* pDebugTree = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTreeCvar);
	const bool debugTreeEnabled = pDebugTree->GetIVal();

	if (debugTreeEnabled != m_pEnableVariablesDebuggerMenuAction->isChecked())
	{
		m_pEnableTreeDebuggerMenuAction->setChecked(debugTreeEnabled);
	}
}

bool MainWindow::GetShowVariablesDebuggerCvar() const
{
	if (!m_pEnableVariablesDebuggerMenuAction)
	{
		return false;
	}

	const ICVar* pDebugVariables = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugVariablesCvar);
	return pDebugVariables->GetIVal();
}

void MainWindow::SetShowVariablesDebugger(const bool enableFlag)
{
	if (!m_pEnableVariablesDebuggerMenuAction)
	{
		return;
	}

	m_pEnableVariablesDebuggerMenuAction->setChecked(enableFlag);

	ICVar* pDebugVariables = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugVariablesCvar);
	pDebugVariables->Set(enableFlag);

	if (!enableFlag)
	{
		SetShowAllDebugger(false);
	}
}

void MainWindow::OnChanged_ShowVariablesDebugger()
{
	const ICVar* pDebugVariables = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugVariablesCvar);
	const bool debugVariablesEnabled= pDebugVariables->GetIVal();

	if (debugVariablesEnabled != m_pEnableVariablesDebuggerMenuAction->isChecked())
	{
		m_pEnableVariablesDebuggerMenuAction->setChecked(debugVariablesEnabled);
	}
}

bool MainWindow::GetShowTimestampsDebuggerCvar() const
{
	if (!m_pEnableTimestampsDebuggerMenuAction)
	{
		return false;
	}

	const ICVar* pDebugTimestamps = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTimestampsCvar);
	return pDebugTimestamps->GetIVal();
}

void MainWindow::SetShowTimestampsDebugger(const bool enableFlag)
{
	if (!m_pEnableTimestampsDebuggerMenuAction)
	{
		return;
	}

	m_pEnableTimestampsDebuggerMenuAction->setChecked(enableFlag);

	ICVar* pDebugTimestamps = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTimestampsCvar);
	pDebugTimestamps->Set(enableFlag);

	if (!enableFlag)
	{
		SetShowAllDebugger(false);
	}
}

void MainWindow::OnChanged_ShowTimestampsDebugger()
{
	const ICVar* pDebugTimestamps = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugTimestampsCvar);
	const bool debugTimestampsEnabled = pDebugTimestamps->GetIVal();

	if (debugTimestampsEnabled != m_pEnableTimestampsDebuggerMenuAction->isChecked())
	{
		m_pEnableTimestampsDebuggerMenuAction->setChecked(debugTimestampsEnabled);
	}
}

bool MainWindow::GetShowEventsDebuggerCvar() const
{
	if (!m_pEnableEventsDebuggerMenuAction)
	{
		return false;
	}

	const ICVar* pDebugEvents = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugEventsCvar);
	return pDebugEvents->GetIVal();
}

void MainWindow::SetShowEventsDebugger(const bool enableFlag)
{
	if (!m_pEnableEventsDebuggerMenuAction)
	{
		return;
	}

	m_pEnableEventsDebuggerMenuAction->setChecked(enableFlag);

	ICVar* pDebugEvents = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugEventsCvar);
	pDebugEvents->Set(enableFlag);

	if (!enableFlag)
	{
		SetShowAllDebugger(false);
	}
}

void MainWindow::OnChanged_ShowEventsDebugger()
{
	const ICVar* pDebugEvents = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugEventsCvar);
	const bool debugEventsEnabled = pDebugEvents->GetIVal();

	if (debugEventsEnabled != m_pEnableEventsDebuggerMenuAction->isChecked())
	{
		m_pEnableEventsDebuggerMenuAction->setChecked(debugEventsEnabled);
	}
}

bool MainWindow::GetShowLogDebuggerCvar() const
{
	if (!m_pEnableLogDebuggerMenuAction)
	{
		return false;
	}

	const ICVar* pDebugLog = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugLogCvar);
	return pDebugLog->GetIVal();
}

void MainWindow::SetShowLogDebugger(const bool enableFlag)
{
	if (!m_pEnableLogDebuggerMenuAction)
	{
		return;
	}

	m_pEnableLogDebuggerMenuAction->setChecked(enableFlag);

	ICVar* pDebugLog = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugLogCvar);
	pDebugLog->Set(enableFlag);

	if (!enableFlag)
	{
		SetShowAllDebugger(false);
	}
}

void MainWindow::OnChanged_ShowLogDebugger()
{
	const ICVar* pDebugLog = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugLogCvar);
	const bool debugLogEnabled = pDebugLog->GetIVal();

	if (debugLogEnabled != m_pEnableLogDebuggerMenuAction->isChecked())
	{
		m_pEnableLogDebuggerMenuAction->setChecked(debugLogEnabled);
	}
}

bool MainWindow::GetShowAllDebugger() const
{
	if (!m_pEnableAllDebuggerMenuAction)
	{
		return false;
	}

	return m_pEnableAllDebuggerMenuAction->isChecked();
}

void MainWindow::SetShowAllDebugger(const bool enableFlag)
{
	if (!m_pEnableStatisticsMenuAction)
	{
		return;
	}

	m_pEnableStatisticsMenuAction->setChecked(enableFlag);
}

bool MainWindow::GetShowStatisticsCvar() const
{
	if (!m_pEnableStatisticsMenuAction)
	{
		return false;
	}

	const ICVar* pDebugStatistics = gEnv->pConsole->GetCVar(k_modularBehaviorTreeStatisticsCvar);
	return pDebugStatistics->GetIVal();
}

void MainWindow::SetShowStatisticsCvar(const bool enableFlag)
{
	if (!m_pEnableStatisticsMenuAction)
	{
		return;
	}

	m_pEnableStatisticsMenuAction->setChecked(enableFlag);

	ICVar* pDebugStatistics = gEnv->pConsole->GetCVar(k_modularBehaviorTreeStatisticsCvar);
	pDebugStatistics->Set(enableFlag);
}

void MainWindow::OnChanged_ShowStatistics()
{
	const ICVar* pDebugStatistics = gEnv->pConsole->GetCVar(k_modularBehaviorTreeStatisticsCvar);
	const bool debugStatisticsEnabled = pDebugStatistics->GetIVal();

	if (debugStatisticsEnabled != m_pEnableStatisticsMenuAction->isChecked())
	{
		m_pEnableStatisticsMenuAction->setChecked(debugStatisticsEnabled);
	}
}

int MainWindow::GetEnableExecutionHistoryLogCvar() const
{
	if (!m_pEnableExecutionHistoryLogCurrentEntityMenuAction)
	{
		return false;
	}

	const ICVar* pExecutionLog = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugExecutionLogCvar);
	return pExecutionLog->GetIVal();
}

void MainWindow::SetEnableExecutionHistoryLog(const int enableFlag)
{
	if (!m_pEnableExecutionHistoryLogCurrentEntityMenuAction || !m_pEnableExecutionHistoryLogAllEntitiesMenuAction)
	{
		return;
	}

	m_pEnableExecutionHistoryLogCurrentEntityMenuAction->setChecked(enableFlag == 1 ? true : false);
	m_pEnableExecutionHistoryLogAllEntitiesMenuAction->setChecked(enableFlag == 2 ? true : false);

	ICVar* pExecutionLog = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugExecutionLogCvar);
	pExecutionLog->Set(enableFlag);
}

void MainWindow::OnChanged_ExecutionHistoryLog()
{
	const ICVar* pDebugExecutionLog = gEnv->pConsole->GetCVar(k_modularBehaviorTreeDebugExecutionLogCvar);
	
	switch (pDebugExecutionLog->GetIVal())
	{
	case 0:
		m_pEnableExecutionHistoryLogCurrentEntityMenuAction->setChecked(false);
		m_pEnableExecutionHistoryLogAllEntitiesMenuAction->setChecked(false);
		break;
	case 1:
		m_pEnableExecutionHistoryLogCurrentEntityMenuAction->setChecked(true);
		m_pEnableExecutionHistoryLogAllEntitiesMenuAction->setChecked(false);
		break;
	case 2:
		m_pEnableExecutionHistoryLogCurrentEntityMenuAction->setChecked(false);
		m_pEnableExecutionHistoryLogAllEntitiesMenuAction->setChecked(true);
		break;
	default:
		CRY_ASSERT_MESSAGE(false, "Unknown value for Cvar");
		break;
	}
}
