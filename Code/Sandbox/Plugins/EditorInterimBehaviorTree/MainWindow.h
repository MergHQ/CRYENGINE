// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QMainWindow>
#include <QSharedPointer>
#include <IEditor.h>
#include <IPlugin.h>

#include "TreePanel.h"

class MainWindow : public CDockableWindow, public IEditorNotifyListener
{
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();

	// CDockableWindow
	virtual const char* GetPaneTitle() const override;
	// ~CDockableWindow

	// IEditorNotifyListener
	void OnEditorNotifyEvent(EEditorNotifyEvent editorNotifyEvent) override;
	// ~IEditorNotifyListener

	void Serialize(Serialization::IArchive& archive);

	void PromoteFileInRecentFileHistory(const char* szFileName);

protected:
	void closeEvent(QCloseEvent* closeEvent) override;

protected slots:
	void OnMenuActionNew();
	void OnMenuActionOpen();
	void OnMenuActionSave();
	void OnMenuActionSaveToFile();
	void OnMenuActionRecentFile();
	void OnMenuActionShowXmlLineNumbers();
	void OnMenuActionShowComments();

	// Events
	void OnMenuActionShowCryEngineSignals();
	void OnMenuActionShowGameSDKSignals();
	void OnMenuActionShowDeprecatedSignals();

	// Debug
	void OnMenuActionShowTreeDebugger();
	void OnMenuActionShowVariablesDebugger();
	void OnMenuActionShowTimestampsDebugger();
	void OnMenuActionShowEventsDebugger();
	void OnMenuActionShowLogDebugger();
	void OnMenuActionShowAllDebugger();
	void OnMenuActionShowStatistics();
	void OnMenuActionEnableExecutionHistoryLogForCurrentEntity();
	void OnMenuActionEnableExecutionHistoryLogForAllEntities();

private:

	// Editor state I/O:
	void SaveState();
	void LoadState();

	// Menu control:
	void RebuildRecentFilesMenu();

	// Settings control:
	bool GetShowXmlLineNumbers() const;
	void SetShowXmlLineNumbers(const bool showFlag);
	bool GetShowComments() const;
	void SetShowComments(const bool showFlag);

	// Events
	bool GetEnableCryEngineSignals() const;
	void SetEnableCryEngineSignals(const bool showFlag);
	bool GetEnableGameSDKSignals() const;
	void SetEnableGameSDKSignals(const bool showFlag);
	bool GetEnableDeprecatedSignals() const;
	void SetEnableDeprecatedSignals(const bool enableFlag);

	// Debug
	bool GetShowTreeDebuggerCvar() const;
	void SetShowTreeDebugger(const bool enableFlag);
	void OnChanged_ShowTreeDebugger();

	bool GetShowVariablesDebuggerCvar() const;
	void SetShowVariablesDebugger(const bool enableFlag);
	void OnChanged_ShowVariablesDebugger();

	bool GetShowTimestampsDebuggerCvar() const;
	void SetShowTimestampsDebugger(const bool enableFlag);
	void OnChanged_ShowTimestampsDebugger();

	bool GetShowEventsDebuggerCvar() const;
	void SetShowEventsDebugger(const bool enableFlag);
	void OnChanged_ShowEventsDebugger();

	bool GetShowLogDebuggerCvar() const;
	void SetShowLogDebugger(const bool enableFlag);
	void OnChanged_ShowLogDebugger();

	bool GetShowAllDebugger() const;
	void SetShowAllDebugger(const bool enableFlag);

	bool GetShowStatisticsCvar() const;
	void SetShowStatisticsCvar(const bool enableFlag);
	void OnChanged_ShowStatistics();

	int GetEnableExecutionHistoryLogCvar() const;
	void SetEnableExecutionHistoryLog(const int enableFlag);
	void OnChanged_ExecutionHistoryLog();

private:
	TreePanel*              m_treePanel;
	std::vector<string>     m_recentFileNamesHistory;
	QSharedPointer<QMenu>   m_pRecentFilesMenu;
	QSharedPointer<QAction> m_pShowXmlLineNumberMenuAction;
	QSharedPointer<QAction> m_pShowCommentsMenuAction;

	// Events
	QSharedPointer<QAction> m_pEnableCryEngineSignalsMenuAction;
	QSharedPointer<QAction> m_pEnableGameSDKSignalsMenuAction;
	QSharedPointer<QAction> m_pEnableDeprecatedSignalsMenuAction;

	// Debug
	QSharedPointer<QAction> m_pEnableTreeDebuggerMenuAction;
	QSharedPointer<QAction> m_pEnableVariablesDebuggerMenuAction;
	QSharedPointer<QAction> m_pEnableTimestampsDebuggerMenuAction;
	QSharedPointer<QAction> m_pEnableEventsDebuggerMenuAction;
	QSharedPointer<QAction> m_pEnableLogDebuggerMenuAction;
	QSharedPointer<QAction> m_pEnableAllDebuggerMenuAction;
	QSharedPointer<QAction> m_pEnableStatisticsMenuAction;

	QSharedPointer<QAction> m_pEnableExecutionHistoryLogCurrentEntityMenuAction;
	QSharedPointer<QAction> m_pEnableExecutionHistoryLogAllEntitiesMenuAction;

	// CVars
	const string k_modularBehaviorTreeDebugTreeCvar = "ai_ModularBehaviorTreeDebugTree";
	const string k_modularBehaviorTreeDebugVariablesCvar = "ai_ModularBehaviorTreeDebugVariables";
	const string k_modularBehaviorTreeDebugTimestampsCvar = "ai_ModularBehaviorTreeDebugTimestamps";
	const string k_modularBehaviorTreeDebugEventsCvar = "ai_ModularBehaviorTreeDebugEvents";
	const string k_modularBehaviorTreeDebugLogCvar = "ai_ModularBehaviorTreeDebugLog";
	const string k_modularBehaviorTreeStatisticsCvar = "ai_DrawModularBehaviorTreeStatistics";
	const string k_modularBehaviorTreeDebugExecutionLogCvar = "ai_ModularBehaviorTreeDebugExecutionStacks";

	// CVars callbacks handles
	uint64 m_modularBehaviorTreeDebugTreeCvarHandle;
	uint64 m_modularBehaviorTreeDebugVariablesCvarHandle;
	uint64 m_modularBehaviorTreeDebugTimestampsCvarHandle;
	uint64 m_modularBehaviorTreeDebugEventsCvarHandle;
	uint64 m_modularBehaviorTreeDebugLogCvarHandle;
	uint64 m_modularBehaviorTreeStatisticsCvarHandle;
	uint64 m_modularBehaviorTreeDebugExecutionLogCvarHandle;
};
