// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "IHttpPlugin.h"

#include <CrySystem/ITimer.h>

#ifdef CRY_PLATFORM_WINDOWS
#include <winsock2.h>
#include <CryCore/Platform/CryWindows.h>

#ifdef DELETE
#undef DELETE
#endif
#endif

#include <curl/curl.h>

class CHttpImplementationCurl final : public IHttpPlugin
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IHttpPlugin)
		CRYINTERFACE_ADD(ICryPlugin)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS_GUID(CHttpImplementationCurl, "Plugin_HTTP", "{D6F141B9-CDD8-44AB-ACF5-9C70EBC646CB}"_cry_guid)

	CHttpImplementationCurl() = default;
	virtual ~CHttpImplementationCurl();

public:
	// ICryPlugin
	virtual const char* GetName() const override { return "CryHTTP"; }
	virtual const char* GetCategory() const override { return "Default"; }
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
	virtual void OnPluginUpdate(EPluginUpdateType updateType) override;
	// ~ICryPlugin

	// IPlugin_HTTP
	virtual EUpdateResult ProcessRequests() final;
	virtual void Send(ERequestType requestType, const char* szURL, const char* szBody, TResponseCallback resultCallback, const DynArray<const char*>& headers) final;
	// ~IPlugin_HTTP

protected:
	static size_t CurlWriteResponse(char *ptr, size_t size, size_t nmemb, void *userData);
	static int CurlProgress(void *userData, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

protected:
	struct SRequest
	{
		SRequest(CURL* pRequestHandle, TResponseCallback callback)
			: pHandle(pRequestHandle)
			, resultCallback(callback)
			, pHeaderList(nullptr)
			, lastTimeHandledData(gEnv->pTimer->GetAsyncCurTime())
		{
		}

		SRequest(SRequest& request) = delete;
		SRequest& operator=(SRequest&) = delete;

		SRequest(SRequest&& request)
			: pHandle(request.pHandle)
			, resultCallback(request.resultCallback)
			, result(std::move(request.result))
			, pHeaderList(request.pHeaderList)
			, lastTimeHandledData(request.lastTimeHandledData)
		{
			request.pHandle = nullptr;
			request.pHeaderList = nullptr;
		}
		SRequest& operator=(SRequest&&) = default;

		~SRequest()
		{
			if (pHeaderList != nullptr)
			{
				curl_slist_free_all(pHeaderList);
			}

			curl_easy_cleanup(pHandle);
		}

		CURL* pHandle;
		curl_slist* pHeaderList;

		TResponseCallback resultCallback;
		string result;
		// Time at which we last handled data, used for timeouts
		float lastTimeHandledData;
	};

	CURLM* m_pMultiHandle = nullptr;
	std::unordered_map<CURL*, SRequest> m_requestMap;

	float m_requestTimeOutInSeconds = 20.f;
};