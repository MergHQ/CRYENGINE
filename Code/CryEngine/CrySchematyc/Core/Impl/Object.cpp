// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Object.h"

#include <Schematyc/Component.h>
#include <Schematyc/IObjectProperties.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvComponent.h>
#include <Schematyc/Runtime/IRuntimeClass.h>
#include <Schematyc/Services/IUpdateScheduler.h>
#include <Schematyc/Utils/Assert.h>
#include <Schematyc/Utils/IProperties.h>
#include <Schematyc/Utils/STLUtils.h>

#include "Core.h"
#include "CVars.h"
#include "CoreEnv/CoreEnvSignals.h"
#include "Runtime/RuntimeClass.h"
#include "Runtime/RuntimeRegistry.h"

namespace Schematyc
{
CObject::SStateMachine::SStateMachine()
	: stateIdx(InvalidIdx)
{}

CObject::SComponent::SComponent(const CComponentPtr& _pComponent, const CTransform& _transform, const IProperties* _pProperties, IComponentPreviewer* _pPreviewer, uint32 _parentIdx)
	: pComponent(_pComponent)
	, transform(_transform)
	, pProperties(_pProperties ? _pProperties->Clone() : IPropertiesPtr())
	, pPreviewer(_pPreviewer)
	, parentIdx(_parentIdx)
{}

CObject::STimer::STimer(CObject* _pObject, const SGUID& _guid, TimerFlags _flags)
	: pObject(_pObject)
	, guid(_guid)
	, flags(_flags)
{}

void CObject::STimer::Activate()
{
	SCHEMATYC_CORE_ASSERT(pObject);
	if (pObject)
	{
		StackRuntimeParams inputs; // #SchematycTODO : Would it be helpful to pass delta time and perhaps even a counter?
		pObject->ProcessSignal(guid, inputs);
	}
}

CObject::CObject(ObjectId id)
	: m_id(id)
	, m_simulationMode(ESimulationMode::Idle)
	, m_bQueueSignals(false)
{}

CObject::~CObject()
{
	Stop();
	DestroyTimers();
	DestroyComponents();
	DestroyStateMachines();
}

bool CObject::Init(const CRuntimeClassConstPtr& pClass, void* pCustomData, const IObjectPropertiesConstPtr& pProperties, ESimulationMode simulationMode)
{
	if (!SetClass(pClass))
	{
		return false;
	}

	m_pCustomData = pCustomData;
	m_pProperties = pProperties;

	if (!Start(simulationMode))
	{
		return false;
	}

	return true;
}

ObjectId CObject::GetId() const
{
	return m_id;
}

const IRuntimeClass& CObject::GetClass() const
{
	SCHEMATYC_CORE_ASSERT(m_pClass);
	return *m_pClass;
}

void* CObject::GetCustomData() const
{
	return m_pCustomData;
}

ESimulationMode CObject::GetSimulationMode() const
{
	return m_simulationMode;
}

bool CObject::Reset(ESimulationMode simulationMode, EObjectResetPolicy resetPolicy)
{
	if ((simulationMode != m_simulationMode) || (resetPolicy == EObjectResetPolicy::Always))
	{
		if (m_simulationMode != ESimulationMode::Idle)
		{
			Stop();
		}

		if (simulationMode != ESimulationMode::Idle)
		{
			CRuntimeClassConstPtr pClass = CCore::GetInstance().GetRuntimeRegistryImpl().GetClassImpl(m_pClass->GetGUID());
			if (pClass->GetTimeStamp() > m_pClass->GetTimeStamp())
			{
				SetClass(pClass);
			}

			if (!Start(simulationMode))
			{
				Stop();
				return false;
			}
		}
	}
	return true;
}

void CObject::ProcessSignal(const SGUID& signalGUID, CRuntimeParams& params)
{
	if (m_bQueueSignals)
	{
		m_signalQueue.emplace_back(signalGUID, params);
	}
	else
	{
		m_bQueueSignals = true;

		ExecuteSignalReceivers(signalGUID, params);

		const uint32 stateMachineCount = m_stateMachines.size();
		for (uint32 stateMachineIdx = 0; stateMachineIdx < stateMachineCount; ++stateMachineIdx)
		{
			ExecuteStateSignalReceivers(stateMachineIdx, signalGUID, params);
		}

		for (uint32 stateMachineIdx = 0; stateMachineIdx < stateMachineCount; ++stateMachineIdx)
		{
			if (EvaluateStateTransitions(stateMachineIdx, signalGUID, params))
			{
				break;
			}
		}

		SCHEMATYC_CORE_ASSERT(m_bQueueSignals);
		m_bQueueSignals = false;
		ProcessSignalQueue();
	}
}

EVisitResult CObject::VisitComponents(const ObjectComponentConstVisitor& visitor) const
{
	SCHEMATYC_CORE_ASSERT(!visitor.IsEmpty());
	if (!visitor.IsEmpty())
	{
		for (const SComponent& component : m_components)
		{
			const EVisitStatus status = visitor(*component.pComponent);
			switch (status)
			{
			case EVisitStatus::Stop:
				{
					return EVisitResult::Stopped;
				}
			case EVisitStatus::Error:
				{
					return EVisitResult::Error;
				}
			}
		}
	}
	return EVisitResult::Complete;
}

void CObject::Dump(IObjectDump& dump, const ObjectDumpFlags& flags) const
{
	if (flags.Check(EObjectDumpFlags::States))
	{
		const RuntimeClassStateMachines& classStateMachines = m_pClass->GetStateMachines();
		const RuntimeClassStates& classStates = m_pClass->GetStates();
		for (uint32 stateMachineIdx = 0, stateMachineCount = m_stateMachines.size(); stateMachineIdx < stateMachineCount; ++stateMachineIdx)
		{
			const SRuntimeClassStateMachine& classStateMachine = classStateMachines[stateMachineIdx];
			IObjectDump::SStateMachine dumpStateMachine(classStateMachine.guid, classStateMachine.name.c_str());

			const uint32 stateIdx = m_stateMachines[stateMachineIdx].stateIdx;
			if (stateIdx != InvalidIdx)
			{
				const SRuntimeClassState& classState = classStates[stateIdx];
				dumpStateMachine.state.guid = classState.guid;
				dumpStateMachine.state.szName = classState.name.c_str();
			}
			dump(dumpStateMachine);
		}
	}

	if (flags.Check(EObjectDumpFlags::Variables))
	{
		const RuntimeClassVariables& classVariables = m_pClass->GetVariables();
		for (const SRuntimeClassVariable& classVariable : classVariables)
		{
			const IObjectDump::SVariable dumpVariable(classVariable.guid, classVariable.name.c_str(), *m_scratchPad.Get(classVariable.pos));
			dump(dumpVariable);
		}
	}

	if (flags.Check(EObjectDumpFlags::Timers))
	{
		ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
		const RuntimeClassTimers& classTimers = m_pClass->GetTimers();
		for (uint32 timerIdx = 0, timerCount = m_timers.size(); timerIdx < timerCount; ++timerIdx)
		{
			const SRuntimeClassTimer& classTimer = classTimers[timerIdx];
			IObjectDump::STimer dumpTimer(classTimer.guid, classTimer.name.c_str(), timerSystem.GetTimeRemaining(m_timers[timerIdx].id));
			dump(dumpTimer);
		}
	}
}

CScratchpad& CObject::GetScratchpad()
{
	return m_scratchPad;
}

CComponent* CObject::GetComponent(uint32 componentIdx)
{
	return componentIdx < m_components.size() ? m_components[componentIdx].pComponent.get() : nullptr;
}

void CObject::ExecuteFunction(uint32 functionIdx, CRuntimeParams& params)
{
	const bool bPrevQueueSignals = m_bQueueSignals;
	m_bQueueSignals = true;

	const RuntimeClassFunctions& classFunctions = m_pClass->GetFunctions();
	if (functionIdx < classFunctions.size())
	{
		const SRuntimeClassFunction& classFunction = classFunctions[functionIdx];
		ExecuteFunction(classFunction.graphIdx, params, classFunction.activationParams);
	}

	if (!bPrevQueueSignals)
	{
		ProcessSignalQueue();
	}
	m_bQueueSignals = bPrevQueueSignals;
}

bool CObject::SetClass(const CRuntimeClassConstPtr& pClass)
{
	SCHEMATYC_CORE_ASSERT(pClass)
	if (!pClass)
	{
		return false;
	}

	DestroyTimers();
	DestroyComponents();
	DestroyStateMachines();

	m_graphs.clear();
	m_stateMachines.clear();
	m_components.clear();
	m_timers.clear();

	m_pClass = pClass;
	m_scratchPad = pClass->GetScratchpad(); // #SchematycTODO : Do we still need to do this here? We also reset the scratchpad in the start function.

	CreateGraphs();

	if (!CreateStateMachines())
	{
		return false;
	}

	if (!CreateComponents())
	{
		return false;
	}

	if (!CreateTimers())
	{
		return false;
	}

	RegisterForUpdate();

	return true;
}

bool CObject::Start(ESimulationMode simulationMode)
{
	m_scratchPad = m_pClass->GetScratchpad();

	ResetGraphs();

	if (!ReadProperties())
	{
		return false;
	}

	if (!InitComponents())
	{
		return false;
	}

	ExecuteConstructors(simulationMode);
	RunComponents(simulationMode);

	switch (simulationMode)
	{
	case ESimulationMode::Game:
		{
			StackRuntimeParams params;
			ExecuteSignalReceivers(g_startSignalGUID, params);

			StartStateMachines(simulationMode);
			StartTimers(simulationMode);
			break;
		}
	}

	m_simulationMode = simulationMode;

	return true;
}

void CObject::Stop()
{
	StopTimers();
	StopStateMachines();

	switch (m_simulationMode)
	{
	case ESimulationMode::Game:
		{
			StackRuntimeParams params;
			ExecuteSignalReceivers(g_stopSignalGUID, params);
			break;
		}
	}

	ShutdownComponents();

	m_simulationMode = ESimulationMode::Idle;
}

void CObject::Update(const SUpdateContext& updateContext)
{
	StackRuntimeParams runtimeParams;
	runtimeParams.SetInput('time', updateContext.frameTime);
	ProcessSignal(g_updateSignalGUID, runtimeParams);
}

void CObject::CreateGraphs()
{
	const uint32 graphCount = m_pClass->GetGraphCount();
	m_graphs.reserve(graphCount);
	for (uint32 graphIdx = 0; graphIdx < graphCount; ++graphIdx)
	{
		m_graphs.push_back(CRuntimeGraphInstance(*m_pClass->GetGraph(graphIdx)));
	}
}

void CObject::ResetGraphs()
{
	for (CRuntimeGraphInstance& graph : m_graphs)
	{
		graph.Reset();
	}
}

bool CObject::ReadProperties()
{
	if (m_pProperties)
	{
		const RuntimeClassComponentInstances& classComponentInstances = m_pClass->GetComponentInstances();
		for (uint32 componentIdx = 0, componentCount = m_components.size(); componentIdx < componentCount; ++componentIdx)
		{
			const SRuntimeClassComponentInstance& classComponentInstance = classComponentInstances[componentIdx];
			if (classComponentInstance.bPublic)
			{
				const IProperties* pComponentProperties = m_pProperties->GetComponentProperties(classComponentInstance.guid);
				if (pComponentProperties)
				{
					m_components[componentIdx].pProperties = pComponentProperties->Clone();
				}
				else
				{
					SCHEMATYC_CORE_ERROR("Failed to initialize component properties: class = %s, component = %s", m_pClass->GetName(), classComponentInstance.name.c_str());
					return false;
				}
			}
		}

		for (const SRuntimeClassVariable& variable : m_pClass->GetVariables())
		{
			if (variable.bPublic)
			{
				if (!m_pProperties->ReadVariable(*m_scratchPad.Get(variable.pos), variable.guid))
				{
					SCHEMATYC_CORE_ERROR("Failed to initialize variable: class = %s, variable = %s", m_pClass->GetName(), variable.name.c_str());
					return false;
				}
			}
		}
	}

	return true;
}

void CObject::ExecuteConstructors(ESimulationMode simulationMode)
{
	StackRuntimeParams params;
	for (const SRuntimeClassConstructor& classConstructor : m_pClass->GetConstructors())
	{
		ExecuteFunction(classConstructor.graphIdx, params, classConstructor.activationParams);
	}
}

bool CObject::CreateStateMachines()
{
	m_stateMachines.resize(m_pClass->GetStateMachines().size());

	// #SchematycTODO : Move to separate CreateStates function?
	const RuntimeClassStates& classStates = m_pClass->GetStates();
	const uint32 stateCount = classStates.size();
	m_states.resize(stateCount);

	ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
	for (uint32 stateIdx = 0; stateIdx < stateCount; ++stateIdx)
	{
		const SRuntimeClassState& classState = classStates[stateIdx];
		SState& state = m_states[stateIdx];

		const uint32 stateTimerCount = classState.timers.size();
		state.timers.reserve(stateTimerCount);

		for (const SRuntimeClassStateTimer& classStateTimer : classState.timers)
		{
			state.timers.emplace_back(this, classStateTimer.guid, classStateTimer.params.flags);
			STimer& timer = state.timers.back();

			STimerParams timerParams = classStateTimer.params;
			timerParams.flags.Remove(ETimerFlags::AutoStart);

			timer.id = timerSystem.CreateTimer(timerParams, Delegate::Make(timer, &STimer::Activate));
		}
	}

	return true;
}

void CObject::StartStateMachines(ESimulationMode simulationMode)
{
	const RuntimeClassStateMachines& classStateMachines = m_pClass->GetStateMachines();
	for (uint32 stateMachineIdx = 0, stateMachineCount = m_stateMachines.size(); stateMachineIdx < stateMachineCount; ++stateMachineIdx)
	{
		StackRuntimeParams params;

		ExecuteFunction(classStateMachines[stateMachineIdx].beginFunction, params);

		const uint32* pStateIdx = DynamicCast<uint32>(params.GetOutput(0));
		if (pStateIdx)
		{
			ChangeState(stateMachineIdx, *pStateIdx);
		}
	}
}

void CObject::StopStateMachines()
{
	for (uint32 stateMachineIdx = 0, stateMachineCount = m_stateMachines.size(); stateMachineIdx < stateMachineCount; ++stateMachineIdx)
	{
		ChangeState(stateMachineIdx, InvalidIdx);
	}
}

void CObject::DestroyStateMachines()
{
	// #SchematycTODO : Move to separate DestroyStates function?
	ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
	for (SState& state : m_states)
	{
		for (STimer& timer : state.timers)
		{
			timerSystem.DestroyTimer(timer.id);
			timer.id = TimerId::Invalid;
		}
	}
}

bool CObject::CreateComponents()
{
	const RuntimeClassComponentInstances& classComponentInstances = m_pClass->GetComponentInstances();
	const uint32 componentCount = classComponentInstances.size();

	m_components.reserve(componentCount);

	for (const SRuntimeClassComponentInstance& classComponentInstance : classComponentInstances)
	{
		const IEnvComponent* pEnvComponent = gEnv->pSchematyc->GetEnvRegistry().GetComponent(classComponentInstance.componentTypeGUID);
		if (!pEnvComponent)
		{
			return false;
		}

		CComponentPtr pComponent = pEnvComponent->Create();
		if (!pComponent)
		{
			return false;
		}

		m_components.emplace_back(pComponent, classComponentInstance.transform, classComponentInstance.pProperties.get(), pEnvComponent->GetPreviewer(), classComponentInstance.parentIdx);
	}
	return true;
}

bool CObject::InitComponents()
{
	for (SComponent& component : m_components)
	{
		SComponentParams componentParams(*this, component.transform);
		componentParams.pProperties = component.pProperties ? component.pProperties->GetValue() : nullptr;
		componentParams.pPreviewer = component.pPreviewer;
		componentParams.pParent = component.parentIdx != InvalidIdx ? m_components[component.parentIdx].pComponent.get() : nullptr;
		//componentParams.pNetworkSpawnParams = pNetworkSpawnParams ? pNetworkSpawnParams->GetComponentSpawnParams(componentInstanceIdx) : INetworkSpawnParamsPtr();
		component.pComponent->PreInit(componentParams);
		if (!component.pComponent->Init())
		{
			return false;
		}
	}
	return true;
}

void CObject::RunComponents(ESimulationMode simulationMode)
{
	for (SComponent& component : m_components)
	{
		component.pComponent->Run(simulationMode);
	}
}

void CObject::ShutdownComponents()
{
	for (SComponent& component : stl::reverse(m_components))
	{
		component.pComponent->Shutdown();
	}
}

void CObject::DestroyComponents()
{
	for (SComponent& component : stl::reverse(m_components))
	{
		component.pComponent->Shutdown();
	}
}

bool CObject::CreateTimers()
{
	const RuntimeClassTimers& classTimers = m_pClass->GetTimers();
	m_timers.reserve(classTimers.size());

	ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
	for (const SRuntimeClassTimer& classTimer : classTimers)
	{
		m_timers.emplace_back(this, classTimer.guid, classTimer.params.flags);
		STimer& timer = m_timers.back();

		STimerParams timerParams = classTimer.params;
		timerParams.flags.Remove(ETimerFlags::AutoStart);

		timer.id = timerSystem.CreateTimer(timerParams, Delegate::Make(timer, &STimer::Activate));
	}
	return true;
}

void CObject::StartTimers(ESimulationMode simulationMode)
{
	if (simulationMode == ESimulationMode::Game)
	{
		ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
		for (const STimer& timer : m_timers)
		{
			if (timer.flags.Check(ETimerFlags::AutoStart))
			{
				timerSystem.StartTimer(timer.id);
			}
		}
	}
}

void CObject::StopTimers()
{
	ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
	for (const STimer& timer : m_timers)
	{
		timerSystem.StopTimer(timer.id);
	}
}

void CObject::DestroyTimers()
{
	ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
	for (STimer& timer : m_timers)
	{
		timerSystem.DestroyTimer(timer.id);
		timer.id = TimerId::Invalid;
	}
}

void CObject::RegisterForUpdate()
{
	if (m_pClass->CountSignalReceviers(g_updateSignalGUID))
	{
		SUpdateParams updateParams(Delegate::Make(*this, &CObject::Update), m_connectionScope);
		updateParams.frequency = EUpdateFrequency::EveryFrame;
		// #SchematycTODO : Create an update filter?
		gEnv->pSchematyc->GetUpdateScheduler().Connect(updateParams);
	}
}

void CObject::ExecuteSignalReceivers(const SGUID& signalGUID, CRuntimeParams& params)
{
	StackRuntimeParams outputs;
	for (const SRuntimeClassSignalReceiver& classSignalReceiver : m_pClass->GetSignalReceivers())
	{
		if (classSignalReceiver.signalGUID == signalGUID)
		{
			ExecuteFunction(classSignalReceiver.graphIdx, params, classSignalReceiver.activationParams);
		}
	}
}

void CObject::StartStateTimers(uint32 stateMachineIdx)
{
	const uint32 stateIdx = m_stateMachines[stateMachineIdx].stateIdx;
	if (stateIdx != InvalidIdx)
	{
		ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
		for (const STimer& timer : m_states[stateIdx].timers)
		{
			if (timer.flags.Check(ETimerFlags::AutoStart))
			{
				timerSystem.StartTimer(timer.id);
			}
		}
	}
}

void CObject::StopStateTimers(uint32 stateMachineIdx)
{
	const uint32 stateIdx = m_stateMachines[stateMachineIdx].stateIdx;
	if (stateIdx != InvalidIdx)
	{
		ITimerSystem& timerSystem = gEnv->pSchematyc->GetTimerSystem();
		for (const STimer& timer : m_states[stateIdx].timers)
		{
			timerSystem.StopTimer(timer.id);
		}
	}
}

void CObject::ExecuteStateSignalReceivers(uint32 stateMachineIdx, const SGUID& signalGUID, CRuntimeParams& params)
{
	const uint32 stateIdx = m_stateMachines[stateMachineIdx].stateIdx;
	if (stateIdx != InvalidIdx)
	{
		const RuntimeClassStates& classStates = m_pClass->GetStates();
		for (const SRuntimeClassStateSignalReceiver& classStateSignalReceiver : classStates[stateIdx].signalReceivers)
		{
			if (classStateSignalReceiver.signalGUID == signalGUID)
			{
				ExecuteFunction(classStateSignalReceiver.graphIdx, params, classStateSignalReceiver.activationParams);
			}
		}
	}
}

bool CObject::EvaluateStateTransitions(uint32 stateMachineIdx, const SGUID& signalGUID, CRuntimeParams& params)
{
	const uint32 stateIdx = m_stateMachines[stateMachineIdx].stateIdx;
	if (stateIdx != InvalidIdx)
	{
		const RuntimeClassStates& classStates = m_pClass->GetStates();
		for (const SRuntimeClassStateTransition& classStateTransition : classStates[stateIdx].transitions)
		{
			if (classStateTransition.signalGUID == signalGUID)
			{
				ExecuteFunction(classStateTransition.graphIdx, params, classStateTransition.activationParams);

				const uint32* pStateIdx = DynamicCast<uint32>(params.GetOutput(0));
				if (pStateIdx)
				{
					ChangeState(stateMachineIdx, *pStateIdx);
					return true;
				}
			}
		}
	}
	return false;
}

void CObject::ChangeState(uint32 stateMachineIdx, uint32 stateIdx)
{
	StackRuntimeParams params;
	StopStateTimers(stateMachineIdx);
	ExecuteStateSignalReceivers(stateMachineIdx, g_stopSignalGUID, params);
	m_stateMachines[stateMachineIdx].stateIdx = stateIdx;
	ExecuteStateSignalReceivers(stateMachineIdx, g_startSignalGUID, params);
	StartStateTimers(stateMachineIdx);
}

void CObject::ExecuteFunction(const SRuntimeFunction& function, CRuntimeParams& params)
{
	if (function.graphIdx != InvalidIdx)
	{
		m_graphs[function.graphIdx].Execute(this, params, function.activationParams);
	}
}

void CObject::ExecuteFunction(uint32 graphIdx, CRuntimeParams& params, SRuntimeActivationParams activationParams)
{
	m_graphs[graphIdx].Execute(this, params, activationParams);
}

void CObject::ProcessSignalQueue()
{
	while (!m_signalQueue.empty())
	{
		SObjectSignal signal = m_signalQueue.front();
		m_signalQueue.pop_front();
		ProcessSignal(signal.guid, signal.params);
	}
}
} // Schematyc
