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
	gEnv->pConsole->AddCommand(consoleCommandName, consoleCommandFunc, nFlags, consoleCommandHelp, isManaged);
}

void CConsoleCommandInterface::RegisterFunctions(std::function<void(const void* pMethod, const char* methodName)> func)
{
	func(AddConsoleCommandFunction, "AddConsoleCommandFunction");
}