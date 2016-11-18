// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/IObject.h>
#include <Schematyc/Runtime/RuntimeGraph.h>
#include <Schematyc/Runtime/RuntimeParams.h>
#include <Schematyc/Services/ITimerSystem.h>
#include <Schematyc/Utils/Transform.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IComponentPreviewer;
struct IObjectProperties;
struct IProperties;
// Forward declare structures.
struct SRuntimeFunction;
struct SUpdateContext;
// Forward declare classes.
class CComponent;
class CRuntimeClass;
class CScratchpad;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IObjectProperties)
DECLARE_SHARED_POINTERS(IProperties)
DECLARE_SHARED_POINTERS(CComponent)
DECLARE_SHARED_POINTERS(CRuntimeClass)

struct SObjectSignal
{
	inline SObjectSignal(const SGUID& _guid, const CRuntimeParams& _params)
		: guid(_guid)
		, params(_params)
	{}

	SGUID              guid;
	StackRuntimeParams params;
};

typedef std::deque<SObjectSignal> ObjectSignalQueue;

class CObject : public IObject
{
private:

	typedef std::vector<CRuntimeGraphInstance> Graphs;

	struct SStateMachine
	{
		SStateMachine();

		uint32 stateIdx;
	};

	typedef std::vector<SStateMachine> StateMachines;

	struct SComponent
	{
		SComponent(const CComponentPtr& _pComponent, const CTransform& _transform, const IProperties* _pProperties, IComponentPreviewer* _pPreviewer, uint32 _parentIdx);

		CComponentPtr        pComponent;
		CTransform           transform;
		IPropertiesPtr       pProperties; // #SchematycTODO : Can we avoid duplicating this (stored both here and in the component itself)?
		IComponentPreviewer* pPreviewer;  // #SchematycTODO : Can we avoid duplicating this (stored both here and in the component itself)?
		uint32               parentIdx;   // #SchematycTODO : Can we store a raw pointer to the component rather than referencing by index?
	};

	typedef std::vector<SComponent> Components;

	struct STimer
	{
		STimer(CObject* _pObject, const SGUID& _guid, TimerFlags _flags);

		void Activate();

		CObject*   pObject;
		SGUID      guid;
		TimerFlags flags;
		TimerId    id;
	};

	typedef std::vector<STimer> Timers;

	struct SState
	{
		Timers timers;
	};

	typedef std::vector<SState> States;

public:

	CObject(ObjectId id);

	~CObject();

	bool Init(const CRuntimeClassConstPtr& pClass, void* pCustomData, const IObjectPropertiesConstPtr& pProperties, ESimulationMode simulationMode);

	// IObject

	virtual ObjectId             GetId() const override;
	virtual const IRuntimeClass& GetClass() const override;
	virtual void*                GetCustomData() const override;
	virtual ESimulationMode      GetSimulationMode() const override;

	virtual bool                 Reset(ESimulationMode simulationMode, EObjectResetPolicy resetPolicy) override;
	virtual void                 ProcessSignal(const SGUID& signalGUID, CRuntimeParams& params) override;

	virtual EVisitResult         VisitComponents(const ObjectComponentConstVisitor& visitor) const override;
	virtual void                 Dump(IObjectDump& dump, const ObjectDumpFlags& flags = EObjectDumpFlags::All) const override;

	// ~IObject

	CScratchpad& GetScratchpad();
	CComponent*  GetComponent(uint32 componentIdx);
	void         ExecuteFunction(uint32 functionIdx, CRuntimeParams& params);

private:

	bool SetClass(const CRuntimeClassConstPtr& pClass);

	bool Start(ESimulationMode simulationMode);
	void Stop();
	void Update(const SUpdateContext& updateContext);

	void CreateGraphs();
	void ResetGraphs();

	bool ReadProperties();

	void ExecuteConstructors(ESimulationMode simulationMode);

	bool CreateStateMachines();
	void StartStateMachines(ESimulationMode simulationMode);
	void StopStateMachines();
	void DestroyStateMachines();

	bool CreateComponents();
	bool InitComponents();
	void RunComponents(ESimulationMode simulationMode);
	void ShutdownComponents();
	void DestroyComponents();

	bool CreateTimers();
	void StartTimers(ESimulationMode simulationMode);
	void StopTimers();
	void DestroyTimers();

	void RegisterForUpdate();

	void ExecuteSignalReceivers(const SGUID& signalGUID, CRuntimeParams& params);

	void StartStateTimers(uint32 stateMachineIdx);
	void StopStateTimers(uint32 stateMachineIdx);
	void ExecuteStateSignalReceivers(uint32 stateMachineIdx, const SGUID& signalGUID, CRuntimeParams& params);
	bool EvaluateStateTransitions(uint32 stateMachineIdx, const SGUID& signalGUID, CRuntimeParams& params);
	void ChangeState(uint32 stateMachineIdx, uint32 stateIdx);

	void ExecuteFunction(const SRuntimeFunction& function, CRuntimeParams& params);
	void ExecuteFunction(uint32 graphIdx, CRuntimeParams& params, SRuntimeActivationParams activationParams);
	void ProcessSignalQueue();

private:

	ObjectId                  m_id;

	CRuntimeClassConstPtr     m_pClass;
	void*                     m_pCustomData;
	IObjectPropertiesConstPtr m_pProperties;
	ESimulationMode           m_simulationMode;

	HeapScratchpad            m_scratchPad;
	Graphs                    m_graphs;

	StateMachines             m_stateMachines;
	Components                m_components;
	Timers                    m_timers;
	States                    m_states;

	bool                      m_bQueueSignals;
	ObjectSignalQueue         m_signalQueue;

	CConnectionScope          m_connectionScope;
};
} // Schematyc
