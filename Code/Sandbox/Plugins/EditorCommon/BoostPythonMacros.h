// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CryCore/BoostHelpers.h>

// Qt "slots" is reserved word. Compilation fix
#if defined(slots)
#pragma push_macro("slots")
#undef slots
#include <boost/python.hpp>
#pragma pop_macro("slots")
#endif // slots

#include "IEditor.h"
#include "Commands/ICommandManager.h"

struct SPythonCommand;
struct SPythonModule;

struct IPythonManager
{
	virtual ~IPythonManager() {}
	virtual void  RegisterPythonCommand(const SPythonCommand& command) = 0;
	virtual void  RegisterPythonModule(const SPythonModule& module) = 0;
	virtual void  Execute(const char* command) = 0;
	virtual float GetAsFloat(const char* variable) = 0;
};

struct EDITOR_COMMON_API SPythonCommand
{
	SPythonCommand(const char* moduleName, const char* functionName, const char* description, const char* example, std::function<void()> registerFunc)
		: m_module(moduleName)
		, m_name(functionName)
		, m_description(description)
		, m_example(example)
		, m_registerFunc(registerFunc)
	{}

	void Register();

	string                m_module;
	string                m_name;
	string                m_description;
	string                m_example;
	std::function<void()> m_registerFunc;
};

struct SPythonModule
{
	SPythonModule(const char* name)
		: m_name(name)
	{}

	bool operator==(const SPythonModule& other) const
	{
		return m_name == other.m_name;
	}

	string                      m_name;
	std::vector<SPythonCommand> m_commands;
};

typedef CAutoRegister<SPythonModule>  CAutoRegisterPythonModuleHelper;
typedef CAutoRegister<SPythonCommand> CAutoRegisterPythonCommandHelper;

namespace Private_CryPython
{
	EDITOR_COMMON_API void RegisterSandboxPythonSubModule(const char* szModuleName);
	EDITOR_COMMON_API void RegisterSandboxPythonCommand(const char* szModuleName, const char* szFunctionName, const char* szDescription, const char* szExample, std::function<void()> regFunction);

	//T2 can be either:
	// 1. const char* szDescription
	// 2. boost::python overloaded object
	template<typename Fn, typename T2>
	inline std::function<void()> CreateExposedCppFunction(const char* szFunctionName, const char* szModuleName, Fn functionPtr, T2 extraParam)
	{
		auto func = [=]() 
		{
			namespace bp = boost::python;
			bp::docstring_options local_docstring_options(true, true, false);
			bp::object mainModule = bp::import("__main__");
			bp::scope mainScope = mainModule;
			
			const string submoduleName = string("sandbox.") + szModuleName;
			bp::object currentModule(bp::handle<>(bp::borrowed(PyImport_AddModule(submoduleName.c_str()))));
			CRY_ASSERT(!PyErr_Occurred());
			
			bp::scope submoduleScope = currentModule;
			boost::python::def(szFunctionName, functionPtr, extraParam);
		};
		return std::function<void()>(func);
	}
}

#define DECLARE_PYTHON_MODULE(moduleName)																			                    \
	namespace Private_CryPython																						                    \
	{																													                \
		CAutoRegisterPythonModuleHelper g_pythonSubModuleRegisterer_ ## moduleName([](){RegisterSandboxPythonSubModule(#moduleName);}); \
	}

#define REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example)                                          \
	namespace Private_CryPython																						                                    \
	{	                                                                                                                                                \
		CAutoRegisterPythonCommandHelper g_AutoRegPythonCmdHelper ## moduleName ## functionName([]()    						                        \
			{	                                                                                                                                        \
				/* Expose a function from current process to Python*/                                                                                   \
				auto exposedCppFunctionToPython = ::Private_CryPython::CreateExposedCppFunction(#functionName, #moduleName, functionPtr, #description); \
				::Private_CryPython::RegisterSandboxPythonCommand(#moduleName, #functionName, #description, #example, exposedCppFunctionToPython);      \
			});                                                                                                                                         \
	}

#define REGISTER_PYTHON_OVERLOAD_COMMAND(functionPtr, moduleName, functionName, functionOverload, description, example)                                       \
	namespace Private_CryPython                                                                                                                               \
	{                                                                                                                                                         \
		CAutoRegisterPythonCommandHelper g_AutoRegPythonCmdHelper ## moduleName ## functionName([]()    						                              \
			{	                                                                                                                                              \
				/* Expose an overloaded function from current process to Python*/                                                                             \
				auto exposedCppFunctionToPython = ::Private_CryPython::CreateExposedCppFunction(#functionName, #moduleName, functionPtr, functionOverload()); \
				::Private_CryPython::RegisterSandboxPythonCommand(#moduleName, #functionName, #description, #example, exposedCppFunctionToPython);            \
			});                                                                                                                                               \
	}

#define REGISTER_ONLY_PYTHON_COMMAND(functionPtr, moduleName, functionName, description) \
	REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, "");

	#define REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example)                                \
	  namespace Private_CryPython                                                                                                            \
	  {                                                                                                                                      \
	  void RegisterCommand ## moduleName ## functionName()                                                                                   \
	  {                                                                                                                                      \
	    ICommandManager* cmdMgr = GetIEditor()->GetICommandManager();                                                                        \
	    CommandManagerHelper::RegisterCommand(cmdMgr, # moduleName, # functionName, CCommandDescription(description), functor(functionPtr)); \
	    cmdMgr->GetCommand( # moduleName ## "." ## # functionName)->SetAvailableInScripting();                                               \
	  }                                                                                                                                      \
	  CAutoRegisterCommandHelper g_AutoRegCmdHelper ## moduleName ## functionName(RegisterCommand ## moduleName ## functionName);            \
	  }                                                                                                                                      \
	  REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, example);

#define REGISTER_PYTHON_COMMAND(functionPtr, moduleName, functionName, description) \
	REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(functionPtr, moduleName, functionName, description, "");

//TODO : !("REGISTER_PYTHON_SERIALIZABLE_ENUM must do !");
#define REGISTER_PYTHON_ENUM_BEGIN(enumType, moduleName, enumName)                                                                      \
	namespace Private_CryPython                                                                                                         \
	{                                                                                                                                   \
		void DoRegisterPythonEnum ## moduleName ## enumName();                                                                          \
		void RegisterPythonEnum ## moduleName ## enumName()                                                                             \
	{                                                                                                                                   \
		SPythonCommand cmd( # moduleName, # enumName, "enumeration", "", DoRegisterPythonEnum ## moduleName ## enumName);               \
		cmd.Register();                                                                                                                 \
	}                                                                                                                                   \
	CAutoRegisterPythonCommandHelper g_AutoRegPythonEnumHelper ## moduleName ## enumName(RegisterPythonEnum ## moduleName ## enumName); \
	void DoRegisterPythonEnum ## moduleName ## enumName()                                                                               \
	{                                                                                                                                   \
		namespace bp = boost::python;                                                                                                   \
		bp::docstring_options local_docstring_options(true, true, false);																\
		bp::object mainModule = bp::import("__main__");																					\
		bp::scope mainScope = mainModule;																								\
																																		\
		const string submoduleName = string("sandbox.") + #moduleName;																    \
		bp::object currentModule(bp::handle<>(bp::borrowed(PyImport_AddModule(submoduleName.c_str()))));								\
		CRY_ASSERT(!PyErr_Occurred());																									\
																																		\
		bp::scope submoduleScope = currentModule;																						\
		boost::python::enum_<enumType>( # enumName)                                                                                     \

#define REGISTER_PYTHON_ENUM_ITEM(item, name) .value( # name, item)

#define REGISTER_PYTHON_ENUM_END ; } \
	}
