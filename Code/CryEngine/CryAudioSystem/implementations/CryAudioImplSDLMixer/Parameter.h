// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CParameter final : public IParameter, public CPoolObject<CParameter, stl::PSyncNone>
{
public:

	CParameter() = delete;
	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	explicit CParameter(
		SampleId const sampleId,
		float const multiplier,
		float const shift,
		char const* const szName);

	virtual ~CParameter() override = default;

	SampleId                                               GetSampleId() const   { return m_sampleId; }
	float                                                  GetMultiplier() const { return m_multiplier; }
	float                                                  GetShift() const      { return m_shift; }
	CryFixedStringT<CryAudio::MaxControlNameLength> const& GetName() const       { return m_name; }

private:

	SampleId const m_sampleId;
	float const    m_multiplier;
	float const    m_shift;
	CryFixedStringT<CryAudio::MaxControlNameLength> const m_name;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio