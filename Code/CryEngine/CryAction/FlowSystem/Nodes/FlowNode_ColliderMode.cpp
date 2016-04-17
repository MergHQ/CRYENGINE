// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlowBaseNode.h"
#include "../../GameObjects/GameObject.h"
#include "IAnimationGraph.h"

//--------------------------------------------------------------------------------

class CFlowNode_ColliderMode : public CFlowBaseNode<eNCT_Singleton>
{

public:

	CFlowNode_ColliderMode(SActivationInfo* pActInfo)
	{
	}

	~CFlowNode_ColliderMode()
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static CryFixedStringT<256> cfg;
		static bool initialized = false;
		if (!initialized)
		{
			CryFixedStringT<256> cfgTemp("enum_int:");
			CryFixedStringT<256> mode;
			for (int i = 0; i < eColliderMode_COUNT; i++)
			{
				mode.Format("%s=%d", g_szColliderModeString[i], i);
				cfgTemp += mode;

				if (i < (eColliderMode_COUNT - 1))
					cfgTemp += ",";
			}

			cfg = cfgTemp;
			initialized = true;
		}

		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Trigger",      _HELP("Trigger to request collider mode in animated character.")),
			InputPortConfig<int>("ColliderMode", eColliderMode_Undefined,                                          0,0, _UICONFIG(cfg.c_str())),
			{ 0 }
		};

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig_Void("Done", _HELP("Triggered when Done.")),
			{ 0 }
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("ColliderMode Node");
		config.SetCategory(EFLN_ADVANCED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (event != eFE_Activate)
			return;

		if (!IsPortActive(pActInfo, 0))
			return;

		if (pActInfo->pEntity == NULL)
			return;

		CGameObject* pGameObject = (CGameObject*)(pActInfo->pEntity->GetProxy(ENTITY_PROXY_USER));
		if (pGameObject == NULL)
			return;

		IAnimatedCharacter* pAnimChar = (IAnimatedCharacter*)(pGameObject->QueryExtension("AnimatedCharacter"));
		if (pAnimChar == NULL)
			return;

		int colliderMode = GetPortInt(pActInfo, 1);
		pAnimChar->RequestPhysicalColliderMode((EColliderMode)colliderMode, eColliderModeLayer_FlowGraph);
		ActivateOutput(pActInfo, 0, true);
	}

	virtual void GetMemoryUsage(ICrySizer* crySizer) const
	{
		crySizer->AddObject(this, sizeof(*this));
	}

};

//--------------------------------------------------------------------------------

REGISTER_FLOW_NODE("AnimatedCharacter:ColliderMode", CFlowNode_ColliderMode);

//--------------------------------------------------------------------------------
