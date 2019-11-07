// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "fmod.hpp"
#include "BiquadIIFilterBank.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
namespace Plugins
{
extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

constexpr int g_maxDelay = 24;
constexpr int g_maxBufferSize = 2048;
constexpr float g_defaultFilterQuality = 0.7f;
constexpr float g_linearFadeStrength = 0.0f;

constexpr float g_smallFadeLength = 10.0f;
constexpr int g_smallFadeLengthInteger = 10; // fade length in samples

constexpr float g_mediumFadeLength = 200.0f;
constexpr int g_mediumFadeLengthInteger = 200; // fade length in samples

constexpr float g_largeFadeLength = 300.0f;
constexpr int g_largeFadeLengthInteger = 300; // fade length in samples

constexpr int g_parameterIndexPosition = 0;
constexpr int g_numParameters = 1;

enum class ESourceDirection
{
	None,
	Left,
	Right,
};

enum class EFadeType
{
	None,
	FadeinLogarithmic,
	FadeoutLogarithmic,
	FadeinHyperbole,
	FadeoutHyperbole,
};

class CrySpatialState final
{
public:

	CrySpatialState() = default;

	float ComputeFade(float const volumeFactor, float const strength, EFadeType const fadeType);
	float DecibelToVolume(float const decibel);
	float VolumeToDecibel(float const volume);
	void  GetPositionalData(float const x /*azimuth front*/, float const y /*elevation*/, float const z /*azimuth side*/, int& outQuadrant, float& outQuadrantFine, float& outElevation, float& outDistance);
	void  ConsumeInput(float* pInbuffer, float* pOutbuffer, unsigned int const length, int const channels);
	void  Reset();
	void  CreateFilters();
	void  DeleteFilters();
	void  ComputeDelayChannelData(int& outDelay);
	void  FilterBuffers(float* pInChannel, unsigned int const inputFrames, ESourceDirection const currentSourceDirection);
	void  DelayAndFadeBuffers(int const delayCurrent, int const maxFrames, ESourceDirection const currentSourceDirection);
	void  HRTFMonoToBinaural(float* pInBuffer, float* pOutBuffer, unsigned int const frameLength);

	int                             m_sampleRate;
	FMOD_DSP_PARAMETER_3DATTRIBUTES m_position;

private:

	// positioning
	int   m_quadrant = 0;
	float m_quadrantFine = 0.0f;
	float m_elevation = 0.0f;
	float m_distance = 0.0f;

	// buffers
	float m_delayBuffer[g_maxDelay];
	float m_inputBuffer[g_maxBufferSize];
	float m_directChannelBuffer[g_maxBufferSize];
	float m_concealedChannelBuffer[g_maxBufferSize];
	float m_concealedChannelBufferIntermediate[g_maxBufferSize];

	// userData
	ESourceDirection     m_lastSourceDirection = ESourceDirection::None;
	int                  m_delayPrev = 0;
	int                  m_currentDelay = 0;
	int                  m_currentCycle = 0;
	int                  m_numInputChannels = 0;
	float                m_bufferFadeStrength = 1.0f;

	SBiquadIIFilterBank* m_pFilterBankA;
	SBiquadIIFilterBank* m_pFilterBankB;

};

// Fmod Callbacks
FMOD_RESULT F_CALLBACK CrySpatialCreate(FMOD_DSP_STATE* pDspState);
FMOD_RESULT F_CALLBACK CrySpatialRelease(FMOD_DSP_STATE* pDspState);
FMOD_RESULT F_CALLBACK CrySpatialReset(FMOD_DSP_STATE* pDspState);
FMOD_RESULT F_CALLBACK CrySpatialProcess(FMOD_DSP_STATE* pDspState, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* pInBufferArray, FMOD_DSP_BUFFER_ARRAY* pOutBufferArray, FMOD_BOOL isInputIdle, FMOD_DSP_PROCESS_OPERATION op);
FMOD_RESULT F_CALLBACK CrySpatialSetParamData(FMOD_DSP_STATE* pDspState, int index, void* pData, unsigned int length);
FMOD_RESULT F_CALLBACK CrySpatialShouldIProcess(FMOD_DSP_STATE* pDspState, FMOD_BOOL isInputIdle, unsigned int length, FMOD_CHANNELMASK inChannelMask, int numInChannels, FMOD_SPEAKERMODE speakerMode);
FMOD_RESULT F_CALLBACK CrySpatialSysRegister(FMOD_DSP_STATE* pDspState);
FMOD_RESULT F_CALLBACK CrySpatialSysDeregister(FMOD_DSP_STATE* pDspState);
FMOD_RESULT F_CALLBACK CrySpatialSysMix(FMOD_DSP_STATE* pDspState, int stage);

static bool s_isPluginRunning = false;
static FMOD_DSP_PARAMETER_DESC s_paramEmitterPosition;

FMOD_DSP_PARAMETER_DESC* m_pParamDescription[g_numParameters] =
{
	&s_paramEmitterPosition };

FMOD_DSP_DESCRIPTION m_pluginDescription =
{
	FMOD_PLUGIN_SDK_VERSION,
	"CrySpatialFmod", // name
	0x00010000,       // plug-in version
	1,                // number of input buffers to process
	1,                // number of output buffers to process
	CrySpatialCreate,
	CrySpatialRelease,
	CrySpatialReset,
	nullptr,  // dsp read
	CrySpatialProcess,
	nullptr,  // CrySpatial_dspsetposition
	g_numParameters,
	m_pParamDescription,
	nullptr, // CrySpatial_dspsetparamfloat,
	nullptr, // CrySpatial_dspsetparamint,
	nullptr, // CrySpatial_dspsetparambool,
	CrySpatialSetParamData,
	nullptr, // CrySpatial_dspgetparamfloat,
	nullptr, // CrySpatial_dspgetparamint,
	nullptr, // CrySpatial_dspgetparambool,
	nullptr, // CrySpatial_dspgetparamdata,
	CrySpatialShouldIProcess,
	nullptr,  // userdata
	CrySpatialSysRegister,
	CrySpatialSysDeregister,
	CrySpatialSysMix };

extern "C"
{
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription()
	{
		FMOD_DSP_INIT_PARAMDESC_DATA(s_paramEmitterPosition, "emitterPosition", "", "", -2)
		return &m_pluginDescription;
	}
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialCreate(FMOD_DSP_STATE* pDspState)
{
	pDspState->plugindata = static_cast<CrySpatialState*>(FMOD_DSP_ALLOC(pDspState, sizeof(CrySpatialState)));

	if (!pDspState->plugindata)
	{
		return FMOD_ERR_MEMORY;
	}

	CrySpatialState* const pState = static_cast<CrySpatialState*>(pDspState->plugindata);
	pDspState->functions->getsamplerate(pDspState, &pState->m_sampleRate);

	pState->CreateFilters();
	pState->Reset();

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialRelease(FMOD_DSP_STATE* pDspState)
{
	CrySpatialState* const pState = static_cast<CrySpatialState*>(pDspState->plugindata);
	pState->DeleteFilters();
	FMOD_DSP_FREE(pDspState, pState);
	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialProcess(FMOD_DSP_STATE* pDspState, unsigned int length, const FMOD_DSP_BUFFER_ARRAY* pInBufferArray, FMOD_DSP_BUFFER_ARRAY* pOutBufferArray, FMOD_BOOL isInputIdle, FMOD_DSP_PROCESS_OPERATION op)
{
	CrySpatialState* const pState = static_cast<CrySpatialState*>(pDspState->plugindata);

	if (op == FMOD_DSP_PROCESS_QUERY)
	{
		if (pOutBufferArray && pInBufferArray)
		{
			pOutBufferArray[0].bufferchannelmask[0] = FMOD_CHANNELMASK_STEREO;
			pOutBufferArray[0].buffernumchannels[0] = 2;
			pOutBufferArray[0].speakermode = FMOD_SPEAKERMODE_RAW;
		}

		if (isInputIdle)
		{
			return FMOD_ERR_DSP_DONTPROCESS;
		}
	}
	else
	{
		pState->ConsumeInput(pInBufferArray[0].buffers[0], pOutBufferArray[0].buffers[0], length, pInBufferArray[0].buffernumchannels[0]);
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialReset(FMOD_DSP_STATE* pDspState)
{
	CrySpatialState* const pState = static_cast<CrySpatialState*>(pDspState->plugindata);
	pState->Reset();
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK CrySpatialShouldIProcess(FMOD_DSP_STATE* /*pDspState*/, FMOD_BOOL isInputIdle, unsigned int /*length*/, FMOD_CHANNELMASK /*inmask*/, int /*inchannels*/, FMOD_SPEAKERMODE /*speakermode*/)
{
	if (isInputIdle)
	{
		return FMOD_ERR_DSP_DONTPROCESS;
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialSetParamData(FMOD_DSP_STATE* pDspState, int index, void* pData, unsigned int length)
{
	CrySpatialState* const pState = static_cast<CrySpatialState*>(pDspState->plugindata);

	switch (index)
	{
	case g_parameterIndexPosition:
		{
			pState->m_position = *static_cast<FMOD_DSP_PARAMETER_3DATTRIBUTES*>(pData); // update relative emitter position
			return FMOD_OK;
		}
	default:
		{
			break;
		}
	}

	return FMOD_ERR_INVALID_PARAM;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialSysRegister(FMOD_DSP_STATE* /*pDspState*/)
{
	s_isPluginRunning = true;
	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialSysDeregister(FMOD_DSP_STATE* /*pDspState*/)
{
	s_isPluginRunning = false;
	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK CrySpatialSysMix(FMOD_DSP_STATE* /*pDspState*/, int /*stage*/)
{
	return FMOD_OK;
}
} // namespace Plugins
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio