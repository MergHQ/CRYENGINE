// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "CVars.h"
#include "Common.h"
#include "Environment.h"
#include "Event.h"
#include "File.h"
#include "Listener.h"
#include "Object.h"
#include "Parameter.h"
#include "Setting.h"
#include "StandaloneFile.h"
#include "SwitchState.h"
#include "Trigger.h"

#include <FileInfo.h>
#include <Logger.h>
#include <CrySystem/File/CryFile.h>
#include <CryAudio/IAudioSystem.h>
#include <CrySystem/IProjectManager.h>
#include <CrySystem/IConsole.h>

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

// SDL Mixer
#include <SDL_mixer.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
SPoolSizes g_poolSizes;
SPoolSizes g_poolSizesLevelSpecific;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
SPoolSizes g_debugPoolSizes;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CountPoolSizes(XmlNodeRef const pNode, SPoolSizes& poolSizes)
{
	uint16 numTriggers = 0;
	pNode->getAttr(s_szTriggersAttribute, numTriggers);
	poolSizes.triggers += numTriggers;

	uint16 numParameters = 0;
	pNode->getAttr(s_szParametersAttribute, numParameters);
	poolSizes.parameters += numParameters;

	uint16 numSwitchStates = 0;
	pNode->getAttr(s_szSwitchStatesAttribute, numSwitchStates);
	poolSizes.switchStates += numSwitchStates;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer Object Pool");
	CObject::CreateAllocator(objectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer Event Pool");
	CEvent::CreateAllocator(eventPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer Trigger Pool");
	CTrigger::CreateAllocator(g_poolSizes.triggers);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer Parameter Pool");
	CParameter::CreateAllocator(g_poolSizes.parameters);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "SDL Mixer Switch State Pool");
	CSwitchState::CreateAllocator(g_poolSizes.switchStates);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CObject::FreeMemoryPool();
	CEvent::FreeMemoryPool();
	CTrigger::FreeMemoryPool();
	CParameter::FreeMemoryPool();
	CSwitchState::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
string GetFullFilePath(char const* const szFileName, char const* const szPath)
{
	string fullFilePath;

	if ((szPath != nullptr) && (szPath[0] != '\0'))
	{
		fullFilePath = szPath;
		fullFilePath += "/";
		fullFilePath += szFileName;
	}
	else
	{
		fullFilePath = szFileName;
	}

	return fullFilePath;
}

///////////////////////////////////////////////////////////////////////////
void OnEventFinished(CryAudio::CEvent& audioEvent)
{
	gEnv->pAudioSystem->ReportFinishedEvent(audioEvent, true);
}

///////////////////////////////////////////////////////////////////////////
void OnStandaloneFileFinished(CryAudio::CStandaloneFile& standaloneFile, const char* szFile)
{
	gEnv->pAudioSystem->ReportStoppedFile(standaloneFile);
}

///////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_pCVarFileExtension(nullptr)
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	, m_name("SDL Mixer 2.0.2")
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
{
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
void CImpl::Update()
{
	SoundEngine::Update();
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	g_objects.reserve(static_cast<size_t>(objectPoolSize));
	AllocateMemoryPools(objectPoolSize, eventPoolSize);

	m_pCVarFileExtension = REGISTER_STRING("s_SDLMixerStandaloneFileExtension", ".mp3", 0, "the expected file extension for standalone files, played via the sdl_mixer");

	if (ICVar* const pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		SetLanguage(pCVar->GetString());
	}

	if (SoundEngine::Init())
	{
		SoundEngine::RegisterEventFinishedCallback(OnEventFinished);
		SoundEngine::RegisterStandaloneFileFinishedCallback(OnStandaloneFileFinished);
		return ERequestStatus::Success;
	}

	return ERequestStatus::Failure;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
}

///////////////////////////////////////////////////////////////////////////
void CImpl::Release()
{
	if (m_pCVarFileExtension != nullptr)
	{
		m_pCVarFileExtension->Release();
		m_pCVarFileExtension = nullptr;
	}

	SoundEngine::Release();

	delete this;
	g_cvars.UnregisterVariables();

	FreeMemoryPools();
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnRefresh()
{
	SoundEngine::Refresh();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLibraryData(XmlNodeRef const pNode, bool const isLevelSpecific)
{
	if (isLevelSpecific)
	{
		SPoolSizes levelPoolSizes;
		CountPoolSizes(pNode, levelPoolSizes);

		g_poolSizesLevelSpecific.triggers = std::max(g_poolSizesLevelSpecific.triggers, levelPoolSizes.triggers);
		g_poolSizesLevelSpecific.parameters = std::max(g_poolSizesLevelSpecific.parameters, levelPoolSizes.parameters);
		g_poolSizesLevelSpecific.switchStates = std::max(g_poolSizesLevelSpecific.switchStates, levelPoolSizes.switchStates);
	}
	else
	{
		CountPoolSizes(pNode, g_poolSizes);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeLibraryDataChanged()
{
	ZeroStruct(g_poolSizes);
	ZeroStruct(g_poolSizesLevelSpecific);

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	ZeroStruct(g_debugPoolSizes);
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_poolSizes.triggers += g_poolSizesLevelSpecific.triggers;
	g_poolSizes.parameters += g_poolSizesLevelSpecific.parameters;
	g_poolSizes.switchStates += g_poolSizesLevelSpecific.switchStates;

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	g_poolSizes.triggers = std::max<uint16>(1, g_poolSizes.triggers);
	g_poolSizes.parameters = std::max<uint16>(1, g_poolSizes.parameters);
	g_poolSizes.switchStates = std::max<uint16>(1, g_poolSizes.switchStates);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnLoseFocus()
{
	SoundEngine::Mute();
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnGetFocus()
{
	SoundEngine::UnMute();
}

///////////////////////////////////////////////////////////////////////////
void CImpl::MuteAll()
{
	SoundEngine::Mute();
}

///////////////////////////////////////////////////////////////////////////
void CImpl::UnmuteAll()
{
	SoundEngine::UnMute();
}

///////////////////////////////////////////////////////////////////////////
void CImpl::PauseAll()
{
	SoundEngine::Pause();
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ResumeAll()
{
	SoundEngine::Resume();
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::StopAllSounds()
{
	SoundEngine::Stop();
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalParameter(IParameterConnection const* const pIParameterConnection, float const value)
{
	auto const pParameter = static_cast<CParameter const*>(pIParameterConnection);

	if (pParameter != nullptr)
	{
		for (auto const pObject : g_objects)
		{
			pObject->SetParameter(pParameter, value);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid parameter pointer passed to the SDL Mixer implementation of %s", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalSwitchState(ISwitchStateConnection const* const pISwitchStateConnection)
{
	auto const pSwitchState = static_cast<CSwitchState const*>(pISwitchStateConnection);

	if (pSwitchState != nullptr)
	{
		for (auto const pObject : g_objects)
		{
			pObject->SetSwitchState(pSwitchState);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid switch state pointer passed to the SDL Mixer implementation of %s", __FUNCTION__);
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if (pFileInfo != nullptr)
	{
		CFile* const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			pFileData->sampleId = SoundEngine::LoadSampleFromMemory(pFileInfo->pFileData, pFileInfo->size, pFileInfo->szFileName);
			result = ERequestStatus::Success;
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the SDL Mixer implementation of RegisterInMemoryFile");
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if (pFileInfo != nullptr)
	{
		CFile* const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			SoundEngine::UnloadSample(pFileData->sampleId);
			result = ERequestStatus::Success;
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the SDL Mixer implementation of UnregisterInMemoryFile");
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ConstructFile(XmlNodeRef const pRootNode, SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if ((_stricmp(pRootNode->getTag(), s_szFileTag) == 0) && (pFileInfo != nullptr))
	{
		char const* const szFileName = pRootNode->getAttr(s_szNameAttribute);
		char const* const szPath = pRootNode->getAttr(s_szPathAttribute);
		CryFixedStringT<MaxFilePathLength> fullFilePath;
		if (szPath)
		{
			fullFilePath = szPath;
			fullFilePath += "/";
			fullFilePath += szFileName;
		}
		else
		{
			fullFilePath = szFileName;
		}

		// Currently the SDLMixer Implementation does not support localized files.
		pFileInfo->bLocalized = false;

		if (!fullFilePath.empty())
		{
			pFileInfo->szFileName = fullFilePath.c_str();
			pFileInfo->memoryBlockAlignment = m_memoryAlignment;
			pFileInfo->pImplData = new CFile;
			result = ERequestStatus::Success;
		}
		else
		{
			pFileInfo->szFileName = nullptr;
			pFileInfo->memoryBlockAlignment = 0;
			pFileInfo->pImplData = nullptr;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructFile(IFile* const pIFile)
{
	delete pIFile;
}

///////////////////////////////////////////////////////////////////////////
char const* const CImpl::GetFileLocation(SFileInfo* const pFileInfo)
{
	static CryFixedStringT<MaxFilePathLength> s_path;
	s_path = AUDIO_SYSTEM_DATA_ROOT "/";
	s_path += s_szImplFolderName;
	s_path += "/";
	s_path += s_szAssetsFolderName;
	s_path += "/";

	return s_path.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetInfo(SImplInfo& implInfo) const
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
	implInfo.folderName = s_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
ITriggerConnection const* CImpl::ConstructTriggerConnection(XmlNodeRef const pRootNode, float& radius)
{
	CTrigger* pTrigger = nullptr;

	if (_stricmp(pRootNode->getTag(), s_szEventTag) == 0)
	{
		char const* const szFileName = pRootNode->getAttr(s_szNameAttribute);
		char const* const szPath = pRootNode->getAttr(s_szPathAttribute);
		string const fullFilePath = GetFullFilePath(szFileName, szPath);

		char const* const szLocalized = pRootNode->getAttr(s_szLocalizedAttribute);
		bool const isLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, s_szTrueValue) == 0);

		SampleId const sampleId = SoundEngine::LoadSample(fullFilePath, true, isLocalized);
		EEventType type = EEventType::Start;

		if (_stricmp(pRootNode->getAttr(s_szTypeAttribute), s_szStopValue) == 0)
		{
			type = EEventType::Stop;
		}
		else if (_stricmp(pRootNode->getAttr(s_szTypeAttribute), s_szPauseValue) == 0)
		{
			type = EEventType::Pause;
		}
		else if (_stricmp(pRootNode->getAttr(s_szTypeAttribute), s_szResumeValue) == 0)
		{
			type = EEventType::Resume;
		}

		if (type == EEventType::Start)
		{
			bool const isPanningEnabled = (_stricmp(pRootNode->getAttr(s_szPanningEnabledAttribute), s_szTrueValue) == 0);
			bool const isAttenuationEnabled = (_stricmp(pRootNode->getAttr(s_szAttenuationEnabledAttribute), s_szTrueValue) == 0);

			float minDistance = -1.0f;
			float maxDistance = -1.0f;

			if (isAttenuationEnabled)
			{
				pRootNode->getAttr(s_szAttenuationMinDistanceAttribute, minDistance);
				pRootNode->getAttr(s_szAttenuationMaxDistanceAttribute, maxDistance);

				minDistance = std::max(0.0f, minDistance);
				maxDistance = std::max(0.0f, maxDistance);

				if (minDistance > maxDistance)
				{
					Cry::Audio::Log(ELogType::Warning, "Min distance (%f) was greater than max distance (%f) of %s", minDistance, maxDistance, szFileName);
					std::swap(minDistance, maxDistance);
				}

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
				radius = maxDistance;
#endif        // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
			}

			// Translate decibel to normalized value.
			static const int maxVolume = 128;
			float volume = 0.0f;
			pRootNode->getAttr(s_szVolumeAttribute, volume);
			auto const normalizedVolume = static_cast<int>(pow_tpl(10.0f, volume / 20.0f) * maxVolume);

			int numLoops = 0;
			pRootNode->getAttr(s_szLoopCountAttribute, numLoops);
			// --numLoops because -1: play infinite, 0: play once, 1: play twice, etc...
			--numLoops;
			// Max to -1 to stay backwards compatible.
			numLoops = std::max(-1, numLoops);

			float fadeInTimeSec = s_defaultFadeInTime;
			pRootNode->getAttr(s_szFadeInTimeAttribute, fadeInTimeSec);
			auto const fadeInTimeMs = static_cast<int>(fadeInTimeSec * 1000.0f);

			float fadeOutTimeSec = s_defaultFadeOutTime;
			pRootNode->getAttr(s_szFadeOutTimeAttribute, fadeOutTimeSec);
			auto const fadeOutTimeMs = static_cast<int>(fadeOutTimeSec * 1000.0f);

			pTrigger = new CTrigger(type, sampleId, minDistance, maxDistance, normalizedVolume, numLoops, fadeInTimeMs, fadeOutTimeMs, isPanningEnabled);
		}
		else
		{
			pTrigger = new CTrigger(type, sampleId);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown SDL Mixer tag: %s", pRootNode->getTag());
	}

	return static_cast<ITriggerConnection*>(pTrigger);
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection const* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	ITriggerConnection const* pITriggerConnection = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		string const fullFilePath = GetFullFilePath(pTriggerInfo->name.c_str(), pTriggerInfo->path.c_str());
		SampleId const sampleId = SoundEngine::LoadSample(fullFilePath, true, pTriggerInfo->isLocalized);

		pITriggerConnection = static_cast<ITriggerConnection const*>(new CTrigger(EEventType::Start, sampleId, -1.0f, -1.0f, 128, 0, 0, 0, false));
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	if (pITriggerConnection != nullptr)
	{
		auto const pTrigger = static_cast<CTrigger const* const>(pITriggerConnection);
		SoundEngine::StopTrigger(pTrigger);
		delete pTrigger;
	}
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection const* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	CParameter* pParameter = nullptr;

	if (_stricmp(pRootNode->getTag(), s_szEventTag) == 0)
	{
		char const* const szFileName = pRootNode->getAttr(s_szNameAttribute);
		char const* const szPath = pRootNode->getAttr(s_szPathAttribute);
		string const fullFilePath = GetFullFilePath(szFileName, szPath);

		char const* const szLocalized = pRootNode->getAttr(s_szLocalizedAttribute);
		bool const isLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, s_szTrueValue) == 0);

		SampleId const sampleId = SoundEngine::LoadSample(fullFilePath, true, isLocalized);

		float multiplier = s_defaultParamMultiplier;
		float shift = s_defaultParamShift;
		pRootNode->getAttr(s_szMutiplierAttribute, multiplier);
		multiplier = std::max(0.0f, multiplier);
		pRootNode->getAttr(s_szShiftAttribute, shift);

		pParameter = new CParameter(sampleId, multiplier, shift, szFileName);
	}

	return static_cast<IParameterConnection*>(pParameter);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	delete pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection const* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	CSwitchState* pSwitchState = nullptr;

	if (_stricmp(pRootNode->getTag(), s_szEventTag) == 0)
	{
		char const* const szFileName = pRootNode->getAttr(s_szNameAttribute);
		char const* const szPath = pRootNode->getAttr(s_szPathAttribute);
		string const fullFilePath = GetFullFilePath(szFileName, szPath);

		char const* const szLocalized = pRootNode->getAttr(s_szLocalizedAttribute);
		bool const isLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, s_szTrueValue) == 0);

		SampleId const sampleId = SoundEngine::LoadSample(fullFilePath, true, isLocalized);

		float value = s_defaultStateValue;
		pRootNode->getAttr(s_szValueAttribute, value);
		value = crymath::clamp(value, 0.0f, 1.0f);

		pSwitchState = new CSwitchState(sampleId, value, szFileName);
	}

	return static_cast<ISwitchStateConnection*>(pSwitchState);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	delete pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection const* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	return static_cast<IEnvironmentConnection*>(new CEnvironment);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
	delete pIEnvironmentConnection;
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection const* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	return static_cast<ISettingConnection*>(new CSetting);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructSettingConnection(ISettingConnection const* const pISettingConnection)
{
	delete pISettingConnection;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	g_pObject = new CObject(CTransformation::GetEmptyObject(), 0);
	g_objects.push_back(g_pObject);
	return static_cast<IObject*>(g_pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	static uint32 id = 1;
	auto const pObject = new CObject(transformation, id++);
	g_objects.push_back(pObject);

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject const*>(pIObject);
		stl::find_and_erase(g_objects, pObject);

		if (pObject == g_pObject)
		{
			delete pObject;
			g_pObject = nullptr;
		}
		else
		{
			delete pObject;
		}
	}
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	static ListenerId id = 0;
	g_pListener = new CListener(transformation, id++);

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	if (szName != nullptr)
	{
		g_pListener->SetName(szName);
	}
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

	return static_cast<IListener*>(g_pListener);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructListener(IListener* const pIListener)
{
	CRY_ASSERT_MESSAGE(pIListener == g_pListener, "pIListener is not g_pListener during %s", __FUNCTION__);
	delete g_pListener;
	g_pListener = nullptr;
}

///////////////////////////////////////////////////////////////////////////
IEvent* CImpl::ConstructEvent(CryAudio::CEvent& event)
{
	return static_cast<IEvent*>(new CEvent(event));
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEvent(IEvent const* const pIEvent)
{
	delete pIEvent;
}

///////////////////////////////////////////////////////////////////////////
IStandaloneFileConnection* CImpl::ConstructStandaloneFileConnection(CryAudio::CStandaloneFile& standaloneFile, char const* const szFile, bool const bLocalized, ITriggerConnection const* pITriggerConnection /*= nullptr*/)
{
	static string s_localizedfilesFolder = PathUtil::GetLocalizationFolder() + "/" + m_language + "/";
	static string filePath;

	if (bLocalized)
	{
		filePath = s_localizedfilesFolder + szFile + m_pCVarFileExtension->GetString();
	}
	else
	{
		filePath = string(szFile) + m_pCVarFileExtension->GetString();
	}

	return static_cast<IStandaloneFileConnection*>(new CStandaloneFile(filePath, standaloneFile));
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructStandaloneFileConnection(IStandaloneFileConnection const* const pIStandaloneFileConnection)
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	auto const pStandaloneEvent = static_cast<CStandaloneFile const*>(pIStandaloneFileConnection);
	CRY_ASSERT_MESSAGE(pStandaloneEvent->m_channels.size() == 0, "Events always have to be stopped/finished before they get deleted during %s", __FUNCTION__);
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
	delete pIStandaloneFileConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::GamepadConnected(DeviceId const deviceUniqueID)
{}

///////////////////////////////////////////////////////////////////////////
void CImpl::GamepadDisconnected(DeviceId const deviceUniqueID)
{}

///////////////////////////////////////////////////////////////////////////
void CImpl::SetLanguage(char const* const szLanguage)
{
	if (szLanguage != nullptr)
	{
		bool const shouldReload = !m_language.IsEmpty() && (m_language.compareNoCase(szLanguage) != 0);

		m_language = szLanguage;
		s_localizedAssetsPath = PathUtil::GetLocalizationFolder().c_str();
		s_localizedAssetsPath += "/";
		s_localizedAssetsPath += m_language.c_str();
		s_localizedAssetsPath += "/";
		s_localizedAssetsPath += AUDIO_SYSTEM_DATA_ROOT;
		s_localizedAssetsPath += "/";
		s_localizedAssetsPath += s_szImplFolderName;
		s_localizedAssetsPath += "/";
		s_localizedAssetsPath += s_szAssetsFolderName;
		s_localizedAssetsPath += "/";

		if (shouldReload)
		{
			SoundEngine::Refresh();
		}
	}
}

///////////////////////////////////////////////////////////////////////////
void CImpl::GetFileData(char const* const szName, SFileData& fileData) const
{
}

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void DrawMemoryPoolInfo(
	IRenderAuxGeom& auxGeom,
	float const posX,
	float& posY,
	stl::SPoolMemoryUsage const& mem,
	stl::SMemoryUsage const& pool,
	char const* const szType)
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

	posY += g_debugSystemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false,
	                    "[%s] In Use: %" PRISIZE_T " | Constructed: %" PRISIZE_T " (%s) | Memory Pool: %s",
	                    szType, pool.nUsed, pool.nAlloc, memUsedString.c_str(), memAllocString.c_str());
}
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
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

	auxGeom.Draw2dLabel(posX, posY, g_debugSystemHeaderFontSize, g_debugSystemColorHeader.data(), false, memInfoString.c_str());

	if (showDetailedInfo)
	{
		{
			auto& allocator = CObject::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects");
		}

		{
			auto& allocator = CEvent::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Events");
		}

		if (g_debugPoolSizes.triggers > 0)
		{
			auto& allocator = CTrigger::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Triggers");
		}

		if (g_debugPoolSizes.parameters > 0)
		{
			auto& allocator = CParameter::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters");
		}

		if (g_debugPoolSizes.switchStates > 0)
		{
			auto& allocator = CSwitchState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "SwitchStates");
		}
	}

	Vec3 const& listenerPosition = g_pListener->GetTransformation().GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	char const* const szName = g_pListener->GetName();

	posY += g_debugSystemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorListenerActive.data(), false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f", szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z);
#endif  // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
