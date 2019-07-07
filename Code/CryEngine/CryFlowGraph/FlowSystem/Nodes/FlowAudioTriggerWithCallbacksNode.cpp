// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryFlowGraph/IFlowBaseNode.h>

class CFlowNode_AudioTriggerWithCallbacks final : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioTriggerWithCallbacks(SActivationInfo* pActInfo)
		: m_flags(eFlowNodeAudioTriggerFlags_None)
		, m_playTriggerId(CryAudio::InvalidControlId)
		, m_stopTriggerId(CryAudio::InvalidControlId)
		, m_requestUserData(CryAudio::ERequestFlags::None, this)
		, m_systemEvents(CryAudio::ESystemEvents::TriggerFinished)
	{}

	//////////////////////////////////////////////////////////////////////////
	virtual ~CFlowNode_AudioTriggerWithCallbacks() override
	{
		if ((m_flags & eFlowNodeAudioTriggerFlags_IsListener) != 0)
		{
			RemoveRequestListener();
		}
	}

	CFlowNode_AudioTriggerWithCallbacks(CFlowNode_AudioTriggerWithCallbacks const&) = delete;
	CFlowNode_AudioTriggerWithCallbacks(CFlowNode_AudioTriggerWithCallbacks&&) = delete;
	CFlowNode_AudioTriggerWithCallbacks& operator=(CFlowNode_AudioTriggerWithCallbacks const&) = delete;
	CFlowNode_AudioTriggerWithCallbacks& operator=(CFlowNode_AudioTriggerWithCallbacks&&) = delete;

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override
	{
		return new CFlowNode_AudioTriggerWithCallbacks(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS
	{
		eIn_PlayTrigger,
		eIn_StopTrigger,
		eIn_Play,
		eIn_Stop,
		eIn_OnBar,
		eIn_OnBeat,
		eIn_OnEntry,
		eIn_OnExit,
		eIn_OnGrid,
		eIn_OnSyncPoint,
		eIn_OnUserMarker,
	};

	enum OUTPUTS
	{
		eOut_Done,
		eOut_OnBar,
		eOut_OnBeat,
		eOut_OnEntry,
		eOut_OnExit,
		eOut_OnGrid,
		eOut_OnSyncPoint,
		eOut_OnUserMarker,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("audioTrigger_PlayTrigger", _HELP("name of the Play Trigger"),                                                                        "PlayTrigger"),
			InputPortConfig<string>("audioTrigger_StopTrigger", _HELP("name of the Stop Trigger"),                                                                        "StopTrigger"),

			InputPortConfig_Void("Play",                        _HELP("Executes the Play Trigger. Only callbacks from this trigger are processed.")),
			InputPortConfig_Void("Stop",                        _HELP("Executes the Stop Trigger if it is provided, o/w stops all events started by the Start Trigger")),

			InputPortConfig<bool>("OnBar",                      false,                                                                                                    _HELP("Listens to bar callbacks if true")),
			InputPortConfig<bool>("OnBeat",                     false,                                                                                                    _HELP("Listens to beat callbacks if true")),
			InputPortConfig<bool>("OnEntry",                    false,                                                                                                    _HELP("Listens to entry callbacks if true")),
			InputPortConfig<bool>("OnExit",                     false,                                                                                                    _HELP("Listens to exit callbacks if true")),
			InputPortConfig<bool>("OnGrid",                     false,                                                                                                    _HELP("Listens to grid callbacks if true")),
			InputPortConfig<bool>("OnSyncPoint",                false,                                                                                                    _HELP("Listens to sync point callbacks if true")),
			InputPortConfig<bool>("OnUserMarker",               false,                                                                                                    _HELP("Listens to user marker callbacks if true")),
			{ 0 } };

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig_Void("Done",         _HELP("Activated when all of the triggered events have finished playing")),
			OutputPortConfig_Void("OnBar",        _HELP("Activated when a bar callback is received")),
			OutputPortConfig_Void("OnBeat",       _HELP("Activated when a beat callback is received")),
			OutputPortConfig_Void("OnEntry",      _HELP("Activated when an entry callback is received")),
			OutputPortConfig_Void("OnExit",       _HELP("Activated when an exit callback is received")),
			OutputPortConfig_Void("OnGrid",       _HELP("Activated when a grid callback is received")),
			OutputPortConfig_Void("OnSyncPoint",  _HELP("Activated when a sync point callback is received")),
			OutputPortConfig_Void("OnUserMarker", _HELP("Activated when a user marker callback is received")),
			{ 0 } };

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node executes audio triggers and receives callbacks.");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
	{
		bool bPlay = ((m_flags & eFlowNodeAudioTriggerFlags_IsPlaying) > 0);
		ser.BeginGroup("FlowAudioTriggerWithCallbacksNode");
		ser.Value("play", bPlay);
		ser.EndGroup();

		if (ser.IsReading())
		{
			Init(pActInfo, bPlay);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Stop(pActInfo->pEntity);
				Init(pActInfo, false);

				break;
			}
		case eFE_ConnectOutputPort:
			{
				if ((m_flags & eFlowNodeAudioTriggerFlags_IsListener) == 0)
				{
					AddRequestListener();
				}

				break;
			}
		case eFE_DisconnectOutputPort:
			{
				if (((m_flags & eFlowNodeAudioTriggerFlags_IsListener) != 0) && !IsAnyOutputConnected(pActInfo))
				{
					RemoveRequestListener();
				}

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
		default:
			{
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void GetMemoryUsage(ICrySizer* s) const override
	{
		s->Add(*this);
	}

private:

	enum EPlayMode : CryAudio::EnumFlagsType
	{
		ePlayMode_None = 0,
		ePlayMode_Play = 1,
		ePlayMode_PlayStop = 2,
		ePlayMode_ForceStop = 3, };

	enum EFlowNodeAudioTriggerFlags : CryAudio::EnumFlagsType
	{
		eFlowNodeAudioTriggerFlags_None = 0,
		eFlowNodeAudioTriggerFlags_IsPlaying = BIT(0),
		eFlowNodeAudioTriggerFlags_IsListener = BIT(1), };

	//////////////////////////////////////////////////////////////////////////
	void TriggerFinished(CryAudio::ControlId const audioTriggerId)
	{
		if (audioTriggerId == m_playTriggerId)
		{
			ActivateOutput(&m_playActivationInfo, eOut_Done, true);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void OnCallback(CryAudio::ESystemEvents const systemEvent)
	{
		OUTPUTS output = eOut_Done;

		switch (systemEvent)
		{
		case CryAudio::ESystemEvents::OnBar:
			{
				output = eOut_OnBar;
				break;
			}
		case CryAudio::ESystemEvents::OnBeat:
			{
				output = eOut_OnBeat;
				break;
			}
		case CryAudio::ESystemEvents::OnEntry:
			{
				output = eOut_OnEntry;
				break;
			}
		case CryAudio::ESystemEvents::OnExit:
			{
				output = eOut_OnExit;
				break;
			}
		case CryAudio::ESystemEvents::OnGrid:
			{
				output = eOut_OnGrid;
				break;
			}
		case CryAudio::ESystemEvents::OnSyncPoint:
			{
				output = eOut_OnSyncPoint;
				break;
			}
		case CryAudio::ESystemEvents::OnUserMarker:
			{
				output = eOut_OnUserMarker;
				break;
			}
		default:
			{
				CRY_ASSERT(false, "Unsupported audio system callback type during %s", __FUNCTION__);
				break;
			}
		}

		if (output != eOut_Done)
		{
			ActivateOutput(&m_playActivationInfo, output, true);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	static void OnAudioTriggerCallback(CryAudio::SRequestInfo const* const pRequestInfo)
	{
		TFlowGraphId const id = static_cast<TFlowGraphId>(reinterpret_cast<UINT_PTR>(pRequestInfo->pUserData));

		if (gEnv->pFlowSystem->GetGraphById(id) != nullptr)
		{
			// Here we are sure that this FlowGraphNode is still valid.
			auto const pAudioTrigger = static_cast<CFlowNode_AudioTriggerWithCallbacks*>(pRequestInfo->pOwner);

			switch (pRequestInfo->systemEvent)
			{
			case CryAudio::ESystemEvents::TriggerFinished:
				{
					pAudioTrigger->TriggerFinished(pRequestInfo->audioControlId);
					break;
				}
			case CryAudio::ESystemEvents::OnBar:        // Intentional fall-through.
			case CryAudio::ESystemEvents::OnBeat:       // Intentional fall-through.
			case CryAudio::ESystemEvents::OnEntry:      // Intentional fall-through.
			case CryAudio::ESystemEvents::OnExit:       // Intentional fall-through.
			case CryAudio::ESystemEvents::OnGrid:       // Intentional fall-through.
			case CryAudio::ESystemEvents::OnSyncPoint:  // Intentional fall-through.
			case CryAudio::ESystemEvents::OnUserMarker: // Intentional fall-through.
				{
					pAudioTrigger->OnCallback(pRequestInfo->systemEvent);
					break;
				}
			default:
				{
					break;
				}
			}

		}
	}

	//////////////////////////////////////////////////////////////////////////
	void GetTriggerID(SActivationInfo* const pActInfo, uint32 const portIndex, CryAudio::ControlId& outTriggerId)
	{
		outTriggerId = CryAudio::InvalidControlId;
		string const& triggerName = GetPortString(pActInfo, portIndex);

		if (!triggerName.empty())
		{
			outTriggerId = CryAudio::StringToId(triggerName.c_str());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool IsAnyOutputConnected(SActivationInfo* const pActInfo)
	{
		bool isConnected = false;

		if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_Done)) && IsOutputConnected(pActInfo, eOut_Done))
		{
			isConnected = true;
		}
		else if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_OnBar)) && IsOutputConnected(pActInfo, eOut_OnBar))
		{
			isConnected = true;
		}
		else if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_OnBeat)) && IsOutputConnected(pActInfo, eOut_OnBeat))
		{
			isConnected = true;
		}
		else if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_OnEntry)) && IsOutputConnected(pActInfo, eOut_OnEntry))
		{
			isConnected = true;
		}
		else if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_OnExit)) && IsOutputConnected(pActInfo, eOut_OnExit))
		{
			isConnected = true;
		}
		else if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_OnGrid)) && IsOutputConnected(pActInfo, eOut_OnGrid))
		{
			isConnected = true;
		}
		else if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_OnSyncPoint)) && IsOutputConnected(pActInfo, eOut_OnSyncPoint))
		{
			isConnected = true;
		}
		else if ((pActInfo->connectPort != static_cast<TFlowPortId>(eOut_OnUserMarker)) && IsOutputConnected(pActInfo, eOut_OnUserMarker))
		{
			isConnected = true;
		}

		return isConnected;
	}

	//////////////////////////////////////////////////////////////////////////
	void SetCallbackTypes(SActivationInfo* const pActInfo)
	{
		CryAudio::ESystemEvents const events = m_systemEvents;

		GetPortBool(pActInfo, eIn_OnBar) ? (m_systemEvents |= CryAudio::ESystemEvents::OnBar) : (m_systemEvents &= ~CryAudio::ESystemEvents::OnBar);
		GetPortBool(pActInfo, eIn_OnBeat) ? (m_systemEvents |= CryAudio::ESystemEvents::OnBeat) : (m_systemEvents &= ~CryAudio::ESystemEvents::OnBeat);
		GetPortBool(pActInfo, eIn_OnEntry) ? (m_systemEvents |= CryAudio::ESystemEvents::OnEntry) : (m_systemEvents &= ~CryAudio::ESystemEvents::OnEntry);
		GetPortBool(pActInfo, eIn_OnExit) ? (m_systemEvents |= CryAudio::ESystemEvents::OnExit) : (m_systemEvents &= ~CryAudio::ESystemEvents::OnExit);
		GetPortBool(pActInfo, eIn_OnGrid) ? (m_systemEvents |= CryAudio::ESystemEvents::OnGrid) : (m_systemEvents &= ~CryAudio::ESystemEvents::OnGrid);
		GetPortBool(pActInfo, eIn_OnSyncPoint) ? (m_systemEvents |= CryAudio::ESystemEvents::OnSyncPoint) : (m_systemEvents &= ~CryAudio::ESystemEvents::OnSyncPoint);
		GetPortBool(pActInfo, eIn_OnUserMarker) ? (m_systemEvents |= CryAudio::ESystemEvents::OnUserMarker) : (m_systemEvents &= ~CryAudio::ESystemEvents::OnUserMarker);

		if (events != m_systemEvents)
		{
			RemoveRequestListener();
			AddRequestListener();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo, bool const bPlay)
	{
		m_flags &= ~eFlowNodeAudioTriggerFlags_IsPlaying;
		SetCallbackTypes(pActInfo);

		if (gEnv->pAudioSystem != nullptr)
		{
			GetTriggerID(pActInfo, eIn_PlayTrigger, m_playTriggerId);
			GetTriggerID(pActInfo, eIn_StopTrigger, m_stopTriggerId);
		}

		if (IsAnyOutputConnected(pActInfo))
		{
			if ((m_flags & eFlowNodeAudioTriggerFlags_IsListener) == 0)
			{
				AddRequestListener();
			}
		}
		else if ((m_flags & eFlowNodeAudioTriggerFlags_IsListener) != 0)
		{
			RemoveRequestListener();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void ExecuteOnProxy(IEntity* const pIEntity, CryAudio::ControlId const triggerId, EPlayMode const playMode)
	{
		IEntityAudioComponent* const pIEntityAudioComponent = pIEntity->GetOrCreateComponent<IEntityAudioComponent>();

		if (pIEntityAudioComponent != nullptr)
		{
			switch (playMode)
			{
			case ePlayMode_Play:
				{
					CryAudio::STriggerCallbackData const callbackData(triggerId, m_systemEvents, &CFlowNode_AudioTriggerWithCallbacks::OnAudioTriggerCallback);
					CryAudio::SRequestUserData const userData(CryAudio::ERequestFlags::SubsequentCallbackOnExternalThread, this, reinterpret_cast<void*>(static_cast<UINT_PTR>(m_playActivationInfo.pGraph->GetGraphId())), this);
					pIEntityAudioComponent->SetCurrentEnvironments();
					pIEntityAudioComponent->ExecuteTriggerWithCallbacks(callbackData, CryAudio::DefaultAuxObjectId, INVALID_ENTITYID, userData);

					break;
				}
			case ePlayMode_PlayStop:
				{
					pIEntityAudioComponent->SetCurrentEnvironments();
					pIEntityAudioComponent->ExecuteTrigger(triggerId);

					break;
				}
			case ePlayMode_ForceStop:
				{
					pIEntityAudioComponent->StopTrigger(triggerId);

					break;
				}
			default:
				{
					CRY_ASSERT(false, "Unknown playmode during %s", __FUNCTION__);

					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void ExecuteOnGlobalObject(CryAudio::ControlId const triggerId, EPlayMode const playMode)
	{
		switch (playMode)
		{
		case ePlayMode_Play:
			{
				CryAudio::STriggerCallbackData const callbackData(triggerId, m_systemEvents, &CFlowNode_AudioTriggerWithCallbacks::OnAudioTriggerCallback);
				CryAudio::SRequestUserData const userData(CryAudio::ERequestFlags::SubsequentCallbackOnExternalThread, this, reinterpret_cast<void*>(static_cast<UINT_PTR>(m_playActivationInfo.pGraph->GetGraphId())), this);
				gEnv->pAudioSystem->ExecuteTriggerWithCallbacks(callbackData, userData);

				break;
			}

		case ePlayMode_PlayStop:
			{
				gEnv->pAudioSystem->ExecuteTrigger(triggerId);

				break;
			}
		case ePlayMode_ForceStop:
			{
				gEnv->pAudioSystem->StopTrigger(triggerId);

				break;
			}
		default:
			{
				CRY_ASSERT(false, "Unknown playmode during %s", __FUNCTION__);

				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Play(IEntity* const pIEntity)
	{
		if (m_playTriggerId != CryAudio::InvalidControlId)
		{
			if ((m_flags & eFlowNodeAudioTriggerFlags_IsListener) != 0)
			{
				SetCallbackTypes(&m_playActivationInfo);
			}

			if (pIEntity != nullptr)
			{
				ExecuteOnProxy(pIEntity, m_playTriggerId, ePlayMode_Play);
			}
			else
			{
				ExecuteOnGlobalObject(m_playTriggerId, ePlayMode_Play);
			}

			m_flags |= eFlowNodeAudioTriggerFlags_IsPlaying;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Stop(IEntity* const pIEntity)
	{
		if ((m_flags & eFlowNodeAudioTriggerFlags_IsPlaying) != 0)
		{
			if (m_stopTriggerId != CryAudio::InvalidControlId)
			{
				if (pIEntity != nullptr)
				{
					ExecuteOnProxy(pIEntity, m_stopTriggerId, ePlayMode_PlayStop);
				}
				else
				{
					ExecuteOnGlobalObject(m_stopTriggerId, ePlayMode_PlayStop);
				}
			}
			else
			{
				if (pIEntity != nullptr)
				{
					ExecuteOnProxy(pIEntity, m_playTriggerId, ePlayMode_ForceStop);
				}
				else
				{
					ExecuteOnGlobalObject(m_playTriggerId, ePlayMode_ForceStop);
				}
			}

			m_flags &= ~eFlowNodeAudioTriggerFlags_IsPlaying;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void AddRequestListener()
	{
		gEnv->pAudioSystem->AddRequestListener(&CFlowNode_AudioTriggerWithCallbacks::OnAudioTriggerCallback, this, m_systemEvents);
		m_flags |= eFlowNodeAudioTriggerFlags_IsListener;
	}

	//////////////////////////////////////////////////////////////////////////
	void RemoveRequestListener()
	{
		gEnv->pAudioSystem->RemoveRequestListener(&CFlowNode_AudioTriggerWithCallbacks::OnAudioTriggerCallback, this);
		m_flags &= ~eFlowNodeAudioTriggerFlags_IsListener;
	}

	//////////////////////////////////////////////////////////////////////////
	CryAudio::EnumFlagsType          m_flags;
	CryAudio::ControlId              m_playTriggerId;
	CryAudio::ControlId              m_stopTriggerId;
	SActivationInfo                  m_playActivationInfo;
	CryAudio::SRequestUserData const m_requestUserData;
	CryAudio::ESystemEvents          m_systemEvents;
};

REGISTER_FLOW_NODE("Audio:TriggerWithCallbacks", CFlowNode_AudioTriggerWithCallbacks);
