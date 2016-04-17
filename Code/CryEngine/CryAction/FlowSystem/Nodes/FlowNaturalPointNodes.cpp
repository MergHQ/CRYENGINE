// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_TrackIR : public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowNode_TrackIR(SActivationInfo* pActivationInfo) : m_frequency(0.0f)
	{
	}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_TrackIR(pActInfo); }

	enum EInputPorts
	{
		eIP_Sync = 0,
		eIP_Auto,
		eIP_Freq,
	};

	enum EOutputPorts
	{
		eOP_Pitch = 0,
		eOP_Yaw,
		eOP_Roll,
		eOP_X,
		eOP_Y,
		eOP_Z
	};

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputConfig[] =
		{
			InputPortConfig_Void("Sync",   _HELP("Synchronize")),
			InputPortConfig<bool>("Auto",  false,                _HELP("Auto update")),
			InputPortConfig<float>("Freq", 0.0f,                 _HELP("Auto update frequency (0 to update every frame)")),
			{ 0 }
		};

		static const SOutputPortConfig outputConfig[] =
		{
			OutputPortConfig<float>("Pitch", _HELP("Pitch")),
			OutputPortConfig<float>("Yaw",   _HELP("Yaw")),
			OutputPortConfig<float>("Roll",  _HELP("Roll")),
			OutputPortConfig<float>("X",     _HELP("Position X")),
			OutputPortConfig<float>("Y",     _HELP("Position Y")),
			OutputPortConfig<float>("Z",     _HELP("Position Z")),
			{ 0 }
		};

		config.sDescription = _HELP("Get status of joint in Kinect skeleton");
		config.pInputPorts = inputConfig;
		config.pOutputPorts = outputConfig;

		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActivationInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				SetAutoUpdate(pActivationInfo, GetPortBool(pActivationInfo, eIP_Auto));

				break;
			}

		case eFE_Activate:
			{
				if (IsPortActive(pActivationInfo, eIP_Auto))
				{
					SetAutoUpdate(pActivationInfo, GetPortBool(pActivationInfo, eIP_Auto));
				}
				else if (IsPortActive(pActivationInfo, eIP_Freq))
				{
					m_frequency = GetPortFloat(pActivationInfo, eIP_Freq);
				}
				else if (IsPortActive(pActivationInfo, eIP_Sync))
				{
					Sync(pActivationInfo);
				}

				break;
			}

		case eFE_Update:
			{
				CTimeValue delta = gEnv->pTimer->GetFrameStartTime() - m_prevTime;

				if (delta.GetSeconds() >= m_frequency)
				{
					Sync(pActivationInfo);
				}

				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->Add(*this);
	}

private:

	void SetAutoUpdate(SActivationInfo* pActivationInfo, bool enable)
	{
		pActivationInfo->pGraph->SetRegularlyUpdated(pActivationInfo->myID, enable);

		if (enable)
		{
			m_prevTime = gEnv->pTimer->GetFrameStartTime();
		}
	}

	void Sync(SActivationInfo* pActivationInfo)
	{
		if (INaturalPointInput* pNPInput = gEnv->pSystem->GetIInput()->GetNaturalPointInput())
		{
			NP_RawData currentNPData;

			if (pNPInput->GetNaturalPointData(currentNPData))
			{
				ActivateOutput(pActivationInfo, eOP_Pitch, currentNPData.fNPPitch);
				ActivateOutput(pActivationInfo, eOP_Yaw, currentNPData.fNPYaw);
				ActivateOutput(pActivationInfo, eOP_Roll, currentNPData.fNPRoll);
				ActivateOutput(pActivationInfo, eOP_X, currentNPData.fNPX);
				ActivateOutput(pActivationInfo, eOP_Y, currentNPData.fNPY);
				ActivateOutput(pActivationInfo, eOP_Z, currentNPData.fNPZ);
			}

		}

		m_prevTime = gEnv->pTimer->GetFrameStartTime();
	}

	float      m_frequency;

	CTimeValue m_prevTime;
};

REGISTER_FLOW_NODE("NaturalPoint:GetNPData", CFlowNode_TrackIR);
