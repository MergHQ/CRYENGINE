// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CrySpatialFXParams.h"
#include "CrySpatialFXAttachmentParams.h"
#include "AkUserData.h"
#include <AK/SoundEngine/Common/IAkMixerPlugin.h>
#include <vector>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
constexpr int g_maxDelay = 24;
constexpr float g_defaultFilterQuality = 0.7f;
constexpr float g_linearFadeStrength = 0.0f;

constexpr float g_smallFadeLength = 10.0f;
constexpr int g_smallFadeLengthInteger = 10; // fade length in samples

constexpr float g_mediumFadeLength = 200.0f;
constexpr int g_mediumFadeLengthInteger = 200; // fade length in samples

constexpr float g_largeFadeLength = 300.0f;
constexpr int g_largeFadeLengthInteger = 300; // fade length in samples

enum class EFadeType
{
	None,
	FadeinLogarithmic,
	FadeoutLogarithmic,
	FadeinHyperbole,
	FadeoutHyperbole,
};

class CrySpatialFX final : public AK::IAkMixerEffectPlugin
{
public:

	CrySpatialFX() = default;
	~CrySpatialFX() = default;

	// Plug-in initialization.
	// Prepares the plug-in for data processing, allocates memory and sets up the initial conditions.
	AKRESULT Init(
		AK::IAkPluginMemAlloc* pAllocator,
		AK::IAkMixerPluginContext* pContext,
		AK::IAkPluginParam* pParams,
		AkAudioFormat& audioFormat) override;

	// Free all individual DelayBuffers in case OnInputDisconnected() wasn't called.
	// m_pMemAlloc is a proxy for DelayBuffer creation so it doesn't need individual cleanup.
	AKRESULT Term(AK::IAkPluginMemAlloc* pAllocator) override;

	// The reset action should perform any actions required to reinitialize the
	// state of the plug-in to its original state (e.g. after Init() or on effect bypass).
	AKRESULT Reset() override;

	// Plug-in information query mechanism used when the sound engine requires
	// information about the plug-in to determine its behavior.
	AKRESULT GetPluginInfo(AkPluginInfo& outPluginInfo) override;

	// This function is called whenever a new input is connected to the underlying mixing bus.
	void OnInputConnected(AK::IAkMixerInputContext* pInput) override;

	// Delete pDelayBuffer and remove from m_lActiveDelayBuffers
	void OnInputDisconnected(AK::IAkMixerInputContext* pInput) override;

	// This function is called whenever an input (voice or bus) needs to be mixed.
	// During an audio frame, ConsumeInput() will be called for each input that need to be mixed.
	void ConsumeInput(
		AK::IAkMixerInputContext* pInputContext,
		AkRamp baseVolume,
		AkRamp emitterListenerVolume,
		AkAudioBuffer* pInputBuffer,
		AkAudioBuffer* pMixBuffer) override;

	// This function is called once every audio frame, when all inputs have been mixed in
	// with ConsumeInput(). It is the time when the plugin may perform final DSP/bookkeeping.
	// After the bus buffer io_pMixBuffer has been pushed to the bus downstream
	// (or the output device), it is cleared out for the next frame.
	void OnMixDone(AkAudioBuffer* pMixBuffer) override;

	// This function is called once every audio frame, after all insert effects
	// on the bus have been processed, which occur after the post mix pass of OnMixDone().
	// After the bus buffer io_pMixBuffer has been pushed to the bus downstream
	// (or the output device), it is cleared out for the next frame.
	void OnEffectsProcessed(AkAudioBuffer* pMixBuffer) override;

	// This function is called once every audio frame, after all insert effects
	// on the bus have been processed, and after metering has been processed
	// (if applicable), which occur after OnEffectsProcessed().
	// After the bus buffer io_pMixBuffer has been pushed to the bus downstream
	// (or the output device), it is cleared out for the next frame.
	// Mixer plugins may use this hook for processing the signal after it has been metered.
	void OnFrameEnd(AkAudioBuffer* pMixBuffer, AK::IAkMetering* pMetering) override;

	// Consumes a mono input buffer that has ListenerRelativeRouting and spatializes it.
	void HrtfMonoToBinaural(
		AK::IAkMixerInputContext* const pInputContext,
		AkRamp const baseVolume,
		AkRamp const busVolume,
		AkAudioBuffer* const pInputBuffer,
		AkAudioBuffer* const pMixBuffer);

	// Consumes a mono input buffer that has ListenerRelativeRouting and pans it to surround.
	void HrtfMonoTo5_1(
		AK::IAkMixerInputContext* const pInputContext,
		AkRamp const baseVolume,
		AkRamp const busVolume,
		AkAudioBuffer* pInputBuffer,
		AkAudioBuffer* pMixBuffer);

	// Consumes a mono input buffer that has ListenerRelativeRouting and pans it to surround.
	void HrtfMonoTo7_1(
		AK::IAkMixerInputContext* const pInputContext,
		AkRamp const baseVolume,
		AkRamp const busVolume,
		AkAudioBuffer* pInputBuffer,
		AkAudioBuffer* pMixBuffer);

	// Consumes inputBuffers of Buses and Voices that are not ListenerRelative.
	void NonHrtfStereoToStereo(
		AK::IAkMixerInputContext* pInputContext,
		AkRamp const baseVolume,
		AkRamp const busVolume,
		AkAudioBuffer* pInputBuffer,
		AkAudioBuffer* pMixBuffer);

private:

	float ComputeFade(float const volumeFactor, float const strength, EFadeType const fadeType);
	float DecibelToVolume(float const decibels);
	float VolumeToDecibel(float const volume);
	void  ComputeQuadrant(AkReal32 const gameAzimuth, int& outQuadrant, float& outQuadrantFine);
	void  ComputeDelayChannelData(AkReal32 const gameElevation, int const quadrant, float const quadrantFine, int& outDelay);

	void  FillDelayBuffer(
		AkSampleType* pInChannelDelayBuffer,
		AkSampleType* const pOutChannelDelayBuffer,
		int const delayCurrent);

	void FilterBuffers(
		AK::IAkMixerInputContext* const pInputContext,
		AkAudioBuffer* const pInputBuffer,
		AkSampleType* pIntermediateBufferConcealed,
		ESourceDirection const sourceDirection);

	void DelayAndFadeBuffers(
		AkSampleType* pInChannelConcealed,
		AkSampleType* pOutChannelConcealed,
		AkSampleType* pChannelDirect,
		AkSampleType* pDelBuffer,
		SAkUserData* const userData,
		int const delayCurrent,
		int const maxFrames,
		ESourceDirection const sourceDirection);

private:

	AkUInt16                   m_maxDelay = g_maxDelay;
	float                      m_bufferFadeStrength = 1.0f;
	float                      m_EQFadeStrength = g_linearFadeStrength;

	float                      m_sampleRate = 0.0f;
	int                        m_numOutputChannels = 0;
	AkUInt32                   m_bufferSize = 0;
	AkUInt16                   m_delayBufferSize = g_maxDelay;

	CrySpatialFXParams*        m_pParams = nullptr;
	AK::IAkPluginMemAlloc*     m_pAllocator = nullptr;
	AK::IAkMixerPluginContext* m_pContext = nullptr;
	AkSampleType*              m_pMemAlloc = nullptr;
	AkSampleType*              m_pBufferConcealed = nullptr;
	AkSampleType*              m_pIntermediateBufferConcealed = nullptr;

	std::vector<SAkUserData*>  m_activeUserData;
};
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
