// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryFlowGraph/IFlowBaseNode.h>

class CFlowNode_ColorGradient : public CFlowBaseNode<eNCT_Instanced>
{
public:
	static const SInputPortConfig inputPorts[];

	CFlowNode_ColorGradient(SActivationInfo* pActivationInformation);
	~CFlowNode_ColorGradient();

	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo* pActivationInformation);
	virtual void         GetMemoryUsage(ICrySizer* sizer) const;

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_ColorGradient(pActInfo); }

	enum EInputPorts
	{
		eInputPorts_Trigger,
		eInputPorts_TexturePath,
		eInputPorts_TransitionTime,
		eInputPorts_Count,
	};

private:
	//	IGameEnvironment& m_environment;
	ITexture* m_pTexture;
};
