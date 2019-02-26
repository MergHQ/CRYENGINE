// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CParameter final : public IParameterConnection, public CPoolObject<CParameter, stl::PSyncNone>
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	explicit CParameter(
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
	explicit CParameter(
		uint32 const id,
		float const multiplier,
		float const shift)
		: m_id(id)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

	virtual ~CParameter() override;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection

private:

	uint32 const m_id;
	float const  m_multiplier;
	float const  m_shift;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
