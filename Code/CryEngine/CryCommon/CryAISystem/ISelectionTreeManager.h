// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __ISelectionTreeManager_h__
#define __ISelectionTreeManager_h__

#pragma  once

struct ISelectionTreeObserver
{
	virtual ~ISelectionTreeObserver() {}

	typedef std::map<string, bool> TVariableStateMap;
	struct STreeNodeInfo
	{
		uint16     Id;
		typedef std::vector<STreeNodeInfo> TChildList;
		TChildList Childs;
	};

	virtual void SetSelectionTree(const char* name, const STreeNodeInfo& rootNode) = 0;
	virtual void DumpVars(const TVariableStateMap& vars) = 0;
	virtual void StartEval() = 0;
	virtual void EvalNode(uint16 nodeId) = 0;
	virtual void EvalNodeCondition(uint16 nodeId, bool condition) = 0;
	virtual void EvalStateCondition(uint16 nodeId, bool condition) = 0;
	virtual void StopEval(uint16 nodeId) = 0;
};

struct ISelectionTreeDebugger
{
	virtual ~ISelectionTreeDebugger(){}

	virtual void SetObserver(ISelectionTreeObserver* pListener, const char* nameAI) = 0;
};

struct ISelectionTreeManager
{
	// <interfuscator:shuffle>
	virtual ~ISelectionTreeManager(){}
	virtual uint32                  GetSelectionTreeCount() const = 0;
	virtual uint32                  GetSelectionTreeCountOfType(const char* typeName) const = 0;

	virtual const char*             GetSelectionTreeName(uint32 index) const = 0;
	virtual const char*             GetSelectionTreeNameOfType(const char* typeName, uint32 index) const = 0;

	virtual void                    Reload() = 0;

	virtual ISelectionTreeDebugger* GetDebugger() const = 0;
	// </interfuscator:shuffle>
};

#endif
