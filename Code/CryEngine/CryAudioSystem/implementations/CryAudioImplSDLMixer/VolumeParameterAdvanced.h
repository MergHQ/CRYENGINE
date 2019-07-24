// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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
class CVolumeParameterAdvanced final : public IParameterConnection, public CPoolObject<CVolumeParameterAdvanced, stl::PSyncNone>
{
public:

	CVolumeParameterAdvanced() = delete;
	CVolumeParameterAdvanced(CVolumeParameterAdvanced const&) = delete;
	CVolumeParameterAdvanced(CVolumeParameterAdvanced&&) = delete;
	CVolumeParameterAdvanced& operator=(CVolumeParameterAdvanced const&) = delete;
	CVolumeParameterAdvanced& operator=(CVolumeParameterAdvanced&&) = delete;

#if defined(CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE)
	explicit CVolumeParameterAdvanced(
		SampleId const sampleId,
		float const multiplier,
		float const shift,
		char const* const szName)
		: m_sampleId(sampleId)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}
#else
	explicit CVolumeParameterAdvanced(
		SampleId const sampleId,
		float const multiplier,
		float const shift)
		: m_sampleId(sampleId)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}
#endif  // CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE

	virtual ~CVolumeParameterAdvanced() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection

private:

	SampleId const m_sampleId;
	float const    m_multiplier;
	float const    m_shift;

#if defined(CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE)
	CryFixedStringT<MaxControlNameLength> const m_name;
#endif  // CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio