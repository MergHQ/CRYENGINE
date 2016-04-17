// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AIFlowBaseNode.h"

CAIAutoRegFlowNodeBase* CAIAutoRegFlowNodeBase::m_pFirst = 0;
CAIAutoRegFlowNodeBase* CAIAutoRegFlowNodeBase::m_pLast = 0;

void AIFlowBaseNode::RegisterFactory()
{
	IFlowSystem* pFlow = GetISystem()->GetIFlowSystem();
	assert(pFlow);
	if (!pFlow)
		return;

	CAIAutoRegFlowNodeBase* pFactory = CAIAutoRegFlowNodeBase::m_pFirst;
	while (pFactory)
	{
		pFlow->RegisterType(pFactory->m_sClassName, pFactory);
		pFactory = pFactory->m_pNext;
	}
}
