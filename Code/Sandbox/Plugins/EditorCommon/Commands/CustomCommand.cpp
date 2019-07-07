// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>
#include "CustomCommand.h"

#include "CommandModuleDescription.h"
#include "ICommandManager.h"

CCustomCommand::CCustomCommand(const string& name, const string& command)
	: CCommand0("custom", name, CCommandDescription(""), Functor0())
	, m_command(command)
{
}

void CCustomCommand::Register()
{
	GetIEditor()->GetICommandManager()->AddCustomCommand(this);
}

void CCustomCommand::Unregister()
{
	GetIEditor()->GetICommandManager()->RemoveCustomCommand(this);
}

void CCustomCommand::SetName(const char* name)
{
	m_name = name;
}

void CCustomCommand::SetCommandString(const char* commandStr)
{
	m_command = commandStr;
}
