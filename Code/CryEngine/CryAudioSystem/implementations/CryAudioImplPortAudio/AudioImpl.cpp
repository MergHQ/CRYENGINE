// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "Common.h"
#include "AudioTrigger.h"
#include "AudioEvent.h"
#include "AudioObject.h"
#include "AudioImplCVars.h"
#include "ATLEntities.h"
#include <sndfile.hh>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>
#include <CryString/CryPath.h>

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::PortAudio;

char const* const CAudioImpl::s_szPortAudioEventTag = "PortAudioEvent";
char const* const CAudioImpl::s_szPortAudioEventNameAttribute = "portaudio_name";
char const* const CAudioImpl::s_szPortAudioEventTypeAttribute = "event_type";
char const* const CAudioImpl::s_szPortAudioEventNumLoopsAttribute = "num_loops";

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Port Audio Object Pool");
	CAudioObject::CreateAllocator(audioObjectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Port Audio Event Pool");
	CAudioEvent::CreateAllocator(eventPoolSize);

	char const* const szAssetDirectory = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryRelative();
	if (strlen(szAssetDirectory) == 0)
	{
		CryFatalError("<Audio - PortAudio>: Needs a valid asset folder to proceed!");
	}

	m_regularSoundBankFolder = szAssetDirectory;
	m_regularSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
	m_regularSoundBankFolder += PORTAUDIO_IMPL_DATA_ROOT;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

	PaError const err = Pa_Initialize();

	if (err != paNoError)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Failed to initialize PortAudio: %s", Pa_GetErrorText(err));
	}

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	m_fullImplString = Pa_GetVersionText();
	m_fullImplString += " (";
	m_fullImplString += szAssetDirectory;
	m_fullImplString += CRY_NATIVE_PATH_SEPSTR PORTAUDIO_IMPL_DATA_ROOT ")";
#endif // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnBeforeShutDown()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ShutDown()
{
	PaError const err = Pa_Terminate();

	if (err != paNoError)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Failed to shut down PortAudio: %s", Pa_GetErrorText(err));
	}

	return (err == paNoError) ? ERequestStatus::Success : ERequestStatus::Failure;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Release()
{
	delete this;
	g_audioImplCVars.UnregisterVariables();

	CAudioObject::FreeMemoryPool();
	CAudioEvent::FreeMemoryPool();

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnLoseFocus()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnGetFocus()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::MuteAll()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnmuteAll()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::StopAllSounds()
{
	return ERequestStatus::Success;
}
//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::RegisterInMemoryFile(SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if (pFileEntryInfo != nullptr)
	{
		CAudioFileEntry* const pPAAudioFileEntry = static_cast<CAudioFileEntry*>(pFileEntryInfo->pImplData);

		if (pPAAudioFileEntry != nullptr)
		{
			requestResult = ERequestStatus::Success;
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the PortAudio implementation of RegisterInMemoryFile");
		}
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnregisterInMemoryFile(SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if (pFileEntryInfo != nullptr)
	{
		CAudioFileEntry* const pPAAudioFileEntry = static_cast<CAudioFileEntry*>(pFileEntryInfo->pImplData);

		if (pPAAudioFileEntry != nullptr)
		{
			requestResult = ERequestStatus::Success;
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the PortAudio implementation of UnregisterInMemoryFile");
		}
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ParseAudioFileEntry(
  XmlNodeRef const pAudioFileEntryNode,
  SAudioFileEntryInfo* const pFileEntryInfo)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioFileEntry(IAudioFileEntry* const pIFileEntry)
{
	delete pIFileEntry;
}

//////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetAudioFileLocation(SAudioFileEntryInfo* const pFileEntryInfo)
{
	char const* sResult = nullptr;

	if (pFileEntryInfo != nullptr)
	{
		sResult = pFileEntryInfo->bLocalized ? m_localizedSoundBankFolder.c_str() : m_regularSoundBankFolder.c_str();
	}

	return sResult;
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructGlobalAudioObject()
{
	return new CAudioObject();
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructAudioObject(char const* const szAudioObjectName)
{
	CAudioObject* pPAAudioObject = new CAudioObject();
	stl::push_back_unique(m_constructedAudioObjects, pPAAudioObject);

	return pPAAudioObject;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioObject(IAudioObject const* const pAudioObject)
{
	stl::find_and_erase(m_constructedAudioObjects, static_cast<CAudioObject const*>(pAudioObject));
	delete pAudioObject;
}

///////////////////////////////////////////////////////////////////////////
IAudioListener* CAudioImpl::ConstructAudioListener()
{
	return new CAudioListener();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioListener(IAudioListener* const pIListener)
{
	delete pIListener;
}

//////////////////////////////////////////////////////////////////////////
IAudioEvent* CAudioImpl::ConstructAudioEvent(CATLEvent& audioEvent)
{
	return new CAudioEvent(audioEvent);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioEvent(IAudioEvent const* const pAudioEvent)
{
	delete pAudioEvent;
}

//////////////////////////////////////////////////////////////////////////
IAudioStandaloneFile* CAudioImpl::ConstructAudioStandaloneFile(CATLStandaloneFile& atlStandaloneFile, char const* const szFile, bool const bLocalized, IAudioTrigger const* pTrigger /*= nullptr*/)
{
	return new CAudioStandaloneFile();
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioStandaloneFile(IAudioStandaloneFile const* const pIFile)
{
	delete pIFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadConnected(TAudioGamepadUniqueID const deviceUniqueID)
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadDisconnected(TAudioGamepadUniqueID const deviceUniqueID)
{
}

///////////////////////////////////////////////////////////////////////////
IAudioTrigger const* CAudioImpl::NewAudioTrigger(XmlNodeRef const pAudioTriggerNode)
{
	CAudioTrigger* pAudioTrigger = nullptr;
	char const* const szTag = pAudioTriggerNode->getTag();

	if (_stricmp(szTag, s_szPortAudioEventTag) == 0)
	{
		stack_string path = m_regularSoundBankFolder.c_str();
		path += CRY_NATIVE_PATH_SEPSTR;
		path += pAudioTriggerNode->getAttr(s_szPortAudioEventNameAttribute);

		if (!path.empty())
		{
			SF_INFO sfInfo;
			ZeroStruct(sfInfo);

			SNDFILE* const pSndFile = sf_open(path.c_str(), SFM_READ, &sfInfo);

			if (pSndFile != nullptr)
			{
				CryFixedStringT<16> const eventTypeString(pAudioTriggerNode->getAttr(s_szPortAudioEventTypeAttribute));
				EPortAudioEventType const eventType = eventTypeString.compareNoCase("start") == 0 ? EPortAudioEventType::Start : EPortAudioEventType::Stop;

				// numLoops -1 == infinite, 0 == once, 1 == twice etc
				int numLoops = 0;
				pAudioTriggerNode->getAttr(s_szPortAudioEventNumLoopsAttribute, numLoops);
				PaStreamParameters streamParameters;
				streamParameters.device = Pa_GetDefaultOutputDevice();
				streamParameters.channelCount = sfInfo.channels;
				streamParameters.suggestedLatency = Pa_GetDeviceInfo(streamParameters.device)->defaultLowOutputLatency;
				streamParameters.hostApiSpecificStreamInfo = nullptr;
				int const subFormat = sfInfo.format & SF_FORMAT_SUBMASK;

				switch (subFormat)
				{
				case SF_FORMAT_PCM_16:
				case SF_FORMAT_PCM_24:
					streamParameters.sampleFormat = paInt16;
					break;
				case SF_FORMAT_PCM_32:
					streamParameters.sampleFormat = paInt32;
					break;
				case SF_FORMAT_FLOAT:
				case SF_FORMAT_VORBIS:
					streamParameters.sampleFormat = paFloat32;
					break;
				}

				pAudioTrigger = new CAudioTrigger(
				  AudioStringToId(path.c_str()),
				  numLoops,
				  static_cast<double>(sfInfo.samplerate),
				  eventType,
				  path.c_str(),
				  streamParameters);

				int const failure = sf_close(pSndFile);

				if (failure)
				{
					g_audioImplLogger.Log(eAudioLogType_Error, "Failed to close SNDFILE during CAudioImpl::NewAudioTrigger");
				}
			}
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown PortAudio tag: %s", szTag);
	}

	return pAudioTrigger;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioTrigger(IAudioTrigger const* const pITrigger)
{
	delete pITrigger;
}

///////////////////////////////////////////////////////////////////////////
IParameter const* CAudioImpl::NewAudioParameter(XmlNodeRef const pAudioParameterNode)
{
	return new CAudioParameter();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioParameter(IParameter const* const pIParameter)
{
	delete pIParameter;
}

///////////////////////////////////////////////////////////////////////////
IAudioSwitchState const* CAudioImpl::NewAudioSwitchState(XmlNodeRef const pAudioSwitchNode)
{
	return new CAudioSwitchState();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioSwitchState(IAudioSwitchState const* const pISwitchState)
{
	delete pISwitchState;
}

///////////////////////////////////////////////////////////////////////////
IAudioEnvironment const* CAudioImpl::NewAudioEnvironment(XmlNodeRef const pAudioEnvironmentNode)
{
	return new CAudioEnvironment();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioEnvironment(IAudioEnvironment const* const pIEnvironment)
{
	delete pIEnvironment;
}

///////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetImplementationNameString() const
{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	return m_fullImplString.c_str();
#endif // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

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
		auto& allocator = CAudioObject::GetAllocator();
		auto mem = allocator.GetTotalMemory();
		auto pool = allocator.GetCounts();
		memoryInfo.poolUsedObjects = pool.nUsed;
		memoryInfo.poolConstructedObjects = pool.nAlloc;
		memoryInfo.poolUsedMemory = mem.nUsed;
		memoryInfo.poolAllocatedMemory = mem.nAlloc;
	}

	{
		auto& allocator = CAudioEvent::GetAllocator();
		auto mem = allocator.GetTotalMemory();
		auto pool = allocator.GetCounts();
		memoryInfo.poolUsedObjects += pool.nUsed;
		memoryInfo.poolConstructedObjects += pool.nAlloc;
		memoryInfo.poolUsedMemory += mem.nUsed;
		memoryInfo.poolAllocatedMemory += mem.nAlloc;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::OnAudioSystemRefresh()
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::SetLanguage(char const* const szLanguage)
{
	if (szLanguage != nullptr)
	{
		m_localizedSoundBankFolder = PathUtil::GetGameFolder().c_str();
		m_localizedSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
		m_localizedSoundBankFolder += PathUtil::GetLocalizationFolder();
		m_localizedSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
		m_localizedSoundBankFolder += szLanguage;
		m_localizedSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
		m_localizedSoundBankFolder += PORTAUDIO_IMPL_DATA_ROOT;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const
{
}
