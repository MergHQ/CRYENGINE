// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <IParameterConnection.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CParameter final : public IParameterConnection, public CPoolObject<CParameter, stl::PSyncNone>
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
		char const* const szName)
		: m_sampleId(sampleId)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}

	virtual ~CParameter() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	// ~IParameterConnection

	SampleId                                     GetSampleId() const   { return m_sampleId; }
	float                                        GetMultiplier() const { return m_multiplier; }
	float                                        GetShift() const      { return m_shift; }
	CryFixedStringT<MaxControlNameLength> const& GetName() const       { return m_name; }

private:

	SampleId const                              m_sampleId;
	float const                                 m_multiplier;
	float const                                 m_shift;
	CryFixedStringT<MaxControlNameLength> const m_name;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio