// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ITriggerConnection.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CEvent final : public ITriggerConnection, public CPoolObject<CEvent, stl::PSyncNone>
{
public:

	CEvent() = delete;
	CEvent(CEvent const&) = delete;
	CEvent(CEvent&&) = delete;
	CEvent& operator=(CEvent const&) = delete;
	CEvent& operator=(CEvent&&) = delete;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	explicit CEvent(AkUniqueID const id, float const maxAttenuation, char const* const szName)
		: m_id(id)
		, m_maxAttenuation(maxAttenuation)
		, m_name(szName)
	{}
#else
	explicit CEvent(AkUniqueID const id, float const maxAttenuation)
		: m_id(id)
		, m_maxAttenuation(maxAttenuation)
	{}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	virtual ~CEvent() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ERequestStatus Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId) override;
	virtual void           Stop(IObject* const pIObject) override;
	virtual ERequestStatus Load() const override;
	virtual ERequestStatus Unload() const override;
	virtual ERequestStatus LoadAsync(TriggerInstanceId const triggerInstanceId) const override;
	virtual ERequestStatus UnloadAsync(TriggerInstanceId const triggerInstanceId) const override;
	// ~CryAudio::Impl::ITriggerConnection

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

private:

	ERequestStatus SetLoaded(bool const bLoad) const;
	ERequestStatus SetLoadedAsync(TriggerInstanceId const triggerInstanceId, bool const bLoad) const;

	AkUniqueID const m_id;
	float const      m_maxAttenuation;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
