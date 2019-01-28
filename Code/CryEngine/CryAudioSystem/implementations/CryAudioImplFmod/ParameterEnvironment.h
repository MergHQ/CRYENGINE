// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEnvironmentConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CParameterEnvironment final : public IEnvironmentConnection, public CPoolObject<CParameterEnvironment, stl::PSyncNone>
{
public:

	CParameterEnvironment() = delete;
	CParameterEnvironment(CParameterEnvironment const&) = delete;
	CParameterEnvironment(CParameterEnvironment&&) = delete;
	CParameterEnvironment& operator=(CParameterEnvironment const&) = delete;
	CParameterEnvironment& operator=(CParameterEnvironment&&) = delete;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	explicit CParameterEnvironment(
		uint32 const id,
		float const multiplier,
		float const shift,
		char const* const szName)
		: m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}
#else
	explicit CParameterEnvironment(
		uint32 const id,
		float const multiplier,
		float const shift)
		: m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	virtual ~CParameterEnvironment() override;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	uint32 const m_id;
	float const  m_multiplier;
	float const  m_shift;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
