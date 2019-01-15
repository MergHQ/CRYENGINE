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
class CObject;

enum class EEventState : EnumFlagsType
{
	None,
	Playing,
	Loading,
	Unloading,
	Virtual,
};

class CEvent final : public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

	CEvent(TriggerInstanceId const triggerInstanceId)
		: m_triggerInstanceId(triggerInstanceId)
		, m_state(EEventState::None)
		, m_id(AK_INVALID_UNIQUE_ID)
		, m_triggerId(AK_INVALID_UNIQUE_ID)
		, m_pObject(nullptr)
		, m_maxAttenuation(0.0f)
		, m_toBeRemoved(false)
	{}

	~CEvent() = default;

	void Stop();
	void UpdateVirtualState();

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	void        SetName(char const* const szName) { m_name = szName; }
	char const* GetName() const                   { return m_name.c_str(); }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	TriggerInstanceId const m_triggerInstanceId;
	EEventState             m_state;
	AkUniqueID              m_id;
	AkUniqueID              m_triggerId;
	CObject*                m_pObject;
	float                   m_maxAttenuation;
	std::atomic_bool        m_toBeRemoved;

private:

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
