// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_AudioPreloadData : public CFlowBaseNode<eNCT_Instanced>
{
public:

	explicit CFlowNode_AudioPreloadData(SActivationInfo* pActInfo)
		: m_bEnable(false)
	{}

	~CFlowNode_AudioPreloadData() {}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_AudioPreloadData(pActInfo);
	}

	enum INPUTS
	{
		eIn_PreloadRequestFirst = 0,
		eIn_PreloadRequest1,
		eIn_PreloadRequest2,
		eIn_PreloadRequest3,
		eIn_PreloadRequest4,
		eIn_PreloadRequest5,
		eIn_PreloadRequest6,
		eIn_PreloadRequestLast,
		eIn_Enable,
		eIn_Disable,
	};

	enum OUTPUTS
	{
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest0", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest1", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest2", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest3", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest4", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest5", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest6", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig<string>("audioPreloadRequest_PreloadRequest7", _HELP("name of preload request"),                "Preload Request"),
			InputPortConfig_Void("Load",                                   _HELP("loads all supplied preload requests")),
			InputPortConfig_Void("Unload",                                 _HELP("unloads all supplied preload requests")),
			{ 0 }
		};

		static const SOutputPortConfig outputs[] =
		{
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Node that allows for handling audio preload requests.");
		config.SetCategory(EFLN_APPROVED);
	}

	void Enable(SActivationInfo* pActInfo, bool bEnable)
	{
		if (m_bEnable != bEnable && gEnv->pAudioSystem != nullptr)
		{
			for (uint32 i = eIn_PreloadRequestFirst; i <= eIn_PreloadRequestLast; ++i)
			{
				string const& preloadName = GetPortString(pActInfo, static_cast<int>(i));

				if (!preloadName.empty())
				{
					SAudioRequest request;
					AudioPreloadRequestId preloadRequestId = INVALID_AUDIO_PRELOAD_REQUEST_ID;

					if (gEnv->pAudioSystem->GetAudioPreloadRequestId(preloadName.c_str(), preloadRequestId))
					{
						if (bEnable)
						{
							SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> requestData(preloadRequestId, false);
							request.pData = &requestData;
							gEnv->pAudioSystem->PushRequest(request);
						}
						else
						{
							SAudioManagerRequestData<eAudioManagerRequestType_UnloadSingleRequest> requestData(preloadRequestId);
							request.pData = &requestData;
							gEnv->pAudioSystem->PushRequest(request);
						}
					}
				}
			}

			m_bEnable = bEnable;
		}
	}

	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		bool bEnable = m_bEnable;
		ser.BeginGroup("FlowAudioPreloadData");
		ser.Value("enable", bEnable);
		ser.EndGroup();

		if (ser.IsReading())
		{
			Enable(pActInfo, bEnable);
		}
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				Enable(pActInfo, false);

				break;
			}
		case eFE_Activate:
			{
				// Enable
				if (IsPortActive(pActInfo, eIn_Enable))
				{
					Enable(pActInfo, true);
				}

				// Disable
				if (IsPortActive(pActInfo, eIn_Disable))
				{
					Enable(pActInfo, false);
				}

				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:

	bool m_bEnable;
};

REGISTER_FLOW_NODE("Audio:PreloadData", CFlowNode_AudioPreloadData);
