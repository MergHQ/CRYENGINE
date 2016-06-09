// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SDLMixerSoundEngine.h"
#include "SDLMixerSoundEngineUtil.h"
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

#include <SDL.h>
#include <SDL_mixer.h>

#define SDL_MIXER_NUM_CHANNELS 512
#define SDL_MIXER_PROJECT_PATH AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "sdlmixer" CRY_NATIVE_PATH_SEPSTR

using namespace CryAudio::Impl;

namespace SDLMixer
{
static const int g_nSupportedFormats = MIX_INIT_OGG | MIX_INIT_MP3;
static const int g_nNumMixChannels = SDL_MIXER_NUM_CHANNELS;

static const TSampleID SDL_MIXER_INVALID_SAMPLE_ID = 0;
const int g_nSampleRate = 48000;
const int g_nBufferSize = 4096;

namespace SoundEngine
{
// Samples
string g_sampleDataRootDir;
typedef std::unordered_map<TSampleID, Mix_Chunk*> TSampleDataMap;
TSampleDataMap g_sampleData;

typedef std::unordered_map<TSampleID, string> TSampleNameMap;
TSampleNameMap g_samplePaths;

typedef std::unordered_map<TSampleID, int> TSampleIdUsageCounterMap;
TSampleIdUsageCounterMap g_usageCounters;

// Channels
struct SChannelData
{
	SATLAudioObjectData_sdlmixer* pAudioObject;
};
SChannelData g_channels[SDL_MIXER_NUM_CHANNELS];

typedef std::queue<int> TChannelQueue;
TChannelQueue g_freeChannels;

enum EChannelFinishedRequestQueueID
{
	eCFRQID_ONE = 0,
	eCFRQID_TWO = 1,
	eCFRQID_COUNT
};
typedef std::deque<int> TChannelFinishedRequests;
TChannelFinishedRequests g_channelFinishedRequests[eCFRQID_COUNT];
CryCriticalSection g_channelFinishedCriticalSection;

// Audio Objects
typedef std::vector<SATLAudioObjectData_sdlmixer*, STLSoundAllocator<SATLAudioObjectData_sdlmixer*>> TAudioObjectList;
TAudioObjectList g_audioObjects;

// Listeners
CAudioObjectTransformation g_listenerPosition;
bool g_bListenerPosChanged;
bool g_bMuted;

TFnEventCallback g_fnEventFinishedCallback;
TFnStandaloneFileCallback g_fnStandaloneFileFinishedCallback;

void RegisterEventFinishedCallback(TFnEventCallback pCallbackFunction)
{
	g_fnEventFinishedCallback = pCallbackFunction;
}
void RegisterStandaloneFileFinishedCallback(TFnStandaloneFileCallback pCallbackFunction)
{
	g_fnStandaloneFileFinishedCallback = pCallbackFunction;
}

void EventFinishedPlaying(AudioEventId nEventID)
{
	if (g_fnEventFinishedCallback)
	{
		g_fnEventFinishedCallback(nEventID);
	}
}

void StandaloneFileFinishedPlaying(AudioStandaloneFileId const fileInstanceId, char const* szFileName)
{
	if (g_fnStandaloneFileFinishedCallback)
	{
		g_fnStandaloneFileFinishedCallback(fileInstanceId, szFileName);
	}
}

void ProcessChannelFinishedRequests(TChannelFinishedRequests& queue)
{
	if (!queue.empty())
	{
		for (const int finishedChannelId : queue)
		{
			SATLAudioObjectData_sdlmixer* pAudioObject = g_channels[finishedChannelId].pAudioObject;
			if (pAudioObject)
			{
				const TEventInstanceList::iterator eventsEnd = pAudioObject->events.end();
				for (TEventInstanceList::iterator eventsIt = pAudioObject->events.begin(); eventsIt != eventsEnd; ++eventsIt)
				{
					SATLEventData_sdlmixer* pEventInstance = *eventsIt;
					if (pEventInstance)
					{
						const TChannelList::iterator channelsEnd = pEventInstance->channels.end();
						for (TChannelList::iterator channelIt = pEventInstance->channels.begin(); channelIt != channelsEnd; ++channelIt)
						{
							if (*channelIt == finishedChannelId)
							{
								pEventInstance->channels.erase(channelIt);
								if (pEventInstance->channels.empty())
								{
									pAudioObject->events.erase(eventsIt);
									EventFinishedPlaying(pEventInstance->nEventID);
								}
								break;
							}
						}
					}
				}
				const TStandAloneFileInstanceList::iterator standaloneFilesEnd = pAudioObject->standaloneFiles.end();
				for (TStandAloneFileInstanceList::iterator standaloneFilesIt = pAudioObject->standaloneFiles.begin(); standaloneFilesIt != standaloneFilesEnd; ++standaloneFilesIt)
				{
					CAudioStandaloneFile_sdlmixer* pStandaloneFileInstance = *standaloneFilesIt;
					if (pStandaloneFileInstance)
					{
						const TChannelList::iterator channelsEnd = pStandaloneFileInstance->channels.end();
						for (TChannelList::iterator channelIt = pStandaloneFileInstance->channels.begin(); channelIt != channelsEnd; ++channelIt)
						{
							if (*channelIt == finishedChannelId)
							{
								pStandaloneFileInstance->channels.erase(channelIt);
								if (pStandaloneFileInstance->channels.empty())
								{
									pAudioObject->standaloneFiles.erase(standaloneFilesIt);
									StandaloneFileFinishedPlaying(pStandaloneFileInstance->fileInstanceId, pStandaloneFileInstance->fileName.c_str());

									TSampleIdUsageCounterMap::iterator it = g_usageCounters.find(pStandaloneFileInstance->fileId);
									CRY_ASSERT(it != g_usageCounters.end() && it->second > 0);
									if (--it->second == 0)
									{
										UnloadSample(pStandaloneFileInstance->fileInstanceId);
									}
								}
								break;
							}
						}
					}
				}
				g_channels[finishedChannelId].pAudioObject = nullptr;
			}
			g_freeChannels.push(finishedChannelId);
		}
		queue.clear();
	}
}

void ChannelFinishedPlaying(int nChannel)
{
	if (nChannel >= 0 && nChannel < g_nNumMixChannels)
	{
		CryAutoLock<CryCriticalSection> autoLock(g_channelFinishedCriticalSection);
		g_channelFinishedRequests[eCFRQID_ONE].push_back(nChannel);
	}
}

void LoadMetadata(const string& sSDLMixerAssetPath)
{
	g_sampleDataRootDir = PathUtil::GetPath(sSDLMixerAssetPath);
	_finddata_t fd;
	ICryPak* pCryPak = gEnv->pCryPak;
	intptr_t handle = pCryPak->FindFirst(sSDLMixerAssetPath + "*.*", &fd);
	if (handle != -1)
	{
		do
		{
			const string sName = fd.name;
			if (sName != "." && sName != ".." && !sName.empty())
			{
				if (sName.find(".wav") != string::npos ||
				    sName.find(".ogg") != string::npos ||
				    sName.find(".mp3") != string::npos)
				{
					// For now there's a 1 to 1 mapping between sample files and events
					g_samplePaths[GetIDFromFilePath(sName)] = g_sampleDataRootDir + sName;
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);
		pCryPak->FindClose(handle);
	}
}

bool Init()
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "SDL::SDL_Init() returned: %s", SDL_GetError());
		return false;
	}

	int loadedFormats = Mix_Init(g_nSupportedFormats);
	if ((loadedFormats & g_nSupportedFormats) != g_nSupportedFormats)
	{
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "SDLMixer::Mix_Init() failed to init support for format flags %d with error \"%s\"", g_nSupportedFormats, Mix_GetError());
		return false;
	}

	if (Mix_OpenAudio(g_nSampleRate, MIX_DEFAULT_FORMAT, 2, g_nBufferSize) < 0)
	{
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "SDLMixer::Mix_OpenAudio() failed to init the SDL Mixer API with error \"%s\"", Mix_GetError());
		return false;
	}

	Mix_AllocateChannels(g_nNumMixChannels);

	g_bMuted = false;
	Mix_Volume(-1, SDL_MIX_MAXVOLUME);
	for (int i = 0; i < SDL_MIXER_NUM_CHANNELS; ++i)
	{
		g_freeChannels.push(i);
	}

	Mix_ChannelFinished(ChannelFinishedPlaying);

	LoadMetadata(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR SDL_MIXER_PROJECT_PATH);
	g_bListenerPosChanged = false;

	// need to reinit as the global variable might have been initialized with wrong values
	g_listenerPosition = CAudioObjectTransformation();

	return true;
}

void FreeAllSampleData()
{
	Mix_HaltChannel(-1);
	TSampleDataMap::const_iterator it = g_sampleData.begin();
	TSampleDataMap::const_iterator end = g_sampleData.end();
	for (; it != end; ++it)
	{
		Mix_FreeChunk(it->second);
	}
	g_sampleData.clear();
}

void Release()
{
	FreeAllSampleData();
	g_audioObjects.clear();
	Mix_Quit();
	Mix_CloseAudio();
	SDL_Quit();
}

void Refresh()
{
	FreeAllSampleData();
	LoadMetadata(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR SDL_MIXER_PROJECT_PATH);
}

bool LoadSampleImpl(const TSampleID nID, const string& sSamplePath)
{
	bool bSuccess = true;
	Mix_Chunk* pSample = Mix_LoadWAV(sSamplePath.c_str());
	if (pSample != nullptr)
	{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
		TSampleNameMap::const_iterator it = g_samplePaths.find(nID);
		if (it != g_samplePaths.end() && it->second != sSamplePath)
		{
			g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "Loaded a Sample with the already existing ID %u, but from a different path source path '%s' <-> '%s'.", static_cast<uint>(nID), it->second.c_str(), sSamplePath.c_str());
		}
		if (stl::find_in_map(g_sampleData, nID, nullptr) != nullptr)
		{
			g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "Loading sample '%s' which had already been loaded", sSamplePath.c_str());
		}
#endif
		g_sampleData[nID] = pSample;
		g_samplePaths[nID] = sSamplePath;
	}
	else
	{
		// Sample could be inside a pak file so we need to open it manually and load it from the raw file
		const size_t nFileSize = gEnv->pCryPak->FGetSize(sSamplePath);
		FILE* const pFile = gEnv->pCryPak->FOpen(sSamplePath, "rbx", ICryPak::FOPEN_HINT_DIRECT_OPERATION);
		if (pFile && nFileSize > 0)
		{
			void* const pData = AUDIO_ALLOCATOR_MEMORY_POOL.AllocateRaw(nFileSize, AUDIO_MEMORY_ALIGNMENT, "SDLMixerSample");
			gEnv->pCryPak->FReadRawAll(pData, nFileSize, pFile);
			const TSampleID nNewID = LoadSampleFromMemory(pData, nFileSize, sSamplePath, nID);
			if (nNewID == SDL_MIXER_INVALID_SAMPLE_ID)
			{
				g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "SDL Mixer failed to load sample %s. Error: \"%s\"", sSamplePath.c_str(), Mix_GetError());
				bSuccess = false;
			}
			AUDIO_ALLOCATOR_MEMORY_POOL.Free(pData);
		}
	}
	return bSuccess;
}

const TSampleID LoadSample(const string& sampleFilePath, bool bOnlyMetadata)
{
	const TSampleID id = GetIDFromFilePath(sampleFilePath);
	if (stl::find_in_map(g_sampleData, id, nullptr) == nullptr)
	{
		if (bOnlyMetadata)
		{
			g_samplePaths[id] = g_sampleDataRootDir + sampleFilePath;
		}
		else if (!LoadSampleImpl(id, sampleFilePath))
		{
			return SDL_MIXER_INVALID_SAMPLE_ID;
		}
	}
	return id;
}

const TSampleID LoadSampleFromMemory(void* pMemory, const size_t nSize, const string& sSamplePath, const TSampleID overrideId)
{
	const TSampleID nID = (overrideId != 0) ? overrideId : GetIDFromFilePath(sSamplePath);
	Mix_Chunk* pSample = stl::find_in_map(g_sampleData, nID, nullptr);
	if (pSample != nullptr)
	{
		Mix_FreeChunk(pSample);
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Warning, "Loading sample %s which had already been loaded", sSamplePath.c_str());
	}
	SDL_RWops* pData = SDL_RWFromMem(pMemory, nSize);
	if (pData)
	{
		Mix_Chunk* pSample = Mix_LoadWAV_RW(pData, 0);
		if (pSample != nullptr)
		{
			g_sampleData[nID] = pSample;
			g_samplePaths[nID] = sSamplePath;
			return nID;
		}
		else
		{
			g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "SDL Mixer failed to load sample. Error: \"%s\"", Mix_GetError());
		}
	}
	else
	{
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "SDL Mixer failed to transform the audio data. Error: \"%s\"", SDL_GetError());
	}
	return SDL_MIXER_INVALID_SAMPLE_ID;
}

void UnloadSample(const TSampleID nID)
{
	Mix_Chunk* pSample = stl::find_in_map(g_sampleData, nID, nullptr);
	if (pSample != nullptr)
	{
		Mix_FreeChunk(pSample);
		stl::member_find_and_erase(g_sampleData, nID);
	}
	else
	{
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "Could not find sample with id %d", nID);
	}
}

void Pause()
{
	Mix_Pause(-1);
	Mix_PauseMusic();
}

void Resume()
{
	Mix_Resume(-1);
	Mix_ResumeMusic();
}

void Stop()
{
	Mix_HaltChannel(-1);
}

void Mute()
{
	Mix_Volume(-1, 0);
	g_bMuted = true;
}

void UnMute()
{
	TAudioObjectList::const_iterator audioObjectIt = g_audioObjects.begin();
	const TAudioObjectList::const_iterator audioObjectEnd = g_audioObjects.end();
	for (; audioObjectIt != audioObjectEnd; ++audioObjectIt)
	{
		SATLAudioObjectData_sdlmixer* pAudioObject = *audioObjectIt;
		if (pAudioObject)
		{
			TEventInstanceList::const_iterator eventIt = pAudioObject->events.begin();
			const TEventInstanceList::const_iterator eventEnd = pAudioObject->events.end();
			for (; eventIt != eventEnd; ++eventIt)
			{
				SATLEventData_sdlmixer* pEventInstance = *eventIt;
				if (pEventInstance)
				{
					TChannelList::const_iterator channelIt = pEventInstance->channels.begin();
					TChannelList::const_iterator channelEnd = pEventInstance->channels.end();
					for (; channelIt != channelEnd; ++channelIt)
					{
						Mix_Volume(*channelIt, pEventInstance->pStaticData->nVolume);
					}
				}
			}
		}
	}
	g_bMuted = false;
}

void SetChannelPosition(const SATLTriggerImplData_sdlmixer* pStaticData, const int channelID, const float fDistance, const float fAngle)
{
	static const uint8 nSDLMaxDistance = 255;
	const float fMin = pStaticData->fAttenuationMinDistance;
	const float fMax = pStaticData->fAttenuationMaxDistance;
	if (fMin <= fMax)
	{
		uint8 nDistance = 0;
		if (fMax >= 0.0f && fDistance > fMin)
		{
			if (fMin != fMax)
			{
				const float fFinalDistance = fDistance - fMin;
				const float fRange = fMax - fMin;
				nDistance = static_cast<uint8>((std::min((fFinalDistance / fRange), 1.0f) * nSDLMaxDistance) + 0.5f);
			}
			else
			{
				nDistance = nSDLMaxDistance;
			}
		}
		//Temp code, to be reviewed during the SetChannelPosition rewrite:
		Mix_SetDistance(channelID, nDistance);

		if (pStaticData->bPanningEnabled)
		{
			//Temp code, to be reviewed during the SetChannelPosition rewrite:
			float const fAbsAngle = fabs(fAngle);
			float const fFrontAngle = (fAngle > 0.0f ? 1.0f : -1.0f) * (fAbsAngle > 90.0f ? 180.f - fAbsAngle : fAbsAngle);
			float const fRightVolume = (fFrontAngle + 90.0f) / 180.0f;
			float const fLeftVolume = 1.0f - fRightVolume;
			Mix_SetPanning(channelID,
			               static_cast<uint8>(255.0f * fLeftVolume),
			               static_cast<uint8>(255.0f * fRightVolume));
		}
	}
	else
	{
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "The minimum attenuation distance value is higher than the maximum");
	}
}

bool ExecuteEvent(SATLAudioObjectData_sdlmixer* const pAudioObject, SATLTriggerImplData_sdlmixer const* const pEventStaticData, SATLEventData_sdlmixer* const pEventInstance)
{
	bool bSuccess = false;

	if (pAudioObject && pEventStaticData && pEventInstance)
	{
		if (pEventStaticData->bStartEvent)
		{
			// start playing samples
			pEventInstance->pStaticData = pEventStaticData;

			Mix_Chunk* pSample = stl::find_in_map(g_sampleData, pEventStaticData->nSampleID, nullptr);
			if (pSample == nullptr)
			{
				// Trying to play sample that hasn't been loaded yet, load it in place
				// NOTE: This should be avoided as it can cause lag in audio playback
				const string& samplePath = g_samplePaths[pEventStaticData->nSampleID];
				if (LoadSampleImpl(pEventStaticData->nSampleID, samplePath))
				{
					pSample = stl::find_in_map(g_sampleData, pEventStaticData->nSampleID, nullptr);
				}
				if (pSample == nullptr)
				{
					return false;
				}
			}

			int nLoopCount = pEventStaticData->nLoopCount;
			if (nLoopCount > 0)
			{
				// For SDL Mixer 0 loops means play only once, 1 loop play twice, etc ...
				--nLoopCount;
			}

			if (!g_freeChannels.empty())
			{
				int nChannelID = Mix_PlayChannel(g_freeChannels.front(), pSample, nLoopCount);
				if (nChannelID >= 0)
				{
					g_freeChannels.pop();
					Mix_Volume(nChannelID, g_bMuted ? 0 : pEventStaticData->nVolume);

					// Get distance and angle from the listener to the audio object
					float fDistance = 0.0f;
					float fAngle = 0.0f;
					GetDistanceAngleToObject(g_listenerPosition, pAudioObject->position, fDistance, fAngle);
					SetChannelPosition(pEventInstance->pStaticData, nChannelID, fDistance, fAngle);

					g_channels[nChannelID].pAudioObject = pAudioObject;
					pEventInstance->channels.push_back(nChannelID);
				}
				else
				{
					g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "Could not play sample. Error: %s", Mix_GetError());
				}
			}
			else
			{
				g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "Ran out of free audio channels. Are you trying to play more than %d samples?", SDL_MIXER_NUM_CHANNELS);
			}

			if (!pEventInstance->channels.empty())
			{
				// if any sample was added then add the event to the audio object
				pAudioObject->events.push_back(pEventInstance);
				bSuccess = true;
			}
		}
		else
		{
			// stop event in audio object
			const TSampleID id = pEventStaticData->nSampleID;
			for (SATLEventData_sdlmixer* pEvent : pAudioObject->events)
			{
				if (pEvent && (id == pEvent->pStaticData->nSampleID))
				{
					StopEvent(pEvent);
				}
			}
		}
	}

	return bSuccess;
}

bool PlayFile(CryAudio::Impl::SATLAudioObjectData_sdlmixer* const pAudioObject
              , CryAudio::Impl::CAudioStandaloneFile_sdlmixer* const pEventInstance
              , const CryAudio::Impl::SATLTriggerImplData_sdlmixer* const pUsedTrigger
              , const char* const szFilePath)
{
	if (!pUsedTrigger)
	{
		return false;
	}

	TSampleID idForThisFile = pEventInstance->fileId;
	Mix_Chunk* pSample = stl::find_in_map(g_sampleData, idForThisFile, nullptr);

	if (!pSample)
	{
		if (LoadSampleImpl(idForThisFile, szFilePath))
		{
			pSample = stl::find_in_map(g_sampleData, idForThisFile, nullptr);
		}
		if (!pSample)
		{
			return false;
		}
	}

	TSampleIdUsageCounterMap::iterator it = g_usageCounters.find(idForThisFile);
	if (it != g_usageCounters.end())
	{
		++it->second;
	}
	else
	{
		g_usageCounters[idForThisFile] = 1;
	}

	if (!g_freeChannels.empty())
	{
		int loopCount = pUsedTrigger->nLoopCount;
		if (loopCount > 0)
		{
			// For SDL Mixer 0 loops means play only once, 1 loop play twice, etc ...
			--loopCount;
		}

		int channelId = Mix_PlayChannel(g_freeChannels.front(), pSample, loopCount);
		if (channelId >= 0)
		{
			g_freeChannels.pop();
			Mix_Volume(channelId, g_bMuted ? 0 : pUsedTrigger->nVolume);

			// Get distance and angle from the listener to the audio object
			float fDistance = 0.0f;
			float fAngle = 0.0f;
			GetDistanceAngleToObject(g_listenerPosition, pAudioObject->position, fDistance, fAngle);
			SetChannelPosition(pUsedTrigger, channelId, fDistance, fAngle);

			g_channels[channelId].pAudioObject = pAudioObject;
			pEventInstance->channels.push_back(channelId);
		}
		else
		{
			g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "Could not play sample. Error: %s", Mix_GetError());
		}
	}
	else
	{
		g_audioImplLogger_sdlmixer.Log(eAudioLogType_Error, "Ran out of free audio channels. Are you trying to play more than %d samples?", SDL_MIXER_NUM_CHANNELS);
	}

	if (!pEventInstance->channels.empty())
	{
		// if any sample was added then add the event to the audio object
		pAudioObject->standaloneFiles.push_back(pEventInstance);
	}

	return true;
}

bool StopFile(CryAudio::Impl::SATLAudioObjectData_sdlmixer* const pAudioObject, AudioStandaloneFileId const fileInstanceID)
{
	bool bResult = false;
	if (pAudioObject)
	{
		for (CAudioStandaloneFile_sdlmixer* const standaloneFileEvent : pAudioObject->standaloneFiles)
		{
			if (standaloneFileEvent->fileInstanceId = fileInstanceID)
			{
				// need to make a copy because the callback
				// registered with Mix_ChannelFinished can edit the list
				TChannelList channels = standaloneFileEvent->channels;
				for (int channel : channels)
				{
					Mix_HaltChannel(channel);
				}
				bResult = true;
			}
		}
	}
	return bResult;
}

bool SetListenerPosition(const TListenerID nListenerID, const CAudioObjectTransformation& position)
{
	g_listenerPosition = position;
	g_bListenerPosChanged = true;
	return true;
}

bool RegisterAudioObject(SATLAudioObjectData_sdlmixer* pAudioObjectData)
{
	if (pAudioObjectData)
	{
		g_audioObjects.push_back(pAudioObjectData);
		return true;
	}
	return false;
}

bool UnregisterAudioObject(SATLAudioObjectData_sdlmixer* pAudioObjectData)
{
	if (pAudioObjectData)
	{
		stl::find_and_erase(g_audioObjects, pAudioObjectData);
		return true;
	}
	return false;
}

bool SetAudioObjectPosition(SATLAudioObjectData_sdlmixer* pAudioObjectData, const CAudioObjectTransformation& position)
{
	if (pAudioObjectData)
	{
		pAudioObjectData->position = position;
		pAudioObjectData->bPositionChanged = true;
		return true;
	}
	return false;
}

bool StopEvent(SATLEventData_sdlmixer const* const pEventInstance)
{
	if (pEventInstance)
	{
		// need to make a copy because the callback
		// registered with Mix_ChannelFinished can edit the list
		TChannelList channels = pEventInstance->channels;
		for (int channel : channels)
		{
			Mix_HaltChannel(channel);
		}
		return true;
	}
	return false;
}

bool StopTrigger(SATLTriggerImplData_sdlmixer const* const pEventData)
{
	bool bResult = false;
	TAudioObjectList::const_iterator audioObjectIt = g_audioObjects.begin();
	const TAudioObjectList::const_iterator audioObjectEnd = g_audioObjects.end();
	for (; audioObjectIt != audioObjectEnd; ++audioObjectIt)
	{
		SATLAudioObjectData_sdlmixer* pAudioObject = *audioObjectIt;
		if (pAudioObject)
		{
			TEventInstanceList::const_iterator eventIt = pAudioObject->events.begin();
			const TEventInstanceList::const_iterator eventEnd = pAudioObject->events.end();
			for (; eventIt != eventEnd; ++eventIt)
			{
				SATLEventData_sdlmixer* pEventInstance = *eventIt;
				if (pEventInstance && pEventInstance->pStaticData == pEventData)
				{
					StopEvent(pEventInstance);
					bResult = true;
				}
			}
		}
	}
	return bResult;
}

SATLTriggerImplData_sdlmixer* CreateEventData()
{
	SATLTriggerImplData_sdlmixer* pNewTriggerImpl = nullptr;
	POOL_NEW(SATLTriggerImplData_sdlmixer, pNewTriggerImpl)();
	return pNewTriggerImpl;
}

void Update()
{
	ProcessChannelFinishedRequests(g_channelFinishedRequests[eCFRQID_TWO]);
	{
		CryAutoLock<CryCriticalSection> oAutoLock(g_channelFinishedCriticalSection);
		g_channelFinishedRequests[eCFRQID_ONE].swap(g_channelFinishedRequests[eCFRQID_TWO]);
	}

	TAudioObjectList::const_iterator audioObjectIt = g_audioObjects.begin();
	const TAudioObjectList::const_iterator audioObjectEnd = g_audioObjects.end();
	for (; audioObjectIt != audioObjectEnd; ++audioObjectIt)
	{
		SATLAudioObjectData_sdlmixer* pAudioObject = *audioObjectIt;
		if (pAudioObject && (pAudioObject->bPositionChanged || g_bListenerPosChanged))
		{
			// Get distance and angle from the listener to the audio object
			float fDistance = 0.0f;
			float fAngle = 0.0f;
			GetDistanceAngleToObject(g_listenerPosition, pAudioObject->position, fDistance, fAngle);
			const uint8 nSDLMaxDistance = 255;

			TEventInstanceList::const_iterator eventIt = pAudioObject->events.begin();
			const TEventInstanceList::const_iterator eventEnd = pAudioObject->events.end();
			for (; eventIt != eventEnd; ++eventIt)
			{
				SATLEventData_sdlmixer* pEventInstance = *eventIt;
				if (pEventInstance && pEventInstance->pStaticData)
				{
					for (int channelIndex : pEventInstance->channels)
					{
						SetChannelPosition(pEventInstance->pStaticData, channelIndex, fDistance, fAngle);
					}
				}
			}
			pAudioObject->bPositionChanged = false;
		}
	}
	g_bListenerPosChanged = false;
}
}
}
