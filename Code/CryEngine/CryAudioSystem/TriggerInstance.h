// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include "Common/PoolObject.h"
#include <CryEntitySystem/IEntityBasicTypes.h>

namespace CryAudio
{
class CTriggerInstance final : public CPoolObject<CTriggerInstance, stl::PSyncNone>
{
public:

	CTriggerInstance() = delete;
	CTriggerInstance(CTriggerInstance const&) = delete;
	CTriggerInstance(CTriggerInstance&&) = delete;
	CTriggerInstance& operator=(CTriggerInstance const&) = delete;
	CTriggerInstance& operator=(CTriggerInstance&&) = delete;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	explicit CTriggerInstance(
		ControlId const triggerId,
		uint16 const numPlayingConnectionInstances,
		uint16 const numPendingConnectionInstances,
		ERequestFlags const flags,
		void* const pOwner,
		void* const pUserData,
		void* const pUserDataOwner,
		float const radius)
		: m_triggerId(triggerId)
		, m_numPlayingConnectionInstances(numPlayingConnectionInstances)
		, m_numPendingConnectionInstances(numPendingConnectionInstances)
		, m_flags(flags)
		, m_pOwner(pOwner)
		, m_pUserData(pUserData)
		, m_pUserDataOwner(pUserDataOwner)
		, m_radius(radius)
	{}
#else
	explicit CTriggerInstance(
		ControlId const triggerId,
		uint16 const numPlayingConnectionInstances,
		uint16 const numPendingConnectionInstances,
		ERequestFlags const flags,
		void* const pOwner,
		void* const pUserData,
		void* const pUserDataOwner)
		: m_triggerId(triggerId)
		, m_numPlayingConnectionInstances(numPlayingConnectionInstances)
		, m_numPendingConnectionInstances(numPendingConnectionInstances)
		, m_flags(flags)
		, m_pOwner(pOwner)
		, m_pUserData(pUserData)
		, m_pUserDataOwner(pUserDataOwner)
	{}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	ControlId GetTriggerId() const                     { return m_triggerId; }

	void      IncrementNumPlayingConnectionInstances() { ++m_numPlayingConnectionInstances; }
	void      IncrementNumPendingConnectionInstances() { ++m_numPendingConnectionInstances; }

	bool      IsPlayingInstanceFinished();
	bool      IsPendingInstanceFinished();

	void      SetPendingToPlaying();
	void      SendFinishedRequest(EntityId const entityId);
	void      Release();

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	float GetRadius() const             { return m_radius; }
	void  SetRadius(float const radius) { m_radius = radius; }
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

private:

	ControlId const     m_triggerId;
	uint16              m_numPlayingConnectionInstances;
	uint16              m_numPendingConnectionInstances;
	ERequestFlags const m_flags;

	void* const         m_pOwner;
	void* const         m_pUserData;
	void* const         m_pUserDataOwner;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	float m_radius;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};

using TriggerInstances = std::map<TriggerInstanceId, CTriggerInstance*>;
} // namespace CryAudio
