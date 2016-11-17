// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ObjectProperties.h"

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/Runtime/IRuntimeClass.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/Services/ITimerSystem.h>
#include <Schematyc/Utils/GUID.h>
#include <Schematyc/Utils/Scratchpad.h>
#include <Schematyc/Utils/Transform.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IProperties;
// Forward declare classes.
class CAnyConstPtr;
class CAnyConstRef;
class CAnyValue;
class CObjectProperties;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IProperties)
DECLARE_SHARED_POINTERS(CAnyValue)
DECLARE_SHARED_POINTERS(CRuntimeGraph)

struct SRuntimeFunction
{
	SRuntimeFunction();
	SRuntimeFunction(uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	uint32                   graphIdx;
	SRuntimeActivationParams activationParams;
};

struct SRuntimeClassFunction : public SRuntimeFunction
{
	SRuntimeClassFunction(const SGUID& _guid);
	SRuntimeClassFunction(uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	SGUID guid;
};

typedef std::vector<SRuntimeClassFunction> RuntimeClassFunctions;

struct SRuntimeClassConstructor : public SRuntimeFunction
{
	SRuntimeClassConstructor(uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);
};

typedef std::vector<SRuntimeClassConstructor> RuntimeClassConstructors;

struct SRuntimeClassDestructor : public SRuntimeFunction
{
	SRuntimeClassDestructor(uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);
};

typedef std::vector<SRuntimeClassDestructor> RuntimeClassDestructors;

struct SRuntimeClassStateMachine
{
	SRuntimeClassStateMachine(const SGUID& _guid, const char* _szName);

	SGUID            guid;
	string           name;
	SRuntimeFunction beginFunction;
};

typedef std::vector<SRuntimeClassStateMachine> RuntimeClassStateMachines;

struct SRuntimeClassStateTimer : public SRuntimeFunction
{
	SRuntimeClassStateTimer(const SGUID& _guid, const char* _szName, const STimerParams& _params);

	SGUID        guid;
	string       name;
	STimerParams params;
};

typedef std::vector<SRuntimeClassStateTimer> RuntimeClassStateTimers;

struct SRuntimeClassStateSignalReceiver : public SRuntimeFunction
{
	SRuntimeClassStateSignalReceiver(const SGUID& _signalGUID, uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	SGUID signalGUID;
};

typedef std::vector<SRuntimeClassStateSignalReceiver> RuntimeClassStateSignalReceivers;

struct SRuntimeClassStateTransition : public SRuntimeFunction
{
	SRuntimeClassStateTransition(const SGUID& _signalGUID, uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	SGUID signalGUID;
};

typedef std::vector<SRuntimeClassStateTransition> RuntimeClassStateTransitions;

struct SRuntimeClassState
{
	SRuntimeClassState(const SGUID& _guid, const char* _szName);

	SGUID                            guid;
	string                           name;
	RuntimeClassStateTimers          timers;
	RuntimeClassStateSignalReceivers signalReceivers;
	RuntimeClassStateTransitions     transitions;
};

typedef std::vector<SRuntimeClassState> RuntimeClassStates;

struct SRuntimeClassVariable
{
	SRuntimeClassVariable(const SGUID& _guid, const char* _szName, bool _bPublic, uint32 _pos);

	SGUID  guid;
	string name;
	bool   bPublic;
	uint32 pos;
};

typedef std::vector<SRuntimeClassVariable> RuntimeClassVariables;

struct SRuntimeClassTimer
{
	SRuntimeClassTimer(const SGUID& _guid, const char* _szName, const STimerParams& _params);

	SGUID        guid;
	string       name;
	STimerParams params;
};

typedef std::vector<SRuntimeClassTimer> RuntimeClassTimers;

struct SRuntimeClassComponentInstance
{
	SRuntimeClassComponentInstance(const SGUID& _guid, const char* _szName, const SGUID& _componentTypeGUID, const CTransform& _transform, const IProperties* _pProperties, uint32 _parentIdx);

	SGUID          guid;
	string         name;
	SGUID          componentTypeGUID; // #SchematycTODO : Can we store a raw pointer to the env component rather than referencing by GUID?
	CTransform     transform;
	IPropertiesPtr pProperties;
	uint32         parentIdx;
};

typedef std::vector<SRuntimeClassComponentInstance> RuntimeClassComponentInstances;

struct SRuntimeClassSignalReceiver : public SRuntimeFunction
{
	SRuntimeClassSignalReceiver(const SGUID& _signalGUID, uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	SGUID signalGUID;
};

typedef std::vector<SRuntimeClassSignalReceiver> RuntimeClassSignalReceivers;

class CRuntimeClass : public IRuntimeClass
{
private:

	typedef std::unique_ptr<CObjectProperties> PropertiesPtr;
	typedef std::vector<CRuntimeGraphPtr>      Graphs;

public:

	CRuntimeClass(time_t timeStamp, const SGUID& guid, const char* szName, const SGUID& envClassGUID, const CAnyConstPtr& pEnvClassProperties);

	// IRuntimeClass

	virtual time_t                   GetTimeStamp() const override;
	virtual SGUID                    GetGUID() const override;
	virtual const char*              GetName() const override;

	virtual const IObjectProperties& GetDefaultProperties() const override;
	virtual SGUID                    GetEnvClassGUID() const override;
	virtual CAnyConstPtr             GetEnvClassProperties() const override;
	virtual const CScratchpad&       GetScratchpad() const override;

	// ~IRuntimeClass

	uint32                                AddGraph(const SGUID& guid, const char* szName);
	uint32                                FindGraph(const SGUID& guid) const;
	uint32                                GetGraphCount() const;
	CRuntimeGraph*                        GetGraph(uint32 graphIdx);
	const CRuntimeGraph*                  GetGraph(uint32 graphIdx) const;

	uint32                                AddFunction(uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	uint32                                AddFunction(const SGUID& guid, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	uint32                                FindOrReserveFunction(const SGUID& guid);
	const RuntimeClassFunctions&          GetFunctions() const;

	void                                  AddConstructor(uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassConstructors&       GetConstructors() const;

	void                                  AddDestructor(uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassDestructors&        GetDestructors() const;

	uint32                                AddStateMachine(const SGUID& guid, const char* szName);
	uint32                                FindStateMachine(const SGUID& guid) const;
	void                                  SetStateMachineBeginFunction(uint32 stateMachineIdx, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassStateMachines&      GetStateMachines() const;

	uint32                                AddState(const SGUID& guid, const char* szName);
	uint32                                FindState(const SGUID& guid) const;
	void                                  AddStateTimer(uint32 stateIdx, const SGUID& guid, const char* szName, const STimerParams& params);
	void                                  AddStateSignalReceiver(uint32 stateIdx, const SGUID& signalGUID, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	void                                  AddStateTransition(uint32 stateIdx, const SGUID& signalGUID, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassStates&             GetStates() const;

	uint32                                AddVariable(const SGUID& guid, const char* szName, const CAnyConstRef& value);
	uint32                                AddPublicVariable(const SGUID& guid, const char* szName, const CAnyConstRef& value);
	const RuntimeClassVariables&          GetVariables() const;
	uint32                                GetVariablePos(const SGUID& guid) const;

	uint32                                AddTimer(const SGUID& guid, const char* szName, const STimerParams& params);
	const RuntimeClassTimers&             GetTimers() const;

	uint32                                AddComponentInstance(const SGUID& guid, const char* szName, const SGUID& componentTypeGUID, const CTransform& transform, const IProperties* pProperties, uint32 parentIdx);
	uint32                                FindComponentInstance(const SGUID& guid) const;
	const RuntimeClassComponentInstances& GetComponentInstances() const;

	uint32                                AddSignalReceiver(const SGUID& signalGUID, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassSignalReceivers& GetSignalReceivers() const;

	uint32                             CountSignalReceviers(const SGUID& signalGUID) const;
	void                               FinalizeComponentInstances();
	void                               Finalize();

private:

	time_t                         m_timeStamp;
	SGUID                          m_guid;
	string                         m_name;
	PropertiesPtr                  m_pDefaultProperties;

	SGUID                          m_envClassGUID;
	CAnyValuePtr                   m_pEnvClassProperties;

	HeapScratchpad                 m_scratchPad;
	Graphs                         m_graphs;

	RuntimeClassFunctions          m_functions;
	RuntimeClassConstructors       m_constructors;
	RuntimeClassDestructors        m_destructors;
	RuntimeClassStateMachines      m_stateMachines;
	RuntimeClassStates             m_states;
	RuntimeClassVariables          m_variables;
	RuntimeClassTimers             m_timers;
	RuntimeClassComponentInstances m_componentInstances;
	RuntimeClassSignalReceivers    m_signalReceivers;
};
} // Schematyc
