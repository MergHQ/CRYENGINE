// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <atomic>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CBaseObject;
class CEvent;

enum class EEventInstanceState : EnumFlagsType
{
	None,
	Playing,
	Loading,
	Unloading,
	Virtual,
};

class CEventInstance final : public CPoolObject<CEventInstance, stl::PSyncNone>
{
public:

	CEventInstance() = delete;
	CEventInstance(CEventInstance const&) = delete;
	CEventInstance(CEventInstance&&) = delete;
	CEventInstance& operator=(CEventInstance const&) = delete;
	CEventInstance& operator=(CEventInstance&&) = delete;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CEventInstance(
		TriggerInstanceId const triggerInstanceId,
		AkUniqueID const eventId,
		float const maxAttenuation,
		CBaseObject const* const pBaseObject,
		CEvent const* const pEvent)
		: m_triggerInstanceId(triggerInstanceId)
		, m_eventId(eventId)
		, m_maxAttenuation(maxAttenuation)
		, m_state(EEventInstanceState::None)
		, m_playingId(AK_INVALID_UNIQUE_ID)
		, m_toBeRemoved(false)
		, m_pBaseObject(pBaseObject)
		, m_pEvent(pEvent)
	{}
#else
	CEventInstance(
		TriggerInstanceId const triggerInstanceId,
		AkUniqueID const eventId,
		float const maxAttenuation)
		: m_triggerInstanceId(triggerInstanceId)
		, m_eventId(eventId)
		, m_maxAttenuation(maxAttenuation)
		, m_state(EEventInstanceState::None)
		, m_playingId(AK_INVALID_UNIQUE_ID)
		, m_toBeRemoved(false)
	{}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	~CEventInstance() = default;

	TriggerInstanceId   GetTriggerInstanceId() const              { return m_triggerInstanceId; }

	EEventInstanceState GetState() const                          { return m_state; }
	void                SetState(EEventInstanceState const state) { m_state = state; }

	AkUniqueID          GetPlayingId() const                      { return m_playingId; }
	void                SetPlayingId(AkUniqueID const playingId)  { m_playingId = playingId; }

	AkUniqueID          GetEventId() const                        { return m_eventId; }

	float               GetMaxAttenuation() const                 { return m_maxAttenuation; }

	bool                IsToBeRemoved() const                     { return m_toBeRemoved; }
	void                SetToBeRemoved()                          { m_toBeRemoved = true; }

	void                Stop();
	void                UpdateVirtualState(float const distance);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CBaseObject const* GetObject() const { return m_pBaseObject; }
	CEvent const*      GetEvent() const  { return m_pEvent; }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

private:

	TriggerInstanceId const m_triggerInstanceId;
	AkUniqueID const        m_eventId;
	float const             m_maxAttenuation;
	EEventInstanceState     m_state;
	AkUniqueID              m_playingId;
	std::atomic_bool        m_toBeRemoved;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CBaseObject const* const m_pBaseObject;
	CEvent const* const      m_pEvent;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
