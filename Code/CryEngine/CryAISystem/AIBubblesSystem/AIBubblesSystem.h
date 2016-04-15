// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/********************************************************************
   -------------------------------------------------------------------------
   File name:   AIBubblesSystem.h
   Description: Central class to create and display speech bubbles over the
   AI agents to notify messages to the designers

   -------------------------------------------------------------------------
   History:
   - 09:11:2011 : Created by Francesco Roccucci
 *********************************************************************/

#ifndef __AIBubblesSystem_h__
#define __AIBubblesSystem_h__

#include "IAIBubblesSystem.h"

#ifdef CRYAISYSTEM_DEBUG

class CAIBubblesSystem : public IAIBubblesSystem
{
public:
	// IAIBubblesSystem
	virtual void Init();
	virtual void Reset();
	virtual void QueueMessage(const char* messageName, const SAIBubbleRequest& request);
	virtual void Update();
	// ~IAIBubblesSystem

private:

	// SAIBubbleRequestContainer needs only to be known internally in the
	// BubblesNotifier
	struct SAIBubbleRequestContainer
	{
		SAIBubbleRequestContainer(const uint32 _requestId, const SAIBubbleRequest& _request)
			: requestId(_requestId)
			, expiringTime(.0f)
			, request(_request)
			, startExpiringTime(true)
		{
		}

		const SAIBubbleRequest& GetRequest()
		{
			if (startExpiringTime)
			{
				UpdateDuration(request.GetDuration());
				startExpiringTime = false;
			}
			return request;
		}
		uint32 GetRequestId() const { return requestId; }

		bool   IsOld(const CTimeValue currentTimestamp) const
		{
			return currentTimestamp > expiringTime;
		}

	private:
		void UpdateDuration(const float duration)
		{
			expiringTime = gEnv->pTimer->GetFrameStartTime() + (duration ? CTimeValue(duration) : CTimeValue(gAIEnv.CVars.BubblesSystemDecayTime));
		}

		uint32           requestId;
		CTimeValue       expiringTime;
		bool             startExpiringTime;
		SAIBubbleRequest request;
	};

	struct SRequestFinder
	{
		SRequestFinder(uint32 _requestIndex) : requestIndex(_requestIndex) {}
		bool operator()(const SAIBubbleRequestContainer& container) const { return container.GetRequestId() == requestIndex; }
		unsigned requestIndex;
	};

	bool DisplaySpeechBubble(const SAIBubbleRequest& request) const;
	// Drawing functions
	void DrawBubble(const char* const message, const Vec3& pos, const size_t numberOfLines) const;

	// Information logging/popping up
	void LogMessage(const char* const message, const TBubbleRequestOptionFlags flags) const;
	void PopupBlockingAlert(const char* const message, const TBubbleRequestOptionFlags flags) const;

	bool ShouldSuppressMessageVisibility(const SAIBubbleRequest::ERequestType requestType = SAIBubbleRequest::eRT_ErrorMessage) const;

	typedef std::list<SAIBubbleRequestContainer>                         RequestsList;
	typedef std::unordered_map<EntityId, RequestsList, stl::hash_uint32> EntityRequestsMap;
	EntityRequestsMap m_entityRequestsMap;
};
#endif // CRYAISYSTEM_DEBUG

#endif // __AIBubblesSystem_h__
