// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditor.h>

#include <LogWidget.h>

#include <CrySandbox/CryInterop.h>
#include <CrySerialization/Forward.h>
#include <IPlugin.h>

#include <EditorFramework/Editor.h>

#include <Schematyc/Utils/ScopedConnection.h>

class CInspector;
class QAction;
class QVBoxLayout;

namespace Schematyc {

// Forward declare structures.
struct SScriptGraphViewSelection;
struct SScriptBrowserSelection;

// Forward declare interfaces.
struct IDetailItem;

// Forward declare classes.
class CEnvBrowserWidget;
class CPreviewWidget;
class CScriptBrowserWidget;
class CSourceControlManager;

}

namespace CrySchematycEditor {

class CObjectModel;
class CComponentsWidget;
class CObjectStructureWidget;
class CNodeGraphView;
class CVariablesWidget;

class CAbstractObjectStructureModelItem;

struct IDetailItem;

class CMainWindow : public CDockableEditor, public IEditorNotifyListener
{
	Q_OBJECT

public:
	CMainWindow();
	~CMainWindow();

	// CEditor
	virtual const char* GetEditorName() const override { return "Schematyc"; };
	virtual void        SetLayout(const QVariantMap& state) override;
	virtual QVariantMap GetLayout() const override;
	// ~CEditor

	// IEditorNotifyListener
	void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
	//~IEditorNotifyListener

	void Serialize(Serialization::IArchive& archive);
	void Show(const Schematyc::SGUID& elementGUID, const Schematyc::SGUID& detailGUID = Schematyc::SGUID());

protected:
	void SaveState();
	void LoadState();

protected Q_SLOTS:
	void OnCompileAll();
	void OnRefreshEnv();

private:
	void                       InitMenu();
	void                       InitToolbar(QVBoxLayout* pWindowLayout);

	virtual bool               OnNew() override;
	virtual bool               OnSave() override;
	virtual bool               OnUndo() override;
	virtual bool               OnRedo() override;
	virtual bool               OnCopy() override;
	virtual bool               OnCut() override;
	virtual bool               OnPaste() override;
	virtual bool               OnDelete() override;

	void                       ConfigureLogs();
	void                       LoadSettings();
	void                       SaveSettings();

	void                       OnScriptBrowserSelection(const Schematyc::SScriptBrowserSelection& selection);

	void                       ClearLog();
	void                       ClearCompilerLog();

	void                       ShowLogSettings();
	void                       ShowPreviewSettings();

	Schematyc::CLogWidget*     CreateLog();
	Schematyc::CLogWidget*     CreateCompilerLog();
	Schematyc::CPreviewWidget* CreatePreview();

private:
	Schematyc::CScriptBrowserWidget* m_pScriptBrowser;

	Schematyc::CLogWidget*           m_pLog;
	Schematyc::SLogSettings          m_logSettings;
	Schematyc::CLogWidget*           m_pCompilerLog;
	Schematyc::SLogSettings          m_compilerLogSettings;

	Schematyc::CPreviewWidget*       m_pPreview;
	Schematyc::CConnectionScope      m_connectionScope;

	QAction*                         m_pCompileAllMenuAction;
	QAction*                         m_pRefreshEnvironmentMenuAction;

	QAction*                         m_pClearLogToolbarAction;
	QAction*                         m_pClearCompilerLogToolbarAction;
	QAction*                         m_pShowLogSettingsToolbarAction;
	QAction*                         m_pShowPreviewSettingsToolbarAction;

	CInspector*                      m_pInspector;
	CNodeGraphView*                  m_pGraphView;
};

} // Schematyc
