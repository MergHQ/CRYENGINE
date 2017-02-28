#include "StdAfx.h"
#include "Console.h"

#include "MonoRuntime.h"

#include "Wrappers/MonoDomain.h"
#include "Wrappers/MonoClass.h"

#include <mono/metadata/class.h>
#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>

#include <CrySystem/IConsole.h>

static void AddConsoleCommandFunction(ConsoleCommandFunc consoleCommandFunc, MonoString* commandName, int nFlags, MonoString* sHelp, bool isManaged)
{
	const char* consoleCommandName = mono_string_to_utf8(commandName);
	const char* consoleCommandHelp = mono_string_to_utf8(sHelp);
	ConsoleRegistrationHelper::AddCommand(consoleCommandName, consoleCommandFunc, nFlags, consoleCommandHelp, isManaged);
}

static void AddConsoleVariableString(ConsoleVarFunc consoleVariableFunc, MonoString* variableName, MonoString* variableValue, int nFlags, MonoString* sHelp)
{
	const char* szConsoleVariableName = mono_string_to_utf8(variableName);
	const char* sZConsoleVariableValue = mono_string_to_utf8(variableValue);
	const char* szConsoleVariableHelp = mono_string_to_utf8(sHelp);
	ConsoleRegistrationHelper::RegisterString(szConsoleVariableName, sZConsoleVariableValue, nFlags, szConsoleVariableHelp, consoleVariableFunc);
}

static void AddConsoleVariableInt64(ConsoleVarFunc consoleVariableFunc, MonoString* variableName, int64 variableValue, int nFlags, MonoString* sHelp)
{
	const char* szConsoleVariableName = mono_string_to_utf8(variableName);
	const char* szConsoleVariableHelp = mono_string_to_utf8(sHelp);
	ConsoleRegistrationHelper::RegisterInt64(szConsoleVariableName, variableValue, nFlags, szConsoleVariableHelp, consoleVariableFunc);
}

static void AddConsoleVariableInt(ConsoleVarFunc consoleVariableFunc, MonoString* variableName, int variableValue, int nFlags, MonoString* sHelp)
{
	const char* szConsoleVariableName = mono_string_to_utf8(variableName);
	const char* szConsoleVariableHelp = mono_string_to_utf8(sHelp);
	ConsoleRegistrationHelper::RegisterInt(szConsoleVariableName, variableValue, nFlags, szConsoleVariableHelp, consoleVariableFunc);
}

static void AddConsoleVariableFloat(ConsoleVarFunc consoleVariableFunc, MonoString* variableName, float variableValue, int nFlags, MonoString* sHelp)
{
	const char* szConsoleVariableName = mono_string_to_utf8(variableName);
	const char* szConsoleVariableHelp = mono_string_to_utf8(sHelp);
	ConsoleRegistrationHelper::RegisterFloat(szConsoleVariableName, variableValue, nFlags, szConsoleVariableHelp, consoleVariableFunc);
}

void CConsoleCommandInterface::RegisterFunctions(std::function<void(const void* pMethod, const char* szMethodName)> func)
{
	func(AddConsoleCommandFunction, "AddConsoleCommandFunction");
	func(AddConsoleVariableString, "AddConsoleVariableString");
	func(AddConsoleVariableInt64, "AddConsoleVariableInt64");
	func(AddConsoleVariableInt, "AddConsoleVariableInt");
	func(AddConsoleVariableFloat, "AddConsoleVariableFloat");
}

