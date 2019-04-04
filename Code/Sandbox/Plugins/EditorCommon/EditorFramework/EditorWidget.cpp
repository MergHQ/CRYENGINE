// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>
#include "EditorWidget.h"

#include "Commands/ICommandManager.h"
#include "Commands/QCommandAction.h"
#include "Events.h"
#include <IEditor.h>

QCommandAction* CEditorWidget::GetAction_Deprecated(const string& actionId) const
{
	QCommandAction* pAction = GetIEditor()->GetICommandManager()->GetCommandAction(actionId.c_str());

	if (!pAction)
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR_DBGBRK, "Command not found");

	return pAction;
}

void CEditorWidget::customEvent(QEvent* event)
{
	if (event->type() == SandboxEvent::Command)
	{
		CommandEvent* pCommandEvent = static_cast<CommandEvent*>(event);
		event->setAccepted(m_commandRegistry.ExecuteCommand(pCommandEvent->GetCommand()));
		if (event->isAccepted())
		{
			return;
		}
	}

	QWidget::customEvent(event);
}

QCommandAction* CEditorWidget::RegisterCommandAction(const string& actionId)
{
	const auto it = m_actions.find(actionId);
	if (it != m_actions.cend())
	{
		return it->second;
	}

	QCommandAction* pAction = GetIEditor()->GetICommandManager()->CreateNewAction(actionId);
	m_actions[actionId] = pAction;
	addAction(pAction);

	CCommand* pCommand = GetIEditor()->GetICommandManager()->GetCommand(actionId.c_str());
	if (pCommand)
	{
		m_commands.push_back(pCommand);
	}
	return pAction;
}

QCommandAction* CEditorWidget::GetAction(const string& actionId) const
{
	const auto& it = m_actions.find(actionId);
	if (it != m_actions.cend())
	{
		return it->second;
	}
	CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR_DBGBRK, string().Format("Command %s not found", actionId));
	return nullptr;
}

QCommandAction* CEditorWidget::SetActionEnabled(const string& actionId, bool enable)
{
	QCommandAction* pAction = GetAction(actionId);
	if (!pAction)
	{
		return nullptr;
	}
	pAction->setEnabled(enable);
	return pAction;
}

QCommandAction* CEditorWidget::SetActionChecked(const string& actionId, bool check)
{
	QCommandAction* pAction = GetAction(actionId);
	if (!pAction)
	{
		return nullptr;
	}
	pAction->setChecked(check);
	return pAction;
}

QCommandAction* CEditorWidget::SetActionText(const string& actionId, const QString& text)
{
	QCommandAction* pAction = GetAction(actionId);
	if (!pAction)
	{
		return nullptr;
	}
	pAction->setText(text);
	return pAction;
}
