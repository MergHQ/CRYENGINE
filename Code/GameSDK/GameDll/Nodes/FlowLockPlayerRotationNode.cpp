// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Actor.h"
#include "Nodes/G2FlowBaseNode.h"

class CFlowLockPlayerRotationNode : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowLockPlayerRotationNode(SActivationInfo *pActInfo)
	{
	}

	~CFlowLockPlayerRotationNode()
	{
	}

	enum EInputPorts
	{
		EIP_Enable = 0,
		EIP_Disable,
		EIP_LimitDir,
		EIP_LimitYaw,
		EIP_LimitPitch,
	};

	enum EOutputPorts
	{
		EOP_Done = 0,
	};

	virtual void GetConfiguration(SFlowNodeConfig &config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void  ("Enable", _HELP("Enable")),
			InputPortConfig_Void  ("Disable", _HELP("Disable")),
			InputPortConfig<Vec3> ("ViewLimitDir", _HELP("Center direction from where the player is limited by yaw/pitch. If (0,0,0) there will be no yaw/pitch limit.")),
			InputPortConfig<float>("ViewLimitYaw", _HELP("ViewLimitYaw, in degrees. 0 = no limit, -1 = total lock")),
			InputPortConfig<float>("ViewLimitPitch", _HELP("ViewLimitPitch, in degrees. 0 = no limit, -1 = total lock")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<bool> ("Done", _HELP("Trigger for Chaining")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Lock the player's view direction");
		config.SetCategory(EFLN_OBSOLETE);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, EIP_Enable))
			{
				const Vec3 &dir = GetPortVec3(pActInfo, EIP_LimitDir);
				const float rangeH = max(GetPortFloat(pActInfo, EIP_LimitYaw), 0.001f);
				const float rangeV = max(GetPortFloat(pActInfo, EIP_LimitPitch), 0.001f);
				CActor *pPlayerActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
				if (pPlayerActor)
				{
					pPlayerActor->SetViewLimits(dir, DEG2RAD(rangeH), DEG2RAD(rangeV));
				}
				ActivateOutput(pActInfo, EOP_Done, true);
			}
			if (IsPortActive(pActInfo, EIP_Disable))
			{
				CActor *pPlayerActor = static_cast<CActor*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
				if (pPlayerActor)
				{
					pPlayerActor->SetViewLimits(Vec3(ZERO), 0.0f, 0.0f);
				}
				ActivateOutput(pActInfo, EOP_Done, false);
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer *s) const
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE("Game:LockPlayerRotation", CFlowLockPlayerRotationNode);

