// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CrySystem/ISystem.h>
#include "FlowBaseNode.h"

class CComputeLighting_Node : public CFlowBaseNode<eNCT_Singleton>
{

public:
	CComputeLighting_Node(SActivationInfo* pActInfo)
	{
	};
	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<Vec3>("Position", _HELP("Position to compute lighting at")),
			InputPortConfig<float>("Radius",  1.0f,                                       _HELP("Radius around the position")),
			InputPortConfig<bool>("Accuracy", _HELP("TRUE for accurate, FALSE for fast")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<float>("Light", _HELP("Light output")),
			{ 0 }
		};
		config.sDescription = _HELP("Compute the amount of light at a given point");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Activate:
			{
				float r = gEnv->p3DEngine->GetLightAmountInRange(
				  GetPortVec3(pActInfo, 0),
				  GetPortFloat(pActInfo, 1),
				  GetPortBool(pActInfo, 2));
				ActivateOutput(pActInfo, 0, r);
				break;
			}

		case eFE_Initialize:
			{
				ActivateOutput(pActInfo, 0, 0.0f);
			}

		case eFE_Update:
			{
				break;
			}
		}
		;
	};
};

class CComputeStaticShadows_Node : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CComputeStaticShadows_Node(SActivationInfo* pActInfo)
	{
	}

	enum EInputs
	{
		eI_Trigger = 0,
		eI_Min,
		eI_Max,
		eI_NextCascadeScale
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Trigger",             _HELP("Trigger this to recompute all cached shadows")),
			InputPortConfig<Vec3>("Min",                Vec3(ZERO),                                            _HELP("Minimum position of shadowed area")),
			InputPortConfig<Vec3>("Max",                Vec3(ZERO),                                            _HELP("Maximum position of shadowed area")),
			InputPortConfig<float>("NextCascadesScale", 2.f,                                                   _HELP("Scale factor for subsequent cached shadow cascades")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] = {
			{ 0 }
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Triggers recalculation of cached shadow maps");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		EFlowEvent eventType = event;

		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eI_Trigger))
				{
					pActInfo->pGraph->RequestFinalActivation(pActInfo->myID);
				}

				break;
			}

		case eFE_FinalActivate:
			{
				Vec3 minCorner = GetPortVec3(pActInfo, eI_Min);
				Vec3 maxCorner = GetPortVec3(pActInfo, eI_Max);
				float nextCascadeScale = GetPortFloat(pActInfo, eI_NextCascadeScale);

				gEnv->p3DEngine->SetCachedShadowBounds(AABB(minCorner, maxCorner), nextCascadeScale);
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

class CPerEntityShadow_Node : public CFlowBaseNode<eNCT_Instanced>
{

public:
	CPerEntityShadow_Node(SActivationInfo* pActInfo)
	{
	}

	enum EInputs
	{
		eI_Enable = 0,
		eI_Trigger,
		eI_ConstBias,
		eI_SlopeBias,
		eI_Jittering,
		eI_BBoxScale,
		eI_ShadowMapSize,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig<bool>("Enabled",      true,                        "Enables node",            "Enabled"),
			InputPortConfig_Void("Trigger",       _HELP("Update the engine")),
			InputPortConfig<float>("ConstBias",   0.001f,                      _HELP("Constant Bias")),
			InputPortConfig<float>("SlopeBias",   0.01f,                       _HELP("Slope Bias")),
			InputPortConfig<float>("Jittering",   0.01f,                       _HELP("Jittering")),
			InputPortConfig<Vec3>("BBoxScale",    Vec3(1,                      1,                         1),        _HELP("Object Bounding box scale")),
			InputPortConfig<int>("ShadowMapSize", 1024,                        _HELP("Shadow Map size")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] = {
			{ 0 }
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Per Entity Shadows");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		EFlowEvent eventType = event;

		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eI_Trigger) && gEnv->pAISystem->IsEnabled())
				{
					pActInfo->pGraph->RequestFinalActivation(pActInfo->myID);
				}

				break;
			}

		case eFE_FinalActivate:
			{
				if (!pActInfo->pEntity)
					break;

				if (pActInfo->pEntity->GetId() != INVALID_ENTITYID)
				{
					IEntity* pEntity = gEnv->pEntitySystem->GetEntity(pActInfo->pEntity->GetId());
					if (pEntity && pEntity->GetProxy(ENTITY_PROXY_RENDER))
					{
						IEntityRenderProxy* pRenderProxy = static_cast<IEntityRenderProxy*>(pEntity->GetProxy(ENTITY_PROXY_RENDER));
						if (IRenderNode* pRenderNode = pRenderProxy->GetRenderNode())
						{
							const bool bActivate = GetPortBool(pActInfo, eI_Enable);
							if (bActivate)
							{
								float fConstBias = GetPortFloat(pActInfo, eI_ConstBias);
								float fSlopeBias = GetPortFloat(pActInfo, eI_SlopeBias);
								float fJittering = GetPortFloat(pActInfo, eI_Jittering);
								Vec3 vBBoxScale = GetPortVec3(pActInfo, eI_BBoxScale);
								uint nShadowMapSize = GetPortInt(pActInfo, eI_ShadowMapSize);

								gEnv->p3DEngine->AddPerObjectShadow(pRenderNode, fConstBias, fSlopeBias, fJittering, vBBoxScale, nShadowMapSize);
							}
							else
							{
								gEnv->p3DEngine->RemovePerObjectShadow(pRenderNode);
							}
						}
					}
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CPerEntityShadow_Node(pActInfo); }

};

REGISTER_FLOW_NODE("Environment:ComputeLighting", CComputeLighting_Node);
REGISTER_FLOW_NODE("Environment:RecomputeStaticShadows", CComputeStaticShadows_Node);
REGISTER_FLOW_NODE("Environment:PerEntityShadows", CPerEntityShadow_Node);
