// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include "BiquadIIFilter.h"
#include "CrySpatialFX.h"
#include "CrySpatialConfig.h"
#include "CrySpatialMath.h"
#include "AkUserData.h"
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <memory>
#include <algorithm>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
AK_IMPLEMENT_PLUGIN_FACTORY(CrySpatialFX, AkPluginTypeMixer, CrySpatialConfig::g_companyID, CrySpatialConfig::g_pluginID)

//////////////////////////////////////////////////////////////////////////
AK::IAkPlugin* CreateCrySpatialFX(AK::IAkPluginMemAlloc* pAllocator)
{
	return AK_PLUGIN_NEW(pAllocator, CrySpatialFX());
}

//////////////////////////////////////////////////////////////////////////
AK::IAkPluginParam* CreateCrySpatialFXParams(AK::IAkPluginMemAlloc* pAllocator)
{
	return AK_PLUGIN_NEW(pAllocator, CrySpatialFXParams());
}

//////////////////////////////////////////////////////////////////////////
AK::IAkPluginParam* CreateCrySpatialFXAttachmentParams(AK::IAkPluginMemAlloc* pAllocator)
{
	return AK_PLUGIN_NEW(pAllocator, CrySpatialFXAttachmentParams());
}

//////////////////////////////////////////////////////////////////////////
AK::PluginRegistration CrySpatialFXAttachmentParamsRegistration(
	AkPluginTypeEffect,
	CrySpatialAttachmentConfig::g_companyID,
	CrySpatialAttachmentConfig::g_pluginID,
	nullptr,
	CreateCrySpatialFXAttachmentParams);

//////////////////////////////////////////////////////////////////////////
float CrySpatialFX::DecibelToVolume(float const decibel)
{
	return powf(10.0f, 0.05f * decibel);
}

//////////////////////////////////////////////////////////////////////////
float CrySpatialFX::VolumeToDecibel(float const volume)
{
	return 20.0f * log10f(volume);
}

//////////////////////////////////////////////////////////////////////////
float CrySpatialFX::ComputeFade(float const volumeFactor, float const strength = 1.5f, EFadeType const fadeType = EFadeType::FadeinHyperbole)
{
	float returnVolume = 0.0f;
	switch (fadeType)
	{
	case EFadeType::FadeinHyperbole:
		{
			returnVolume = (powf(g_euler, (strength * (volumeFactor - 1.0f))) * volumeFactor);
			break;
		}
	case EFadeType::FadeoutHyperbole:
		{
			returnVolume = (powf(g_euler, (strength * (-volumeFactor))) * (-volumeFactor + 1.0f));
			break;
		}
	case EFadeType::FadeinLogarithmic:
		{
			returnVolume = (powf(g_euler, (strength * (-volumeFactor))) * (volumeFactor - 1.0f)) + 1.0f;
			break;
		}
	case EFadeType::FadeoutLogarithmic:
		{
			returnVolume = (powf(g_euler, (strength * (volumeFactor - 1.0f))) * (-volumeFactor)) + 1.0f;
			break;
		}
	default:    // EFadeType::FadeinHyperbole
		{
			returnVolume = (powf(g_euler, (strength * (volumeFactor - 1.0f))) * volumeFactor);
			break;
		}
	}

	return returnVolume;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::ComputeQuadrant(AkReal32 const gameAzimuth, int& outQuadrant, float& outQuadrantFine)
{
	float quadrant = 0.0f;
	auto quadrantFine = static_cast<float>(fabs(gameAzimuth / static_cast<AkReal32>(g_piHalf)));
	quadrantFine = modff(quadrantFine, &quadrant);

	if (gameAzimuth < 0.0f)
	{
		quadrant = (quadrant == 0.0f) ? 3.0f : 2.0f;
	}

	outQuadrant = static_cast<int>(quadrant);
	outQuadrantFine = quadrantFine;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFX::Init(
	AK::IAkPluginMemAlloc* pAllocator,
	AK::IAkMixerPluginContext* pContext,
	AK::IAkPluginParam* pParams,
	AkAudioFormat& audioFormat)
{
	m_pParams = (CrySpatialFXParams*)pParams;
	m_pAllocator = pAllocator;
	m_pContext = pContext;
	m_numOutputChannels = audioFormat.GetNumChannels();
	m_sampleRate = static_cast<float>(pContext->GlobalContext()->GetSampleRate());

	AkAudioSettings settings;
	pContext->GlobalContext()->GetAudioSettings(settings);
	m_bufferSize = settings.uNumSamplesPerFrame;

	m_pBufferConcealed = static_cast<AkSampleType*>(AK_PLUGIN_ALLOC(m_pAllocator, sizeof(AkSampleType) * m_bufferSize));
	AKPLATFORM::AkMemSet(m_pBufferConcealed, 0, sizeof(AkSampleType) * m_bufferSize);

	m_pIntermediateBufferConcealed = static_cast<AkSampleType*>(AK_PLUGIN_ALLOC(m_pAllocator, sizeof(AkSampleType) * m_bufferSize));
	AKPLATFORM::AkMemSet(m_pIntermediateBufferConcealed, 0, sizeof(AkSampleType) * m_bufferSize);

	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFX::Term(AK::IAkPluginMemAlloc* pAllocator)
{
	AK_PLUGIN_FREE(m_pAllocator, m_pBufferConcealed);
	AK_PLUGIN_FREE(m_pAllocator, m_pIntermediateBufferConcealed);

	for (SAkUserData* const pUserData : m_activeUserData)
	{
		AK_PLUGIN_FREE(m_pAllocator, pUserData->pVoiceDelayBuffer->GetChannel(0));
		delete pUserData;
	}

	m_activeUserData.clear();

	AK_PLUGIN_DELETE(pAllocator, this);

	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFX::Reset()
{
	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////
AKRESULT CrySpatialFX::GetPluginInfo(AkPluginInfo& outPluginInfo)
{
	outPluginInfo.eType = AkPluginTypeMixer;
	outPluginInfo.bIsInPlace = true;
	outPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;
	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::OnInputConnected(AK::IAkMixerInputContext* pInput)
{
	SAkUserData* pUserData = new SAkUserData();

	// DelayBuffer Allocation
	AkChannelConfig channelConfiguration;
	channelConfiguration.SetStandard(AK_SPEAKER_SETUP_MONO);

	{
		std::unique_ptr<AkAudioBuffer> pDelayBuffer = std::make_unique<AkAudioBuffer>();
		m_pMemAlloc = (AkSampleType*)(AK_PLUGIN_ALLOC(m_pAllocator, sizeof(AkSampleType) * m_delayBufferSize));
		pDelayBuffer->AttachContiguousDeinterleavedData(m_pMemAlloc, m_delayBufferSize, 0, channelConfiguration);
		pDelayBuffer->ZeroPadToMaxFrames();
		pDelayBuffer->uValidFrames = pDelayBuffer->MaxFrames();
		pUserData->pVoiceDelayBuffer = std::move(pDelayBuffer);
	}

	// Setup FilterBanks
	{
		float const sampleRate = m_sampleRate;
		pUserData->pFilterBankA = new SBiquadIIFilterBank(sampleRate);
		pUserData->pFilterBankB = new SBiquadIIFilterBank(sampleRate);
	}

	AkEmitterListenerPair emitterListener;

	if (pInput->HasListenerRelativeRouting())
	{
		// Set Azimuth and Elevation
		pInput->Get3DPosition(0, emitterListener);
		m_pContext->GlobalContext()->ComputeSphericalCoordinates(emitterListener, pUserData->gameAzimuth, pUserData->gameElevation);

		pUserData->lastSourceDirection = (pUserData->gameAzimuth > 0) ? ESourceDirection::Right : ESourceDirection::Left;
		pUserData->inputType = EVoiceType::IsSpatialized;
	}
	else
	{
		pUserData->inputType = EVoiceType::NotSpatialized;
	}

	pInput->SetUserData(pUserData);
	m_activeUserData.push_back(pUserData);
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::OnInputDisconnected(AK::IAkMixerInputContext* pInput)
{
	SAkUserData* const pUserData = static_cast<SAkUserData*>(pInput->GetUserData());
	AK_PLUGIN_FREE(m_pAllocator, pUserData->pVoiceDelayBuffer->GetChannel(0));
	m_activeUserData.erase(
		std::remove_if(m_activeUserData.begin(), m_activeUserData.end(), [pUserData](SAkUserData* const element) { return element == pUserData; }),
		m_activeUserData.end());

	delete pUserData;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::ConsumeInput(
	AK::IAkMixerInputContext* pInputContext,
	AkRamp baseVolume,
	AkRamp emitterListenerVolume,
	AkAudioBuffer* pInputBuffer,
	AkAudioBuffer* pMixBuffer)
{
	SAkUserData const* const pUserData = static_cast<SAkUserData const*>(pInputContext->GetUserData());

	switch (pUserData->inputType)
	{
	case EVoiceType::IsSpatialized:
		{
			switch (m_numOutputChannels)
			{
			case 2:
				{
					HrtfMonoToBinaural(
						pInputContext,
						baseVolume,
						emitterListenerVolume,
						pInputBuffer,
						pMixBuffer);

					break;
				}
			case 6:
				{
					HrtfMonoTo5_1(
						pInputContext,
						baseVolume,
						emitterListenerVolume,
						pInputBuffer,
						pMixBuffer);

					break;
				}
			case 8:
				{
					HrtfMonoTo7_1(
						pInputContext,
						baseVolume,
						emitterListenerVolume,
						pInputBuffer,
						pMixBuffer);

					break;
				}
			default:
				{
					break;
				}
			}

			break;
		}
	case EVoiceType::NotSpatialized:
		{
			NonHrtfStereoToStereo(
				pInputContext,
				baseVolume,
				emitterListenerVolume,
				pInputBuffer,
				pMixBuffer);

			break;
		}
	default:
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::OnMixDone(AkAudioBuffer* pMixBuffer)
{
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::OnEffectsProcessed(AkAudioBuffer* pMixBuffer)
{
	// Execute DSP after insert effects have been processed here
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::OnFrameEnd(AkAudioBuffer* pMixBuffer, AK::IAkMetering* pMetering)
{
	// Execute DSP after metering has been processed here
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::FillDelayBuffer(AkSampleType* pInChannelDelayBuffer, AkSampleType* const pOutChannelDelayBuffer, int const delayCurrent)
{
	if (delayCurrent > 0)
	{
		int const offset = m_bufferSize - delayCurrent;
		pInChannelDelayBuffer += offset;

		memcpy(pOutChannelDelayBuffer, pInChannelDelayBuffer, sizeof(AkSampleType) * delayCurrent);
	}
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::HrtfMonoToBinaural(
	AK::IAkMixerInputContext* const pInputContext,
	AkRamp const baseVolume,
	AkRamp const busVolume,
	AkAudioBuffer* pInputBuffer,
	AkAudioBuffer* pMixBuffer)
{
	pInputBuffer->ZeroPadToMaxFrames();

	SAkUserData* const pUserData = static_cast<SAkUserData*>(pInputContext->GetUserData());
	AkReal32& gameAzimuth = pUserData->gameAzimuth;
	AkReal32& gameElevation = pUserData->gameElevation;

	int const inputValidFrames = pInputBuffer->uValidFrames;
	int const cycleProxy = pUserData->voiceCycle;

	// Set Azimuth and Elevation
	AkEmitterListenerPair emitterListener;
	pInputContext->Get3DPosition(0, emitterListener);
	m_pContext->GlobalContext()->ComputeSphericalCoordinates(emitterListener, gameAzimuth, gameElevation);

	// Clamp Azimuth and Elevation in case data from game is faulty
	if (gameAzimuth > static_cast<AkReal32>(g_pi))
	{
		gameAzimuth = static_cast<AkReal32>(g_pi);
	}

	if (gameAzimuth < static_cast<AkReal32>(-g_pi))
	{
		gameAzimuth = static_cast<AkReal32>(-g_pi);
	}

	if (gameElevation > static_cast<AkReal32>(g_piHalf))
	{
		gameElevation = static_cast<AkReal32>(g_piHalf);
	}

	if (gameElevation < static_cast<AkReal32>(-g_piHalf))
	{
		gameElevation = static_cast<AkReal32>(-g_piHalf);
	}

	ComputeQuadrant(gameAzimuth, pUserData->quadrant, pUserData->quadrantFine);

	ESourceDirection const sourceDirection = (gameAzimuth > 0) ? ESourceDirection::Right : ESourceDirection::Left;

	FilterBuffers(pInputContext, pInputBuffer, m_pIntermediateBufferConcealed, sourceDirection);

	pUserData->voiceCycle = (pUserData->voiceCycle == 0) ? 2 : (pUserData->voiceCycle == 1) ? 2 : 1;

	// Compute Delay, Volume difference & EQ Gain for delayed Channel
	int delayCurrent = 0;
	ComputeDelayChannelData(gameElevation, pUserData->quadrant, pUserData->quadrantFine, delayCurrent);

	// Fade strength to be relative to correlation through delay
	m_bufferFadeStrength = static_cast<float>(abs(pUserData->voiceDelayPrev - delayCurrent)) / static_cast<float>(m_maxDelay);

	AkSampleType* pDelChannelDirect = pUserData->pVoiceDelayBuffer->GetChannel(0);

	DelayAndFadeBuffers(
		m_pIntermediateBufferConcealed,
		m_pBufferConcealed,
		pInputBuffer->GetChannel(0),
		pDelChannelDirect,
		pUserData,
		delayCurrent,
		m_bufferSize,
		sourceDirection);

	// Calculate Volume for Left and Right Channel
	AkRamp const loudnessMatch(DecibelToVolume(-3.0f), DecibelToVolume(-3.0f));
	AkRamp const localbaseVolume = baseVolume * busVolume * loudnessMatch;

	// Mix into OutBuffer
	int channelNoDelay;
	int channelDelay;

	switch (sourceDirection)
	{
	case ESourceDirection::Right:
		{
			channelNoDelay = 1;
			channelDelay = 0;
			break;
		}
	case ESourceDirection::Left:
		{
			channelNoDelay = 0;
			channelDelay = 1;
			break;
		}
	default:
		{
			break;
		}
	}

	m_pContext->GlobalContext()->MixChannel(
		pInputBuffer->GetChannel(0),
		pMixBuffer->GetChannel(channelNoDelay),
		localbaseVolume.fPrev,
		localbaseVolume.fNext,
		inputValidFrames);

	m_pContext->GlobalContext()->MixChannel(
		m_pBufferConcealed,
		pMixBuffer->GetChannel(channelDelay),
		localbaseVolume.fPrev,
		localbaseVolume.fNext,
		inputValidFrames);

	pMixBuffer->uValidFrames = inputValidFrames;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::HrtfMonoTo5_1(
	AK::IAkMixerInputContext* const pInputContext,
	AkRamp const baseVolume,
	AkRamp const busVolume,
	AkAudioBuffer* pInputBuffer,
	AkAudioBuffer* pMixBuffer)
{
	SAkUserData* const pUserData = static_cast<SAkUserData*>(pInputContext->GetUserData());
	AkReal32& gameAzimuth = pUserData->gameAzimuth;
	AkReal32& gameElevation = pUserData->gameElevation;

	pInputBuffer->ZeroPadToMaxFrames();

	AkRamp const localbaseVolume = baseVolume * busVolume;
	AkUInt32 const allocSize = AK::SpeakerVolumes::Matrix::GetRequiredSize(pInputBuffer->NumChannels(), pMixBuffer->NumChannels());
	AK::SpeakerVolumes::MatrixPtr const volumesPrev = (AK::SpeakerVolumes::MatrixPtr)AkAlloca(allocSize);
	AK::SpeakerVolumes::MatrixPtr const volumesNext = (AK::SpeakerVolumes::MatrixPtr)AkAlloca(allocSize);
	pInputContext->GetSpatializedVolumes(volumesPrev, volumesNext);

	// Set Azimuth and Elevation
	AkEmitterListenerPair emitterListener;
	pInputContext->Get3DPosition(0, emitterListener);
	m_pContext->GlobalContext()->ComputeSphericalCoordinates(emitterListener, gameAzimuth, gameElevation);

	// Clamp azimuth and elevation in case data from game is faulty
	if (gameAzimuth > static_cast<AkReal32>(g_pi))
	{
		gameAzimuth = static_cast<AkReal32>(g_pi);
	}

	if (gameAzimuth < static_cast<AkReal32>(-g_pi))
	{
		gameAzimuth = static_cast<AkReal32>(-g_pi);
	}

	if (gameElevation > static_cast<AkReal32>(g_piHalf))
	{
		gameElevation = static_cast<AkReal32>(g_piHalf);
	}

	if (gameElevation < static_cast<AkReal32>(-g_piHalf))
	{
		gameElevation = static_cast<AkReal32>(-g_piHalf);
	}

	int& quadrant = pUserData->quadrant;
	float& quadrantFine = pUserData->quadrantFine;
	ComputeQuadrant(gameAzimuth, quadrant, quadrantFine);

	float filterVolume = 0.0f;
	int filterFrequency = 800;

	switch (quadrant)
	{
	case 2:     // fall through
	case 1:
		{
			filterVolume = (quadrantFine * (-5.0f));
			filterFrequency -= static_cast<int>(quadrantFine * 200);
			break;
		}
	default:
		{
			break;
		}
	}

	// Filter Sources behind Listener
	{
		BiquadIIFilter* pFilterDominant = &pUserData->pFilterBankB->filterBand11; // FilterDominant
		BiquadIIFilter* pFilterResidual = &pUserData->pFilterBankA->filterBand11; // FilterResidual

		switch (pUserData->voiceCycle)
		{
		case 0:
			{
				pFilterDominant = &pUserData->pFilterBankA->filterBand11;
				break;
			}
		case 1:
			{
				pFilterDominant = &pUserData->pFilterBankA->filterBand11;
				pFilterResidual = &pUserData->pFilterBankB->filterBand11;
				break;
			}
		case 2:
			{
				pFilterDominant = &pUserData->pFilterBankB->filterBand11;
				pFilterResidual = &pUserData->pFilterBankA->filterBand11;
				break;
			}
		default:
			{
				break;
			}
		}

		pFilterDominant->ComputeCoefficients(filterFrequency, g_defaultFilterQuality, filterVolume);
		AkSampleType* pChannelInPlace = pInputBuffer->GetChannel(0);

		int const inputValidFrames = pInputBuffer->uValidFrames;

		switch (pUserData->voiceCycle)
		{
		case 0:
			{
				for (int i = 1; i <= g_mediumFadeLengthInteger; ++i)
				{
					AkSampleType const sampleFiltered = pFilterDominant->ProcessSample(*pChannelInPlace);

					*pChannelInPlace = sampleFiltered * ComputeFade((i / g_mediumFadeLength), g_linearFadeStrength);
					++pChannelInPlace;
				}

				for (int i = g_mediumFadeLengthInteger; i < inputValidFrames; ++i)
				{
					*pChannelInPlace = pFilterDominant->ProcessSample(*pChannelInPlace);
					++pChannelInPlace;
				}

				pUserData->voiceCycle = 2;
				break;
			}
		case 1:     // Fall through
		case 2:
			{
				for (int i = 1; i <= g_mediumFadeLengthInteger; ++i) // Blend
				{
					AkReal32 const filteredSampleDominant = pFilterDominant->ProcessSample(*pChannelInPlace);
					AkReal32 const filteredSampleResidual = pFilterResidual->ProcessSample(*pChannelInPlace);

					float const fadeValue = (i / g_mediumFadeLength);

					*pChannelInPlace =
						((filteredSampleResidual * ComputeFade((1.0f - fadeValue), g_linearFadeStrength)) +
						 (filteredSampleDominant * ComputeFade(fadeValue, g_linearFadeStrength))); // Linear Fade because correlation is 1

					++pChannelInPlace;
				}

				for (int i = g_mediumFadeLengthInteger; i < inputValidFrames; ++i) // Write Rest with new Filters
				{
					*pChannelInPlace = pFilterDominant->ProcessSample(*pChannelInPlace);
					++pChannelInPlace;
				}

				pUserData->voiceCycle = (pUserData->voiceCycle == 2) ? 1 : 2;
				break;
			}
		default:
			{
				break;
			}
		}
	}

	m_pContext->GlobalContext()->Mix1inNChannels(
		pInputBuffer->GetChannel(0),
		pMixBuffer,
		localbaseVolume.fPrev,
		localbaseVolume.fNext,
		volumesPrev,
		volumesNext
		);

	pMixBuffer->uValidFrames = pInputBuffer->uValidFrames;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::HrtfMonoTo7_1(
	AK::IAkMixerInputContext* const pInputContext,
	AkRamp const baseVolume,
	AkRamp const busVolume,
	AkAudioBuffer* pInputBuffer,
	AkAudioBuffer* pMixBuffer)
{
	SAkUserData* const pUserData = static_cast<SAkUserData*>(pInputContext->GetUserData());
	AkReal32& gameAzimuth = pUserData->gameAzimuth;
	AkReal32& gameElevation = pUserData->gameElevation;

	pInputBuffer->ZeroPadToMaxFrames();

	AkRamp const localbaseVolume = baseVolume * busVolume;
	AkUInt32 const allocSize = AK::SpeakerVolumes::Matrix::GetRequiredSize(pInputBuffer->NumChannels(), pMixBuffer->NumChannels());
	AK::SpeakerVolumes::MatrixPtr const volumesPrev = (AK::SpeakerVolumes::MatrixPtr)AkAlloca(allocSize);
	AK::SpeakerVolumes::MatrixPtr const volumesNext = (AK::SpeakerVolumes::MatrixPtr)AkAlloca(allocSize);
	pInputContext->GetSpatializedVolumes(volumesPrev, volumesNext);

	// Set Azimuth and Elevation
	AkEmitterListenerPair emitterListener;
	pInputContext->Get3DPosition(0, emitterListener);
	m_pContext->GlobalContext()->ComputeSphericalCoordinates(emitterListener, gameAzimuth, gameElevation);

	// Clamp azimuth and elevation in case data from game is faulty
	if (gameAzimuth > static_cast<AkReal32>(g_pi))
	{
		gameAzimuth = static_cast<AkReal32>(g_pi);
	}

	if (gameAzimuth < static_cast<AkReal32>(-g_pi))
	{
		gameAzimuth = static_cast<AkReal32>(-g_pi);
	}

	if (gameElevation > static_cast<AkReal32>(g_piHalf))
	{
		gameElevation = static_cast<AkReal32>(g_piHalf);
	}

	if (gameElevation < static_cast<AkReal32>(-g_piHalf))
	{
		gameElevation = static_cast<AkReal32>(-g_piHalf);
	}

	int& quadrant = pUserData->quadrant;
	float& quadrantFine = pUserData->quadrantFine;
	ComputeQuadrant(pUserData->gameAzimuth, quadrant, quadrantFine);

	// filter sources behind Listener
	float filterVolume = 0.0f;
	int filterFrequency = 800;

	switch (quadrant)
	{
	case 2:     // fall through
	case 1:
		{
			filterVolume = (quadrantFine * (-5.0f));
			filterFrequency -= static_cast<int>(quadrantFine * 200);
			break;
		}
	default:
		{
			break;
		}
	}

	{
		BiquadIIFilter* pFilterDominant = &pUserData->pFilterBankB->filterBand11; // FilterDominant
		BiquadIIFilter* pFilterResidual = &pUserData->pFilterBankA->filterBand11; // FilterResidual

		switch (pUserData->voiceCycle)
		{
		case 0:
			{
				pFilterDominant = &pUserData->pFilterBankA->filterBand11;
				break;
			}
		case 1:
			{
				pFilterDominant = &pUserData->pFilterBankA->filterBand11;
				pFilterResidual = &pUserData->pFilterBankB->filterBand11;
				break;
			}
		case 2:
			{
				pFilterDominant = &pUserData->pFilterBankB->filterBand11;
				pFilterResidual = &pUserData->pFilterBankA->filterBand11;
				break;
			}
		default:
			{
				break;
			}

		}

		pFilterDominant->ComputeCoefficients(filterFrequency, g_defaultFilterQuality, filterVolume);
		AkSampleType* pChannelInPlace = pInputBuffer->GetChannel(0);

		int inputValidFrames = pInputBuffer->uValidFrames;

		switch (pUserData->voiceCycle)
		{
		case 0:
			{
				for (int i = 1; i <= g_mediumFadeLengthInteger; ++i)
				{
					AkSampleType const filteredSample = pFilterDominant->ProcessSample(*pChannelInPlace);

					*pChannelInPlace = filteredSample * ComputeFade((i / g_mediumFadeLength), g_linearFadeStrength);
					++pChannelInPlace;
				}

				for (int i = g_mediumFadeLengthInteger; i < inputValidFrames; ++i)
				{
					*pChannelInPlace = pFilterDominant->ProcessSample(*pChannelInPlace);
					++pChannelInPlace;
				}

				pUserData->voiceCycle = 2;
				break;
			}
		case 1:     // Fall through
		case 2:
			{
				for (int i = 1; i <= g_mediumFadeLengthInteger; ++i)     // Blend
				{
					AkReal32 const filteredSampleDominant = pFilterDominant->ProcessSample(*pChannelInPlace);
					AkReal32 const filteredSampleResidual = pFilterResidual->ProcessSample(*pChannelInPlace);

					float fadeValue = (i / g_mediumFadeLength);

					*pChannelInPlace = ((filteredSampleResidual * ComputeFade((1.0f - fadeValue), g_linearFadeStrength)) +
					                    (filteredSampleDominant * ComputeFade(fadeValue, g_linearFadeStrength))); // Linear Fade because correlation is 1

					++pChannelInPlace;
				}

				for (int i = g_mediumFadeLengthInteger; i < inputValidFrames; ++i)     // Write Rest with new Filters
				{
					*pChannelInPlace = pFilterDominant->ProcessSample(*pChannelInPlace);
					++pChannelInPlace;
				}

				pUserData->voiceCycle = (pUserData->voiceCycle == 2) ? 1 : 2;
				break;
			}
		default:
			{
				break;
			}
		}
	}

	m_pContext->GlobalContext()->Mix1inNChannels(
		pInputBuffer->GetChannel(0),
		pMixBuffer,
		localbaseVolume.fPrev,
		localbaseVolume.fNext,
		volumesPrev,
		volumesNext
		);

	pMixBuffer->uValidFrames = pInputBuffer->uValidFrames;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::NonHrtfStereoToStereo(
	AK::IAkMixerInputContext* pInputContext,
	AkRamp const baseVolume,
	AkRamp const busVolume,
	AkAudioBuffer* pInputBuffer,
	AkAudioBuffer* pMixBuffer)
{
	AkRamp const localbaseVolume = baseVolume * busVolume;

	m_pContext->GlobalContext()->MixChannel(
		pInputBuffer->GetChannel(0),
		pMixBuffer->GetChannel(0),
		localbaseVolume.fPrev,
		localbaseVolume.fNext,
		pInputBuffer->uValidFrames);

	m_pContext->GlobalContext()->MixChannel(
		pInputBuffer->GetChannel(1),
		pMixBuffer->GetChannel(1),
		localbaseVolume.fPrev,
		localbaseVolume.fNext,
		pInputBuffer->uValidFrames);

	pMixBuffer->uValidFrames = pInputBuffer->uValidFrames;
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::ComputeDelayChannelData(AkReal32 const gameElevation, int const quadrant, float const quadrantFine, int& outDelay)
{
	auto const elevationFactor = static_cast<float>(fabs(gameElevation) / static_cast<AkReal32>(g_piHalf));
	float const elevationFactorInversedClamp = (elevationFactor > 0.85f) ? 0 : 1 - (elevationFactor / 0.85f);

	if ((quadrant == 0) || (quadrant == 3)) // azimuth 0 to (Pi/2)
	{
		outDelay =
			static_cast<int>(
				(g_maxDelay * ComputeFade(quadrantFine, 2.4f, EFadeType::FadeinLogarithmic))
				* ComputeFade(elevationFactorInversedClamp, 3.6f, EFadeType::FadeinLogarithmic));
	}
	else      // azimuth (Pi/2) to (Pi)
	{
		outDelay =
			static_cast<int>(
				(g_maxDelay * ComputeFade(quadrantFine, 2.4f, EFadeType::FadeoutLogarithmic))
				* ComputeFade(elevationFactorInversedClamp, 3.6f, EFadeType::FadeinLogarithmic));
	}
}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::FilterBuffers(
	AK::IAkMixerInputContext* const pInputContext,
	AkAudioBuffer* const pInputBuffer,
	AkSampleType* pIntermediateBufferConcealed,
	ESourceDirection const sourceDirection)
{
	SAkUserData* const pUserData = static_cast<SAkUserData*>(pInputContext->GetUserData());
	float const gameElevation = pUserData->gameElevation;
	int const inputValidFrames = pInputBuffer->uValidFrames;

	float const elevationFactor = (fabsf(gameElevation) / static_cast<float>(g_piHalf));
	float const elevationFactorInversedClamp = (elevationFactor > 0.85f) ? 0.0f : 1.0f - (elevationFactor / 0.85f);

	BiquadIIFilter
	* filterDominant00,
	* filterDominant01,
	* filterDominant02,
	* filterDominant03,
	* filterDominant04,
	* filterDominant05,
	* filterDominant06,
	* filterDominant07,
	* filterDominant08,
	* filterDominant09,
	* filterDominant10,
	* filterDominant11,

	* filterResidual00,
	* filterResidual01,
	* filterResidual02,
	* filterResidual03,
	* filterResidual04,
	* filterResidual05,
	* filterResidual06,
	* filterResidual07,
	* filterResidual08,
	* filterResidual09,
	* filterResidual10,
	* filterResidual11;

	// Dominant || Residual
	switch (pUserData->voiceCycle)
	{
	case 0:
		{
			filterDominant00 = &pUserData->pFilterBankA->filterBand00;
			filterDominant01 = &pUserData->pFilterBankA->filterBand01;
			filterDominant02 = &pUserData->pFilterBankA->filterBand02;
			filterDominant03 = &pUserData->pFilterBankA->filterBand03;
			filterDominant04 = &pUserData->pFilterBankA->filterBand04;
			filterDominant05 = &pUserData->pFilterBankA->filterBand05;
			filterDominant06 = &pUserData->pFilterBankA->filterBand06;
			filterDominant07 = &pUserData->pFilterBankA->filterBand07;
			filterDominant08 = &pUserData->pFilterBankA->filterBand08;
			filterDominant09 = &pUserData->pFilterBankA->filterBand09;
			filterDominant10 = &pUserData->pFilterBankA->filterBand10;
			filterDominant11 = &pUserData->pFilterBankA->filterBand11;

			break;
		}
	case 1:
		{
			filterDominant00 = &pUserData->pFilterBankA->filterBand00;
			filterDominant01 = &pUserData->pFilterBankA->filterBand01;
			filterDominant02 = &pUserData->pFilterBankA->filterBand02;
			filterDominant03 = &pUserData->pFilterBankA->filterBand03;
			filterDominant04 = &pUserData->pFilterBankA->filterBand04;
			filterDominant05 = &pUserData->pFilterBankA->filterBand05;
			filterDominant06 = &pUserData->pFilterBankA->filterBand06;
			filterDominant07 = &pUserData->pFilterBankA->filterBand07;
			filterDominant08 = &pUserData->pFilterBankA->filterBand08;
			filterDominant09 = &pUserData->pFilterBankA->filterBand09;
			filterDominant10 = &pUserData->pFilterBankA->filterBand10;
			filterDominant11 = &pUserData->pFilterBankA->filterBand11;

			filterResidual00 = &pUserData->pFilterBankB->filterBand00;
			filterResidual01 = &pUserData->pFilterBankB->filterBand01;
			filterResidual02 = &pUserData->pFilterBankB->filterBand02;
			filterResidual03 = &pUserData->pFilterBankB->filterBand03;
			filterResidual04 = &pUserData->pFilterBankB->filterBand04;
			filterResidual05 = &pUserData->pFilterBankB->filterBand05;
			filterResidual06 = &pUserData->pFilterBankB->filterBand06;
			filterResidual07 = &pUserData->pFilterBankB->filterBand07;
			filterResidual08 = &pUserData->pFilterBankB->filterBand08;
			filterResidual09 = &pUserData->pFilterBankB->filterBand09;
			filterResidual10 = &pUserData->pFilterBankB->filterBand10;
			filterResidual11 = &pUserData->pFilterBankB->filterBand11;

			break;
		}
	case 2:
		{
			filterResidual00 = &pUserData->pFilterBankA->filterBand00;
			filterResidual01 = &pUserData->pFilterBankA->filterBand01;
			filterResidual02 = &pUserData->pFilterBankA->filterBand02;
			filterResidual03 = &pUserData->pFilterBankA->filterBand03;
			filterResidual04 = &pUserData->pFilterBankA->filterBand04;
			filterResidual05 = &pUserData->pFilterBankA->filterBand05;
			filterResidual06 = &pUserData->pFilterBankA->filterBand06;
			filterResidual07 = &pUserData->pFilterBankA->filterBand07;
			filterResidual08 = &pUserData->pFilterBankA->filterBand08;
			filterDominant09 = &pUserData->pFilterBankB->filterBand09;
			filterDominant10 = &pUserData->pFilterBankB->filterBand10;
			filterDominant11 = &pUserData->pFilterBankB->filterBand11;

			filterDominant00 = &pUserData->pFilterBankB->filterBand00;
			filterDominant01 = &pUserData->pFilterBankB->filterBand01;
			filterDominant02 = &pUserData->pFilterBankB->filterBand02;
			filterDominant03 = &pUserData->pFilterBankB->filterBand03;
			filterDominant04 = &pUserData->pFilterBankB->filterBand04;
			filterDominant05 = &pUserData->pFilterBankB->filterBand05;
			filterDominant06 = &pUserData->pFilterBankB->filterBand06;
			filterDominant07 = &pUserData->pFilterBankB->filterBand07;
			filterDominant08 = &pUserData->pFilterBankB->filterBand08;
			filterResidual09 = &pUserData->pFilterBankA->filterBand09;
			filterResidual10 = &pUserData->pFilterBankA->filterBand10;
			filterResidual11 = &pUserData->pFilterBankA->filterBand11;

			break;
		}
	default:
		{

			break;
		}
	}

	// AZIMUTH COMMON FILTERS
	//
	int const quadrant = pUserData->quadrant;
	float const quadrantFine = pUserData->quadrantFine;

	// BAND00 500Hz
	int band00Frequency;
	float band00Quality = 3.0f;
	float band00Gain;

	switch (quadrant)
	{
	case 0:     // Fall through
	case 3:
		{
			band00Frequency = 500 + static_cast<int>(400.0f * ComputeFade(quadrantFine, 1.47f, EFadeType::FadeinHyperbole));
			band00Gain = (2.0f - 1.0f * quadrantFine) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band00Frequency = 900 + static_cast<int>(400.0f * ComputeFade(quadrantFine, 0.6f, EFadeType::FadeinLogarithmic));
			band00Quality = 3.0f - (0.8f * ComputeFade(quadrantFine, 1.7f, EFadeType::FadeinLogarithmic));
			band00Gain = (1.0f + (5.0f * ComputeFade(quadrantFine, 0.53f, EFadeType::FadeinLogarithmic))) * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant00->ComputeCoefficients(band00Frequency, band00Quality, band00Gain);

	// BAND01 1000Hz
	int band01Frequency;
	float band01Quality;
	float band01Gain;

	switch (quadrant)
	{
	case 0:     // Fall through
	case 3:
		{
			band01Frequency = (1000 + static_cast<int>(1000.0f * ComputeFade(quadrantFine, 0.87f, EFadeType::FadeinHyperbole)));
			band01Quality = 3.5f + (1.0f * ComputeFade(quadrantFine, 1.0f, EFadeType::FadeinHyperbole));
			band01Gain = (-6.0f + (1.0f * quadrantFine)) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band01Frequency = (2000 + static_cast<int>(1500.0f * ComputeFade(quadrantFine, 1.39f, EFadeType::FadeinLogarithmic)));
			band01Quality = 4.5f + (1.5f * ComputeFade(quadrantFine, 0.55f, EFadeType::FadeinLogarithmic));
			band01Gain = (-5.0f + (2.0f * ComputeFade(quadrantFine, 2.29f, EFadeType::FadeinLogarithmic))) * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant01->ComputeCoefficients(band01Frequency, band01Quality, band01Gain);

	// BAND02 3000 Hz
	int band02Frequency;
	float band02Quality;
	float band02Gain;

	switch (quadrant)
	{
	case 0:     // Fall through
	case 3:
		{
			band02Frequency = 3100 + static_cast<int>(3400.0f * ComputeFade(quadrantFine, 2.32f, EFadeType::FadeinHyperbole));
			band02Quality = 1.3f + (-1.2f * ComputeFade(quadrantFine, 1.64f, EFadeType::FadeinHyperbole));
			band02Gain = (6.0f - (3.0f * quadrantFine)) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band02Frequency = 6500 + static_cast<int>(4300.0f * ComputeFade(quadrantFine, 1.06f, EFadeType::FadeinLogarithmic));
			band02Quality = 0.1f + (1.2f * ComputeFade(quadrantFine, 1.83f, EFadeType::FadeinLogarithmic));
			band02Gain = 3.0f * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant02->ComputeCoefficients(band02Frequency, band02Quality, band02Gain);

	// BAND03
	int band03Frequency;
	float band03Quality;
	float band03Gain;

	switch (quadrant)
	{
	case 0:     // Fall through
	case 3:
		{
			band03Frequency = 8300 - static_cast<int>(2300.0f * quadrantFine);
			band03Quality = 10.0f - (3.0f * quadrantFine);
			band03Gain = ((-6.0f) * quadrantFine) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band03Frequency = 6000 - static_cast<int>(2250.0f * quadrantFine);
			band03Quality = 7.0f - (2.5f * ComputeFade(quadrantFine, 0.77f, EFadeType::FadeinLogarithmic));
			band03Gain = (-6.0f + (3.0f * quadrantFine)) * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant03->ComputeCoefficients(band03Frequency, band03Quality, band03Gain);

	// BAND04
	int band04Frequency;
	float band04Gain;
	float band04Quality;

	switch (quadrant)
	{
	case 0:     // Fall through
	case 3:
		{
			band04Frequency = 10650 - static_cast<int>(1650.0f * ComputeFade(quadrantFine, 2.3f, EFadeType::FadeinHyperbole));
			band04Quality = (7.7f + (2.3f * quadrantFine));
			band04Gain = (-8.0f * quadrantFine) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band04Frequency = 9000 - static_cast<int>(1500.0f * ComputeFade(quadrantFine, 2.24f, EFadeType::FadeinLogarithmic));
			band04Quality = (10.0f - (2.1f * ComputeFade(quadrantFine, 1.64f, EFadeType::FadeinLogarithmic)));
			band04Gain = (-8.0f * ComputeFade(quadrantFine, 1.87f, EFadeType::FadeoutHyperbole)) * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant04->ComputeCoefficients(band04Frequency, band04Quality, band04Gain);

	// BAND05
	int band05Frequency;
	float band05Gain;
	float band05Quality;

	switch (quadrant)
	{
	case 0:     // fall through
	case 3:
		{
			band05Frequency = 12850 - static_cast<int>(3850.0f * quadrantFine);
			band05Quality = 10.0f - (5.0f * quadrantFine);
			band05Gain = (-1.0f * quadrantFine) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band05Frequency = 9000 + static_cast<int>(3000.0f * ComputeFade(quadrantFine, 1.17f, EFadeType::FadeinLogarithmic));
			band05Quality = 5.0f - (4.1f * ComputeFade(quadrantFine, 1.64f, EFadeType::FadeinLogarithmic));
			band05Gain = (-1.0f + (3.0f * ComputeFade(quadrantFine, 1.35f, EFadeType::FadeinLogarithmic))) * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant05->ComputeCoefficients(band05Frequency, band05Quality, band05Gain);

	// ELEVATION
	//
	int band06Frequency;
	float band06Quality;
	float band06Gain;

	int band07Frequency;
	float band07Quality;
	float band07Gain;

	int band08Frequency;
	float band08Quality;
	float band08Gain;

	if (gameElevation > 0)
	{
		band06Frequency = 6000 - static_cast<int>(3700.0f * ComputeFade(elevationFactor, 0.9f, EFadeType::FadeinLogarithmic));
		band07Frequency = 6000 + static_cast<int>(3100.0f * ComputeFade(elevationFactor, 0.4f, EFadeType::FadeinLogarithmic));
		band08Frequency = 4000 + static_cast<int>(8000.0f * ComputeFade(elevationFactor, 1.0f, EFadeType::FadeinLogarithmic));

		band06Quality = 3.0f + (7.0f * ComputeFade(elevationFactor, 0.8f, EFadeType::FadeinLogarithmic));
		band07Quality = 4.5f + (5.5f * ComputeFade(elevationFactor, 2.7f, EFadeType::FadeinHyperbole));
		band08Quality = 4.5f + (5.5f * ComputeFade(elevationFactor, 1.7f, EFadeType::FadeinLogarithmic));

		band06Gain = -15.0f * ComputeFade(elevationFactor, 0.8f, EFadeType::FadeinLogarithmic);
		band07Gain = 6.0f * ComputeFade(elevationFactor, 1.7f, EFadeType::FadeinLogarithmic);
		band08Gain = -15.0f * ComputeFade(elevationFactor, 2.5f, EFadeType::FadeinLogarithmic);
	}
	else
	{
		band06Frequency = 6000 + static_cast<int>(4700.0f * elevationFactor);
		band07Frequency = 6000 - static_cast<int>(900.0f * elevationFactor);
		band08Frequency = 4000 - static_cast<int>(800.0f * elevationFactor);

		band06Quality = 3.0f + (4.7f * elevationFactor);
		band07Quality = 4.5f - (1.2f * elevationFactor);
		band08Quality = 4.5f + (5.5f * elevationFactor);

		band06Gain = -10.0f * elevationFactor;
		band07Gain = -14.0f * ComputeFade(elevationFactor, 1.5f, EFadeType::FadeinLogarithmic);
		band08Gain = -8.0f * ComputeFade(elevationFactor, 1.0f, EFadeType::FadeinLogarithmic);
	}

	filterDominant06->ComputeCoefficients(band06Frequency, band06Quality, band06Gain);
	filterDominant07->ComputeCoefficients(band07Frequency, band07Quality, band07Gain);
	filterDominant08->ComputeCoefficients(band08Frequency, band08Quality, band08Gain);

	// AZIMUTH SPECIFIC
	// 09 Direct, 10 Concealed, 11 Concealed
	int band09Frequency;
	float band09Quality = 1.0f;
	float band09Gain;

	switch (quadrant)
	{
	case 0:     // Fall through
	case 3:
		{
			band09Frequency = 9000 - static_cast<int>(7700.0f * ComputeFade(quadrantFine, 3.11f, EFadeType::FadeinLogarithmic));
			band09Gain = (6.0f * ComputeFade(quadrantFine, 1.67f, EFadeType::FadeinLogarithmic)) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band09Frequency = 1300 + static_cast<int>(4200.0f * ComputeFade(quadrantFine, 0.67f, EFadeType::FadeinHyperbole));
			band09Gain = (6.0f * ComputeFade(quadrantFine, 1.06f, EFadeType::FadeoutLogarithmic)) * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant09->ComputeCoefficients(band09Frequency, band09Quality, band09Gain);

	int band10Frequency = 1000;
	float band10Quality;
	float band10Gain;

	int band11Frequency;
	float band11Quality = 1.0f;
	float band11Gain;

	switch (quadrant)
	{
	case 0:     // Fall through
	case 3:
		{
			band10Quality = 0.8f + (1.2f * quadrantFine);
			band10Gain = (4.0f * ComputeFade(quadrantFine, 3.32f, EFadeType::FadeinHyperbole)) * elevationFactorInversedClamp;

			band11Frequency = 9000 - static_cast<int>(6000.0f * quadrantFine);
			band11Gain = (-13.0f * ComputeFade(quadrantFine, 0.64f, EFadeType::FadeinHyperbole)) * elevationFactorInversedClamp;
			break;
		}
	case 1:     // Fall through
	case 2:
		{
			band10Frequency += static_cast<int>(1000.0f * quadrantFine);
			band10Quality = 2.0f - (1.7f * ComputeFade(quadrantFine, 1.64f, EFadeType::FadeinHyperbole));
			band10Gain = (4.0f * ComputeFade(quadrantFine, 3.4f, EFadeType::FadeoutLogarithmic)) * elevationFactorInversedClamp;

			band11Frequency = 3000 + static_cast<int>(5000.0f * ComputeFade(quadrantFine, 1.1f, EFadeType::FadeinLogarithmic));
			band11Gain = (-13.0f * ComputeFade(quadrantFine, 2.4f, EFadeType::FadeoutLogarithmic)) * elevationFactorInversedClamp;
			break;
		}
	default:
		{
			break;
		}
	}

	filterDominant10->ComputeCoefficients(band10Frequency, band10Quality, band10Gain);
	filterDominant11->ComputeCoefficients(band11Frequency, band11Quality, band11Gain);

	AkSampleType* pChannel = pInputBuffer->GetChannel(0);

	switch (pUserData->voiceCycle)
	{
	case 0:
		{
			float const fadeFactor = 1.0f / g_smallFadeLength;

			for (int i = 0; i < g_smallFadeLengthInteger; ++i)
			{
				float const sampleFiltered =
					filterDominant08->ProcessSample(
						filterDominant07->ProcessSample(
							filterDominant06->ProcessSample(
								filterDominant05->ProcessSample(
									filterDominant04->ProcessSample(
										filterDominant03->ProcessSample(
											filterDominant02->ProcessSample(
												filterDominant01->ProcessSample(
													filterDominant00->ProcessSample(*pChannel)))))))));

				*pChannel = filterDominant09->ProcessSample(sampleFiltered) * (i * fadeFactor);
				++pChannel;

				*pIntermediateBufferConcealed = filterDominant10->ProcessSample(filterDominant11->ProcessSample(sampleFiltered)) * (i * fadeFactor);
				++pIntermediateBufferConcealed;
			}

			int const cyclesRemaining = inputValidFrames - g_smallFadeLengthInteger;

			for (int i = 0; i < cyclesRemaining; ++i)
			{
				float const sampleFiltered =
					filterDominant08->ProcessSample(
						filterDominant07->ProcessSample(
							filterDominant06->ProcessSample(
								filterDominant05->ProcessSample(
									filterDominant04->ProcessSample(
										filterDominant03->ProcessSample(
											filterDominant02->ProcessSample(
												filterDominant01->ProcessSample(
													filterDominant00->ProcessSample(*pChannel)))))))));

				*pChannel = filterDominant09->ProcessSample(sampleFiltered);
				++pChannel;

				*pIntermediateBufferConcealed = filterDominant10->ProcessSample(filterDominant11->ProcessSample(sampleFiltered));
				++pIntermediateBufferConcealed;
			}

			break;
		}
	case 1:     // fall through
	case 2:
		{
			float const fadeFactor = 1.0f / g_largeFadeLength;

			if (pUserData->lastSourceDirection == sourceDirection)
			{

				for (int i = 0; i < g_largeFadeLengthInteger; ++i) // Blend
				{
					float const newFilter =
						filterDominant08->ProcessSample(
							filterDominant07->ProcessSample(
								filterDominant06->ProcessSample(
									filterDominant05->ProcessSample(
										filterDominant04->ProcessSample(
											filterDominant03->ProcessSample(
												filterDominant02->ProcessSample(
													filterDominant01->ProcessSample(
														filterDominant00->ProcessSample(*pChannel)))))))));

					float const oldFilter =
						filterResidual08->ProcessSample(
							filterResidual07->ProcessSample(
								filterResidual06->ProcessSample(
									filterResidual05->ProcessSample(
										filterResidual04->ProcessSample(
											filterResidual03->ProcessSample(
												filterResidual02->ProcessSample(
													filterResidual01->ProcessSample(
														filterResidual00->ProcessSample(*pChannel)))))))));

					float const fadeValue = i * fadeFactor;

					*pChannel =
						(filterDominant09->ProcessSample(newFilter) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeinLogarithmic))
						+ (filterResidual09->ProcessSample(oldFilter) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeoutLogarithmic));

					++pChannel;

					*pIntermediateBufferConcealed =
						(filterDominant10->ProcessSample(
							 filterDominant11->ProcessSample(newFilter)) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeinLogarithmic))
						+ (filterResidual10->ProcessSample(
								 filterResidual11->ProcessSample(oldFilter)) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeoutLogarithmic));

					++pIntermediateBufferConcealed;
				}

				int const cyclesRemaining = inputValidFrames - g_largeFadeLengthInteger;

				for (int i = 0; i < cyclesRemaining; ++i) // Write Rest with new Filters
				{
					float const sampleFiltered =
						filterDominant08->ProcessSample(
							filterDominant07->ProcessSample(
								filterDominant06->ProcessSample(
									filterDominant05->ProcessSample(
										filterDominant04->ProcessSample(
											filterDominant03->ProcessSample(
												filterDominant02->ProcessSample(
													filterDominant01->ProcessSample(
														filterDominant00->ProcessSample(*pChannel)))))))));

					*pChannel = filterDominant09->ProcessSample(sampleFiltered);
					++pChannel;

					*pIntermediateBufferConcealed = filterDominant10->ProcessSample(filterDominant11->ProcessSample(sampleFiltered));
					++pIntermediateBufferConcealed;
				}
			}
			else
			{
				for (int i = 0; i < g_largeFadeLengthInteger; ++i) // Blend
				{
					float const newFilter =
						filterDominant08->ProcessSample(
							filterDominant07->ProcessSample(
								filterDominant06->ProcessSample(
									filterDominant05->ProcessSample(
										filterDominant04->ProcessSample(
											filterDominant03->ProcessSample(
												filterDominant02->ProcessSample(
													filterDominant01->ProcessSample(
														filterDominant00->ProcessSample(*pChannel)))))))));

					float const oldFilter =
						filterResidual08->ProcessSample(
							filterResidual07->ProcessSample(
								filterResidual06->ProcessSample(
									filterResidual05->ProcessSample(
										filterResidual04->ProcessSample(
											filterResidual03->ProcessSample(
												filterResidual02->ProcessSample(
													filterResidual01->ProcessSample(
														filterResidual00->ProcessSample(*pChannel)))))))));

					float const fadeValue = i * fadeFactor;

					*pChannel =
						(filterDominant09->ProcessSample(newFilter) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeinLogarithmic))
						+ (filterResidual10->ProcessSample(
								 filterResidual11->ProcessSample(oldFilter)) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeoutLogarithmic));

					++pChannel;

					*pIntermediateBufferConcealed =
						(filterDominant10->ProcessSample(
							 filterDominant11->ProcessSample(newFilter)) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeinLogarithmic))
						+ (filterResidual09->ProcessSample(oldFilter) * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeoutLogarithmic));

					++pIntermediateBufferConcealed;
				}

				int const cyclesRemaining = inputValidFrames - g_largeFadeLengthInteger;

				for (int i = 0; i < cyclesRemaining; ++i) // Write Rest with new Filters
				{
					float const sampleFiltered =
						filterDominant08->ProcessSample(
							filterDominant07->ProcessSample(
								filterDominant06->ProcessSample(
									filterDominant05->ProcessSample(
										filterDominant04->ProcessSample(
											filterDominant03->ProcessSample(
												filterDominant02->ProcessSample(
													filterDominant01->ProcessSample(
														filterDominant00->ProcessSample(*pChannel)))))))));

					*pChannel = filterDominant09->ProcessSample(sampleFiltered);
					++pChannel;

					*pIntermediateBufferConcealed = filterDominant10->ProcessSample(filterDominant11->ProcessSample(sampleFiltered));
					++pIntermediateBufferConcealed;
				}
			}

			break;
		}
	default:
		{
			break;
		}
	}

}

//////////////////////////////////////////////////////////////////////////
void CrySpatialFX::DelayAndFadeBuffers(
	AkSampleType* pInChannelConcealed,
	AkSampleType* pOutChannelConcealed,
	AkSampleType* pChannelDirect,
	AkSampleType* pDelBuffer,
	SAkUserData* const pUserData,
	int const delayCurrent,
	int const maxFrames,
	ESourceDirection const sourceDirection)
{
	// Delay Concealed Buffer
	{
		int outSampleOffset = 0;                                                                                   // needed to compensate delay changes since the last Engine tick
		int const delayPrev = (sourceDirection == pUserData->lastSourceDirection) ? pUserData->voiceDelayPrev : 0; // If we switch L/R then prev delay is 0
		AkSampleType* pInChannelConcealedProxy = pInChannelConcealed;
		AkSampleType* pOutChannelConcealedProxy = pOutChannelConcealed + delayPrev;

		memcpy(pOutChannelConcealed, pDelBuffer, sizeof(AkSampleType) * delayPrev);

		// Cross fade
		if (delayPrev != delayCurrent)
		{
			if (delayPrev < delayCurrent)
			{
				if (sourceDirection == pUserData->lastSourceDirection)
				{
					AkSampleType* pInChannelConcealedOffsetProxy = pInChannelConcealed;
					int const sampleGap = delayCurrent - delayPrev;

					for (int i = 0; i < sampleGap; ++i)
					{
						*pOutChannelConcealedProxy = *pInChannelConcealedOffsetProxy;
						++pOutChannelConcealedProxy;
						++pInChannelConcealedOffsetProxy;
					}

					float const fadeFactor = 1.0f / g_mediumFadeLength;

					for (int i = 1; i <= g_mediumFadeLengthInteger; ++i)
					{
						float const fadeValue = i * fadeFactor;

						*pOutChannelConcealedProxy =
							((*pInChannelConcealedOffsetProxy * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeoutLogarithmic)) +
							 (*pInChannelConcealedProxy * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeinLogarithmic)));

						++pOutChannelConcealedProxy;
						++pInChannelConcealedProxy;
						++pInChannelConcealedOffsetProxy;
					}

					outSampleOffset = g_mediumFadeLengthInteger + sampleGap;
				}
				else
				{
					AkSampleType* pInChannelConcealedOffsetProxy = pChannelDirect;

					float const fadeFactor = 1.0f / g_largeFadeLength;

					for (int i = 0; i < delayCurrent; ++i)
					{
						*pOutChannelConcealedProxy = *pInChannelConcealedOffsetProxy;

						++pOutChannelConcealedProxy;
						++pInChannelConcealedOffsetProxy;
					}

					for (int i = 1; i <= g_largeFadeLengthInteger; ++i)
					{
						float const fadeValue = i * fadeFactor;

						*pOutChannelConcealedProxy =
							((*pInChannelConcealedOffsetProxy * ComputeFade(fadeValue, 0.5f, EFadeType::FadeoutLogarithmic)) +
							 (*pInChannelConcealedProxy * ComputeFade(fadeValue, 0.5f, EFadeType::FadeinLogarithmic)));

						++pOutChannelConcealedProxy;
						++pInChannelConcealedProxy;
						++pInChannelConcealedOffsetProxy;
					}

					outSampleOffset = g_largeFadeLengthInteger + delayCurrent;
				}
			}
			else      // delay gets smaller
			{
				int const samplesToDrop = delayPrev - delayCurrent;
				AkSampleType* pInChannelConcealedOffsetProxy = pInChannelConcealed;
				pInChannelConcealedProxy += samplesToDrop;

				float const fadeFactor = 1.0f / g_mediumFadeLength;

				for (int i = 1; i <= g_mediumFadeLength; ++i)
				{
					float const fadeValue = i * fadeFactor;

					*pOutChannelConcealedProxy =
						(*pInChannelConcealedOffsetProxy * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeoutLogarithmic)) +
						(*pInChannelConcealedProxy * ComputeFade(fadeValue, g_linearFadeStrength, EFadeType::FadeinLogarithmic));

					++pOutChannelConcealedProxy;
					++pInChannelConcealedOffsetProxy;
					++pInChannelConcealedProxy;
				}

				outSampleOffset = g_mediumFadeLengthInteger;
			}
		}

		// Write from current inputBuffer
		int const cycles = (maxFrames - (delayPrev + outSampleOffset));

		memcpy(pOutChannelConcealedProxy, pInChannelConcealedProxy, sizeof(AkSampleType) * cycles);
	}

	// DirectChannel Fades with Delayed signal
	if (sourceDirection != pUserData->lastSourceDirection)
	{
		AkSampleType* pChannelDirectProxy = pChannelDirect;
		AkSampleType* pDelChannelProxy = pOutChannelConcealed;
		AkSampleType* pDelBufferProxy = pDelBuffer;

		int const delayPrev = pUserData->voiceDelayPrev;

		for (int i = 0; i < delayPrev; ++i)
		{
			*pChannelDirectProxy = *pDelBufferProxy;
			++pChannelDirectProxy;
			++pDelBufferProxy;
		}

		float const fadeFactor = 1.0f / g_mediumFadeLength;

		for (int i = 1; i <= g_mediumFadeLengthInteger; ++i)
		{
			*pChannelDirectProxy =
				(*pChannelDirectProxy * ComputeFade((i * fadeFactor), g_linearFadeStrength, EFadeType::FadeinLogarithmic))
				+ ((*pDelChannelProxy * ComputeFade((i * fadeFactor), g_linearFadeStrength, EFadeType::FadeoutLogarithmic)));

			++pChannelDirectProxy;
			++pDelChannelProxy;
		}
	}

	// Write new Delay Buffer
	if (delayCurrent > 0)
	{
		int const offset = maxFrames - delayCurrent;
		pInChannelConcealed += offset;

		memcpy(pDelBuffer, pInChannelConcealed, sizeof(AkSampleType) * delayCurrent);
	}

	pUserData->pVoiceDelayBuffer->uValidFrames = delayCurrent;
	pUserData->voiceDelayPrev = delayCurrent;
	pUserData->lastSourceDirection = sourceDirection;
}

} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
DEFINE_PLUGIN_REGISTER_HOOK;
