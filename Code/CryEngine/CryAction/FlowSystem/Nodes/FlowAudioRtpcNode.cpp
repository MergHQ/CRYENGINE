// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_AudioRtpc : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioRtpc(SActivationInfo* pActInfo)
		: m_value(0.0f)
		, m_rtpcId(INVALID_AUDIO_CONTROL_ID)
	{}

	//////////////////////////////////////////////////////////////////////////
	~CFlowNode_AudioRtpc() {}

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioRtpc(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS
	{
		eIn_RtpcName,
		eIn_RtpcValue,
	};

	enum OUTPUTS
	{
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("audioRTPC_Name", _HELP("RTPC name"),  "Name"),
			InputPortConfig<float>("value",           _HELP("RTPC value"), "Value"),
			{ 0 }
		};

		static const SOutputPortConfig outputs[] =
		{
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node sets RTPC values.");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		float fValue = m_value;
		ser.BeginGroup("FlowAudioRtpcNode");
		ser.Value("value", fValue);
		ser.EndGroup();

		if (ser.IsReading())
		{
			SetValue(pActInfo->pEntity, fValue);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Init(pActInfo);

				break;
			}
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_RtpcValue))
				{
					SetValue(pActInfo->pEntity, GetPortFloat(pActInfo, eIn_RtpcValue));
				}

				if (IsPortActive(pActInfo, eIn_RtpcName))
				{
					GetRtpcId(pActInfo);
				}

				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:

	static float const EPSILON;

	//////////////////////////////////////////////////////////////////////////
	void GetRtpcId(SActivationInfo* const pActInfo)
	{
		string const& rtpcName = GetPortString(pActInfo, eIn_RtpcName);
		if (!rtpcName.empty())
		{
			gEnv->pAudioSystem->GetAudioRtpcId(rtpcName.c_str(), m_rtpcId);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetValue(IEntity* const pEntity, float const value)
	{
		// Don't optimize here!
		// We always need to set the value as it could have been manipulated by another entity.
		m_value = value;

		if (pEntity != NULL)
		{
			SetOnProxy(pEntity, m_value);
		}
		else
		{
			SetOnGlobalObject(m_value);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo)
	{
		if (gEnv->pAudioSystem != NULL)
		{
			GetRtpcId(pActInfo);

			m_request.pData = &m_requestData;
			m_requestData.audioRtpcId = m_rtpcId;

			float const value = GetPortFloat(pActInfo, eIn_RtpcValue);
			SetValue(pActInfo->pEntity, value);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetOnProxy(IEntity* const pEntity, float const value)
	{
		IEntityAudioProxyPtr const pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));

		if (pIEntityAudioProxy != nullptr)
		{
			pIEntityAudioProxy->SetRtpcValue(m_rtpcId, value);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetOnGlobalObject(float const value)
	{
		m_requestData.value = value;
		gEnv->pAudioSystem->PushRequest(m_request);
	}

	//////////////////////////////////////////////////////////////////////////
	float          m_value;
	AudioControlId m_rtpcId;
	SAudioRequest  m_request;
	SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> m_requestData;
};

REGISTER_FLOW_NODE("Audio:Rtpc", CFlowNode_AudioRtpc);
