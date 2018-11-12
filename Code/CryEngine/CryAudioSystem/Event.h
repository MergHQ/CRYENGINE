// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"
#include "Common/SharedData.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <atomic>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
class CObject;

namespace Impl
{
struct IEvent;
} // namespace Impl

class CEvent final : public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = default;

	void      Release();
	void      Stop();
	bool      IsPlaying() const                { return (m_state == EEventState::Playing) || (m_state == EEventState::Virtual); }
	void      SetTriggerId(ControlId const id) { m_triggerId = id; }
	ControlId GetTriggerId() const             { return m_triggerId; }
	void      SetVirtual();
	void      SetPlaying();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	void              SetTriggerName(char const* const szTriggerName) { m_szTriggerName = szTriggerName; }
	char const* const GetTriggerName() const                          { return m_szTriggerName; }
	void              SetTriggerRadius(float const radius)            { m_triggerRadius = radius; }
	float             GetTriggerRadius() const                        { return m_triggerRadius; }

	std::atomic_bool m_toBeRemoved{ false };
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	CObject*          m_pObject = nullptr;
	TriggerImplId     m_triggerImplId = InvalidTriggerImplId;
	TriggerInstanceId m_triggerInstanceId = InvalidTriggerInstanceId;
	EEventState       m_state = EEventState::None;
	Impl::IEvent*     m_pImplData = nullptr;

private:

	ControlId m_triggerId = InvalidControlId;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	char const* m_szTriggerName = nullptr;
	float       m_triggerRadius = 0.0f;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
