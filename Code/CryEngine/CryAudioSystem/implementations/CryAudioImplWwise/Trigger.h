// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CTrigger final : public ITrigger
{
public:

	CTrigger() = delete;
	CTrigger(CTrigger const&) = delete;
	CTrigger(CTrigger&&) = delete;
	CTrigger& operator=(CTrigger const&) = delete;
	CTrigger& operator=(CTrigger&&) = delete;

	explicit CTrigger(AkUniqueID const id, float const maxAttenuation);
	virtual ~CTrigger() override = default;

	// CryAudio::Impl::ITrigger
	virtual ERequestStatus Load() const override;
	virtual ERequestStatus Unload() const override;
	virtual ERequestStatus LoadAsync(IEvent* const pIEvent) const override;
	virtual ERequestStatus UnloadAsync(IEvent* const pIEvent) const override;
	// ~CryAudio::Impl::ITrigger

	AkUniqueID const m_id;
	float const      m_maxAttenuation;

private:

	ERequestStatus SetLoaded(bool const bLoad) const;
	ERequestStatus SetLoadedAsync(IEvent* const pIEvent, bool const bLoad) const;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
