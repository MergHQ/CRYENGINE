#pragma once

#include "CryFlowGraph/IFlowSystem.h"

// FlowNode factory class for Flash UI nodes
class CFlashUiFlowNodeFactory : public IFlowNodeFactory
{
public:
	CFlashUiFlowNodeFactory(const char* sClassName, IFlowNodePtr pFlowNode) : m_sClassName(sClassName), m_pFlowNode(pFlowNode) {}
	IFlowNodePtr Create(IFlowNode::SActivationInfo* pActInfo) { return m_pFlowNode->Clone(pActInfo); }
	void         GetMemoryUsage(ICrySizer* s) const           { SIZER_SUBCOMPONENT_NAME(s, "CFlashUiFlowNodeFactory"); }
	void         Reset()                                      {}

	const char*  GetNodeTypeName() const { return m_sClassName; }

private:
	const char*  m_sClassName;
	IFlowNodePtr m_pFlowNode;
};

TYPEDEF_AUTOPTR(CFlashUiFlowNodeFactory);
typedef CFlashUiFlowNodeFactory_AutoPtr CFlashUiFlowNodeFactoryPtr;


