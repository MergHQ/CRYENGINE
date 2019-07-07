// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ISequencerSystem.h"

class CSequencerSequence
	: public _i_reference_target_t
{
public:
	CSequencerSequence();
	virtual ~CSequencerSequence();

	void            SetName(const char* name);
	const char*     GetName() const;

	void            SetTimeRange(Range timeRange);
	Range           GetTimeRange() { return m_timeRange; }

	int             GetNodeCount() const;
	CSequencerNode* GetNode(int index) const;

	CSequencerNode* FindNodeByName(const char* sNodeName);
	void            ReorderNode(CSequencerNode* node, CSequencerNode* pPivotNode, bool next);

	bool            AddNode(CSequencerNode* node);
	void            RemoveNode(CSequencerNode* node);

	void            RemoveAll();

	void            UpdateKeys();

private:
	typedef std::vector<_smart_ptr<CSequencerNode>> SequencerNodes;
	SequencerNodes m_nodes;

	string         m_name;
	Range          m_timeRange;
};
