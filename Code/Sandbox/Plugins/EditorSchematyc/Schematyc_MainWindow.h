// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditor.h>

#include <CrySandbox/CryInterop.h>
#include <CrySerialization/Forward.h>
#include <IPlugin.h>

#include <EditorFramework/Editor.h>

#include <Schematyc/Utils/Schematyc_ScopedConnection.h>

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
class CLogWidget;
class CPreviewWidget;
class CScriptBrowserWidget;
class CScriptGraphWidget;
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
	virtual const char* GetEditorName() const override { return "Schematyc Editor"; };
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

private:
	void         InitMenu();
	void         InitToolbar(QVBoxLayout* pWindowLayout);

	virtual bool OnNew() override;
	virtual bool OnSave() override;
	virtual bool OnUndo() override;
	virtual bool OnRedo() override;
	virtual bool OnCopy() override;
	virtual bool OnCut() override;
	virtual bool OnPaste() override;
	virtual bool OnDelete() override;

	// TEMP
	QWidget* InitNewEditor();
	QWidget* InitOldEditor();
	// ~TEMP

protected Q_SLOTS:
	void OnCompile();
	void OnRefreshEnv();

private:
	void ConfigureLogs();
	void LoadSettings();
	void SaveSettings();

	void OnScriptBrowserSelection(const Schematyc::SScriptBrowserSelection& selection);
	void OnDetailModification(IDetailItem* pDetailItem);
	void OnGraphSelection(const Schematyc::SScriptGraphViewSelection& selection);

	void OnObjectStructureSelectionChanged(CAbstractObjectStructureModelItem& item);

private:
	// TODO: To be removed or replaced.
	std::unique_ptr<Schematyc::CSourceControlManager> m_pSourceControlManager;

	Schematyc::CScriptBrowserWidget*                  m_pScriptBrowser;
	Schematyc::CScriptGraphWidget*                    m_pGraph;
	Schematyc::CLogWidget*                            m_pLog;
	Schematyc::CLogWidget*                            m_pCompilerLog;
	Schematyc::CPreviewWidget*                        m_pPreview;
	Schematyc::CConnectionScope                       m_connectionScope;
	// ~TODO;

	QAction*                m_pCompileMenuAction;
	QAction*                m_pRefreshEnvironmentMenuAction;

	CObjectModel*           m_pObjectModel;

	CInspector*             m_pInspector;
	CComponentsWidget*      m_pComponents;
	CObjectStructureWidget* m_pObjectStructure;
	CNodeGraphView*         m_pGraphView;
	CVariablesWidget*       m_pGlobalVariables;
	CVariablesWidget*       m_pLocalVariables;
};

} // Schematyc
