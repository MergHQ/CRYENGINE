// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/IScriptView.h>

namespace Schematyc
{
class CScriptView : public IScriptView
{
public:

	CScriptView(const SGUID& scopeGUID);

	// IScriptView

	virtual const SGUID&                    GetScopeGUID() const override;
	virtual const IEnvClass*                GetEnvClass() const override;
	virtual const IScriptClass*             GetScriptClass() const override;

	virtual void                            VisitEnvDataTypes(const EnvDataTypeConstVisitor& visitor) const override;
	virtual void                            VisitEnvSignals(const EnvSignalConstVisitor& visitor) const override;
	virtual void                            VisitEnvFunctions(const EnvFunctionConstVisitor& visitor) const override;
	virtual void                            VisitEnvClasses(const EnvClassConstVisitor& visitor) const override;
	virtual void                            VisitEnvInterfaces(const EnvInterfaceConstVisitor& visitor) const override;
	virtual void                            VisitEnvComponents(const EnvComponentConstVisitor& visitor) const override;
	virtual void                            VisitEnvActions(const EnvActionConstVisitor& visitor) const override;

	virtual void                            VisitScriptModules(const ScriptModuleConstVisitor& visitor) const override;
	virtual void                            VisitScriptEnums(const ScriptEnumConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptStructs(const ScriptStructConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptSignals(const ScriptSignalConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptFunctions(const ScriptFunctionConstVisitor& visitor) const override;
	virtual void                            VisitScriptClasses(const ScriptClassConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptStates(const ScriptStateConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptStateMachines(const ScriptStateMachineConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptVariables(const ScriptVariableConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptTimers(const ScriptTimerConstVisitor& visitor, EDomainScope scope) const override;
	//virtual void                            VisitInterfaceImpls(const ScriptTimerConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptComponentInstances(const ScriptComponentInstanceConstVisitor& visitor, EDomainScope scope) const override;
	virtual void                            VisitScriptActionInstances(const ScriptActionInstanceConstVisitor& visitor, EDomainScope scope) const override;

	virtual const IScriptStateMachine*      GetScriptStateMachine(const SGUID& guid) const override;
	virtual const IScriptComponentInstance* GetScriptComponentInstance(const SGUID& guid) const override;
	virtual const IScriptActionInstance*    GetScriptActionInstance(const SGUID& guid) const override;
	virtual const IScriptElement*           GetScriptElement(const SGUID& guid) const override;

	virtual bool                            QualifyName(const IScriptComponentInstance& scriptComponentInstance, const IEnvFunction& envFunction, EDomainQualifier qualifier, IString& output) const override;
	virtual bool                            QualifyName(const IEnvInterface& envInterface, IString& output) const override;
	virtual bool                            QualifyName(const IEnvClass& envClass, IString& output) const override;
	virtual bool                            QualifyName(const IEnvInterfaceFunction& envInterfaceFunction, IString& output) const override;
	virtual bool                            QualifyName(const IEnvComponent& envComponent, IString& output) const override;
	virtual void                            QualifyName(const IEnvElement& envElement, IString& output) const override;
	virtual bool                            QualifyName(const IScriptElement& scriptElement, EDomainQualifier qualifier, IString& output) const override;

	// ~IScriptView

private:

	SGUID m_scopeGUID;
};
} // Schematyc
