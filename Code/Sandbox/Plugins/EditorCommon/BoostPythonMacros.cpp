// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "BoostPythonMacros.h"

void SPythonCommand::Register()
{
	GetIEditor()->GetIPythonManager()->RegisterPythonCommand(*this);
}

namespace Private_CryPython
{

void RegisterSandboxPythonSubModule(const char* moduleName)
{
	GetIEditor()->GetIPythonManager()->RegisterPythonModule(SPythonModule(moduleName));
}

void RegisterSandboxPythonCommand(const char* szModuleName, const char* szFunctionName, const char* szDescription, const char* szExample, std::function<void()> regFunction)
{
	SPythonCommand cmd(szModuleName, szFunctionName, szDescription, szExample, regFunction);
	cmd.Register();
}

}
