// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "Common.h"
#include "Event.h"
#include "EventInstance.h"
#include "Object.h"
#include "CVars.h"
#include "Listener.h"
#include "GlobalData.h"

#include <IEnvironmentConnection.h>
#include <IFile.h>
#include <IParameterConnection.h>
#include <ISettingConnection.h>
#include <ISwitchStateConnection.h>
#include <FileInfo.h>
#include <sndfile.hh>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	#include <Logger.h>
	#include <DebugStyle.h>
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
std::vector<CObject*> g_constructedObjects;
std::vector<CEvent*> g_events;

uint16 g_eventsPoolSize = 0;
uint16 g_eventPoolSizeLevelSpecific = 0;

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
uint16 g_debugEventrPoolSize = 0;
uint16 g_objectPoolSize = 0;
uint16 g_eventInstancePoolSize = 0;
EventInstances g_constructedEventInstances;
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CountPoolSizes(XmlNodeRef const pNode, uint16& poolSizes)
{
	uint16 numEvents = 0;
	pNode->getAttr(g_szEventsAttribute, numEvents);
	poolSizes += numEvents;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	CObject::CreateAllocator(objectPoolSize);
	CEventInstance::CreateAllocator(eventPoolSize);
	CEvent::CreateAllocator(g_eventsPoolSize);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CObject::FreeMemoryPool();
	CEventInstance::FreeMemoryPool();
	CEvent::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
bool GetSoundInfo(char const* const szPath, SF_INFO& sfInfo, PaStreamParameters& streamParameters)
{
	bool success = false;

	ZeroStruct(sfInfo);
	SNDFILE* const pSndFile = sf_open(szPath, SFM_READ, &sfInfo);

	if (pSndFile != nullptr)
	{
		streamParameters.device = Pa_GetDefaultOutputDevice();
		streamParameters.channelCount = sfInfo.channels;
		streamParameters.suggestedLatency = Pa_GetDeviceInfo(streamParameters.device)->defaultLowOutputLatency;
		streamParameters.hostApiSpecificStreamInfo = nullptr;
		int const subFormat = sfInfo.format & SF_FORMAT_SUBMASK;

		switch (subFormat)
		{
		case SF_FORMAT_PCM_16: // Intentional fall-through.
		case SF_FORMAT_PCM_24:
			streamParameters.sampleFormat = paInt16;
			break;
		case SF_FORMAT_PCM_32:
			streamParameters.sampleFormat = paInt32;
			break;
		case SF_FORMAT_FLOAT: // Intentional fall-through.
		case SF_FORMAT_VORBIS:
			streamParameters.sampleFormat = paFloat32;
			break;
		default:
			break;
		}

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
		if (sf_close(pSndFile) != 0)
		{
			Cry::Audio::Log(ELogType::Error, "Failed to close sound file %s", szPath);
		}
#else
		sf_close(pSndFile);
#endif    // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

		success = true;
	}
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Failed to open sound file %s", szPath);
	}
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	return success;
}

//////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
{
	g_pImpl = this;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize)
{
	if (g_cvars.m_eventPoolSize < 1)
	{
		g_cvars.m_eventPoolSize = 1;

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, R"(Event pool size must be at least 1. Forcing the cvar "s_PortAudioEventPoolSize" to 1!)");
#endif    // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE
	}

	g_constructedObjects.reserve(static_cast<size_t>(objectPoolSize));
	AllocateMemoryPools(objectPoolSize, static_cast<uint16>(g_cvars.m_eventPoolSize));

	char const* szAssetDirectory = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryRelative();

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	g_objectPoolSize = objectPoolSize;
	g_eventInstancePoolSize = static_cast<uint16>(g_cvars.m_eventPoolSize);

	g_constructedEventInstances.reserve(static_cast<size_t>(g_cvars.m_eventPoolSize));

	if (strlen(szAssetDirectory) == 0)
	{
		Cry::Audio::Log(ELogType::Error, "<Audio - PortAudio>: No asset folder set!");
		szAssetDirectory = "no-asset-folder-set";
	}

	m_name = Pa_GetVersionText();
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	m_regularSoundBankFolder = szAssetDirectory;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += CRY_AUDIO_DATA_ROOT;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += g_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += g_szAssetsFolderName;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

	if (ICVar* const pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		SetLanguage(pCVar->GetString());
	}

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	PaError const err = Pa_Initialize();

	if (err != paNoError)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to initialize PortAudio: %s", Pa_GetErrorText(err));
	}
#else
	Pa_Initialize();
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	PaError const err = Pa_Terminate();

	if (err != paNoError)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to shut down PortAudio: %s", Pa_GetErrorText(err));
	}
#else
	Pa_Terminate();
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::Release()
{
	delete this;
	g_pImpl = nullptr;
	g_cvars.UnregisterVariables();

	FreeMemoryPools();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLibraryData(XmlNodeRef const pNode, bool const isLevelSpecific)
{
	if (isLevelSpecific)
	{
		uint16 eventLevelPoolSize;
		CountPoolSizes(pNode, eventLevelPoolSize);

		g_eventPoolSizeLevelSpecific = std::max(g_eventPoolSizeLevelSpecific, eventLevelPoolSize);
	}
	else
	{
		CountPoolSizes(pNode, g_eventsPoolSize);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeLibraryDataChanged()
{
	g_eventsPoolSize = 0;
	g_eventPoolSizeLevelSpecific = 0;

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	g_debugEventrPoolSize = 0;
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_eventsPoolSize += g_eventPoolSizeLevelSpecific;

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugEventrPoolSize = g_eventsPoolSize;
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	g_eventsPoolSize = std::max<uint16>(1, g_eventsPoolSize);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnLoseFocus()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnGetFocus()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::MuteAll()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::UnmuteAll()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::PauseAll()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ResumeAll()
{
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::StopAllSounds()
{
	for (auto const pObject : g_constructedObjects)
	{
		pObject->StopAllTriggers();
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ConstructFile(XmlNodeRef const pRootNode, SFileInfo* const pFileInfo)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructFile(IFile* const pIFile)
{
	delete pIFile;
}

//////////////////////////////////////////////////////////////////////////
char const* const CImpl::GetFileLocation(SFileInfo* const pFileInfo)
{
	char const* szResult = nullptr;

	if (pFileInfo != nullptr)
	{
		szResult = pFileInfo->bLocalized ? m_localizedSoundBankFolder.c_str() : m_regularSoundBankFolder.c_str();
	}

	return szResult;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetInfo(SImplInfo& implInfo) const
{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE
	implInfo.folderName = g_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CObject");
	auto pObject = new CObject();

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	pObject->SetName("Global Object");
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	stl::push_back_unique(g_constructedObjects, pObject);

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CObject");
	auto const pObject = new CObject();

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	pObject->SetName(szName);
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	stl::push_back_unique(g_constructedObjects, pObject);

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pObject = static_cast<CObject const*>(pIObject);
	stl::find_and_erase(g_constructedObjects, pObject);
	delete pObject;
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CListener");
	g_pListener = new CListener(transformation);

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	g_pListener->SetName(szName);
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	return static_cast<IListener*>(g_pListener);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructListener(IListener* const pIListener)
{
	CRY_ASSERT_MESSAGE(pIListener == g_pListener, "pIListener is not g_pListener during %s", __FUNCTION__);
	delete g_pListener;
	g_pListener = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadConnected(DeviceId const deviceUniqueID)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadDisconnected(DeviceId const deviceUniqueID)
{
}

///////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(XmlNodeRef const pRootNode, float& radius)
{
	ITriggerConnection* pITriggerConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szEventTag) == 0)
	{
		char const* const szLocalized = pRootNode->getAttr(g_szLocalizedAttribute);
		bool const isLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, g_szTrueValue) == 0);

		stack_string path = (isLocalized ? m_localizedSoundBankFolder.c_str() : m_regularSoundBankFolder.c_str());
		path += "/";
		stack_string const folderName = pRootNode->getAttr(g_szPathAttribute);

		if (!folderName.empty())
		{
			path += folderName.c_str();
			path += "/";
		}

		stack_string const name = pRootNode->getAttr(g_szNameAttribute);
		path += name.c_str();

		SF_INFO sfInfo;
		PaStreamParameters streamParameters;

		if (GetSoundInfo(path.c_str(), sfInfo, streamParameters))
		{
			CryFixedStringT<16> const eventTypeString(pRootNode->getAttr(g_szTypeAttribute));
			CEvent::EActionType const actionType = eventTypeString.compareNoCase(g_szStartValue) == 0 ? CEvent::EActionType::Start : CEvent::EActionType::Stop;

			int numLoops = 0;
			pRootNode->getAttr(g_szLoopCountAttribute, numLoops);
			// --numLoops because -1: play infinite, 0: play once, 1: play twice, etc...
			--numLoops;
			// Max to -1 to stay backwards compatible.
			numLoops = std::max(-1, numLoops);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CEvent");
			auto const pEvent = new CEvent(
				StringToId(path.c_str()),
				numLoops,
				static_cast<double>(sfInfo.samplerate),
				actionType,
				path.c_str(),
				streamParameters,
				folderName.c_str(),
				name.c_str(),
				isLocalized);

			g_events.push_back(pEvent);

			pITriggerConnection = static_cast<ITriggerConnection*>(pEvent);
		}
	}
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown PortAudio tag: %s", szTag);
	}
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	return pITriggerConnection;
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	ITriggerConnection* pITriggerConnection = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		stack_string path = (pTriggerInfo->isLocalized ? m_localizedSoundBankFolder.c_str() : m_regularSoundBankFolder.c_str());
		path += "/";
		stack_string const folderName = pTriggerInfo->path.c_str();

		if (!folderName.empty())
		{
			path += folderName.c_str();
			path += "/";
		}

		stack_string const name = pTriggerInfo->name.c_str();
		path += name.c_str();

		SF_INFO sfInfo;
		PaStreamParameters streamParameters;

		if (GetSoundInfo(path.c_str(), sfInfo, streamParameters))
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CEvent");
			pITriggerConnection = static_cast<ITriggerConnection*>(
				new CEvent(
					StringToId(path.c_str()),
					0,
					static_cast<double>(sfInfo.samplerate),
					CEvent::EActionType::Start,
					path.c_str(),
					streamParameters,
					folderName.c_str(),
					name.c_str(),
					pTriggerInfo->isLocalized));
		}
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	auto const pEvent = static_cast<CEvent const*>(pITriggerConnection);
	pEvent->SetToBeDestructed();

	if (pEvent->CanBeDestructed())
	{
		delete pEvent;
	}
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	delete pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	delete pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
	delete pIEnvironmentConnection;
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructSettingConnection(ISettingConnection const* const pISettingConnection)
{
	delete pISettingConnection;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnRefresh()
{
	g_events.clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLanguage(char const* const szLanguage)
{
	if (szLanguage != nullptr)
	{
		bool const shouldReload = !m_language.IsEmpty() && (m_language.compareNoCase(szLanguage) != 0);

		m_language = szLanguage;
		char const* szAssetDirectory = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryRelative();
		m_localizedSoundBankFolder = szAssetDirectory;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += PathUtil::GetLocalizationFolder().c_str();
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += szLanguage;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += CRY_AUDIO_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szAssetsFolderName;

		if (shouldReload)
		{
			UpdateLocalizedEvents();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UpdateLocalizedEvents()
{
	for (auto const pEvent : g_events)
	{
		if (pEvent->m_isLocalized)
		{
			stack_string path = m_localizedSoundBankFolder.c_str();
			path += "/";

			if (!pEvent->m_folder.empty())
			{
				path += pEvent->m_folder.c_str();
				path += "/";
			}

			path += pEvent->m_name.c_str();

			SF_INFO sfInfo;
			PaStreamParameters streamParameters;

			GetSoundInfo(path.c_str(), sfInfo, streamParameters);
			pEvent->sampleRate = static_cast<double>(sfInfo.samplerate);
			pEvent->streamParameters = streamParameters;
			pEvent->filePath = path.c_str();
		}
	}
}

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
CEventInstance* CImpl::ConstructEventInstance(TriggerInstanceId const triggerInstanceId, CEvent& event, CObject const& object)
{
	event.IncrementNumInstances();

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CEventInstance");
	auto const pEventInstance = new CEventInstance(triggerInstanceId, event, object);
	g_constructedEventInstances.push_back(pEventInstance);

	return pEventInstance;
}
#else
//////////////////////////////////////////////////////////////////////////
CEventInstance* CImpl::ConstructEventInstance(TriggerInstanceId const triggerInstanceId, CEvent& event)
{
	event.IncrementNumInstances();

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CEventInstance");
	return new CEventInstance(triggerInstanceId, event);
}
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructEventInstance(CEventInstance const* const pEventInstance)
{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "pEventInstance is nullpter during %s", __FUNCTION__);

	auto iter(g_constructedEventInstances.begin());
	auto const iterEnd(g_constructedEventInstances.cend());

	for (; iter != iterEnd; ++iter)
	{
		if ((*iter) == pEventInstance)
		{
			if (iter != (iterEnd - 1))
			{
				(*iter) = g_constructedEventInstances.back();
			}

			g_constructedEventInstances.pop_back();
			break;
		}
	}

#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

	CEvent* const pEvent = &pEventInstance->GetEvent();
	delete pEventInstance;

	pEvent->DecrementNumInstances();

	if (pEvent->CanBeDestructed())
	{
		delete pEvent;
	}
}

#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void DrawMemoryPoolInfo(
	IRenderAuxGeom& auxGeom,
	float const posX,
	float& posY,
	stl::SPoolMemoryUsage const& mem,
	stl::SMemoryUsage const& pool,
	char const* const szType,
	uint16 const poolSize)
{
	CryFixedStringT<MaxMiscStringLength> memAllocString;

	if (mem.nAlloc < 1024)
	{
		memAllocString.Format("%" PRISIZE_T " Byte", mem.nAlloc);
	}
	else
	{
		memAllocString.Format("%" PRISIZE_T " KiB", mem.nAlloc >> 10);
	}

	ColorF const& color = (static_cast<uint16>(pool.nUsed) > poolSize) ? Debug::s_globalColorError : Debug::s_systemColorTextPrimary;

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, color, false,
	                    "[%s] Constructed: %" PRISIZE_T " | Allocated: %" PRISIZE_T " | Preallocated: %u | Pool Size: %s",
	                    szType, pool.nUsed, pool.nAlloc, poolSize, memAllocString.c_str());
}
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugMemoryInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	CryModuleMemoryInfo memInfo;
	ZeroStruct(memInfo);
	CryGetMemoryInfoForModule(&memInfo);

	CryFixedStringT<MaxMiscStringLength> memInfoString;
	auto const memAlloc = static_cast<uint32>(memInfo.allocated - memInfo.freed);

	if (memAlloc < 1024)
	{
		memInfoString.Format("%s (Total Memory: %u Byte)", m_name.c_str(), memAlloc);
	}
	else
	{
		memInfoString.Format("%s (Total Memory: %u KiB)", m_name.c_str(), memAlloc >> 10);
	}

	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemHeaderFontSize, Debug::s_globalColorHeader, false, "%s", memInfoString.c_str());
	posY += Debug::g_systemHeaderLineSpacerHeight;

	if (showDetailedInfo)
	{
		{
			auto& allocator = CObject::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects", g_objectPoolSize);
		}

		{
			auto& allocator = CEventInstance::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Event Instances", g_eventInstancePoolSize);
		}

		if (g_debugEventrPoolSize > 0)
		{
			auto& allocator = CEvent::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Events", g_eventsPoolSize);
		}
	}

	size_t const numEvents = g_constructedEventInstances.size();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextSecondary, false, "Active Events: %" PRISIZE_T, numEvents);

	Vec3 const& listenerPosition = g_pListener->GetTransformation().GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	char const* const szName = g_pListener->GetName();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorListenerActive, false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f", szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z);
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE
}

/////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfoList(IRenderAuxGeom& auxGeom, float& posX, float posY, float const debugDistance, char const* const szTextFilter) const
{
#if defined(CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(szTextFilter);
	lowerCaseSearchString.MakeLower();

	auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Port Audio Events [%" PRISIZE_T "]", g_constructedEventInstances.size());
	posY += Debug::g_listHeaderLineHeight;

	for (auto const pEventInstance : g_constructedEventInstances)
	{
		Vec3 const& position = pEventInstance->GetObject().GetTransformation().GetPosition();
		float const distance = position.GetDistance(g_pListener->GetTransformation().GetPosition());

		if ((debugDistance <= 0.0f) || ((debugDistance > 0.0f) && (distance < debugDistance)))
		{
			char const* const szEventName = pEventInstance->GetEvent().m_name.c_str();
			CryFixedStringT<MaxControlNameLength> lowerCaseEventName(szEventName);
			lowerCaseEventName.MakeLower();
			bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseEventName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

			if (draw)
			{
				auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, Debug::s_listColorItemActive, false, "%s on %s", szEventName, pEventInstance->GetObject().GetName());

				posY += Debug::g_listLineHeight;
			}
		}
	}

	posX += 600.0f;
#endif  // CRY_AUDIO_IMPL_PORTAUDIO_USE_PRODUCTION_CODE
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
