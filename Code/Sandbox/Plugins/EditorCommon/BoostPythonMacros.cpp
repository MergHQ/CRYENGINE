// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
	namespace bp = boost::python;

	bp::docstring_options local_docstring_options(true, true, false);
	bp::object mainModule = bp::import("__main__");
	bp::scope main_scope = mainModule;

	const string submoduleName = string("sandbox.") + moduleName;
	bp::object currentModule(bp::handle<>(bp::borrowed(PyImport_AddModule(submoduleName.c_str()))));
	CRY_ASSERT(!PyErr_Occurred());

	bp::scope submoduleScope = currentModule;

	GetIEditor()->GetIPythonManager()->RegisterPythonModule(SPythonModule(moduleName));
}

void RegisterSandboxPythonCommand(const char* szModuleName, const char* szFunctionName, const char* szDescription, const char* szExample, std::function<void()> regFunction)
{
	SPythonCommand cmd(szModuleName, szFunctionName, szDescription, szExample, regFunction);
	cmd.Register();
}

}
