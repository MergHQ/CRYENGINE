// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironmentConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CAuxBus final : public IEnvironmentConnection, public CPoolObject<CAuxBus, stl::PSyncNone>
{
public:

	CAuxBus() = delete;
	CAuxBus(CAuxBus const&) = delete;
	CAuxBus(CAuxBus&&) = delete;
	CAuxBus& operator=(CAuxBus const&) = delete;
	CAuxBus& operator=(CAuxBus&&) = delete;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	explicit CAuxBus(AkAuxBusID const busId, char const* const szName)
		: m_busId(busId)
		, m_name(szName)
	{}
#else
	explicit CAuxBus(AkAuxBusID const busId)
		: m_busId(busId)
	{}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	virtual ~CAuxBus() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	AkAuxBusID const m_busId;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> m_name;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
