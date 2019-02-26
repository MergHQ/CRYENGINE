// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISwitchStateConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CParameterState final : public ISwitchStateConnection, public CPoolObject<CParameterState, stl::PSyncNone>
{
public:

	CParameterState() = delete;
	CParameterState(CParameterState const&) = delete;
	CParameterState(CParameterState&&) = delete;
	CParameterState& operator=(CParameterState const&) = delete;
	CParameterState& operator=(CParameterState&&) = delete;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	explicit CParameterState(
		uint32 const id,
		float const value,
		char const* const szName)
		: m_id(id)
		, m_value(value)
		, m_name(szName)
	{}
#else
	explicit CParameterState(
		uint32 const id,
		float const value)
		: m_id(id)
		, m_value(value)
	{}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

	virtual ~CParameterState() override;

	// ISwitchStateConnection
	virtual void Set(IObject* const pIObject) override;
	virtual void SetGlobally() override;
	// ~ISwitchStateConnection

private:

	uint32 const m_id;
	float const  m_value;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
