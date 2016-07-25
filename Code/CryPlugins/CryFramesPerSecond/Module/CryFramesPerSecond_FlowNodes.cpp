// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FramesPerSecond.h"
#include "../Interface/ICryFramesPerSecondPlugin.h"
#include <CryFlowGraph/IFlowBaseNode.h>

//////////////////////////////////////////////////////////////////////
class CFlowNode_FramesPerSecond : public CFlowBaseNode<eNCT_Instanced>
{
	enum EInputs
	{
		eInput_Interval
	};

	enum EOutputs
	{
		eOutput_FPS
	};

public:
	CFlowNode_FramesPerSecond(SActivationInfo* pActInfo)
		: m_pActInfo(pActInfo)
	{
		m_pFramesPerSecond = ICryFramesPerSecondPlugin::GetCryFramesPerSecond()->GetIFramesPerSecond();
	}

	~CFlowNode_FramesPerSecond()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<float>("Interval", m_pFramesPerSecond->GetInterval(), _HELP("How often FPS counter is updated in seconds")),
			{ 0 }
		};

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig<int>("FramesPerSecond", _HELP("Current Frames per second")),
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("FlowNode to adjust FPS update interval");
		config.SetCategory(EFLN_APPROVED);
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_FramesPerSecond(pActInfo);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		m_pActInfo = pActInfo;

		switch (event)
		{
		case eFE_Initialize:
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
			m_pFramesPerSecond->SetInterval(GetPortFloat(pActInfo, eInput_Interval));
			break;

		case eFE_Activate:

			if (IsPortActive(pActInfo, eInput_Interval))
			{
				m_pFramesPerSecond->SetInterval(GetPortFloat(pActInfo, eInput_Interval));
			}
			break;

		case eFE_Update:
			ActivateOutput(pActInfo, eOutput_FPS, m_pFramesPerSecond->GetFPS());
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:
	SActivationInfo*  m_pActInfo;
	IFramesPerSecond* m_pFramesPerSecond;
};

REGISTER_FLOW_NODE("FramesPerSecond", CFlowNode_FramesPerSecond);
