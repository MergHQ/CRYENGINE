// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryPhysics/RayCastQueue.h>
#include <CryEntitySystem/IEntityBasicTypes.h>
#include <CrySystem/ITimer.h>

namespace AIRayCast
{
struct SRequesterDebugInfo
{
	SRequesterDebugInfo(const SRequesterDebugInfo& other)
		: szRequesterString(other.szRequesterString)
		, szCustomString(other.szCustomString)
		, entityId(other.entityId)
	{}
	SRequesterDebugInfo(const char* szRequesterString = "Unknown Requester", const EntityId entityId = INVALID_ENTITYID, const char* szCustomString = nullptr)
		: szRequesterString(szRequesterString)
		, szCustomString(szCustomString)
		, entityId(entityId)
	{}

	const char* szRequesterString;
	const char* szCustomString;
	EntityId    entityId;
};

struct SRequestDebugInfo
{
	enum class EState
	{
		Queued,
		Submitted,
		Completed,
	};

	SRequestDebugInfo(const SRequesterDebugInfo& requesterInfo, const RayCastRequest::Priority priorityClass)
		: requesterString(requesterInfo.szRequesterString)
		, customString(requesterInfo.szCustomString)
		, entityId(requesterInfo.entityId)
		, priorityClass(priorityClass)
		, state(EState::Queued)
		, queuedTime(gEnv->pTimer->GetAsyncTime())
		, queuedFrame(gEnv->nMainFrameID)
	{}

	string                   requesterString;
	string                   customString;
	EntityId                 entityId;
	RayCastRequest::Priority priorityClass;
	EState                   state;
	CTimeValue               queuedTime;
	CTimeValue               completedTime;
	uint32                   queuedFrame;
	uint32                   completedFrame;
};

struct SContentionStats
{
	uint32                      quota;
	uint32                      queueSize;
	uint32                      peakQueueSize;

	uint32                      immediateCount;
	uint32                      peakImmediateCount;
	uint32                      deferredCount;
	uint32                      peakDeferredCount;

	float                       immediateAverage;
	float                       deferredAverage;

	DynArray<SRequestDebugInfo> pendingRequests;
	DynArray<SRequestDebugInfo> recentlyCompletedRequests;
};

struct SContentionExtended : public DefaultContention
{
	typedef DefaultContention Base;

	SContentionStats GetContentionStats()
	{
		Base::ContentionStats baseStats = Base::GetContentionStats();

		SContentionStats stats;
		stats.quota = baseStats.quota;
		stats.immediateCount = baseStats.immediateCount;
		stats.peakImmediateCount = baseStats.peakImmediateCount;
		stats.deferredCount = baseStats.deferredCount;
		stats.peakDeferredCount = baseStats.peakDeferredCount;
		stats.immediateAverage = baseStats.immediateAverage;
		stats.deferredAverage = baseStats.deferredAverage;
		stats.queueSize = baseStats.queueSize;
		stats.peakQueueSize = baseStats.peakQueueSize;

		stats.pendingRequests.reserve(m_pendingRequestsMap.size());
		for (auto& keyValue : m_pendingRequestsMap)
		{
			stats.pendingRequests.push_back(keyValue.second);
		}
		stats.recentlyCompletedRequests = m_completedRequests;
		m_completedRequests.clear();

		return stats;
	}

	void ResetContentionStats()
	{
		m_pendingRequestsMap.clear();
		m_completedRequests.clear();

		Base::ResetContentionStats();
	}

protected:
	typedef std::unordered_map<uint32, SRequestDebugInfo> RequestsMap;

	SContentionExtended(uint32 quota = 64)
		: Base(quota)
	{
	}

	void Queued(const SRequesterDebugInfo& requesterInfo, const uint32 queuedId, const RayCastRequest::Priority priorityClass)
	{
		if (m_isEnabled)
		{
			m_pendingRequestsMap.emplace(queuedId, SRequestDebugInfo(requesterInfo, priorityClass));
		}
	}

	inline void PerformedDeferred(const uint32 queuedId)
	{
		if (m_isEnabled)
		{
			RequestsMap::iterator it = m_pendingRequestsMap.find(queuedId);
			if (it != m_pendingRequestsMap.end())
			{
				it->second.state = SRequestDebugInfo::EState::Submitted;
			}
		}
		Base::PerformedDeferred(queuedId);
	}

	inline void CompletedDeferred(const uint32 queuedId)
	{
		if (m_isEnabled)
		{
			RequestsMap::iterator it = m_pendingRequestsMap.find(queuedId);
			if (it != m_pendingRequestsMap.end())
			{
				SRequestDebugInfo& completedRequest = it->second;
				completedRequest.completedTime = gEnv->pTimer->GetAsyncTime();
				completedRequest.completedFrame = gEnv->nMainFrameID;
				completedRequest.state = SRequestDebugInfo::EState::Completed;
				m_completedRequests.push_back(completedRequest);

				m_pendingRequestsMap.erase(it);
			}
		}
	}

	void Canceled(const uint32 queuedId)
	{
		if (m_isEnabled)
		{
			m_pendingRequestsMap.erase(queuedId);
		}
	}

	bool IsGatheringExtendedStats() const
	{
		return m_isEnabled;
	}

	void EnableGatheringExtendedStats(bool enable)
	{
		if (!enable)
		{
			m_pendingRequestsMap.clear();
			m_completedRequests.clear();
		}
		m_isEnabled = enable;
	}

private:
	bool                        m_isEnabled = false;
	RequestsMap                 m_pendingRequestsMap;
	DynArray<SRequestDebugInfo> m_completedRequests;
};

template<int RayCasterID, bool ExtendedStats = false> class CQueue;

template<int RayCasterID> class CQueue<RayCasterID, true>
	: public DeferredActionQueue<
			DefaultRayCaster<RayCasterID>,
			RayCastRequest,
			RayCastResult,
			SContentionExtended>
{
	typedef DeferredActionQueue<DefaultRayCaster<RayCasterID>, RayCastRequest, RayCastResult, SContentionExtended> BaseType;

public:
	bool   IsGatheringExtendedStats() const          { return BaseType::ContentionPolicy::IsGatheringExtendedStats(); }
	void   EnableGatheringExtendedStats(bool enable) { BaseType::ContentionPolicy::EnableGatheringExtendedStats(enable); }

	uint32 Queue(const typename BaseType::PriorityType& priority, const typename BaseType::ResultCallback& resultCallback,
	             const typename BaseType::SubmitCallback& submitCallback, const SRequesterDebugInfo& requester = SRequesterDebugInfo())
	{
		const uint32 queuedId = BaseType::Queue(priority, resultCallback, submitCallback);
		BaseType::ContentionPolicy::Queued(requester, queuedId, priority);
		return queuedId;
	}

	uint32 Queue(const typename BaseType::PriorityType& priority, const RayCastRequest& request,
	             const typename BaseType::ResultCallback& resultCallback, const typename BaseType::SubmitCallback& submitCallback = 0, const SRequesterDebugInfo& requester = SRequesterDebugInfo())
	{
		const uint32 queuedId = BaseType::Queue(priority, request, resultCallback, submitCallback);
		BaseType::ContentionPolicy::Queued(requester, queuedId, priority);
		return queuedId;
	}
};

template<int RayCasterID> class CQueue<RayCasterID, false>
	: public DeferredActionQueue<
			DefaultRayCaster<RayCasterID>,
			RayCastRequest,
			RayCastResult,
			DefaultContention>
{
	typedef DeferredActionQueue<DefaultRayCaster<RayCasterID>, RayCastRequest, RayCastResult, DefaultContention> BaseType;

public:
	bool   IsGatheringExtendedStats() const          { return false; }
	void   EnableGatheringExtendedStats(bool enable) {}

	uint32 Queue(const typename BaseType::PriorityType& priority, const typename BaseType::ResultCallback& resultCallback,
	             const typename BaseType::SubmitCallback& submitCallback, const SRequesterDebugInfo& requester = 0)
	{
		return BaseType::Queue(priority, resultCallback, submitCallback);
	}

	uint32 Queue(const typename BaseType::PriorityType& priority, const RayCastRequest& request,
	             const typename BaseType::ResultCallback& resultCallback, const typename BaseType::SubmitCallback& submitCallback = 0, const SRequesterDebugInfo& requester = 0)
	{
		return BaseType::Queue(priority, request, resultCallback, submitCallback);
	}
};

} //namespace AIRayCast
