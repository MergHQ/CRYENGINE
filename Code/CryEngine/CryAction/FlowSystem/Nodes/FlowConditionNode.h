// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __FLOWCONDITIONNODE_H__
#define __FLOWCONDITIONNODE_H__

#pragma once

#include "FlowBaseNode.h"

class CFlowNode_Condition : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowNode_Condition(SActivationInfo*) {}

	void         GetConfiguration(SFlowNodeConfig& config);
	void         ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

#endif
