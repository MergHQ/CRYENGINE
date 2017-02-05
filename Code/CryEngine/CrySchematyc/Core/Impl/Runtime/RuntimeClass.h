// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : How can we reduce duplication of functionality between persistent state and intermittent states?
// #SchematycTODO : Create lightweight runtime element id system to replace guids? We can always build an id to guid map (or vice-versa) if we need access to guids at runtime (e.g. for debugging).

#pragma once

#include "ObjectProperties.h"

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/Runtime/IRuntimeClass.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/Services/ITimerSystem.h>
#include <Schematyc/Utils/ClassProperties.h>
#include <Schematyc/Utils/GUID.h>
#include <Schematyc/Utils/Scratchpad.h>
#include <Schematyc/Utils/Transform.h>

namespace Schematyc
{

// Forward declare classes.
class CAnyConstPtr;
class CAnyConstRef;
class CAnyValue;
class CObjectProperties;
// Forward declare shared pointers.
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

struct SRuntimeClassStateSignalReceiver : public SRuntimeFunction // #SchematycTODO : Rename signal receiver to signal mapping / connection?
{
	SRuntimeClassStateSignalReceiver(const SGUID& _signalGUID, const SGUID& _senderGUID, uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	SGUID signalGUID;
	SGUID senderGUID;
};

typedef std::vector<SRuntimeClassStateSignalReceiver> RuntimeClassStateSignalReceivers;

struct SRuntimeClassStateTransition : public SRuntimeFunction
{
	SRuntimeClassStateTransition(const SGUID& _signalGUID, uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	SGUID signalGUID;
};

typedef std::vector<SRuntimeClassStateTransition> RuntimeClassStateTransitions;

struct SRuntimeStateActionDesc
{
	SRuntimeStateActionDesc(const SGUID& _guid, const SGUID& _typeGUID);

	SGUID guid;
	SGUID typeGUID; // #SchematycTODO : Can we store a raw pointer to the env action rather than referencing by GUID?
};

typedef std::vector<SRuntimeStateActionDesc> RuntimeStateActionDescs;

struct SRuntimeClassState
{
	SRuntimeClassState(const SGUID& _guid, const char* _szName);

	SGUID                            guid;
	string                           name;
	RuntimeClassStateTimers          timers;
	RuntimeClassStateSignalReceivers signalReceivers;
	RuntimeClassStateTransitions     transitions;
	RuntimeStateActionDescs          actions;
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
	SRuntimeClassComponentInstance(const SGUID& _guid, const char* _szName, bool _bPublic, const SGUID& _componentTypeGUID, const CTransform& _transform, const CClassProperties& _properties, uint32 _parentIdx);

	SGUID            guid;
	string           name;
	bool             bPublic;
	SGUID            componentTypeGUID;    // #SchematycTODO : Can we store a raw pointer to the env component rather than referencing by GUID?
	CTransform       transform;
	CClassProperties properties;
	uint32           parentIdx;
};

typedef std::vector<SRuntimeClassComponentInstance> RuntimeClassComponentInstances;

struct SRuntimeActionDesc
{
	SRuntimeActionDesc(const SGUID& _guid, const SGUID& _typeGUID);

	SGUID guid;
	SGUID typeGUID; // #SchematycTODO : Can we store a raw pointer to the env action rather than referencing by GUID?
};

typedef std::vector<SRuntimeActionDesc> RuntimeActionDescs;

struct SRuntimeClassSignalReceiver : public SRuntimeFunction // #SchematycTODO : Rename signal receiver to signal mapping / connection?
{
	SRuntimeClassSignalReceiver(const SGUID& _signalGUID, const SGUID& _senderGUID, uint32 _graphIdx, const SRuntimeActivationParams& _activationParams);

	SGUID signalGUID;
	SGUID senderGUID;
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

	uint32                                AddStateMachine(const SGUID& guid, const char* szName);
	uint32                                FindStateMachine(const SGUID& guid) const;
	void                                  SetStateMachineBeginFunction(uint32 stateMachineIdx, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassStateMachines&      GetStateMachines() const;

	uint32                                AddState(const SGUID& guid, const char* szName);
	uint32                                FindState(const SGUID& guid) const;
	void                                  AddStateTimer(uint32 stateIdx, const SGUID& guid, const char* szName, const STimerParams& params);
	void                                  AddStateSignalReceiver(uint32 stateIdx, const SGUID& signalGUID, const SGUID& senderGUID, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	uint32                                AddStateAction(uint32 stateIdx, const SGUID& guid, const SGUID& typeGUID);
	void                                  AddStateTransition(uint32 stateIdx, const SGUID& signalGUID, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassStates&             GetStates() const;

	uint32                                AddVariable(const SGUID& guid, const char* szName, bool bPublic, const CAnyConstRef& value);
	const RuntimeClassVariables&          GetVariables() const;
	uint32                                GetVariablePos(const SGUID& guid) const;

	uint32                                AddTimer(const SGUID& guid, const char* szName, const STimerParams& params);
	const RuntimeClassTimers&             GetTimers() const;

	uint32                                AddComponentInstance(const SGUID& guid, const char* szName, bool bPublic, const SGUID& componentTypeGUID, const CTransform& transform, const CClassProperties& properties, uint32 parentIdx);
	uint32                                FindComponentInstance(const SGUID& guid) const;
	const RuntimeClassComponentInstances& GetComponentInstances() const;

	uint32                                AddSignalReceiver(const SGUID& signalGUID, const SGUID& senderGUID, uint32 graphIdx, const SRuntimeActivationParams& activationParams);
	const RuntimeClassSignalReceivers&    GetSignalReceivers() const;

	uint32                                AddAction(const SGUID& guid, const SGUID& typeGUID);
	const RuntimeActionDescs&             GetActions() const;

	uint32                                CountSignalReceviers(const SGUID& signalGUID) const;
	void                                  FinalizeComponentInstances();
	void                                  Finalize();

private:

	time_t                         m_timeStamp;
	SGUID                          m_guid;
	string                         m_name;
	PropertiesPtr                  m_pDefaultProperties;

	SGUID                          m_envClassGUID;
	CAnyValuePtr                   m_pEnvClassProperties;

	HeapScratchpad                 m_scratchpad;
	Graphs                         m_graphs;

	RuntimeClassFunctions          m_functions;
	RuntimeClassConstructors       m_constructors;
	RuntimeClassStateMachines      m_stateMachines;
	RuntimeClassStates             m_states;
	RuntimeClassVariables          m_variables;
	RuntimeClassTimers             m_timers;
	RuntimeClassComponentInstances m_componentInstances;
	RuntimeClassSignalReceivers    m_signalReceivers;
	RuntimeActionDescs             m_actions;
};

} // Schematyc
