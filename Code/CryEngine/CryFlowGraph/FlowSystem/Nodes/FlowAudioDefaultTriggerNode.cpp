// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryFlowGraph/IFlowBaseNode.h>
#include <CryAudio/IAudioSystem.h>

class CFlowNode_AudioDfaultTrigger final : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioDfaultTrigger(SActivationInfo* pActInfo) {}

	//////////////////////////////////////////////////////////////////////////
	virtual ~CFlowNode_AudioDfaultTrigger() override = default;

	CFlowNode_AudioDfaultTrigger(CFlowNode_AudioDfaultTrigger const&) = delete;
	CFlowNode_AudioDfaultTrigger(CFlowNode_AudioDfaultTrigger&&) = delete;
	CFlowNode_AudioDfaultTrigger& operator=(CFlowNode_AudioDfaultTrigger const&) = delete;
	CFlowNode_AudioDfaultTrigger& operator=(CFlowNode_AudioDfaultTrigger&&) = delete;

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override
	{
		return new CFlowNode_AudioDfaultTrigger(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS
	{
		eIn_TriggerType,
		eIn_Execute,
	};

	enum OUTPUTS
	{
	};

	//////////////////////////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<int>("Trigger", static_cast<int>(CryAudio::EDefaultTriggerType::MuteAll), _HELP("Defines which default trigger will be executed."), _HELP("Trigger"), _UICONFIG("enum_int:mute_all=3,unmute_all=4, pause_all=5, resume_all=6")),

			InputPortConfig_Void("Execute", _HELP("Executes the Trigger")),
			{ 0 } };

		static const SOutputPortConfig outputs[] =
		{
			{ 0 } };

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node executes Default Audio Triggers.");
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
	{
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_Execute))
				{
					auto const triggerType = static_cast<CryAudio::EDefaultTriggerType>(GetPortInt(pActInfo, eIn_TriggerType));

					switch (triggerType)
					{
					case CryAudio::EDefaultTriggerType::MuteAll:
						{
							GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_MUTE, 0, 0);

							break;
						}
					case CryAudio::EDefaultTriggerType::UnmuteAll:
						{
							GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_UNMUTE, 0, 0);

							break;
						}
					case CryAudio::EDefaultTriggerType::PauseAll:
						{
							GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_PAUSE, 0, 0);

							break;
						}
					case CryAudio::EDefaultTriggerType::ResumeAll:
						{
							GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_RESUME, 0, 0);

							break;
						}
					default:
						break;
					}
				}

				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void GetMemoryUsage(ICrySizer* s) const override
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE("Audio:DefaultTrigger", CFlowNode_AudioDfaultTrigger);
