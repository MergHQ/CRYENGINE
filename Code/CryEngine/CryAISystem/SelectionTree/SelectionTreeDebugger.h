// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionTreeDebugger_H__
#define __SelectionTreeDebugger_H__

#ifndef RELEASE
	#define BST_DEBUG_START_EVAL(actor, tree, vars)  { if (gEnv->IsEditor()) CSelectionTreeDebugger::GetInstance()->StartEvaluation(actor, tree, vars); }
	#define BST_DEBUG_EVAL_NODE(node)                { if (gEnv->IsEditor()) CSelectionTreeDebugger::GetInstance()->EvaluateNode(node); }
	#define BST_DEBUG_EVAL_NODECONDITON(node, cond)  { if (gEnv->IsEditor()) CSelectionTreeDebugger::GetInstance()->EvaluateNodeCondition(node, cond); }
	#define BST_DEBUG_EVAL_STATECONDITON(node, cond) { if (gEnv->IsEditor()) CSelectionTreeDebugger::GetInstance()->EvaluateStateCondition(node, cond); }
	#define BST_DEBUG_END_EVAL(actor, nodeid)        { if (gEnv->IsEditor()) CSelectionTreeDebugger::GetInstance()->EndEvaluation(actor, nodeid); }
#else
	#define BST_DEBUG_START_EVAL
	#define BST_DEBUG_EVAL_NODE
	#define BST_DEBUG_EVAL_NODECONDITON
	#define BST_DEBUG_EVAL_STATECONDITON
	#define BST_DEBUG_END_EVAL
#endif

#ifndef RELEASE
	#include <CryAISystem/ISelectionTreeManager.h>
	#include "SelectionTree.h"
	#include "SelectionVariables.h"
	#include "SelectionTreeNode.h"
	#include "SelectionCondition.h"

class CSelectionTreeDebugger
	: public ISelectionTreeDebugger
{
public:
	static CSelectionTreeDebugger* GetInstance();

	void                           StartEvaluation(const CAIActor* pActor, const SelectionTree& tree, const SelectionVariables& vars);
	void                           EvaluateNode(const SelectionTreeNode& node);
	void                           EvaluateNodeCondition(const SelectionTreeNode& node, bool condition);
	void                           EvaluateStateCondition(const SelectionTreeNode& node, bool condition);
	void                           EndEvaluation(const CAIActor* pActor, SelectionNodeID currentNodeId);

	virtual void                   SetObserver(ISelectionTreeObserver* pObserver, const char* nameAI);

private:
	void ReadNodeInfo(const SelectionTree& tree, ISelectionTreeObserver::STreeNodeInfo& outNode, const SelectionTreeNode& inNode);

private:
	CSelectionTreeDebugger();

	ISelectionTreeObserver* m_pObserver;
	string                  m_AIName;
	string                  m_CurrTree;
	const CAIActor*         m_AI;
};
#endif

#endif
