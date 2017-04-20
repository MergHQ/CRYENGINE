// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioEvent.h"
#include "AudioObject.h"
#include <CryAudio/IAudioSystem.h>
#include <CrySystem/ISystem.h> // needed for gEnv in Release builds
#include <portaudio.h>
#include <sndfile.hh>

using namespace CryAudio;
using namespace CryAudio::Impl::PortAudio;

static long unsigned const s_bufferLength = 256;

// Callbacks
//////////////////////////////////////////////////////////////////////////
static int StreamCallback(
  void const* pInputBuffer,
  void* pOutputBuffer,
  long unsigned framesPerBuffer,
  PaStreamCallbackTimeInfo const* pTimeInfo,
  PaStreamCallbackFlags statusFlags,
  void* pUserData)
{
	CRY_ASSERT(framesPerBuffer == s_bufferLength);
	sf_count_t numFramesRead = 0;
	CAudioEvent* const pAudioEvent = static_cast<CAudioEvent*>(pUserData);

	switch (pAudioEvent->sampleFormat)
	{
	case paInt16:
		{
			short* pStreamData = static_cast<short*>(pOutputBuffer);
			numFramesRead = sf_readf_short(pAudioEvent->pSndFile, static_cast<short*>(pAudioEvent->pData), static_cast<sf_count_t>(s_bufferLength));

			for (sf_count_t i = 0; i < numFramesRead; ++i)
			{
				for (int j = 0; j < pAudioEvent->numChannels; ++j)
				{
					*pStreamData++ = static_cast<short*>(pAudioEvent->pData)[i * pAudioEvent->numChannels + j];
				}
			}
		}
		break;
	case paInt32:
		{
			int* pStreamData = static_cast<int*>(pOutputBuffer);
			numFramesRead = sf_readf_int(pAudioEvent->pSndFile, static_cast<int*>(pAudioEvent->pData), static_cast<sf_count_t>(s_bufferLength));

			for (sf_count_t i = 0; i < numFramesRead; ++i)
			{
				for (int j = 0; j < pAudioEvent->numChannels; ++j)
				{
					*pStreamData++ = static_cast<int*>(pAudioEvent->pData)[i * pAudioEvent->numChannels + j];
				}
			}
		}
		break;
	case paFloat32:
		{
			float* pStreamData = static_cast<float*>(pOutputBuffer);
			numFramesRead = sf_readf_float(pAudioEvent->pSndFile, static_cast<float*>(pAudioEvent->pData), static_cast<sf_count_t>(s_bufferLength));

			for (sf_count_t i = 0; i < numFramesRead; ++i)
			{
				for (int j = 0; j < pAudioEvent->numChannels; ++j)
				{
					*pStreamData++ = static_cast<float*>(pAudioEvent->pData)[i * pAudioEvent->numChannels + j];
				}
			}
		}
		break;
	}

	if (numFramesRead != framesPerBuffer)
	{
		pAudioEvent->bDone = true;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
CAudioEvent::CAudioEvent(CATLEvent& _audioEvent)
	: pSndFile(nullptr)
	, pStream(nullptr)
	, pData(nullptr)
	, pPAAudioObject(nullptr)
	, numChannels(0)
	, remainingLoops(0)
	, audioEvent(_audioEvent)
	, bDone(false)
{
}

//////////////////////////////////////////////////////////////////////////
CAudioEvent::~CAudioEvent()
{
	if (pStream != nullptr)
	{
		PaError const err = Pa_CloseStream(pStream);

		if (err != paNoError)
		{
			g_implLogger.Log(ELogType::Error, "CloseStream failed: %s", Pa_GetErrorText(err));
		}
	}

	if (pSndFile != nullptr)
	{
		sf_close(pSndFile);
	}

	if (pData != nullptr)
	{
		delete pData;
	}

	if (pPAAudioObject != nullptr)
	{
		pPAAudioObject->UnregisterAudioEvent(this);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAudioEvent::Execute(
  int const numLoops,
  double const sampleRate,
  CryFixedStringT<MaxFilePathLength> const& filePath,
  PaStreamParameters const& streamParameters)
{
	bool bSuccess = false;
	SF_INFO sfInfo;
	ZeroStruct(sfInfo);
	pSndFile = sf_open(filePath.c_str(), SFM_READ, &sfInfo);

	if (pSndFile != nullptr)
	{
		PaError err = Pa_OpenStream(
		  &pStream,
		  nullptr,
		  &streamParameters,
		  sampleRate,
		  s_bufferLength,
		  paClipOff,
		  &StreamCallback,
		  static_cast<void*>(this));

		if (err == paNoError)
		{
			numChannels = streamParameters.channelCount;
			remainingLoops = numLoops;
			sampleFormat = streamParameters.sampleFormat;
			long unsigned const numEntries = s_bufferLength * numChannels;

			switch (sampleFormat)
			{
			case paInt16:
				{
					pData = CryModuleMalloc(sizeof(short) * static_cast<size_t>(numEntries));
					std::memset(pData, 0, sizeof(short) * static_cast<size_t>(numEntries));
				}
				break;
			case paInt32:
				{
					pData = CryModuleMalloc(sizeof(int) * static_cast<size_t>(numEntries));
					std::memset(pData, 0, sizeof(int) * static_cast<size_t>(numEntries));
				}
				break;
			case paFloat32:
				{
					pData = CryModuleMalloc(sizeof(float) * static_cast<size_t>(numEntries));
					std::memset(pData, 0, sizeof(float) * static_cast<size_t>(numEntries));
				}
				break;
			}

			err = Pa_StartStream(pStream);

			if (err == paNoError)
			{
				bSuccess = true;
			}
			else
			{
				g_implLogger.Log(ELogType::Error, "StartStream failed: %s", Pa_GetErrorText(err));
			}
		}
		else
		{
			g_implLogger.Log(ELogType::Error, "OpenStream failed: %s", Pa_GetErrorText(err));
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CAudioEvent::Update()
{
	if (bDone)
	{
		if (remainingLoops != 0 && pStream != nullptr && pSndFile != nullptr)
		{
			sf_seek(pSndFile, 0, SEEK_SET);

			if (remainingLoops > 0)
			{
				--remainingLoops;
			}
		}
		else
		{
			gEnv->pAudioSystem->ReportFinishedEvent(audioEvent, true);
		}

		bDone = false;
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioEvent::Stop()
{
	gEnv->pAudioSystem->ReportFinishedEvent(audioEvent, true);
	return ERequestStatus::Success;
}
