// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryFlowGraph/IFlowBaseNode.h>

class CFlowNode_AudioSetting final : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioSetting(SActivationInfo* pActInfo) {}

	//////////////////////////////////////////////////////////////////////////
	virtual ~CFlowNode_AudioSetting() override = default;

	CFlowNode_AudioSetting(CFlowNode_AudioSetting const&) = delete;
	CFlowNode_AudioSetting(CFlowNode_AudioSetting&&) = delete;
	CFlowNode_AudioSetting& operator=(CFlowNode_AudioSetting const&) = delete;
	CFlowNode_AudioSetting& operator=(CFlowNode_AudioSetting&&) = delete;

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override
	{
		return new CFlowNode_AudioSetting(pActInfo);
	}

	enum INPUTS
	{
		eIn_SettingName,
		eIn_Load,
		eIn_Unload,
	};

	enum OUTPUTS
	{
	};

	//////////////////////////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("audioSetting_Name", _HELP("name of audio setting"), "Setting"),
			InputPortConfig_Void("Load",                 _HELP("loads the setting")),
			InputPortConfig_Void("Unload",               _HELP("unloads the setting")),
			{ 0 } };

		static const SOutputPortConfig outputs[] =
		{
			{ 0 } };

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Node that allows for handling an audio setting.");
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	void Load(SActivationInfo* pActInfo)
	{
		string const& settingName = GetPortString(pActInfo, static_cast<int>(eIn_SettingName));

		if (!settingName.empty())
		{
			CryAudio::ControlId const id = CryAudio::StringToId(settingName.c_str());
			gEnv->pAudioSystem->LoadSetting(id);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Unload(SActivationInfo* pActInfo)
	{
		string const& settingName = GetPortString(pActInfo, static_cast<int>(eIn_SettingName));

		if (!settingName.empty())
		{
			CryAudio::ControlId const id = CryAudio::StringToId(settingName.c_str());
			gEnv->pAudioSystem->UnloadSetting(id);
		}
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
				if (IsPortActive(pActInfo, eIn_Load))
				{
					Load(pActInfo);
				}

				if (IsPortActive(pActInfo, eIn_Unload))
				{
					Unload(pActInfo);
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

REGISTER_FLOW_NODE("Audio:Setting", CFlowNode_AudioSetting);
