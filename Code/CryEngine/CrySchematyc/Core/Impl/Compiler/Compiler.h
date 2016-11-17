// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Compiler/ICompiler.h>
#include <Schematyc/Utils/GUID.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvClass;
struct IScriptClass;
struct IScriptConstructor;
struct IScriptDestructor;
struct IScriptElement;
struct IScriptGraph;
struct IScriptSignalReceiver;
struct IScriptStateMachine;
struct IScriptState;
struct IScriptTimer;
struct IScriptVariable;
// Forward declare structures.
struct SCompilerContext;
// Forward declare classes.
class CRuntimeClass;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CRuntimeClass)

class CCompiler : public ICompiler
{
private:

	typedef std::unordered_map<SGUID, CRuntimeClassPtr> Classes;

	struct SSignals
	{
		ClassCompilationSignal classCompilation;
	};

	typedef std::vector<const IScriptClass*> InheritanceChain;

public:

	// ICompiler
	virtual void                           CompileAll() override;
	virtual void                           CompileDependencies(const SGUID& guid) override;
	virtual ClassCompilationSignal::Slots& GetClassCompilationSignalSlots() override;
	// ~ICompiler

private:

	bool CompileClass(const IScriptClass& scriptClass);
	bool CompileComponentInstancesRecursive(SCompilerContext& context, CRuntimeClass& runtimeClass, uint32 parentIdx, const IScriptElement& scriptScope) const;
	bool CompileElementsRecursive(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptElement& scriptScope) const;
	bool CompileConstructor(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptConstructor& scriptConstructor) const;
	bool CompileDestructor(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptDestructor& scriptDestructor) const;
	bool CompileStateMachine(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptStateMachine& scriptStateMachine) const;
	bool CompileState(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptState& scriptState) const;
	bool CompileVariable(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptVariable& scriptVariable) const;
	bool CompileTimer(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptTimer& scriptTimer) const;
	bool CompileSignalReceiver(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptSignalReceiver& scriptSignalReceiver) const;
	bool CompileGraph(SCompilerContext& context, CRuntimeClass& runtimeClass, const IScriptGraph& scriptGraph) const;

private:

	SSignals m_signals;
};
} // Schematyc
