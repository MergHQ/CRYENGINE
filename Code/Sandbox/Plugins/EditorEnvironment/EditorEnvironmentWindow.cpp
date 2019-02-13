// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EditorEnvironmentWindow.h"

#include "QTimeOfDayWidget.h"

#include <AssetSystem/EditableAsset.h>
#include <EditorFramework/PersonalizationManager.h>
#include <EditorFramework/Events.h>
#include <FileUtils.h>

#include <Cry3DEngine/I3DEngine.h>

#include <CrySystem/Profilers/FrameProfiler/FrameProfiler.h>

#include <QToolBar>
#include <QVBoxLayout>

namespace Private_EditorEnvironmentWindow
{

class CSession final : public IAssetEditingSession
{
public:
	CSession(const char* szPresetPath, ITimeOfDay::IPreset* pPreset)
		: m_presetPath(szPresetPath)
		, m_pPreset(pPreset)
	{
		CRY_ASSERT(m_pPreset);
	}

	virtual const char* GetEditorName() const { return "Environment Editor"; }

	virtual bool        OnSaveAsset(IEditableAsset& asset)
	{
		asset.SetFiles({ m_presetPath });
		ITimeOfDay* const pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();
		return pTimeOfDay->SavePreset(m_presetPath);
	}

	virtual void DiscardChanges(IEditableAsset& asset)
	{
		ITimeOfDay* const pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();
		pTimeOfDay->DiscardPresetChanges(m_presetPath);
	}

	virtual bool OnCopyAsset(INewAsset& asset)
	{
		const string dataFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());
		if (!FileUtils::Pak::CopyFileAllowOverwrite(m_presetPath, dataFilePath))
		{
			return false;
		}

		ITimeOfDay* const pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();
		if (!pTimeOfDay->LoadPreset(dataFilePath))
		{
			return false;
		}

		ITimeOfDay::IPreset& preset = pTimeOfDay->GetCurrentPreset();
		CSession session(dataFilePath, &preset);
		return session.OnSaveAsset(asset);
	}

	ITimeOfDay::IPreset* GetPreset()
	{
		return m_pPreset;
	}

	static ITimeOfDay::IPreset* GetSessionPreset(CAsset* pAsset)
	{
		IAssetEditingSession* pSession = pAsset->GetEditingSession();
		if (!pSession || strcmp(pSession->GetEditorName(), "Environment Editor") != 0)
		{
			return nullptr;
		}
		return static_cast<CSession*>(pSession)->GetPreset();
	}

private:
	const string               m_presetPath;
	ITimeOfDay::IPreset* const m_pPreset;
};

}

REGISTER_VIEWPANE_FACTORY(CEditorEnvironmentWindow, "Environment Editor", "Tools", false);

CEditorEnvironmentWindow::CEditorEnvironmentWindow()
	: CAssetEditor("Environment")
	, m_pPreset(nullptr)
{
	auto* pTopLayout = new QVBoxLayout;
	pTopLayout->setMargin(0);

	QBoxLayout* pToolBarsLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	pToolBarsLayout->setSizeConstraint(QLayout::SetMaximumSize);

	pToolBarsLayout->addWidget(CreateToolbar(), 0, Qt::AlignLeft);
	pToolBarsLayout->addSpacerItem(new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	pToolBarsLayout->addWidget(CreateInstantEditorToolbar(), 0, Qt::AlignRight);

	pTopLayout->addLayout(pToolBarsLayout);
	SetContent(pTopLayout);

	RegisterDockingWidgets();
}

std::unique_ptr<IAssetEditingSession> CEditorEnvironmentWindow::CreateEditingSession()
{
	using namespace Private_EditorEnvironmentWindow;

	const string& filename = GetAssetBeingEdited()->GetFile(0);
	return std::make_unique<CSession>(filename, m_pPreset);
}

ITimeOfDay::IPreset* CEditorEnvironmentWindow::GetPreset()
{
	return m_pPreset;
}

QWidget* CEditorEnvironmentWindow::CreateToolbar()
{
	QAction* pUndo = GetIEditor()->GetICommandManager()->GetAction("general.undo");
	QAction* pRedo = GetIEditor()->GetICommandManager()->GetAction("general.redo");

	QToolBar* pToolbar = new QToolBar(this);
	pToolbar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	pToolbar->addAction(pUndo);
	pToolbar->addAction(pRedo);
	return pToolbar;
}

void CEditorEnvironmentWindow::RegisterDockingWidgets()
{
	EnableDockingSystem();

	RegisterDockableWidget(tr("TimeOfDay"), [=]() { return new QTimeOfDayWidget(this); }, true, false);
}

void CEditorEnvironmentWindow::CreateDefaultLayout(CDockableContainer* pSender)
{
	pSender->SpawnWidget("TimeOfDay");
}

bool CEditorEnvironmentWindow::OnSaveAsset(CEditableAsset& editAsset)
{
	return GetAssetBeingEdited()->GetEditingSession()->OnSaveAsset(editAsset);
}

bool CEditorEnvironmentWindow::OnOpenAsset(CAsset* pAsset)
{
	using namespace Private_EditorEnvironmentWindow;

	m_pPreset = CSession::GetSessionPreset(pAsset);
	if (!m_pPreset)
	{
		ITimeOfDay* pTimeOfDay = GetIEditor()->Get3DEngine()->GetTimeOfDay();

		const string& filename = pAsset->GetFile(0);
		if (!pTimeOfDay->PreviewPreset(filename))
		{
			return false;
		}

		m_pPreset = &pTimeOfDay->GetCurrentPreset();
	}

	return m_pPreset != nullptr;
}

void CEditorEnvironmentWindow::OnCloseAsset()
{
	m_pPreset = nullptr;
}

void CEditorEnvironmentWindow::OnDiscardAssetChanges(CEditableAsset& editAsset)
{
	CRY_ASSERT(GetAssetBeingEdited());
	CRY_ASSERT(GetAssetBeingEdited()->GetEditingSession());

	CAsset* const pAsset = GetAssetBeingEdited();
	OnCloseAsset();
	GetAssetBeingEdited()->GetEditingSession()->DiscardChanges(editAsset);
	OnOpenAsset(pAsset);
}

void CEditorEnvironmentWindow::customEvent(QEvent* event)
{
	// TODO: This handler should be removed whenever this editor is refactored to be a CDockableEditor
	if (event->type() == SandboxEvent::Command)
	{
		CommandEvent* commandEvent = static_cast<CommandEvent*>(event);

		const string& command = commandEvent->GetCommand();
		if (command == "general.help")
		{
			event->setAccepted(EditorUtils::OpenHelpPage(GetPaneTitle()));
		}
	}

	if (!event->isAccepted())
	{
		CAssetEditor::customEvent(event);
	}
}
