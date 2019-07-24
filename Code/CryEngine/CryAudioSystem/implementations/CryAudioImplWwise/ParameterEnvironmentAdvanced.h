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
class CParameterEnvironmentAdvanced final : public IEnvironmentConnection, public CPoolObject<CParameterEnvironmentAdvanced, stl::PSyncNone>
{
public:

	CParameterEnvironmentAdvanced() = delete;
	CParameterEnvironmentAdvanced(CParameterEnvironmentAdvanced const&) = delete;
	CParameterEnvironmentAdvanced(CParameterEnvironmentAdvanced&&) = delete;
	CParameterEnvironmentAdvanced& operator=(CParameterEnvironmentAdvanced const&) = delete;
	CParameterEnvironmentAdvanced& operator=(CParameterEnvironmentAdvanced&&) = delete;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	explicit CParameterEnvironmentAdvanced(
		AkRtpcID const rtpcId,
		float const multiplier,
		float const shift,
		char const* const szName)
		: m_rtpcId(rtpcId)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}
#else
	explicit CParameterEnvironmentAdvanced(
		AkRtpcID const rtpcId,
		float const multiplier,
		float const shift)
		: m_rtpcId(rtpcId)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE

	virtual ~CParameterEnvironmentAdvanced() override = default;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	AkRtpcID const m_rtpcId;
	float const    m_multiplier;
	float const    m_shift;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
