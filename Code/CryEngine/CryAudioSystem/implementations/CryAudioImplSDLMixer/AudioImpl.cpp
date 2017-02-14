// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "AudioImplCVars.h"
#include "SoundEngineUtil.h"
#include "SoundEngineTypes.h"
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>
#include <CryAudio/IAudioSystem.h>
#include <CrySystem/IProjectManager.h>

// SDL Mixer
#include <SDL_mixer.h>

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::SDL_mixer;

char const* const CAudioImpl::s_szSDLFileTag = "SDLMixerSample";
char const* const CAudioImpl::s_szSDLCommonAttribute = "sdl_name";
char const* const CAudioImpl::s_szSDLPathAttribute = "sdl_path";
char const* const CAudioImpl::s_szSDLEventIdTag = "event_id";
char const* const CAudioImpl::s_szSDLEventTag = "SDLMixerEvent";
char const* const CAudioImpl::s_szSDLSoundLibraryPath = CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "sdlmixer" CRY_NATIVE_PATH_SEPSTR;
char const* const CAudioImpl::s_szSDLEventTypeTag = "event_type";
char const* const CAudioImpl::s_szSDLEventPanningEnabledTag = "enable_panning";
char const* const CAudioImpl::s_szSDLEventAttenuationEnabledTag = "enable_distance_attenuation";
char const* const CAudioImpl::s_szSDLEventAttenuationMinDistanceTag = "attenuation_dist_min";
char const* const CAudioImpl::s_szSDLEventAttenuationMaxDistanceTag = "attenuation_dist_max";
char const* const CAudioImpl::s_szSDLEventVolumeTag = "volume";
char const* const CAudioImpl::s_szSDLEventLoopCountTag = "loop_count";

///////////////////////////////////////////////////////////////////////////
void OnEventFinished(CATLEvent& audioEvent)
{
	gEnv->pAudioSystem->ReportFinishedEvent(audioEvent, true);
}

///////////////////////////////////////////////////////////////////////////
void OnStandaloneFileFinished(CATLStandaloneFile& standaloneFile, const char* szFile)
{
	gEnv->pAudioSystem->ReportStoppedFile(standaloneFile);
}

///////////////////////////////////////////////////////////////////////////
CAudioImpl::CAudioImpl()
	: m_pCVarFileExtension(nullptr)
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	char const* const szAssetDirectory = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryRelative();
	if (strlen(szAssetDirectory) == 0)
	{
		CryFatalError("<Audio - SDLMixer>: Needs a valid asset folder to proceed!");
	}

	m_fullImplString = "SDL Mixer 2.0.1 (";
	m_fullImplString += szAssetDirectory + PathUtil::RemoveSlash(s_szSDLSoundLibraryPath) + ")";
#endif      // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

#if CRY_PLATFORM_WINDOWS
	m_memoryAlignment = 16;
#elif CRY_PLATFORM_MAC
	m_memoryAlignment = 16;
#elif CRY_PLATFORM_LINUX
	m_memoryAlignment = 16;
#elif CRY_PLATFORM_IOS
	m_memoryAlignment = 16;
#elif CRY_PLATFORM_ANDROID
	m_memoryAlignment = 16;
#else
	#error "Undefined platform."
#endif
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::Update(float const deltaTime)
{
	SoundEngine::Update();
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "SDL Mixer Object Pool");
	SAudioObject::CreateAllocator(audioObjectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "SDL Mixer Event Pool");
	SAudioEvent::CreateAllocator(eventPoolSize);

	m_pCVarFileExtension = REGISTER_STRING("s_SDLMixerStandaloneFileExtension", ".mp3", 0, "the expected file extension for standalone files, played via the sdl_mixer");

	if (SoundEngine::Init())
	{
		SoundEngine::RegisterEventFinishedCallback(OnEventFinished);
		SoundEngine::RegisterStandaloneFileFinishedCallback(OnStandaloneFileFinished);
		return eRequestStatus_Success;
	}
	return eRequestStatus_Failure;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnBeforeShutDown()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ShutDown()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Release()
{
	if (m_pCVarFileExtension)
	{
		m_pCVarFileExtension->Release();
		m_pCVarFileExtension = nullptr;
	}

	SoundEngine::Release();
	delete this;

	g_audioImplCVars.UnregisterVariables();

	SAudioObject::FreeMemoryPool();
	SAudioEvent::FreeMemoryPool();

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::OnAudioSystemRefresh()
{
	SoundEngine::Refresh();
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnLoseFocus()
{
	SoundEngine::Pause();
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnGetFocus()
{
	SoundEngine::Resume();
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::MuteAll()
{
	SoundEngine::Mute();
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnmuteAll()
{
	SoundEngine::UnMute();
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::StopAllSounds()
{
	SoundEngine::Stop();
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::RegisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry)
{
	ERequestStatus result = eRequestStatus_Failure;

	if (pAudioFileEntry != nullptr)
	{
		SAudioFileEntry* const pFileData = static_cast<SAudioFileEntry*>(pAudioFileEntry->pImplData);

		if (pFileData != nullptr)
		{
			pFileData->sampleId = SoundEngine::LoadSampleFromMemory(pAudioFileEntry->pFileData, pAudioFileEntry->size, pAudioFileEntry->szFileName);
			result = eRequestStatus_Success;
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the SDL Mixer implementation of RegisterInMemoryFile");
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnregisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry)
{
	ERequestStatus result = eRequestStatus_Failure;

	if (pAudioFileEntry != nullptr)
	{
		SAudioFileEntry* const pFileData = static_cast<SAudioFileEntry*>(pAudioFileEntry->pImplData);

		if (pFileData != nullptr)
		{
			SoundEngine::UnloadSample(pFileData->sampleId);
			result = eRequestStatus_Success;
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the SDL Mixer implementation of UnregisterInMemoryFile");
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ParseAudioFileEntry(XmlNodeRef const pAudioFileEntryNode, SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus result = eRequestStatus_Failure;

	if ((_stricmp(pAudioFileEntryNode->getTag(), s_szSDLFileTag) == 0) && (pFileEntryInfo != nullptr))
	{
		char const* const szFileName = pAudioFileEntryNode->getAttr(s_szSDLCommonAttribute);
		char const* const szPath = pAudioFileEntryNode->getAttr(s_szSDLPathAttribute);
		CryFixedStringT<MaxFilePathLength> fullFilePath;
		if (szPath)
		{
			fullFilePath = szPath;
			fullFilePath += CRY_NATIVE_PATH_SEPSTR;
			fullFilePath += szFileName;
		}
		else
		{
			fullFilePath = szFileName;
		}

		// Currently the SDLMixer Implementation does not support localized files.
		pFileEntryInfo->bLocalized = false;

		if (!fullFilePath.empty())
		{
			pFileEntryInfo->szFileName = fullFilePath.c_str();
			pFileEntryInfo->memoryBlockAlignment = m_memoryAlignment;
			pFileEntryInfo->pImplData = new SAudioFileEntry();
			result = eRequestStatus_Success;
		}
		else
		{
			pFileEntryInfo->szFileName = nullptr;
			pFileEntryInfo->memoryBlockAlignment = 0;
			pFileEntryInfo->pImplData = nullptr;
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioFileEntry(IAudioFileEntry* const pIFileEntry)
{
	delete pIFileEntry;
}

///////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetAudioFileLocation(SAudioFileEntryInfo* const pFileEntryInfo)
{
	static CryFixedStringT<MaxFilePathLength> s_path;
	s_path = PathUtil::GetGameFolder().c_str();
	s_path += s_szSDLSoundLibraryPath;

	return s_path.c_str();
}

///////////////////////////////////////////////////////////////////////////
IAudioTrigger const* CAudioImpl::NewAudioTrigger(XmlNodeRef const pAudioTriggerNode)
{
	SAudioTrigger* pNewTriggerImpl = nullptr;
	if (_stricmp(pAudioTriggerNode->getTag(), s_szSDLEventTag) == 0)
	{
		pNewTriggerImpl = SoundEngine::CreateEventData();
		if (pNewTriggerImpl)
		{
			char const* const szFileName = pAudioTriggerNode->getAttr(s_szSDLCommonAttribute);
			char const* const szPath = pAudioTriggerNode->getAttr(s_szSDLPathAttribute);
			string fullFilePath;
			if (szPath)
			{
				fullFilePath = szPath;
				fullFilePath += CRY_NATIVE_PATH_SEPSTR;
				fullFilePath += szFileName;
			}
			else
			{
				fullFilePath = szFileName;
			}

			pNewTriggerImpl->sampleId = SoundEngine::LoadSample(fullFilePath, true);

			pNewTriggerImpl->bStartEvent = (_stricmp(pAudioTriggerNode->getAttr(s_szSDLEventTypeTag), "stop") != 0);
			if (pNewTriggerImpl->bStartEvent)
			{
				pNewTriggerImpl->bPanningEnabled = (_stricmp(pAudioTriggerNode->getAttr(s_szSDLEventPanningEnabledTag), "true") == 0);
				bool bAttenuationEnabled = (_stricmp(pAudioTriggerNode->getAttr(s_szSDLEventAttenuationEnabledTag), "true") == 0);
				if (bAttenuationEnabled)
				{
					pAudioTriggerNode->getAttr(s_szSDLEventAttenuationMinDistanceTag, pNewTriggerImpl->attenuationMinDistance);
					pAudioTriggerNode->getAttr(s_szSDLEventAttenuationMaxDistanceTag, pNewTriggerImpl->attenuationMaxDistance);
				}
				else
				{
					pNewTriggerImpl->attenuationMinDistance = -1.0f;
					pNewTriggerImpl->attenuationMaxDistance = -1.0f;
				}

				// Translate decibel to normalized value.
				static const int maxVolume = 128;
				float volume = 0.0f;
				pAudioTriggerNode->getAttr(s_szSDLEventVolumeTag, volume);
				pNewTriggerImpl->volume = static_cast<int>(pow_tpl(10.0f, volume / 20.0f) * maxVolume);

				pAudioTriggerNode->getAttr(s_szSDLEventLoopCountTag, pNewTriggerImpl->loopCount);
			}
		}
	}
	return pNewTriggerImpl;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioTrigger(IAudioTrigger const* const pITrigger)
{
	SAudioTrigger const* const pTrigger = static_cast<SAudioTrigger const* const>(pITrigger);
	if (pTrigger != nullptr)
	{
		SoundEngine::StopTrigger(pTrigger);
	}
	delete pITrigger;
}

///////////////////////////////////////////////////////////////////////////
IParameter const* CAudioImpl::NewAudioParameter(XmlNodeRef const pAudioParameterNode)
{
	return new SAudioParameter();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioParameter(IParameter const* const pIParameter)
{
	delete pIParameter;
}

///////////////////////////////////////////////////////////////////////////
IAudioSwitchState const* CAudioImpl::NewAudioSwitchState(XmlNodeRef const pAudioSwitchStateNode)
{
	return new SAudioSwitchState();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioSwitchState(IAudioSwitchState const* const pISwitchState)
{
	delete pISwitchState;
}

///////////////////////////////////////////////////////////////////////////
IAudioEnvironment const* CAudioImpl::NewAudioEnvironment(XmlNodeRef const pAudioEnvironmentNode)
{
	return new SAudioEnvironment();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioEnvironment(IAudioEnvironment const* const pIEnvironment)
{
	delete pIEnvironment;
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructGlobalAudioObject()
{
	return new SAudioObject(0);
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructAudioObject(char const* const szAudioObjectName)
{
	static uint32 objectIDCounter = 1;
	SAudioObject* pSdlMixerObject = new SAudioObject(objectIDCounter++);

	SoundEngine::RegisterAudioObject(pSdlMixerObject);

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	m_idToName[pSdlMixerObject->audioObjectId] = szAudioObjectName;
#endif

	return pSdlMixerObject;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioObject(IAudioObject const* const pObjectData)
{
	SAudioObject const* pSdlMixerObject = static_cast<SAudioObject const*>(pObjectData);
	if (pSdlMixerObject)
	{
		SoundEngine::UnregisterAudioObject(pSdlMixerObject);
	}

	delete pSdlMixerObject;
}

///////////////////////////////////////////////////////////////////////////
IAudioListener* CAudioImpl::ConstructAudioListener()
{
	static ListenerId listenerIDCounter = 0;
	return new SAudioListener(listenerIDCounter++);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioListener(IAudioListener* const pIListener)
{
	delete pIListener;
}

///////////////////////////////////////////////////////////////////////////
IAudioEvent* CAudioImpl::ConstructAudioEvent(CATLEvent& audioEvent)
{
	return new SAudioEvent(audioEvent);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioEvent(IAudioEvent const* const pAudioEvent)
{
	delete pAudioEvent;
}

///////////////////////////////////////////////////////////////////////////
IAudioStandaloneFile* CAudioImpl::ConstructAudioStandaloneFile(CATLStandaloneFile& atlStandaloneFile, char const* const szFile, bool const bLocalized, IAudioTrigger const* pTrigger /*= nullptr*/)
{
	static string s_localizedfilesFolder = PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR + PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + m_language + CRY_NATIVE_PATH_SEPSTR;
	static string s_nonLocalizedfilesFolder = PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR;
	static string filePath;

	if (bLocalized)
	{
		filePath = s_localizedfilesFolder + szFile + m_pCVarFileExtension->GetString();
	}
	else
	{
		filePath = s_nonLocalizedfilesFolder + szFile + m_pCVarFileExtension->GetString();
	}

	return new CAudioStandaloneFile(filePath, atlStandaloneFile);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioStandaloneFile(IAudioStandaloneFile const* const pIFile)
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	const CAudioStandaloneFile* const pStandaloneEvent = static_cast<const CAudioStandaloneFile*>(pIFile);
	CRY_ASSERT_MESSAGE(pStandaloneEvent->channels.size() == 0, "Events always have to be stopped/finished before they get deleted");
#endif
	delete pIFile;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadConnected(TAudioGamepadUniqueID const deviceUniqueID)
{}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadDisconnected(TAudioGamepadUniqueID const deviceUniqueID)
{}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::SetLanguage(char const* const szLanguage)
{
	m_language = szLanguage;
}

///////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetImplementationNameString() const
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	return m_fullImplString.c_str();
#endif      // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const
{
	CryModuleMemoryInfo memInfo;
	ZeroStruct(memInfo);
	CryGetMemoryInfoForModule(&memInfo);

	memoryInfo.totalMemory = static_cast<size_t>(memInfo.allocated - memInfo.freed);
	memoryInfo.secondaryPoolSize = 0;
	memoryInfo.secondaryPoolUsedSize = 0;
	memoryInfo.secondaryPoolAllocations = 0;

	{
		auto& allocator = SAudioObject::GetAllocator();
		auto mem = allocator.GetTotalMemory();
		auto pool = allocator.GetCounts();
		memoryInfo.poolUsedObjects = pool.nUsed;
		memoryInfo.poolConstructedObjects = pool.nAlloc;
		memoryInfo.poolUsedMemory = mem.nUsed;
		memoryInfo.poolAllocatedMemory = mem.nAlloc;
	}

	{
		auto& allocator = SAudioEvent::GetAllocator();
		auto mem = allocator.GetTotalMemory();
		auto pool = allocator.GetCounts();
		memoryInfo.poolUsedObjects += pool.nUsed;
		memoryInfo.poolConstructedObjects += pool.nAlloc;
		memoryInfo.poolUsedMemory += mem.nUsed;
		memoryInfo.poolAllocatedMemory += mem.nAlloc;
	}
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const
{}
