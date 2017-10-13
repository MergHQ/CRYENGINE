#pragma once

#include <CrySystem/ICryPlugin.h>

#ifdef DELETE
#undef DELETE
#endif

class IHttpPlugin
	: public ICryPlugin
{
public:
	enum class ERequestType
	{
		POST = 0,
		GET,
		PUT,
		PATCH,
		DELETE
	};

	CRYINTERFACE_DECLARE_GUID(IHttpPlugin, "{BC0BA532-EAAD-4B91-AA71-C65E435ABDC1}"_cry_guid);

	virtual ~IHttpPlugin() { }

	enum class EUpdateResult
	{
		Idle = 0,
		ProcessingRequests
	};

	//! Processes currently on-going HTTP requests, downloading / uploading and triggering callbacks
	//! This can be called as many times as desired even in a single frame, but will also be automatically done once per frame
	//! \return Idle if no requests are being handled, otherwise ProcessingRequests
	virtual EUpdateResult ProcessRequests() = 0;

	typedef std::function<void(const char* szResponse, long responseCode)> TResponseCallback;
	//! Queues a request for sending over the network
	//! \param requestType Determines the type of HTTP request to send
	//! \param szURL null-terminated address to send the request to
	//! \param szBody Request body
	//! \param resultCallback Callback to be invoked when the server has replied
	//! \param headers Array of headers to sent to the server
	virtual void Send(ERequestType requestType, const char* szURL, const char* szBody, TResponseCallback resultCallback = nullptr, const DynArray<const char*>& headers = DynArray<const char*>()) = 0;
};