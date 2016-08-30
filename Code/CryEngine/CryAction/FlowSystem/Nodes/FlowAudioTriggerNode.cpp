// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryFlowGraph/IFlowBaseNode.h>

class CFlowNode_AudioTrigger : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioTrigger(SActivationInfo* pActInfo)
		: m_bIsPlaying(false)
		, m_playTriggerId(INVALID_AUDIO_CONTROL_ID)
		, m_stopTriggerId(INVALID_AUDIO_CONTROL_ID)
	{
		gEnv->pAudioSystem->AddRequestListener(&CFlowNode_AudioTrigger::OnAudioTriggerFinished, this, eAudioRequestType_AudioCallbackManagerRequest, eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance);
	}

	//////////////////////////////////////////////////////////////////////////
	~CFlowNode_AudioTrigger()
	{
		gEnv->pAudioSystem->RemoveRequestListener(&CFlowNode_AudioTrigger::OnAudioTriggerFinished, this);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioTrigger(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS
	{
		eIn_PlayTrigger,
		eIn_StopTrigger,
		eIn_Play,
		eIn_Stop,
	};

	enum OUTPUTS
	{
		eOut_Done,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("audioTrigger_PlayTrigger", _HELP("name of the Play Trigger"),                                                                        "PlayTrigger"),
			InputPortConfig<string>("audioTrigger_StopTrigger", _HELP("name of the Stop Trigger"),                                                                        "StopTrigger"),

			InputPortConfig_Void("Play",                        _HELP("Executes the Play Trigger")),
			InputPortConfig_Void("Stop",                        _HELP("Executes the Stop Trigger if it is provided, o/w stops all events started by the Start Trigger")),
			{ 0 }
		};

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig_Void("Done", _HELP("Activated when all of the triggered events have finished playing")),
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node executes Audio Triggers.");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		bool bPlay = m_bIsPlaying;
		ser.BeginGroup("FlowAudioTriggerNode");
		ser.Value("play", bPlay);
		ser.EndGroup();

		if (ser.IsReading())
		{
			m_bIsPlaying = false;
			Init(pActInfo, bPlay);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Stop(pActInfo->pEntity);
				Init(pActInfo, false);

				break;
			}
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_Stop))
				{
					Stop(pActInfo->pEntity);
				}

				if (IsPortActive(pActInfo, eIn_Play))
				{
					m_playActivationInfo = *pActInfo;
					Play(pActInfo->pEntity);
				}

				if (IsPortActive(pActInfo, eIn_PlayTrigger))
				{
					Stop(pActInfo->pEntity);
					GetTriggerID(pActInfo, eIn_PlayTrigger, m_playTriggerId);
				}

				if (IsPortActive(pActInfo, eIn_StopTrigger))
				{
					GetTriggerID(pActInfo, eIn_StopTrigger, m_stopTriggerId);
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

	//////////////////////////////////////////////////////////////////////////
	void TriggerFinished(AudioControlId const nTriggerID)
	{
		if (nTriggerID == m_playTriggerId)
		{
			ActivateOutput(&m_playActivationInfo, eOut_Done, true);
		}
	}

private:

	enum EPlayMode
	{
		ePM_None      = 0,
		ePM_Play      = 1,
		ePM_PlayStop  = 2,
		ePM_ForceStop = 3,
	};

	//////////////////////////////////////////////////////////////////////////
	static void OnAudioTriggerFinished(SAudioRequestInfo const* const pAudioRequestInfo)
	{
		TFlowGraphId const id = static_cast<TFlowGraphId>(reinterpret_cast<UINT_PTR>(pAudioRequestInfo->pUserData));

		if (gEnv->pFlowSystem->GetGraphById(id) != nullptr)
		{
			// Here we are sure that this FlowGraphNode is still valid.
			CFlowNode_AudioTrigger* pAudioTrigger = static_cast<CFlowNode_AudioTrigger*>(pAudioRequestInfo->pOwner);
			pAudioTrigger->TriggerFinished(pAudioRequestInfo->audioControlId);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void GetTriggerID(SActivationInfo* const pActInfo, uint32 const nPortIdx, AudioControlId& rOutTriggerID)
	{
		rOutTriggerID = INVALID_AUDIO_CONTROL_ID;
		string const& rTriggerName = GetPortString(pActInfo, nPortIdx);

		if (!rTriggerName.empty())
		{
			gEnv->pAudioSystem->GetAudioTriggerId(rTriggerName.c_str(), rOutTriggerID);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo, bool const bPlay)
	{
		m_bIsPlaying = false;

		if (gEnv->pAudioSystem != nullptr)
		{
			GetTriggerID(pActInfo, eIn_PlayTrigger, m_playTriggerId);
			GetTriggerID(pActInfo, eIn_StopTrigger, m_stopTriggerId);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void ExecuteOnProxy(IEntity* const pEntity, AudioControlId const nTriggerID, EPlayMode const ePlayMode)
	{
		IEntityAudioProxyPtr const pIEntityAudioProxy = crycomponent_cast<IEntityAudioProxyPtr>(pEntity->CreateProxy(ENTITY_PROXY_AUDIO));

		if (pIEntityAudioProxy != nullptr)
		{
			switch (ePlayMode)
			{
			case ePM_Play:
				{
					SAudioCallBackInfo const callbackInfo(this, reinterpret_cast<void*>(static_cast<UINT_PTR>(m_playActivationInfo.pGraph->GetGraphId())), this, eAudioRequestFlags_PriorityNormal | eAudioRequestFlags_SyncFinishedCallback);
					pIEntityAudioProxy->SetCurrentEnvironments();
					pIEntityAudioProxy->ExecuteTrigger(nTriggerID, DEFAULT_AUDIO_PROXY_ID, callbackInfo);

					break;
				}
			case ePM_PlayStop:
				{
					pIEntityAudioProxy->SetCurrentEnvironments();
					pIEntityAudioProxy->ExecuteTrigger(nTriggerID);

					break;
				}
			case ePM_ForceStop:
				{
					pIEntityAudioProxy->StopTrigger(nTriggerID);

					break;
				}
			default:
				{
					assert(false);// Unknown play mode!
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void ExecuteOnGlobalObject(AudioControlId const audioTriggerId, EPlayMode const playMode)
	{
		switch (playMode)
		{
		case ePM_Play:
			{
				m_executeRequestData.audioTriggerId = audioTriggerId;
				m_request.pOwner = this;
				m_request.pData = &m_executeRequestData;
				gEnv->pAudioSystem->PushRequest(m_request);

				break;
			}
		case ePM_PlayStop:
			{
				m_executeRequestData.audioTriggerId = audioTriggerId;
				m_request.pData = &m_executeRequestData;
				gEnv->pAudioSystem->PushRequest(m_request);

				break;
			}
		case ePM_ForceStop:
			{
				m_stopRequestData.audioTriggerId = audioTriggerId;
				m_request.pData = &m_stopRequestData;
				gEnv->pAudioSystem->PushRequest(m_request);

				break;
			}
		default:
			{
				assert(false);// Unknown play mode!
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Play(IEntity* const pEntity)
	{
		if (m_playTriggerId != INVALID_AUDIO_CONTROL_ID)
		{
			if (pEntity != nullptr)
			{
				ExecuteOnProxy(pEntity, m_playTriggerId, ePM_Play);
			}
			else
			{
				ExecuteOnGlobalObject(m_playTriggerId, ePM_Play);
			}

			m_bIsPlaying = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Stop(IEntity* const pEntity)
	{
		if (m_bIsPlaying)
		{
			if (m_stopTriggerId != INVALID_AUDIO_CONTROL_ID)
			{
				if (pEntity != nullptr)
				{
					ExecuteOnProxy(pEntity, m_stopTriggerId, ePM_PlayStop);
				}
				else
				{
					ExecuteOnGlobalObject(m_stopTriggerId, ePM_PlayStop);
				}
			}
			else
			{
				if (pEntity != nullptr)
				{
					ExecuteOnProxy(pEntity, m_playTriggerId, ePM_ForceStop);
				}
				else
				{
					ExecuteOnGlobalObject(m_playTriggerId, ePM_ForceStop);
				}
			}

			m_bIsPlaying = false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool            m_bIsPlaying;
	AudioControlId  m_playTriggerId;
	AudioControlId  m_stopTriggerId;
	SActivationInfo m_playActivationInfo;
	SAudioRequest   m_request;
	SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> m_executeRequestData;
	SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger>    m_stopRequestData;
};

REGISTER_FLOW_NODE("Audio:Trigger", CFlowNode_AudioTrigger);
