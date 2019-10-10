// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryFlowGraph/IFlowBaseNode.h>

class CFlowNode_AudioPreload final : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioPreload(SActivationInfo* pActInfo)
		: m_preloadRequestId(CryAudio::InvalidPreloadRequestId)
		, m_isLoaded(false)
		, m_isListener(false)
	{}

	//////////////////////////////////////////////////////////////////////////
	virtual ~CFlowNode_AudioPreload() override
	{
		if (m_isListener)
		{
			RemoveRequestListener();
		}
	}

	CFlowNode_AudioPreload(CFlowNode_AudioPreload const&) = delete;
	CFlowNode_AudioPreload(CFlowNode_AudioPreload&&) = delete;
	CFlowNode_AudioPreload& operator=(CFlowNode_AudioPreload const&) = delete;
	CFlowNode_AudioPreload& operator=(CFlowNode_AudioPreload&&) = delete;

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) override
	{
		return new CFlowNode_AudioPreload(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS
	{
		eIn_PreloadName,
		eIn_Load,
		eIn_Unload,
	};

	enum OUTPUTS
	{
		eOut_Done,
	};

	//////////////////////////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config) override
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("audioPreloadRequest_Name", _HELP("Name of the preload request"),   "Name"),

			InputPortConfig_Void("Load",                        _HELP("Loads the preload request")),
			InputPortConfig_Void("Unload",                      _HELP("Unloads the preload requests")),
			{ 0 } };

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig_Void("Loading Done", _HELP("Activated when the preload has finished loading successfully or partially or failed loading")),
			{ 0 } };

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Node that allows for loading and unloading an audio preload.");
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	void Load(SActivationInfo* pActInfo)
	{
		if (!m_isLoaded && (m_preloadRequestId != CryAudio::InvalidPreloadRequestId) && (gEnv->pAudioSystem != nullptr))
		{
			CryAudio::SRequestUserData const userData(CryAudio::ERequestFlags::SubsequentCallbackOnExternalThread, this, reinterpret_cast<void*>(static_cast<UINT_PTR>(m_playActivationInfo.pGraph->GetGraphId())), this);
			gEnv->pAudioSystem->PreloadSingleRequest(m_preloadRequestId, false, userData);
			m_isLoaded = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Unload(SActivationInfo* pActInfo)
	{
		if (m_isLoaded && (m_preloadRequestId != CryAudio::InvalidPreloadRequestId) && (gEnv->pAudioSystem != nullptr))
		{
			gEnv->pAudioSystem->UnloadSingleRequest(m_preloadRequestId);
			m_isLoaded = false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
	{
		bool isLoaded = m_isLoaded;
		ser.BeginGroup("FlowAudioPreloadNode");
		ser.Value("loaded", isLoaded);
		ser.EndGroup();

		if (ser.IsReading())
		{
			if (isLoaded)
			{
				Load(pActInfo);
			}
			else
			{
				Unload(pActInfo);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Init(pActInfo);

				break;
			}
		case eFE_ConnectOutputPort:
			{
				if (!m_isListener)
				{
					AddRequestListener();
				}

				break;
			}
		case eFE_DisconnectOutputPort:
			{
				if (m_isListener)
				{
					RemoveRequestListener();
				}

				break;
			}
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIn_Load))
				{
					m_playActivationInfo = *pActInfo;
					UpdatePreloadId(pActInfo);
					Load(pActInfo);
				}

				if (IsPortActive(pActInfo, eIn_Unload))
				{
					UpdatePreloadId(pActInfo);
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

	//////////////////////////////////////////////////////////////////////////
	void PreloadFinished(CryAudio::PreloadRequestId const preloadRequestId)
	{
		if (preloadRequestId == m_preloadRequestId)
		{
			ActivateOutput(&m_playActivationInfo, eOut_Done, true);
		}
	}

private:

	//////////////////////////////////////////////////////////////////////////
	static void OnPreloadFinished(CryAudio::SRequestInfo const* const pAudioRequestInfo)
	{
		auto const id = static_cast<TFlowGraphId>(reinterpret_cast<UINT_PTR>(pAudioRequestInfo->pUserData));

		if (gEnv->pFlowSystem->GetGraphById(id) != nullptr)
		{
			// Here we are sure that this FlowGraphNode is still valid.
			auto const pPreloadNode = static_cast<CFlowNode_AudioPreload*>(pAudioRequestInfo->pOwner);
			pPreloadNode->PreloadFinished(static_cast<CryAudio::PreloadRequestId>(pAudioRequestInfo->audioControlId));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Init(SActivationInfo* const pActInfo)
	{
		if (gEnv->pAudioSystem != nullptr)
		{
			UpdatePreloadId(pActInfo);

			if (IsOutputConnected(pActInfo, eOut_Done))
			{
				if (!m_isListener)
				{
					AddRequestListener();
				}
			}
			else if (m_isListener)
			{
				RemoveRequestListener();
			}

			Unload(pActInfo);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void UpdatePreloadId(SActivationInfo* const pActInfo)
	{
		m_preloadRequestId = CryAudio::InvalidPreloadRequestId;
		string const& preloadName = GetPortString(pActInfo, eIn_PreloadName);

		if (!preloadName.empty())
		{
			m_preloadRequestId = CryAudio::StringToId(preloadName.c_str());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void AddRequestListener()
	{
		gEnv->pAudioSystem->AddRequestListener(
			&CFlowNode_AudioPreload::OnPreloadFinished,
			this,
			(CryAudio::ESystemEvents::PreloadFinishedSuccess | CryAudio::ESystemEvents::PreloadFinishedFailure));
		m_isListener = true;
	}

	//////////////////////////////////////////////////////////////////////////
	void RemoveRequestListener()
	{
		gEnv->pAudioSystem->RemoveRequestListener(&CFlowNode_AudioPreload::OnPreloadFinished, this);
		m_isListener = false;
	}

	CryAudio::PreloadRequestId m_preloadRequestId;
	SActivationInfo            m_playActivationInfo;
	bool                       m_isLoaded;
	bool                       m_isListener;
};

REGISTER_FLOW_NODE("Audio:Preload", CFlowNode_AudioPreload);
