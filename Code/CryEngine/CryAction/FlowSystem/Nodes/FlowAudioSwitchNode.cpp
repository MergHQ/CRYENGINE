// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_AudioSwitch : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioSwitch(SActivationInfo* pActInfo)
		: m_currentState(0)
		, m_audioSwitchId(INVALID_AUDIO_CONTROL_ID)
	{
		//sanity checks
		assert((eIn_SwitchStateNameLast - eIn_SwitchStateNameFirst) == (NUM_STATES - 1));
		assert((eIn_SetStateLast - eIn_SetStateFirst) == (NUM_STATES - 1));

		for (uint32 i = 0; i < NUM_STATES; ++i)
		{
			m_audioSwitchStates[i] = INVALID_AUDIO_SWITCH_STATE_ID;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	~CFlowNode_AudioSwitch() {}

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioSwitch(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS
	{
		eIn_SwitchName,

		eIn_SwitchStateNameFirst,
		eIn_SwitchStateName2,
		eIn_SwitchStateName3,
		eIn_SwitchStateNameLast,

		eIn_SetStateFirst,
		eIn_SetState2,
		eIn_SetState3,
		eIn_SetStateLast,
	};

	enum OUTPUTS
	{
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("audioSwitch_SwitchName",            _HELP("name of the Audio Switch"),                   "Switch"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName1", _HELP("name of a Switch State"),                     "State1"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName2", _HELP("name of a Switch State"),                     "State2"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName3", _HELP("name of a Switch State"),                     "State3"),
			InputPortConfig<string>("audioSwitchState_SwitchStateName4", _HELP("name of a Switch State"),                     "State4"),

			InputPortConfig_Void("audioSwitchState_SetState1",           _HELP("Sets the switch to the corresponding state"), "SetState1"),
			InputPortConfig_Void("audioSwitchState_SetState2",           _HELP("Sets the switch to the corresponding state"), "SetState2"),
			InputPortConfig_Void("audioSwitchState_SetState3",           _HELP("Sets the switch to the corresponding state"), "SetState3"),
			InputPortConfig_Void("audioSwitchState_SetState4",           _HELP("Sets the switch to the corresponding state"), "SetState4"),
			{ 0 }
		};

		static const SOutputPortConfig outputs[] =
		{
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node allows one to set Audio Switches.");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		uint32 currentState = m_currentState;
		ser.BeginGroup("FlowAudioSwitchNode");
		ser.Value("current_state", currentState);
		ser.EndGroup();

		if (ser.IsReading())
		{
			m_currentState = 0;
			Init(pActInfo, currentState);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Init(pActInfo, 0);

				break;
			}
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_SwitchName))
				{
					GetSwitchID(pActInfo);
				}

				for (uint32 stateIndex = eIn_SwitchStateNameFirst; stateIndex <= eIn_SwitchStateNameLast; ++stateIndex)
				{
					if (IsPortActive(pActInfo, stateIndex))
					{
						GetSwitchStateID(pActInfo, stateIndex);
					}
				}

				for (uint32 statePort = eIn_SetStateFirst; statePort <= eIn_SetStateLast; ++statePort)
				{
					if (IsPortActive(pActInfo, statePort))
					{
						SetState(pActInfo->pEntity, statePort - eIn_SetStateFirst + 1);
						break;// short-circuit behavior: set the first state activated and stop
					}
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

	static const int NUM_STATES = 4;

	//////////////////////////////////////////////////////////////////////////
	void GetSwitchID(SActivationInfo* const pActInfo)
	{
		string const& switchName = GetPortString(pActInfo, eIn_SwitchName);
		if (!switchName.empty())
		{
			gEnv->pAudioSystem->GetAudioSwitchId(switchName.c_str(), m_audioSwitchId);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void GetSwitchStateID(SActivationInfo* const pActInfo, uint32 const stateIndex)
	{
		string const& stateName = GetPortString(pActInfo, stateIndex);
		if (!stateName.empty() && (m_audioSwitchId != INVALID_AUDIO_CONTROL_ID))
		{
			gEnv->pAudioSystem->GetAudioSwitchStateId(
			  m_audioSwitchId,
			  stateName.c_str(),
			  m_audioSwitchStates[stateIndex - eIn_SwitchStateNameFirst]);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo, uint32 const currentState)
	{
		if (gEnv->pAudioSystem != NULL)
		{
			GetSwitchID(pActInfo);

			if (m_audioSwitchId != INVALID_AUDIO_CONTROL_ID)
			{
				for (uint32 iStateName = eIn_SwitchStateNameFirst; iStateName <= eIn_SwitchStateNameLast; ++iStateName)
				{
					GetSwitchStateID(pActInfo, iStateName);
				}
			}

			m_request.pData = &m_requestData;
			m_requestData.audioSwitchId = m_audioSwitchId;

			m_currentState = 0;

			if (currentState != 0)
			{
				SetState(pActInfo->pEntity, currentState);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetStateOnProxy(IEntity* const pEntity, uint32 const stateIndex)
	{
		IEntityAudioProxyPtr const pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));

		if (pIEntityAudioProxy != nullptr)
		{
			pIEntityAudioProxy->SetSwitchState(m_audioSwitchId, m_audioSwitchStates[stateIndex]);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void SetStateOnGlobalObject(uint32 const stateIndex)
	{
		m_requestData.audioSwitchStateId = m_audioSwitchStates[stateIndex];

		gEnv->pAudioSystem->PushRequest(m_request);
	}

	//////////////////////////////////////////////////////////////////////////
	void SetState(IEntity* const pEntity, int const newState)
	{
		CRY_ASSERT((0 < newState) && (newState <= NUM_STATES));

		// Cannot check for m_nCurrentState != nNewState, because there can be several flowgraph nodes
		// setting the states on the same switch. This particular node might need to set the same state again,
		// if another node has set a different one in between the calls to set the state on this node.
		m_currentState = static_cast<uint32>(newState);

		if (pEntity != nullptr)
		{
			SetStateOnProxy(pEntity, m_currentState - 1);
		}
		else
		{
			SetStateOnGlobalObject(m_currentState - 1);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	uint32             m_currentState;
	AudioControlId     m_audioSwitchId;
	AudioSwitchStateId m_audioSwitchStates[NUM_STATES];
	SAudioRequest      m_request;
	SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> m_requestData;
};

REGISTER_FLOW_NODE("Audio:Switch", CFlowNode_AudioSwitch);
