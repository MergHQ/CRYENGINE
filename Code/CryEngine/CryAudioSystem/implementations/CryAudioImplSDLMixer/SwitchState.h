// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ISwitchStateConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CSwitchState final : public ISwitchStateConnection, public CPoolObject<CSwitchState, stl::PSyncNone>
{
public:

	CSwitchState() = delete;
	CSwitchState(CSwitchState const&) = delete;
	CSwitchState(CSwitchState&&) = delete;
	CSwitchState& operator=(CSwitchState const&) = delete;
	CSwitchState& operator=(CSwitchState&&) = delete;

	explicit CSwitchState(
		SampleId const sampleId,
		float const value,
		char const* const szName)
		: m_sampleId(sampleId)
		, m_value(value)
		, m_name(szName)
	{}

	virtual ~CSwitchState() override = default;

	SampleId                                     GetSampleId() const { return m_sampleId; }
	float                                        GetValue() const    { return m_value; }
	CryFixedStringT<MaxControlNameLength> const& GetName() const     { return m_name; }

private:

	SampleId const                              m_sampleId;
	float const                                 m_value;
	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio