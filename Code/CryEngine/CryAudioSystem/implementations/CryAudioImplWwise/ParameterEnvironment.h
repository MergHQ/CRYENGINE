// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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
class CParameterEnvironment final : public IEnvironmentConnection, public CPoolObject<CParameterEnvironment, stl::PSyncNone>
{
public:

	CParameterEnvironment() = delete;
	CParameterEnvironment(CParameterEnvironment const&) = delete;
	CParameterEnvironment(CParameterEnvironment&&) = delete;
	CParameterEnvironment& operator=(CParameterEnvironment const&) = delete;
	CParameterEnvironment& operator=(CParameterEnvironment&&) = delete;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	explicit CParameterEnvironment(AkRtpcID const rtpcId, char const* const szName)
		: m_rtpcId(rtpcId)
		, m_name(szName)
	{}
#else
	explicit CParameterEnvironment(AkRtpcID const rtpcId)
		: m_rtpcId(rtpcId)
	{}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE

	virtual ~CParameterEnvironment() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	AkRtpcID const m_rtpcId;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
