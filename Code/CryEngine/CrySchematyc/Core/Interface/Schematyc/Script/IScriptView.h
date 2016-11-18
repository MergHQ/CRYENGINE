// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Utils/Delegate.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/IString.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvAction;
struct IEnvClass;
struct IEnvComponent;
struct IEnvDataType;
struct IEnvElement;
struct IEnvFunction;
struct IEnvInterface;
struct IEnvInterfaceFunction;
struct IEnvSignal;
struct IScriptActionInstance;
struct IScriptClass;
struct IScriptComponentInstance;
struct IScriptElement;
struct IScriptEnum;
struct IScriptFunction;
struct IScriptModule;
struct IScriptSignal;
struct IScriptState;
struct IScriptStateMachine;
struct IScriptStruct;
struct IScriptTimer;
struct IScriptVariable;

typedef CDelegate<EVisitStatus(const IEnvDataType&)>             EnvDataTypeConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvSignal&)>               EnvSignalConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvFunction&)>             EnvFunctionConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvClass&)>                EnvClassConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvInterface&)>            EnvInterfaceConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvAction&)>               EnvActionConstVisitor;
typedef CDelegate<EVisitStatus(const IEnvComponent&)>            EnvComponentConstVisitor;

typedef CDelegate<EVisitStatus(const IScriptModule&)>            ScriptModuleConstVisitor;
typedef CDelegate<void (const IScriptEnum&)>                     ScriptEnumConstVisitor;
typedef CDelegate<void (const IScriptStruct&)>                   ScriptStructConstVisitor;
typedef CDelegate<void (const IScriptSignal&)>                   ScriptSignalConstVisitor;
typedef CDelegate<EVisitStatus(const IScriptFunction&)>          ScriptFunctionConstVisitor;
typedef CDelegate<EVisitStatus(const IScriptClass&)>             ScriptClassConstVisitor;
typedef CDelegate<EVisitStatus(const IScriptState&)>             ScriptStateConstVisitor;
typedef CDelegate<EVisitStatus(const IScriptStateMachine&)>      ScriptStateMachineConstVisitor;
typedef CDelegate<EVisitStatus(const IScriptVariable&)>          ScriptVariableConstVisitor;
typedef CDelegate<void (const IScriptTimer&)>                    ScriptTimerConstVisitor;
typedef CDelegate<EVisitStatus(const IScriptComponentInstance&)> ScriptComponentInstanceConstVisitor;
typedef CDelegate<EVisitStatus(const IScriptActionInstance&)>    ScriptActionInstanceConstVisitor;

enum class EDomainScope
{
	// #SchematycTODO : Remove 'All'? It's only used internally and kinda defeats the whole purpose of the script view class.

	Local,     // Everything local to scope.
	Derived,   // Everything local to scope and everything with public or protected accessor in derived scopes.
	All        // Everything local to scope and everything in derived scopes.
};

enum class EDomainQualifier
{
	Local,
	Global
};

struct IScriptView
{
	virtual ~IScriptView() {}

	virtual const SGUID&                    GetScopeGUID() const = 0; // #SchematycTODO : Scope using element instead of GUID?
	virtual const IEnvClass*                GetEnvClass() const = 0;
	virtual const IScriptClass*             GetScriptClass() const = 0;

	virtual void                            VisitEnvDataTypes(const EnvDataTypeConstVisitor& visitor) const = 0;   // #SchematycTODO : Move to IEnvContext/View?
	virtual void                            VisitEnvSignals(const EnvSignalConstVisitor& visitor) const = 0;       // #SchematycTODO : Move to IEnvContext/View?
	virtual void                            VisitEnvFunctions(const EnvFunctionConstVisitor& visitor) const = 0;   // #SchematycTODO : Move to IEnvContext/View?
	virtual void                            VisitEnvClasses(const EnvClassConstVisitor& visitor) const = 0;        // #SchematycTODO : Move to IEnvContext/View?
	virtual void                            VisitEnvInterfaces(const EnvInterfaceConstVisitor& visitor) const = 0; // #SchematycTODO : Move to IEnvContext/View?
	virtual void                            VisitEnvComponents(const EnvComponentConstVisitor& visitor) const = 0; // #SchematycTODO : Move to IEnvContext/View?
	virtual void                            VisitEnvActions(const EnvActionConstVisitor& visitor) const = 0;       // #SchematycTODO : Move to IEnvContext/View?

	virtual void                            VisitScriptModules(const ScriptModuleConstVisitor& visitor) const = 0; // #SchematycTODO : Remove!!!
	virtual void                            VisitScriptFunctions(const ScriptFunctionConstVisitor& visitor) const = 0;
	virtual void                            VisitScriptClasses(const ScriptClassConstVisitor& visitor, EDomainScope scope) const = 0;
	virtual void                            VisitScriptStates(const ScriptStateConstVisitor& visitor, EDomainScope scope) const = 0;
	//virtual void                            VisitScriptStateMachines(const ScriptStateMachineConstVisitor& visitor, EDomainScope scope) const = 0;
	virtual void                            VisitScriptVariables(const ScriptVariableConstVisitor& visitor, EDomainScope scope) const = 0;
	//virtual void                            VisitInterfaceImpls(const ScriptTimerConstVisitor& visitor, EDomainScope scope) const = 0;
	virtual void                            VisitScriptComponentInstances(const ScriptComponentInstanceConstVisitor& visitor, EDomainScope scope) const = 0;
	virtual void                            VisitScriptActionInstances(const ScriptActionInstanceConstVisitor& visitor, EDomainScope scope) const = 0;

	virtual void                            VisitEnclosedEnums(const ScriptEnumConstVisitor& visitor) const = 0; // #SchematycTODO : Can't we just access the script registry directly? Or should we remove visit functionality from script registry? IScriptRegistry::CreateView?
	virtual void                            VisitAccesibleEnums(const ScriptEnumConstVisitor& visitor) const = 0;
	virtual void                            VisitEnclosedStructs(const ScriptStructConstVisitor& visitor) const = 0; // #SchematycTODO : Can't we just access the script registry directly? Or should we remove visit functionality from script registry? IScriptRegistry::CreateView?
	virtual void                            VisitAccesibleStructs(const ScriptStructConstVisitor& visitor) const = 0;
	virtual void                            VisitEnclosedSignals(const ScriptSignalConstVisitor& visitor) const = 0; // #SchematycTODO : Can't we just access the script registry directly? Or should we remove visit functionality from script registry? IScriptRegistry::CreateView?
	virtual void                            VisitAccesibleSignals(const ScriptSignalConstVisitor& visitor) const = 0;
	virtual void                            VisitEnclosedTimers(const ScriptTimerConstVisitor& visitor) const = 0; // #SchematycTODO : Can't we just access the script registry directly? Or should we remove visit functionality from script registry? IScriptRegistry::CreateView?
	virtual void                            VisitAccesibleTimers(const ScriptTimerConstVisitor& visitor) const = 0;

	virtual const IScriptStateMachine*      GetScriptStateMachine(const SGUID& guid) const = 0;      // #SchematycTODO : Can't we just access the script registry directly?
	virtual const IScriptComponentInstance* GetScriptComponentInstance(const SGUID& guid) const = 0; // #SchematycTODO : Can't we just access the script registry directly?
	virtual const IScriptActionInstance*    GetScriptActionInstance(const SGUID& guid) const = 0;    // #SchematycTODO : Can't we just access the script registry directly?
	virtual const IScriptElement*           GetScriptElement(const SGUID& guid) const = 0;           // #SchematycTODO : Can't we just access the script registry directly?

	virtual bool                            QualifyName(const IScriptComponentInstance& scriptComponentInstance, const IEnvFunction& envFunction, EDomainQualifier qualifier, IString& output) const = 0; // #SchematycTODO : Do we really need to pass a script component instance here? Shouldn't this be inherent from scope?
	virtual bool                            QualifyName(const IEnvClass& envClass, IString& output) const = 0;                                                                                            // #SchematycTODO : Does this really belong here?
	virtual bool                            QualifyName(const IEnvInterface& envInterface, IString& output) const = 0;                                                                                    // #SchematycTODO : Does this really belong here?
	virtual bool                            QualifyName(const IEnvInterfaceFunction& envInterfaceFunction, IString& output) const = 0;                                                                    // #SchematycTODO : Does this really belong here?
	virtual bool                            QualifyName(const IEnvComponent& envComponent, IString& output) const = 0;                                                                                    // #SchematycTODO : Does this really belong here?
	virtual void                            QualifyName(const IEnvElement& envElement, IString& output) const = 0;                                                                                        // #SchematycTODO : Does this really belong here?
	virtual bool                            QualifyName(const IScriptElement& scriptElement, EDomainQualifier qualifier, IString& output) const = 0;                                                      // #SchematycTODO : Does this really belong here?

	//virtual bool                            QualifyName(const IScriptElement& scriptElement, EScriptElementAccessor accessor, IString& output) const = 0;
};

DECLARE_SHARED_POINTERS(IScriptView)
} // Schematyc
