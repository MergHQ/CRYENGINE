// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CEventInstance final : public CPoolObject<CEventInstance, stl::PSyncNone>
{
public:

	CEventInstance() = delete;
	CEventInstance(CEventInstance const&) = delete;
	CEventInstance(CEventInstance&&) = delete;
	CEventInstance& operator=(CEventInstance const&) = delete;
	CEventInstance& operator=(CEventInstance&&) = delete;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	explicit CEventInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const eventId,
		CEvent const* const pEvent,
		CObject const* const pObject)
		: m_triggerInstanceId(triggerInstanceId)
		, m_eventId(eventId)
		, m_pEvent(pEvent)
		, m_toBeRemoved(false)
		, m_pObject(pObject)
	{}
#else
	explicit CEventInstance(
		TriggerInstanceId const triggerInstanceId,
		uint32 const eventId,
		CEvent const* const pEvent)
		: m_triggerInstanceId(triggerInstanceId)
		, m_eventId(eventId)
		, m_pEvent(pEvent)
		, m_toBeRemoved(false)
	{}
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	~CEventInstance() = default;

	TriggerInstanceId GetTriggerInstanceId() const { return m_triggerInstanceId; }
	uint32            GetEventId() const           { return m_eventId; }
	CEvent const*     GetEvent() const             { return m_pEvent; }

	void              Stop();
	void              Pause();
	void              Resume();

	bool              IsToBeRemoved() const { return m_toBeRemoved; }
	void              SetToBeRemoved()      { m_toBeRemoved = true; }

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	CObject const* GetObject() const { return m_pObject; }
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	ChannelList m_channels;

private:

	TriggerInstanceId const m_triggerInstanceId;
	uint32 const            m_eventId;
	CEvent const* const     m_pEvent;
	bool                    m_toBeRemoved;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	CObject const* const m_pObject;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio