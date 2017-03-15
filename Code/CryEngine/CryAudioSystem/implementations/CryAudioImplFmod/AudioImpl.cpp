// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "AudioEvent.h"
#include "AudioObject.h"
#include "AudioImplCVars.h"
#include "ATLEntities.h"
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>
#include <CryString/CryPath.h>

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::Fmod;

AudioParameterToIndexMap g_parameterToIndex;
FmodSwitchToIndexMap g_switchToIndex;

char const* const CAudioImpl::s_szFmodEventTag = "FmodEvent";
char const* const CAudioImpl::s_szFmodSnapshotTag = "FmodSnapshot";
char const* const CAudioImpl::s_szFmodEventParameterTag = "FmodEventParameter";
char const* const CAudioImpl::s_szFmodSnapshotParameterTag = "FmodSnapshotParameter";
char const* const CAudioImpl::s_szFmodFileTag = "FmodFile";
char const* const CAudioImpl::s_szFmodBusTag = "FmodBus";
char const* const CAudioImpl::s_szFmodNameAttribute = "fmod_name";
char const* const CAudioImpl::s_szFmodValueAttribute = "fmod_value";
char const* const CAudioImpl::s_szFmodMutiplierAttribute = "fmod_value_multiplier";
char const* const CAudioImpl::s_szFmodShiftAttribute = "fmod_value_shift";
char const* const CAudioImpl::s_szFmodPathAttribute = "fmod_path";
char const* const CAudioImpl::s_szFmodLocalizedAttribute = "fmod_localized";
char const* const CAudioImpl::s_szFmodEventTypeAttribute = "fmod_event_type";
char const* const CAudioImpl::s_szFmodEventPrefix = "event:/";
char const* const CAudioImpl::s_szFmodSnapshotPrefix = "snapshot:/";
char const* const CAudioImpl::s_szFmodBusPrefix = "bus:/";

///////////////////////////////////////////////////////////////////////////
CAudioImpl::CAudioImpl()
	: m_pSystem(nullptr)
	, m_pLowLevelSystem(nullptr)
	, m_pMasterBank(nullptr)
	, m_pStringsBank(nullptr)
{
	m_constructedAudioObjects.reserve(256);
}

///////////////////////////////////////////////////////////////////////////
CAudioImpl::~CAudioImpl()
{
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::Update(float const deltaTime)
{
	if (m_pSystem != nullptr)
	{
		FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
		fmodResult = m_pSystem->update();
		ASSERT_FMOD_OK;
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Fmod Object Pool");
	CAudioObject::CreateAllocator(audioObjectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Fmod Event Pool");
	CAudioEvent::CreateAllocator(eventPoolSize);

	char const* const szAssetDirectory = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryRelative();
	if (strlen(szAssetDirectory) == 0)
	{
		CryFatalError("<Audio - FMOD>: Needs a valid asset folder to proceed!");
	}

	m_regularSoundBankFolder = szAssetDirectory;
	m_regularSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
	m_regularSoundBankFolder += FMOD_IMPL_DATA_ROOT;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

	FMOD_RESULT fmodResult = FMOD::Studio::System::create(&m_pSystem);
	ASSERT_FMOD_OK;
	fmodResult = m_pSystem->getLowLevelSystem(&m_pLowLevelSystem);
	ASSERT_FMOD_OK;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	uint32 version = 0;
	fmodResult = m_pLowLevelSystem->getVersion(&version);
	ASSERT_FMOD_OK;

	CryFixedStringT<MaxMiscStringLength> systemVersion;
	systemVersion.Format("%08x", version);
	CryFixedStringT<MaxMiscStringLength> headerVersion;
	headerVersion.Format("%08x", FMOD_VERSION);
	CreateVersionString(systemVersion);
	CreateVersionString(headerVersion);

	m_fullImplString = FMOD_IMPL_INFO_STRING;
	m_fullImplString += "System: ";
	m_fullImplString += systemVersion;
	m_fullImplString += " Header: ";
	m_fullImplString += headerVersion;
	m_fullImplString += " (";
	m_fullImplString += szAssetDirectory;
	m_fullImplString += CRY_NATIVE_PATH_SEPSTR FMOD_IMPL_DATA_ROOT ")";
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	int sampleRate = 0;
	int numRawSpeakers = 0;
	FMOD_SPEAKERMODE speakerMode = FMOD_SPEAKERMODE_DEFAULT;
	fmodResult = m_pLowLevelSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers);
	ASSERT_FMOD_OK;
	fmodResult = m_pLowLevelSystem->setSoftwareFormat(sampleRate, speakerMode, numRawSpeakers);
	ASSERT_FMOD_OK;

	fmodResult = m_pLowLevelSystem->set3DSettings(g_audioImplCVars.m_dopplerScale, g_audioImplCVars.m_distanceFactor, g_audioImplCVars.m_rolloffScale);
	ASSERT_FMOD_OK;

	void* pExtraDriverData = nullptr;
	int initFlags = FMOD_INIT_NORMAL | FMOD_INIT_VOL0_BECOMES_VIRTUAL;
	int studioInitFlags = FMOD_STUDIO_INIT_NORMAL;

	if (g_audioImplCVars.m_enableLiveUpdate > 0)
	{
		studioInitFlags |= FMOD_STUDIO_INIT_LIVEUPDATE;
	}

	if (g_audioImplCVars.m_enableSynchronousUpdate > 0)
	{
		studioInitFlags |= FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE;
	}

	fmodResult = m_pSystem->initialize(g_audioImplCVars.m_maxChannels, studioInitFlags, initFlags, pExtraDriverData);
	ASSERT_FMOD_OK;

	if (!LoadMasterBanks())
	{
		return eRequestStatus_Failure;
	}

	FMOD_3D_ATTRIBUTES attributes = {
		{ 0 }
	};
	attributes.forward.z = 1.0f;
	attributes.up.y = 1.0f;
	fmodResult = m_pSystem->setListenerAttributes(0, &attributes);
	ASSERT_FMOD_OK;

	CAudioObjectBase::s_pSystem = m_pSystem;
	CAudioListener::s_pSystem = m_pSystem;
	CAudioFileBase::s_pLowLevelSystem = m_pLowLevelSystem;

	return (fmodResult == FMOD_OK) ? eRequestStatus_Success : eRequestStatus_Failure;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnBeforeShutDown()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ShutDown()
{
	FMOD_RESULT fmodResult = FMOD_OK;

	if (m_pSystem != nullptr)
	{
		UnloadMasterBanks();

		fmodResult = m_pSystem->release();
		ASSERT_FMOD_OK;
	}

	return (fmodResult == FMOD_OK) ? eRequestStatus_Success : eRequestStatus_Failure;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Release()
{
	delete this;

	g_audioImplCVars.UnregisterVariables();

	CAudioObject::FreeMemoryPool();
	CAudioEvent::FreeMemoryPool();

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnLoseFocus()
{
	return MuteMasterBus(true);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnGetFocus()
{
	return MuteMasterBus(false);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::MuteAll()
{
	return MuteMasterBus(true);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnmuteAll()
{
	return MuteMasterBus(false);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::StopAllSounds()
{
	FMOD::Studio::Bus* pMasterBus = nullptr;
	FMOD_RESULT fmodResult = m_pSystem->getBus("bus:/", &pMasterBus);
	ASSERT_FMOD_OK;

	if (pMasterBus != nullptr)
	{
		fmodResult = pMasterBus->stopAllEvents(FMOD_STUDIO_STOP_IMMEDIATE);
		ASSERT_FMOD_OK;
	}

	return (fmodResult == FMOD_OK) ? eRequestStatus_Success : eRequestStatus_Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::RegisterInMemoryFile(SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus requestResult = eRequestStatus_Failure;

	if (pFileEntryInfo != nullptr)
	{
		CAudioFileEntry* const pFmodAudioFileEntry = static_cast<CAudioFileEntry*>(pFileEntryInfo->pImplData);
		if (pFmodAudioFileEntry != nullptr)
		{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			// loadBankMemory requires 32-byte alignment when using FMOD_STUDIO_LOAD_MEMORY_POINT
			if ((reinterpret_cast<uintptr_t>(pFileEntryInfo->pFileData) & (32 - 1)) > 0)
			{
				CryFatalError("<Audio>: allocation not %d byte aligned!", 32);
			}
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

			FMOD_RESULT const fmodResult = m_pSystem->loadBankMemory(static_cast<char*>(pFileEntryInfo->pFileData), static_cast<int>(pFileEntryInfo->size), FMOD_STUDIO_LOAD_MEMORY_POINT, FMOD_STUDIO_LOAD_BANK_NORMAL, &pFmodAudioFileEntry->pBank);
			ASSERT_FMOD_OK;
			requestResult = (fmodResult == FMOD_OK) ? eRequestStatus_Success : eRequestStatus_Failure;
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the Fmod implementation of RegisterInMemoryFile");
		}
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnregisterInMemoryFile(SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus requestResult = eRequestStatus_Failure;

	if (pFileEntryInfo != nullptr)
	{
		CAudioFileEntry* const pFmodAudioFileEntry = static_cast<CAudioFileEntry*>(pFileEntryInfo->pImplData);

		if (pFmodAudioFileEntry != nullptr)
		{
			FMOD_RESULT fmodResult = pFmodAudioFileEntry->pBank->unload();
			ASSERT_FMOD_OK;

			FMOD_STUDIO_LOADING_STATE loadingState;

			do
			{
				fmodResult = m_pSystem->update();
				ASSERT_FMOD_OK;
				fmodResult = pFmodAudioFileEntry->pBank->getLoadingState(&loadingState);
				ASSERT_FMOD_OK_OR_INVALID_HANDLE;
			}
			while (loadingState == FMOD_STUDIO_LOADING_STATE_UNLOADING);

			pFmodAudioFileEntry->pBank = nullptr;
			requestResult = (fmodResult == FMOD_OK) ? eRequestStatus_Success : eRequestStatus_Failure;
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the Fmod implementation of UnregisterInMemoryFile");
		}
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ParseAudioFileEntry(
  XmlNodeRef const pAudioFileEntryNode,
  SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus result = eRequestStatus_Failure;

	if ((_stricmp(pAudioFileEntryNode->getTag(), s_szFmodFileTag) == 0) && (pFileEntryInfo != nullptr))
	{
		char const* const szFmodAudioFileEntryName = pAudioFileEntryNode->getAttr(s_szFmodNameAttribute);

		if (szFmodAudioFileEntryName != nullptr && szFmodAudioFileEntryName[0] != '\0')
		{
			char const* const szFmodLocalized = pAudioFileEntryNode->getAttr(s_szFmodLocalizedAttribute);
			pFileEntryInfo->bLocalized = (szFmodLocalized != nullptr) && (_stricmp(szFmodLocalized, "true") == 0);
			pFileEntryInfo->szFileName = szFmodAudioFileEntryName;

			// FMOD Studio always uses 32 byte alignment for preloaded banks regardless of the platform.
			pFileEntryInfo->memoryBlockAlignment = 32;

			pFileEntryInfo->pImplData = new CAudioFileEntry();

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
	CAudioObjectBase* pAudioObject = new CGlobalAudioObject(m_constructedAudioObjects);
	if (!stl::push_back_unique(m_constructedAudioObjects, pAudioObject))
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Trying to construct an already registered audio object.");
	}
	return pAudioObject;
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructAudioObject(char const* const /*szAudioObjectName*/)
{
	CAudioObjectBase* pAudioObject = new CAudioObject();
	if (!stl::push_back_unique(m_constructedAudioObjects, pAudioObject))
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Trying to construct an already registered audio object.");
	}

	return pAudioObject;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioObject(IAudioObject const* const pIAudioObject)
{
	CAudioObjectBase const* pAudioObject = static_cast<CAudioObjectBase const*>(pIAudioObject);
	if (!stl::find_and_erase(m_constructedAudioObjects, pAudioObject))
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Trying to delete a non-existing audio object.");
	}

	delete pAudioObject;
}

///////////////////////////////////////////////////////////////////////////
IAudioListener* CAudioImpl::ConstructAudioListener()
{
	static int id = 0;
	return new CAudioListener(id++);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioListener(IAudioListener* const pIListener)
{
	delete pIListener;
}

//////////////////////////////////////////////////////////////////////////
IAudioEvent* CAudioImpl::ConstructAudioEvent(CATLEvent& audioEvent)
{
	return new CAudioEvent(&audioEvent);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioEvent(IAudioEvent const* const pAudioEvent)
{
	CRY_ASSERT(pAudioEvent != nullptr);
	delete pAudioEvent;
}

//////////////////////////////////////////////////////////////////////////
IAudioStandaloneFile* CAudioImpl::ConstructAudioStandaloneFile(CATLStandaloneFile& atlStandaloneFile, char const* const szFile, bool const bLocalized, IAudioTrigger const* pTrigger /*= nullptr*/)
{
	static string s_localizedfilesFolder = PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR + PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + m_language.c_str() + CRY_NATIVE_PATH_SEPSTR;
	static string s_nonLocalizedfilesFolder = PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR;
	string filePath;

	if (bLocalized)
	{
		filePath = s_localizedfilesFolder + szFile + ".mp3";
	}
	else
	{
		filePath = s_nonLocalizedfilesFolder + szFile + ".mp3";
	}

	CAudioFileBase* pFile = nullptr;
	if (pTrigger != nullptr)
	{
		pFile = new CProgrammerSoundAudioFile(filePath, static_cast<CAudioTrigger const* const>(pTrigger)->m_guid, atlStandaloneFile);
	}
	else
	{
		pFile = new CAudioStandaloneFile(filePath, atlStandaloneFile);
	}

	return pFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioStandaloneFile(IAudioStandaloneFile const* const pIFile)
{
	CRY_ASSERT(pIFile != nullptr);
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
	IAudioTrigger* pAudioTrigger = nullptr;
	char const* const szTag = pAudioTriggerNode->getTag();

	if (_stricmp(szTag, s_szFmodEventTag) == 0)
	{
		stack_string path(s_szFmodEventPrefix);
		path += pAudioTriggerNode->getAttr(s_szFmodNameAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			EFmodEventType eventType = eFmodEventType_Start;
			char const* const szFmodEventType = pAudioTriggerNode->getAttr(s_szFmodEventTypeAttribute);

			if (szFmodEventType != nullptr &&
			    szFmodEventType[0] != '\0' && _stricmp(szFmodEventType, "stop") == 0)
			{
				eventType = eFmodEventType_Stop;
			}

#if defined (INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pAudioTrigger = new CAudioTrigger(AudioStringToId(path.c_str()), eventType, nullptr, guid, path.c_str());
#else
			pAudioTrigger = new CAudioTrigger(AudioStringToId(path.c_str()), eventType, nullptr, guid);
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Fmod event: %s", path.c_str());
		}
	}
	else if (_stricmp(szTag, s_szFmodSnapshotTag) == 0)
	{
		stack_string path(s_szFmodSnapshotPrefix);
		path += pAudioTriggerNode->getAttr(s_szFmodNameAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			EFmodEventType eventType = eFmodEventType_Start;
			char const* const szFmodEventType = pAudioTriggerNode->getAttr(s_szFmodEventTypeAttribute);

			if (szFmodEventType != nullptr &&
			    szFmodEventType[0] != '\0' &&
			    _stricmp(szFmodEventType, "stop") == 0)
			{
				eventType = eFmodEventType_Stop;
			}

			FMOD::Studio::EventDescription* pEventDescription = nullptr;
			m_pSystem->getEventByID(&guid, &pEventDescription);

#if defined (INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pAudioTrigger = new CAudioTrigger(AudioStringToId(path.c_str()), eventType, pEventDescription, guid, path.c_str());
#else
			pAudioTrigger = new CAudioTrigger(AudioStringToId(path.c_str()), eventType, pEventDescription, guid);
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Fmod snapshot: %s", path.c_str());
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Fmod tag: %s", szTag);
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
	CAudioParameter* pFmodAudioParameter = nullptr;
	char const* const szTag = pAudioParameterNode->getTag();
	stack_string path;

	if (_stricmp(szTag, s_szFmodEventParameterTag) == 0)
	{
		path = s_szFmodEventPrefix;
	}
	else if (_stricmp(szTag, s_szFmodSnapshotParameterTag) == 0)
	{
		path = s_szFmodSnapshotPrefix;
	}

	if (!path.empty())
	{
		char const* const szName = pAudioParameterNode->getAttr(s_szFmodNameAttribute);
		char const* const szPath = pAudioParameterNode->getAttr(s_szFmodPathAttribute);
		path += szPath;
		uint32 const pathId = AudioStringToId(path.c_str());

		float multiplier = 1.0f;
		float shift = 0.0f;
		pAudioParameterNode->getAttr(s_szFmodMutiplierAttribute, multiplier);
		pAudioParameterNode->getAttr(s_szFmodShiftAttribute, shift);

		pFmodAudioParameter = new CAudioParameter(pathId, multiplier, shift, szName);
		g_parameterToIndex.emplace(std::piecewise_construct, std::make_tuple(pFmodAudioParameter), std::make_tuple(FMOD_IMPL_INVALID_INDEX));
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Fmod tag: %s", szTag);
	}

	return static_cast<IParameter*>(pFmodAudioParameter);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioParameter(IParameter const* const pIParameter)
{
	CAudioParameter const* const pFmodAudioParameter = static_cast<CAudioParameter const* const>(pIParameter);

	for (auto const pAudioObject : m_constructedAudioObjects)
	{
		pAudioObject->RemoveParameter(pFmodAudioParameter);
	}

	g_parameterToIndex.erase(pFmodAudioParameter);
	delete pIParameter;
}

///////////////////////////////////////////////////////////////////////////
IAudioSwitchState const* CAudioImpl::NewAudioSwitchState(XmlNodeRef const pAudioSwitchNode)
{
	CAudioSwitchState* pFmodAudioSwitchState = nullptr;
	char const* const szTag = pAudioSwitchNode->getTag();
	stack_string path;

	if (_stricmp(szTag, s_szFmodEventParameterTag) == 0)
	{
		path = s_szFmodEventPrefix;
	}
	else if (_stricmp(szTag, s_szFmodSnapshotParameterTag) == 0)
	{
		path = s_szFmodSnapshotPrefix;
	}

	if (!path.empty())
	{
		char const* const szFmodParameterName = pAudioSwitchNode->getAttr(s_szFmodNameAttribute);
		char const* const szFmodPath = pAudioSwitchNode->getAttr(s_szFmodPathAttribute);
		char const* const szFmodParameterValue = pAudioSwitchNode->getAttr(s_szFmodValueAttribute);
		path += szFmodPath;
		uint32 const pathId = AudioStringToId(path.c_str());
		float const value = static_cast<float>(atof(szFmodParameterValue));
		pFmodAudioSwitchState = new CAudioSwitchState(pathId, value, szFmodParameterName);
		g_switchToIndex.emplace(std::piecewise_construct, std::make_tuple(pFmodAudioSwitchState), std::make_tuple(FMOD_IMPL_INVALID_INDEX));
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Fmod tag: %s", szTag);
	}

	return static_cast<IAudioSwitchState*>(pFmodAudioSwitchState);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioSwitchState(IAudioSwitchState const* const pISwitchState)
{
	CAudioSwitchState const* const pFmodAudioSwitchState = static_cast<CAudioSwitchState const* const>(pISwitchState);

	for (auto const pAudioObject : m_constructedAudioObjects)
	{
		pAudioObject->RemoveSwitch(pFmodAudioSwitchState);
	}

	g_switchToIndex.erase(pFmodAudioSwitchState);
	delete pISwitchState;
}

///////////////////////////////////////////////////////////////////////////
IAudioEnvironment const* CAudioImpl::NewAudioEnvironment(XmlNodeRef const pAudioEnvironmentNode)
{
	IAudioEnvironment* pAudioEnvironment = nullptr;
	char const* const szTag = pAudioEnvironmentNode->getTag();

	if (_stricmp(szTag, s_szFmodBusTag) == 0)
	{
		stack_string path(s_szFmodBusPrefix);
		path += pAudioEnvironmentNode->getAttr(s_szFmodNameAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::Bus* pBus = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getBusByID(&guid, &pBus);
			ASSERT_FMOD_OK;
			pAudioEnvironment = new CAudioEnvironment(nullptr, pBus);
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Fmod bus: %s", path.c_str());
		}
	}
	else if (_stricmp(szTag, s_szFmodSnapshotTag) == 0)
	{
		stack_string path(s_szFmodSnapshotPrefix);
		path += pAudioEnvironmentNode->getAttr(s_szFmodNameAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::EventDescription* pEventDescription = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getEventByID(&guid, &pEventDescription);
			ASSERT_FMOD_OK;
			pAudioEnvironment = new CAudioEnvironment(pEventDescription, nullptr);
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Fmod snapshot: %s", path.c_str());
		}
	}

	return pAudioEnvironment;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioEnvironment(IAudioEnvironment const* const pIEnvironment)
{
	CAudioEnvironment const* const pFmodAudioEnvironment = static_cast<CAudioEnvironment const* const>(pIEnvironment);

	for (auto const pAudioObject : m_constructedAudioObjects)
	{
		pAudioObject->RemoveEnvironment(pFmodAudioEnvironment);
	}

	delete pIEnvironment;
}

///////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetImplementationNameString() const
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	return m_fullImplString.c_str();
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const
{
	CryModuleMemoryInfo memInfo;
	ZeroStruct(memInfo);
	CryGetMemoryInfoForModule(&memInfo);

	memoryInfo.totalMemory = static_cast<size_t>(memInfo.allocated - memInfo.freed);

#if defined(PROVIDE_FMOD_IMPL_SECONDARY_POOL)
	memoryInfo.secondaryPoolSize = g_audioImplMemoryPoolSecondary.MemSize();
	memoryInfo.secondaryPoolUsedSize = memoryInfo.secondaryPoolSize - g_audioImplMemoryPoolSecondary.MemFree();
	memoryInfo.secondaryPoolAllocations = g_audioImplMemoryPoolSecondary.FragmentCount();
#else
	memoryInfo.secondaryPoolSize = 0;
	memoryInfo.secondaryPoolUsedSize = 0;
	memoryInfo.secondaryPoolAllocations = 0;
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

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
	UnloadMasterBanks();
	LoadMasterBanks();
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::SetLanguage(char const* const szLanguage)
{
	if (szLanguage != nullptr)
	{
		m_language = szLanguage;
		m_localizedSoundBankFolder = PathUtil::GetGameFolder().c_str();
		m_localizedSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
		m_localizedSoundBankFolder += PathUtil::GetLocalizationFolder();
		m_localizedSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
		m_localizedSoundBankFolder += m_language.c_str();
		m_localizedSoundBankFolder += CRY_NATIVE_PATH_SEPSTR;
		m_localizedSoundBankFolder += FMOD_IMPL_DATA_ROOT;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::CreateVersionString(CryFixedStringT<MaxMiscStringLength>& stringOut) const
{
	// Remove the leading zeros on the upper 16 bit and inject the 2 dots between the 3 groups
	size_t const stringLength = stringOut.size();
	for (size_t i = 0; i < stringLength; ++i)
	{
		if (stringOut.c_str()[0] == '0')
		{
			stringOut.erase(0, 1);
		}
		else
		{
			if (i < 4)
			{
				stringOut.insert(4 - i, '.'); // First dot
				stringOut.insert(7 - i, '.'); // Second dot
				break;
			}
			else
			{
				// This shouldn't happen therefore clear the string and back out
				stringOut.clear();
				return;
			}
		}
	}
}

struct SFmodFileData
{
	void*        pData;
	int unsigned fileSize;
};

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK FmodFileOpenCallback(const char* szName, unsigned int* pFileSize, void** pHandle, void* pUserData)
{
	SFmodFileData* const pFileData = static_cast<SFmodFileData*>(pUserData);
	*pHandle = pFileData->pData;
	*pFileSize = pFileData->fileSize;

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK FmodFileCloseCallback(void* pHandle, void* pUserData)
{
	FMOD_RESULT fmodResult = FMOD_ERR_FILE_NOTFOUND;
	FILE* const pFile = static_cast<FILE*>(pHandle);

	if (gEnv->pCryPak->FClose(pFile) == 0)
	{
		fmodResult = FMOD_OK;
	}

	return fmodResult;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK FmodFileReadCallback(void* pHandle, void* pBuffer, unsigned int sizeBytes, unsigned int* pBytesRead, void* pUserData)
{
	FMOD_RESULT fmodResult = FMOD_ERR_FILE_NOTFOUND;
	FILE* const pFile = static_cast<FILE*>(pHandle);
	size_t const bytesRead = gEnv->pCryPak->FReadRaw(pBuffer, 1, static_cast<size_t>(sizeBytes), pFile);
	*pBytesRead = bytesRead;

	if (bytesRead != sizeBytes)
	{
		fmodResult = FMOD_ERR_FILE_EOF;
	}
	else if (bytesRead > 0)
	{
		fmodResult = FMOD_OK;
	}

	return fmodResult;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK FmodFileSeekCallback(void* pHandle, unsigned int pos, void* pUserData)
{
	FMOD_RESULT fmodResult = FMOD_ERR_FILE_COULDNOTSEEK;
	FILE* const pFile = static_cast<FILE*>(pHandle);

	if (gEnv->pCryPak->FSeek(pFile, static_cast<long>(pos), 0) == 0)
	{
		fmodResult = FMOD_OK;
	}

	return fmodResult;
}

//////////////////////////////////////////////////////////////////////////
bool CAudioImpl::LoadMasterBanks()
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
	CryFixedStringT<MaxFileNameLength> masterBankPath;
	CryFixedStringT<MaxFileNameLength> masterBankStringsPath;
	CryFixedStringT<MaxFilePathLength + MaxFileNameLength> search(m_regularSoundBankFolder + CRY_NATIVE_PATH_SEPSTR "*.bank");
	_finddata_t fd;
	intptr_t const handle = gEnv->pCryPak->FindFirst(search.c_str(), &fd);

	if (handle != -1)
	{
		do
		{
			masterBankStringsPath = fd.name;
			size_t const substrPos = masterBankStringsPath.find(".strings.bank");

			if (substrPos != masterBankStringsPath.npos)
			{
				masterBankPath = m_regularSoundBankFolder.c_str();
				masterBankPath += CRY_NATIVE_PATH_SEPSTR;
				masterBankPath += masterBankStringsPath.substr(0, substrPos);
				masterBankPath += ".bank";
				masterBankStringsPath.insert(0, CRY_NATIVE_PATH_SEPSTR);
				masterBankStringsPath.insert(0, m_regularSoundBankFolder.c_str());
				break;
			}

		}
		while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

		gEnv->pCryPak->FindClose(handle);
	}

	if (!masterBankPath.empty() && !masterBankStringsPath.empty())
	{
		size_t const masterBankFileSize = gEnv->pCryPak->FGetSize(masterBankPath.c_str());
		CRY_ASSERT(masterBankFileSize > 0);
		size_t const masterBankStringsFileSize = gEnv->pCryPak->FGetSize(masterBankStringsPath.c_str());
		CRY_ASSERT(masterBankStringsFileSize > 0);

		if (masterBankFileSize > 0 && masterBankStringsFileSize > 0)
		{
			FILE* const pMasterBank = gEnv->pCryPak->FOpen(masterBankPath.c_str(), "rbx", ICryPak::FOPEN_HINT_DIRECT_OPERATION);
			FILE* const pStringsBank = gEnv->pCryPak->FOpen(masterBankStringsPath.c_str(), "rbx", ICryPak::FOPEN_HINT_DIRECT_OPERATION);

			SFmodFileData fileData;
			fileData.pData = static_cast<void*>(pMasterBank);
			fileData.fileSize = static_cast<int>(masterBankFileSize);

			FMOD_STUDIO_BANK_INFO bankInfo;
			ZeroStruct(bankInfo);
			bankInfo.closecallback = &FmodFileCloseCallback;
			bankInfo.opencallback = &FmodFileOpenCallback;
			bankInfo.readcallback = &FmodFileReadCallback;
			bankInfo.seekcallback = &FmodFileSeekCallback;
			bankInfo.size = sizeof(bankInfo);
			bankInfo.userdata = static_cast<void*>(&fileData);
			bankInfo.userdatalength = sizeof(SFmodFileData);

			fmodResult = m_pSystem->loadBankCustom(&bankInfo, FMOD_STUDIO_LOAD_BANK_NORMAL, &m_pMasterBank);
			ASSERT_FMOD_OK;
			fileData.pData = static_cast<void*>(pStringsBank);
			fileData.fileSize = static_cast<int>(masterBankStringsFileSize);
			fmodResult = m_pSystem->loadBankCustom(&bankInfo, FMOD_STUDIO_LOAD_BANK_NORMAL, &m_pStringsBank);
			ASSERT_FMOD_OK;
			if (m_pMasterBank != nullptr)
			{
				int numBuses = 0;
				fmodResult = m_pMasterBank->getBusCount(&numBuses);
				ASSERT_FMOD_OK;

				if (numBuses > 0)
				{
					FMOD::Studio::Bus** pBuses = new FMOD::Studio::Bus*[numBuses];
					int numRetrievedBuses = 0;
					fmodResult = m_pMasterBank->getBusList(pBuses, numBuses, &numRetrievedBuses);
					ASSERT_FMOD_OK;
					CRY_ASSERT(numBuses == numRetrievedBuses);

					for (int i = 0; i < numRetrievedBuses; ++i)
					{
						fmodResult = pBuses[i]->lockChannelGroup();
						ASSERT_FMOD_OK;
					}

					delete[] pBuses;
				}
			}
		}
	}
	else
	{
		// This does not qualify for a fallback to the NULL implementation!
		// Still notify the user about this failure!
		g_audioImplLogger.Log(eAudioLogType_Error, "Fmod failed to load master banks");
		return true;
	}

	return (fmodResult == FMOD_OK);
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::UnloadMasterBanks()
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	if (m_pStringsBank != nullptr)
	{
		fmodResult = m_pStringsBank->unload();
		ASSERT_FMOD_OK;
		m_pStringsBank = nullptr;
	}

	if (m_pMasterBank != nullptr)
	{
		fmodResult = m_pMasterBank->unload();
		ASSERT_FMOD_OK;
		m_pMasterBank = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::MuteMasterBus(bool const bMute)
{
	FMOD::Studio::Bus* pMasterBus = nullptr;
	FMOD_RESULT fmodResult = m_pSystem->getBus(s_szFmodBusPrefix, &pMasterBus);
	ASSERT_FMOD_OK;

	if (pMasterBus != nullptr)
	{
		fmodResult = pMasterBus->setMute(bMute);
		ASSERT_FMOD_OK;
	}

	return (fmodResult == FMOD_OK) ? eRequestStatus_Success : eRequestStatus_Failure;
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const
{
	FMOD::Sound* pSound = nullptr;
	FMOD_CREATESOUNDEXINFO info;
	ZeroStruct(info);
	info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	info.decodebuffersize = 1;
	FMOD_RESULT const fmodResult = m_pLowLevelSystem->createStream(szFilename, FMOD_OPENONLY, &info, &pSound);
	ASSERT_FMOD_OK;

	if (pSound != nullptr)
	{
		unsigned int length = 0;
		pSound->getLength(&length, FMOD_TIMEUNIT_MS);
		audioFileData.duration = length / 1000.0f; // convert to seconds
	}
}
