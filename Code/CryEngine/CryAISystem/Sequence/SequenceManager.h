// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Sequence.h"

namespace AIActionSequence
{

class SequenceManager : public ISequenceManager
{
public:
	SequenceManager()
		: m_sequenceIdCounter(1)
		, m_pAgentAdapter(nullptr)
	{
	}

	~SequenceManager()
	{
	}

	void Reset();

	// AIActionSequence::ISequenceManager
	virtual bool RegisterSequence(EntityId entityId, TFlowNodeId startNodeId, SequenceProperties sequenceProperties, IFlowGraph* flowGraph) override;
	virtual void UnregisterSequence(SequenceId sequenceId) override;

	virtual void StartSequence(SequenceId sequenceId) override;
	virtual void CancelSequence(SequenceId sequenceId) override;
	virtual bool IsSequenceActive(SequenceId sequenceId) override;

	virtual void SequenceBehaviorReady(EntityId entityId) override;
	virtual void SequenceInterruptibleBehaviorLeft(EntityId entityId) override;
	virtual void SequenceNonInterruptibleBehaviorLeft(EntityId entityId) override;
	virtual void AgentDisabled(EntityId entityId) override;

	virtual void RequestActionStart(SequenceId sequenceId, TFlowNodeId actionNodeId) override;
	virtual void RequestActionRestart(SequenceId sequenceId) override;
	virtual void ActionCompleted(SequenceId sequenceId) override;
	virtual void SetBookmark(SequenceId sequenceId, TFlowNodeId bookmarkNodeId) override;

	virtual void SetAgentAdapter(IAgentAdapter* pAgentAdapter) override;
	// ~AIActionSequence::ISequenceManager

	IAgentAdapter* InitAgentAdapter(EntityId entityId);

private:
	SequenceId GenerateUniqueSequenceId();
	Sequence*  GetSequence(SequenceId sequenceId);
	void       CancelSequence(Sequence& sequence);
	void       CancelActiveSequencesForThisEntity(EntityId entityId);

	typedef std::map<SequenceId, Sequence> SequenceMap;
	SequenceMap m_registeredSequences;
	SequenceId  m_sequenceIdCounter;

	IAgentAdapter* m_pAgentAdapter;
};

} // namespace AIActionSequence
