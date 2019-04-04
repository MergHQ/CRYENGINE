// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "EditorFramework/CommandRegistry.h"

#include <CryString/CryString.h>

#include <QWidget>

class QCommandAction;
class CCommand;

//! The widget that encapsulates registration and handling of actions.
class EDITOR_COMMON_API CEditorWidget : public QWidget
{
	Q_OBJECT
public:
	CEditorWidget(QWidget* pParent = nullptr)
		: QWidget(pParent)
	{}

	const std::vector<CCommand*>& GetCommands() const { return m_commands; }

	QCommandAction* GetAction_Deprecated(const string& actionId) const;

	//! Returns registered action.
	QCommandAction* GetAction(const string& actionId) const;

protected:
	virtual void customEvent(QEvent* event) override;

	template<class Func>
	QCommandAction* RegisterAction(const string& actionId, Func f)
	{
		m_commandRegistry.RegisterCommand(actionId, std::move(f));
		return RegisterCommandAction(actionId);
	}

	template<class T, class Ret, class... Params, typename std::enable_if_t<std::is_base_of<CEditorWidget, T>::value, bool> = true>
	QCommandAction* RegisterAction(const string& actionId, Ret(T::*f)(Params...))
	{
		m_commandRegistry.RegisterCommand(actionId, std::move(f), static_cast<T*>(this));
		return RegisterCommandAction(actionId);
	}

	QCommandAction* SetActionEnabled(const string& actionId, bool enable);
	QCommandAction* SetActionChecked(const string& actionId, bool check);
	QCommandAction* SetActionText(const string& actionId, const QString& text);

private:
	QCommandAction* RegisterCommandAction(const string& actionId);

	SCommandRegistry           m_commandRegistry;
	std::vector<CCommand*>     m_commands;
	StringMap<QCommandAction*> m_actions;
};
