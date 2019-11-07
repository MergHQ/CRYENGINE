// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
enum class EBiquadType
{
	None,
	Lowpass,
	Highpass,
	Bandpass,
	Notch,
	Peak,
	Lowshelf,
	Highshelf,
};

class BiquadIIFilter final
{
public:

	BiquadIIFilter() = delete;

	BiquadIIFilter(EBiquadType const filtertype, float const sampleRate)
		: m_sampleRate(sampleRate)
		, m_filterType(filtertype)
		, m_lastSample1(0.0f)
		, m_lastSample2(0.0f)
		, m_coefficientA0(0.0f)
		, m_coefficientA1(0.0f)
		, m_coefficientA2(0.0f)
		, m_coefficientB0(0.0f)
		, m_coefficientB1(0.0f)
	{}

	~BiquadIIFilter() = default;

	void     ComputeCoefficients(int const frequency, float const qualityFactor, float const peakGain);
	AkReal32 ProcessSample(AkReal32 const sample);

private:

	float       m_sampleRate;
	EBiquadType m_filterType;

	AkReal32    m_lastSample1;
	AkReal32    m_lastSample2;

	AkReal32    m_coefficientA0;
	AkReal32    m_coefficientA1;
	AkReal32    m_coefficientA2;
	AkReal32    m_coefficientB0;
	AkReal32    m_coefficientB1;
};
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio