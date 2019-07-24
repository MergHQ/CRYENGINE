// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CParameterAdvanced final : public IParameterConnection, public CPoolObject<CParameterAdvanced, stl::PSyncNone>
{
public:

	CParameterAdvanced() = delete;
	CParameterAdvanced(CParameterAdvanced const&) = delete;
	CParameterAdvanced(CParameterAdvanced&&) = delete;
	CParameterAdvanced& operator=(CParameterAdvanced const&) = delete;
	CParameterAdvanced& operator=(CParameterAdvanced&&) = delete;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	explicit CParameterAdvanced(
		AkRtpcID const id,
		float const mult,
		float const shift,
		char const* const szName)
		: m_multiplier(mult)
		, m_shift(shift)
		, m_id(id)
		, m_name(szName)
	{}
#else
	explicit CParameterAdvanced(
		AkRtpcID const id,
		float const mult,
		float const shift)
		: m_multiplier(mult)
		, m_shift(shift)
		, m_id(id)
	{}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE

	virtual ~CParameterAdvanced() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection

private:

	float const    m_multiplier;
	float const    m_shift;
	AkRtpcID const m_id;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_DEBUG_CODE
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
