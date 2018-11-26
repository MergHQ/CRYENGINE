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
class CTrigger final : public ITriggerConnection, public CPoolObject<CTrigger, stl::PSyncNone>
{
public:

	CTrigger() = delete;
	CTrigger(CTrigger const&) = delete;
	CTrigger(CTrigger&&) = delete;
	CTrigger& operator=(CTrigger const&) = delete;
	CTrigger& operator=(CTrigger&&) = delete;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	explicit CTrigger(AkUniqueID const id, float const maxAttenuation, char const* const szName)
		: m_id(id)
		, m_maxAttenuation(maxAttenuation)
		, m_name(szName)
	{}
#else
	explicit CTrigger(AkUniqueID const id, float const maxAttenuation)
		: m_id(id)
		, m_maxAttenuation(maxAttenuation)
	{}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITriggerConnection
	virtual ERequestStatus Execute(IObject* const pIObject, IEvent* const pIEvent) override;
	virtual ERequestStatus Load() const override;
	virtual ERequestStatus Unload() const override;
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override;
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override;
	// ~CryAudio::Impl::ITriggerConnection

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	char const* GetName() const { return m_name.c_str(); }
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

private:

	ERequestStatus SetLoaded(bool const bLoad) const;
	ERequestStatus SetLoadedAsync(IEvent* const pIEvent, bool const bLoad) const;

	AkUniqueID const m_id;
	float const      m_maxAttenuation;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
