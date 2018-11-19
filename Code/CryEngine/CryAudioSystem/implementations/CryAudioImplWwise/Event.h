// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEvent.h>
#include <SharedData.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <atomic>

namespace CryAudio
{
class CEvent;

namespace Impl
{
namespace Wwise
{
class CObject;

class CEvent final : public IEvent, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	explicit CEvent(CryAudio::CEvent& event_)
		: m_state(EEventState::None)
		, m_id(AK_INVALID_UNIQUE_ID)
		, m_event(event_)
		, m_pObject(nullptr)
		, m_maxAttenuation(0.0f)
		, m_toBeRemoved(false)
	{}

	virtual ~CEvent() override;

	// CryAudio::Impl::IEvent
	virtual ERequestStatus Stop() override;
	// ~CryAudio::Impl::IEvent

	void SetInitialVirtualState(float const distance);
	void UpdateVirtualState(float const distance);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	void        SetName(char const* const szName) { m_name = szName; }
	char const* GetName() const                   { return m_name.c_str(); }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	EEventState       m_state;
	AkUniqueID        m_id;
	CryAudio::CEvent& m_event;
	CObject*          m_pObject;
	float             m_maxAttenuation;
	std::atomic_bool  m_toBeRemoved;

private:

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
