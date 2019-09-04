// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Fmod
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

	void  ComputeCoefficients(int const frequency, float const qualityFactor, float const peakGain);
	float ProcessSample(float const sample);

private:

	float const       m_sampleRate;
	EBiquadType const m_filterType;

	float             m_lastSample1;
	float             m_lastSample2;

	float             m_coefficientA0;
	float             m_coefficientA1;
	float             m_coefficientA2;
	float             m_coefficientB0;
	float             m_coefficientB1;
};
} // namespace Plugins
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio