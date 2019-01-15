// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "Common.h"
#include "Trigger.h"
#include "Event.h"
#include "Object.h"
#include "CVars.h"
#include "Environment.h"
#include "File.h"
#include "Listener.h"
#include "Parameter.h"
#include "Setting.h"
#include "StandaloneFile.h"
#include "SwitchState.h"
#include "GlobalData.h"

#include <FileInfo.h>
#include <Logger.h>
#include <sndfile.hh>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	#include <DebugStyle.h>
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
std::vector<CObject*> g_constructedObjects;
std::vector<CTrigger*> g_triggers;

uint16 g_triggerPoolSize = 0;
uint16 g_triggerPoolSizeLevelSpecific = 0;

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
uint16 g_debugTriggerPoolSize = 0;
uint16 g_objectPoolSize = 0;
uint16 g_eventPoolSize = 0;
Events g_constructedEvents;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CountPoolSizes(XmlNodeRef const pNode, uint16& poolSizes)
{
	uint16 numTriggers = 0;
	pNode->getAttr(s_szTriggersAttribute, numTriggers);
	poolSizes += numTriggers;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	CObject::CreateAllocator(objectPoolSize);
	CEvent::CreateAllocator(eventPoolSize);
	CTrigger::CreateAllocator(g_triggerPoolSize);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CObject::FreeMemoryPool();
	CEvent::FreeMemoryPool();
	CTrigger::FreeMemoryPool();
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

		if (sf_close(pSndFile) != 0)
		{
			Cry::Audio::Log(ELogType::Error, "Failed to close sound file %s", szPath);
		}

		success = true;
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Failed to open sound file %s", szPath);
	}

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
		Cry::Audio::Log(ELogType::Warning, R"(Event pool size must be at least 1. Forcing the cvar "s_PortAudioEventPoolSize" to 1!)");
	}

	g_constructedObjects.reserve(static_cast<size_t>(objectPoolSize));
	AllocateMemoryPools(objectPoolSize, static_cast<uint16>(g_cvars.m_eventPoolSize));

	char const* szAssetDirectory = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryRelative();

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	g_objectPoolSize = objectPoolSize;
	g_eventPoolSize = static_cast<uint16>(g_cvars.m_eventPoolSize);

	g_constructedEvents.reserve(static_cast<size_t>(g_cvars.m_eventPoolSize));

	if (strlen(szAssetDirectory) == 0)
	{
		Cry::Audio::Log(ELogType::Error, "<Audio - PortAudio>: No asset folder set!");
		szAssetDirectory = "no-asset-folder-set";
	}

	m_name = Pa_GetVersionText();
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

	m_regularSoundBankFolder = szAssetDirectory;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += AUDIO_SYSTEM_DATA_ROOT;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szAssetsFolderName;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

	if (ICVar* const pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		SetLanguage(pCVar->GetString());
	}

	PaError const err = Pa_Initialize();

	if (err != paNoError)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to initialize PortAudio: %s", Pa_GetErrorText(err));
	}

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
	PaError const err = Pa_Terminate();

	if (err != paNoError)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to shut down PortAudio: %s", Pa_GetErrorText(err));
	}

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
		uint16 triggerLevelPoolSize;
		CountPoolSizes(pNode, triggerLevelPoolSize);

		g_triggerPoolSizeLevelSpecific = std::max(g_triggerPoolSizeLevelSpecific, triggerLevelPoolSize);
	}
	else
	{
		CountPoolSizes(pNode, g_triggerPoolSize);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeLibraryDataChanged()
{
	g_triggerPoolSize = 0;
	g_triggerPoolSizeLevelSpecific = 0;

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	g_debugTriggerPoolSize = 0;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_triggerPoolSize += g_triggerPoolSizeLevelSpecific;

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugTriggerPoolSize = g_triggerPoolSize;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

	g_triggerPoolSize = std::max<uint16>(1, g_triggerPoolSize);
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
void CImpl::SetGlobalParameter(IParameterConnection* const pIParameterConnection, float const value)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalSwitchState(ISwitchStateConnection* const pISwitchStateConnection)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData == nullptr)
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the PortAudio implementation of %s", __FUNCTION__);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData == nullptr)
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the PortAudio implementation of %s", __FUNCTION__);
		}
	}
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
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
	implInfo.folderName = s_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CObject");
	auto pObject = new CObject();

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	pObject->SetName("Global Object");
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

	stl::push_back_unique(g_constructedObjects, pObject);

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CObject");
	auto pObject = new CObject();

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	if (szName != nullptr)
	{
		pObject->SetName(szName);
	}
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

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

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	if (szName != nullptr)
	{
		g_pListener->SetName(szName);
	}
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

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
IStandaloneFileConnection* CImpl::ConstructStandaloneFileConnection(CryAudio::CStandaloneFile& standaloneFile, char const* const szFile, bool const bLocalized, ITriggerConnection const* pITriggerConnection /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CStandaloneFile");
	return static_cast<IStandaloneFileConnection*>(new CStandaloneFile);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructStandaloneFileConnection(IStandaloneFileConnection const* const pIStandaloneFileConnection)
{
	delete pIStandaloneFileConnection;
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
	CTrigger* pTrigger = nullptr;
	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szEventTag) == 0)
	{
		char const* const szLocalized = pRootNode->getAttr(s_szLocalizedAttribute);
		bool const isLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, s_szTrueValue) == 0);

		stack_string path = (isLocalized ? m_localizedSoundBankFolder.c_str() : m_regularSoundBankFolder.c_str());
		path += "/";
		stack_string const folderName = pRootNode->getAttr(s_szPathAttribute);

		if (!folderName.empty())
		{
			path += folderName.c_str();
			path += "/";
		}

		stack_string const name = pRootNode->getAttr(s_szNameAttribute);
		path += name.c_str();

		SF_INFO sfInfo;
		PaStreamParameters streamParameters;

		if (GetSoundInfo(path.c_str(), sfInfo, streamParameters))
		{
			CryFixedStringT<16> const eventTypeString(pRootNode->getAttr(s_szTypeAttribute));
			EEventType const eventType = eventTypeString.compareNoCase(s_szStartValue) == 0 ? EEventType::Start : EEventType::Stop;

			int numLoops = 0;
			pRootNode->getAttr(s_szLoopCountAttribute, numLoops);
			// --numLoops because -1: play infinite, 0: play once, 1: play twice, etc...
			--numLoops;
			// Max to -1 to stay backwards compatible.
			numLoops = std::max(-1, numLoops);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CTrigger");
			pTrigger = new CTrigger(
				StringToId(path.c_str()),
				numLoops,
				static_cast<double>(sfInfo.samplerate),
				eventType,
				path.c_str(),
				streamParameters,
				folderName.c_str(),
				name.c_str(),
				isLocalized);

			g_triggers.push_back(pTrigger);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown PortAudio tag: %s", szTag);
	}

	return static_cast<ITriggerConnection*>(pTrigger);
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
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
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CTrigger");
			pITriggerConnection = static_cast<ITriggerConnection*>(new CTrigger(StringToId(path.c_str()), 0, static_cast<double>(sfInfo.samplerate), EEventType::Start, path.c_str(), streamParameters, folderName.c_str(), name.c_str(), pTriggerInfo->isLocalized));
		}
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	delete pITriggerConnection;
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CParameter");
	return static_cast<IParameterConnection*>(new CParameter);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	delete pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CSwitchState");
	return static_cast<ISwitchStateConnection*>(new CSwitchState);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	delete pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CEnvironment");
	return static_cast<IEnvironmentConnection*>(new CEnvironment);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
	delete pIEnvironmentConnection;
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CSetting");
	return static_cast<ISettingConnection*>(new CSetting);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructSettingConnection(ISettingConnection const* const pISettingConnection)
{
	delete pISettingConnection;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnRefresh()
{
	g_triggers.clear();
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
		m_localizedSoundBankFolder += AUDIO_SYSTEM_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szAssetsFolderName;

		if (shouldReload)
		{
			UpdateLocalizedTriggers();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UpdateLocalizedTriggers()
{
	for (auto const pTrigger : g_triggers)
	{
		if (pTrigger->m_isLocalized)
		{
			stack_string path = m_localizedSoundBankFolder.c_str();
			path += "/";

			if (!pTrigger->m_folder.empty())
			{
				path += pTrigger->m_folder.c_str();
				path += "/";
			}

			path += pTrigger->m_name.c_str();

			SF_INFO sfInfo;
			PaStreamParameters streamParameters;

			GetSoundInfo(path.c_str(), sfInfo, streamParameters);
			pTrigger->sampleRate = static_cast<double>(sfInfo.samplerate);
			pTrigger->streamParameters = streamParameters;
			pTrigger->filePath = path.c_str();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CEvent* CImpl::ConstructEvent(TriggerInstanceId const triggerInstanceId)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::PortAudio::CEvent");
	auto const pEvent = new CEvent(triggerInstanceId);

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	g_constructedEvents.push_back(pEvent);
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

	return pEvent;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructEvent(CEvent const* const pEvent)
{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	CRY_ASSERT_MESSAGE(pEvent != nullptr, "pEvent is nullpter during %s", __FUNCTION__);

	auto iter(g_constructedEvents.begin());
	auto const iterEnd(g_constructedEvents.cend());

	for (; iter != iterEnd; ++iter)
	{
		if ((*iter) == pEvent)
		{
			if (iter != (iterEnd - 1))
			{
				(*iter) = g_constructedEvents.back();
			}

			g_constructedEvents.pop_back();
			break;
		}
	}

#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

	delete pEvent;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetFileData(char const* const szName, SFileData& fileData) const
{
}

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
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
	CryFixedStringT<MaxMiscStringLength> memUsedString;

	if (mem.nUsed < 1024)
	{
		memUsedString.Format("%" PRISIZE_T " Byte", mem.nUsed);
	}
	else
	{
		memUsedString.Format("%" PRISIZE_T " KiB", mem.nUsed >> 10);
	}

	CryFixedStringT<MaxMiscStringLength> memAllocString;

	if (mem.nAlloc < 1024)
	{
		memAllocString.Format("%" PRISIZE_T " Byte", mem.nAlloc);
	}
	else
	{
		memAllocString.Format("%" PRISIZE_T " KiB", mem.nAlloc >> 10);
	}

	ColorF const color = (static_cast<uint16>(pool.nUsed) > poolSize) ? Debug::s_globalColorError : Debug::s_systemColorTextPrimary;

	posY += Debug::s_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::s_systemFontSize, color, false,
	                    "[%s] Constructed: %" PRISIZE_T " (%s) | Allocated: %" PRISIZE_T " (%s) | Pool Size: %u",
	                    szType, pool.nUsed, memUsedString.c_str(), pool.nAlloc, memAllocString.c_str(), poolSize);
}
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugMemoryInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
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

	auxGeom.Draw2dLabel(posX, posY, Debug::s_systemHeaderFontSize, Debug::s_globalColorHeader, false, memInfoString.c_str());
	posY += Debug::s_systemHeaderLineSpacerHeight;

	if (showDetailedInfo)
	{
		{
			auto& allocator = CObject::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects", g_objectPoolSize);
		}

		{
			auto& allocator = CEvent::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Events", g_eventPoolSize);
		}

		if (g_debugTriggerPoolSize > 0)
		{
			auto& allocator = CTrigger::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Triggers", g_triggerPoolSize);
		}
	}

	size_t const numEvents = g_constructedEvents.size();

	posY += Debug::s_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::s_systemFontSize, Debug::s_systemColorTextSecondary, false, "Events: %" PRISIZE_T, numEvents);

	Vec3 const& listenerPosition = g_pListener->GetTransformation().GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	char const* const szName = g_pListener->GetName();

	posY += Debug::s_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::s_systemFontSize, Debug::s_systemColorListenerActive, false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f", szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z);
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
}

/////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfoList(IRenderAuxGeom& auxGeom, float& posX, float posY, float const debugDistance, char const* const szTextFilter) const
{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(szTextFilter);
	lowerCaseSearchString.MakeLower();

	auxGeom.Draw2dLabel(posX, posY, Debug::s_listHeaderFontSize, Debug::s_globalColorHeader, false, "Port Audio Events [%" PRISIZE_T "]", g_constructedEvents.size());
	posY += Debug::s_listHeaderLineHeight;

	for (auto const pEvent : g_constructedEvents)
	{
		Vec3 const& position = pEvent->pObject->GetTransformation().GetPosition();
		float const distance = position.GetDistance(g_pListener->GetTransformation().GetPosition());

		if ((debugDistance <= 0.0f) || ((debugDistance > 0.0f) && (distance < debugDistance)))
		{
			char const* const szTriggerName = pEvent->GetName();
			CryFixedStringT<MaxControlNameLength> lowerCaseTriggerName(szTriggerName);
			lowerCaseTriggerName.MakeLower();
			bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseTriggerName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

			if (draw)
			{
				auxGeom.Draw2dLabel(posX, posY, Debug::s_listFontSize, Debug::s_listColorItemActive, false, "%s on %s", szTriggerName, pEvent->pObject->GetName());

				posY += Debug::s_listLineHeight;
			}
		}
	}

	posX += 600.0f;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
