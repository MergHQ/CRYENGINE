// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "Common.h"
#include "Event.h"
#include "CVars.h"
#include "EnvironmentBus.h"
#include "EnvironmentParameter.h"
#include "Event.h"
#include "File.h"
#include "Listener.h"
#include "GlobalObject.h"
#include "Object.h"
#include "Parameter.h"
#include "ProgrammerSoundFile.h"
#include "Setting.h"
#include "StandaloneFile.h"
#include "SwitchState.h"
#include "Trigger.h"
#include "VcaParameter.h"
#include "VcaState.h"
#include "GlobalData.h"
#include <FileInfo.h>
#include <Logger.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include <DebugStyle.h>
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
SPoolSizes g_poolSizes;
SPoolSizes g_poolSizesLevelSpecific;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
SPoolSizes g_debugPoolSizes;
Events g_constructedEvents;
uint16 g_objectPoolSize = 0;
uint16 g_eventPoolSize = 0;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

char const* const CImpl::s_szEventPrefix = "event:/";
char const* const CImpl::s_szSnapshotPrefix = "snapshot:/";
char const* const CImpl::s_szBusPrefix = "bus:/";
char const* const CImpl::s_szVcaPrefix = "vca:/";

static constexpr size_t s_maxFileSize = size_t(1) << size_t(31);

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

	uint16 numEnvBuses = 0;
	pNode->getAttr(s_szEnvBusesAttribute, numEnvBuses);
	poolSizes.envBuses += numEnvBuses;

	uint16 numEnvParameters = 0;
	pNode->getAttr(s_szEnvParametersAttribute, numEnvParameters);
	poolSizes.envParameters += numEnvParameters;

	uint16 numVcaParameters = 0;
	pNode->getAttr(s_szVcaParametersAttribute, numVcaParameters);
	poolSizes.vcaParameters += numVcaParameters;

	uint16 numVcaStates = 0;
	pNode->getAttr(s_szVcaStatesAttribute, numVcaStates);
	poolSizes.vcaStates += numVcaStates;

	uint16 numFiles = 0;
	pNode->getAttr(s_szFilesAttribute, numFiles);
	poolSizes.files += numFiles;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	CObject::CreateAllocator(objectPoolSize);
	CEvent::CreateAllocator(eventPoolSize);
	CTrigger::CreateAllocator(g_poolSizes.triggers);
	CParameter::CreateAllocator(g_poolSizes.parameters);
	CSwitchState::CreateAllocator(g_poolSizes.switchStates);
	CEnvironmentBus::CreateAllocator(g_poolSizes.envBuses);
	CEnvironmentParameter::CreateAllocator(g_poolSizes.envParameters);
	CVcaParameter::CreateAllocator(g_poolSizes.vcaParameters);
	CVcaState::CreateAllocator(g_poolSizes.vcaStates);
	CFile::CreateAllocator(g_poolSizes.files);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CObject::FreeMemoryPool();
	CEvent::FreeMemoryPool();
	CTrigger::FreeMemoryPool();
	CParameter::FreeMemoryPool();
	CSwitchState::FreeMemoryPool();
	CEnvironmentBus::FreeMemoryPool();
	CEnvironmentParameter::FreeMemoryPool();
	CVcaParameter::FreeMemoryPool();
	CVcaState::FreeMemoryPool();
	CFile::FreeMemoryPool();
}

///////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_pSystem(nullptr)
	, m_pLowLevelSystem(nullptr)
	, m_pMasterBank(nullptr)
	, m_pMasterAssetsBank(nullptr)
	, m_pMasterStreamsBank(nullptr)
	, m_pMasterStringsBank(nullptr)
{
	g_pImpl = this;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::Update()
{
	if (m_pSystem != nullptr)
	{
		FMOD_RESULT const fmodResult = m_pSystem->update();
		ASSERT_FMOD_OK;
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize)
{
	if (g_cvars.m_eventPoolSize < 1)
	{
		g_cvars.m_eventPoolSize = 1;
		Cry::Audio::Log(ELogType::Warning, R"(Event pool size must be at least 1. Forcing the cvar "s_FmodEventPoolSize" to 1!)");
	}

	g_constructedObjects.reserve(static_cast<size_t>(objectPoolSize));
	AllocateMemoryPools(objectPoolSize, static_cast<uint16>(g_cvars.m_eventPoolSize));

	m_regularSoundBankFolder = AUDIO_SYSTEM_DATA_ROOT;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szAssetsFolderName;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

	FMOD_RESULT fmodResult = FMOD::Studio::System::create(&m_pSystem);
	ASSERT_FMOD_OK;
	fmodResult = m_pSystem->getLowLevelSystem(&m_pLowLevelSystem);
	ASSERT_FMOD_OK;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	g_constructedEvents.reserve(static_cast<size_t>(g_cvars.m_eventPoolSize));

	g_objectPoolSize = objectPoolSize;
	g_eventPoolSize = static_cast<uint16>(g_cvars.m_eventPoolSize);

	uint32 version = 0;
	fmodResult = m_pLowLevelSystem->getVersion(&version);
	ASSERT_FMOD_OK;

	CryFixedStringT<MaxInfoStringLength> systemVersion;
	systemVersion.Format("%08x", version);
	CryFixedStringT<MaxInfoStringLength> headerVersion;
	headerVersion.Format("%08x", FMOD_VERSION);
	CreateVersionString(systemVersion);
	CreateVersionString(headerVersion);

	m_name = FMOD_IMPL_INFO_STRING;
	m_name += "System: ";
	m_name += systemVersion;
	m_name += " - Header: ";
	m_name += headerVersion;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	int sampleRate = 0;
	int numRawSpeakers = 0;
	FMOD_SPEAKERMODE speakerMode = FMOD_SPEAKERMODE_DEFAULT;
	fmodResult = m_pLowLevelSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers);
	ASSERT_FMOD_OK;
	fmodResult = m_pLowLevelSystem->setSoftwareFormat(sampleRate, speakerMode, numRawSpeakers);
	ASSERT_FMOD_OK;

	fmodResult = m_pLowLevelSystem->set3DSettings(g_cvars.m_dopplerScale, g_cvars.m_distanceFactor, g_cvars.m_rolloffScale);
	ASSERT_FMOD_OK;

	void* pExtraDriverData = nullptr;
	int initFlags = FMOD_INIT_NORMAL | FMOD_INIT_VOL0_BECOMES_VIRTUAL;
	int studioInitFlags = FMOD_STUDIO_INIT_NORMAL;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	if (g_cvars.m_enableLiveUpdate > 0)
	{
		studioInitFlags |= FMOD_STUDIO_INIT_LIVEUPDATE;
	}
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	if (g_cvars.m_enableSynchronousUpdate > 0)
	{
		studioInitFlags |= FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE;
	}

	fmodResult = m_pSystem->initialize(g_cvars.m_maxChannels, studioInitFlags, initFlags, pExtraDriverData);
	ASSERT_FMOD_OK;

	LoadMasterBanks();

	FMOD_3D_ATTRIBUTES attributes = {
		{ 0 } };
	attributes.forward.z = 1.0f;
	attributes.up.y = 1.0f;
	fmodResult = m_pSystem->setListenerAttributes(0, &attributes);
	ASSERT_FMOD_OK;

	CBaseObject::s_pSystem = m_pSystem;
	CListener::s_pSystem = m_pSystem;
	CBaseStandaloneFile::s_pLowLevelSystem = m_pLowLevelSystem;

	return (fmodResult == FMOD_OK) ? ERequestStatus::Success : ERequestStatus::Failure;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
	if (m_pSystem != nullptr)
	{
		UnloadMasterBanks();

		FMOD_RESULT const fmodResult = m_pSystem->release();
		ASSERT_FMOD_OK;
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
		SPoolSizes levelPoolSizes;
		CountPoolSizes(pNode, levelPoolSizes);

		g_poolSizesLevelSpecific.triggers = std::max(g_poolSizesLevelSpecific.triggers, levelPoolSizes.triggers);
		g_poolSizesLevelSpecific.parameters = std::max(g_poolSizesLevelSpecific.parameters, levelPoolSizes.parameters);
		g_poolSizesLevelSpecific.switchStates = std::max(g_poolSizesLevelSpecific.switchStates, levelPoolSizes.switchStates);
		g_poolSizesLevelSpecific.envBuses = std::max(g_poolSizesLevelSpecific.envBuses, levelPoolSizes.envBuses);
		g_poolSizesLevelSpecific.envParameters = std::max(g_poolSizesLevelSpecific.envParameters, levelPoolSizes.envParameters);
		g_poolSizesLevelSpecific.vcaParameters = std::max(g_poolSizesLevelSpecific.vcaParameters, levelPoolSizes.vcaParameters);
		g_poolSizesLevelSpecific.vcaStates = std::max(g_poolSizesLevelSpecific.vcaStates, levelPoolSizes.vcaStates);
		g_poolSizesLevelSpecific.files = std::max(g_poolSizesLevelSpecific.files, levelPoolSizes.files);
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

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	ZeroStruct(g_debugPoolSizes);
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_poolSizes.triggers += g_poolSizesLevelSpecific.triggers;
	g_poolSizes.parameters += g_poolSizesLevelSpecific.parameters;
	g_poolSizes.switchStates += g_poolSizesLevelSpecific.switchStates;
	g_poolSizes.envBuses += g_poolSizesLevelSpecific.envBuses;
	g_poolSizes.envParameters += g_poolSizesLevelSpecific.envParameters;
	g_poolSizes.vcaParameters += g_poolSizesLevelSpecific.vcaParameters;
	g_poolSizes.vcaStates += g_poolSizesLevelSpecific.vcaStates;
	g_poolSizes.files += g_poolSizesLevelSpecific.files;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	g_poolSizes.triggers = std::max<uint16>(1, g_poolSizes.triggers);
	g_poolSizes.parameters = std::max<uint16>(1, g_poolSizes.parameters);
	g_poolSizes.switchStates = std::max<uint16>(1, g_poolSizes.switchStates);
	g_poolSizes.envBuses = std::max<uint16>(1, g_poolSizes.envBuses);
	g_poolSizes.envParameters = std::max<uint16>(1, g_poolSizes.envParameters);
	g_poolSizes.vcaParameters = std::max<uint16>(1, g_poolSizes.vcaParameters);
	g_poolSizes.vcaStates = std::max<uint16>(1, g_poolSizes.vcaStates);
	g_poolSizes.files = std::max<uint16>(1, g_poolSizes.files);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnLoseFocus()
{
	MuteMasterBus(true);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnGetFocus()
{
	MuteMasterBus(false);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::MuteAll()
{
	MuteMasterBus(true);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::UnmuteAll()
{
	MuteMasterBus(false);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::PauseAll()
{
	PauseMasterBus(true);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ResumeAll()
{
	PauseMasterBus(false);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::StopAllSounds()
{
	FMOD::Studio::Bus* pMasterBus = nullptr;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	if (m_pSystem->getBus(s_szBusPrefix, &pMasterBus) != FMOD_OK)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to get master bus during %s", __FUNCTION__);
	}
#else
	m_pSystem->getBus(s_szBusPrefix, &pMasterBus);
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	if (pMasterBus != nullptr)
	{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
		if (pMasterBus->stopAllEvents(FMOD_STUDIO_STOP_IMMEDIATE) != FMOD_OK)
		{
			Cry::Audio::Log(ELogType::Error, "Failed to stop all events during %s", __FUNCTION__);
		}
#else
		pMasterBus->stopAllEvents(FMOD_STUDIO_STOP_IMMEDIATE);
#endif    // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			// loadBankMemory requires 32-byte alignment when using FMOD_STUDIO_LOAD_MEMORY_POINT
			if ((reinterpret_cast<uintptr_t>(pFileInfo->pFileData) & (32 - 1)) > 0)
			{
				CryFatalError("<Audio>: allocation not %d byte aligned!", 32);
			}
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

			FMOD_RESULT fmodResult = m_pSystem->loadBankMemory(static_cast<char*>(pFileInfo->pFileData), static_cast<int>(pFileInfo->size), FMOD_STUDIO_LOAD_MEMORY_POINT, FMOD_STUDIO_LOAD_BANK_NORMAL, &pFileData->pBank);
			ASSERT_FMOD_OK;

			CryFixedStringT<MaxFilePathLength> streamsBankPath = pFileInfo->szFilePath;
			PathUtil::RemoveExtension(streamsBankPath);
			streamsBankPath += ".streams.bank";

			size_t const streamsBankFileSize = gEnv->pCryPak->FGetSize(streamsBankPath.c_str());

			if (streamsBankFileSize > 0)
			{
				pFileData->m_streamsBankPath = streamsBankPath;
				fmodResult = LoadBankCustom(pFileData->m_streamsBankPath.c_str(), &pFileData->pStreamsBank);
				ASSERT_FMOD_OK;
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Fmod implementation of %s", __FUNCTION__);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			FMOD_RESULT fmodResult = pFileData->pBank->unload();
			ASSERT_FMOD_OK;

			FMOD_STUDIO_LOADING_STATE loadingState;

			do
			{
				fmodResult = m_pSystem->update();
				ASSERT_FMOD_OK;
				fmodResult = pFileData->pBank->getLoadingState(&loadingState);
				ASSERT_FMOD_OK_OR_INVALID_HANDLE;
			}
			while (loadingState == FMOD_STUDIO_LOADING_STATE_UNLOADING);

			pFileData->pBank = nullptr;

			if (pFileData->pStreamsBank != nullptr)
			{
				fmodResult = pFileData->pStreamsBank->unload();
				ASSERT_FMOD_OK;
				pFileData->pStreamsBank = nullptr;
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Fmod implementation of %s", __FUNCTION__);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ConstructFile(XmlNodeRef const pRootNode, SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if ((_stricmp(pRootNode->getTag(), s_szFileTag) == 0) && (pFileInfo != nullptr))
	{
		char const* const szFileName = pRootNode->getAttr(s_szNameAttribute);

		if (szFileName != nullptr && szFileName[0] != '\0')
		{
			char const* const szLocalized = pRootNode->getAttr(s_szLocalizedAttribute);
			pFileInfo->bLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, s_szTrueValue) == 0);
			pFileInfo->szFileName = szFileName;

			// FMOD Studio always uses 32 byte alignment for preloaded banks regardless of the platform.
			pFileInfo->memoryBlockAlignment = 32;

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CFile");
			pFileInfo->pImplData = new CFile();

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
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	implInfo.folderName = s_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CGlobalObject");
	new CGlobalObject;

	if (!stl::push_back_unique(g_constructedObjects, static_cast<CBaseObject*>(g_pObject)))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}

	return static_cast<IObject*>(g_pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CObject");
	CBaseObject* const pObject = new CObject(transformation);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	if (szName != nullptr)
	{
		pObject->SetName(szName);
	}
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	if (!stl::push_back_unique(g_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pObject = static_cast<CBaseObject const*>(pIObject);

	if (!stl::find_and_erase(g_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to delete a non-existing object.");
	}

	delete pObject;
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	static int id = 0;

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CListener");
	g_pListener = new CListener(transformation, id++);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	if (szName != nullptr)
	{
		g_pListener->SetName(szName);
	}
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

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
	string filePath;

	if (bLocalized)
	{
		filePath = string(m_localizedSoundBankFolder + "/" + szFile) + ".mp3";
	}
	else
	{
		filePath = string(szFile) + ".mp3";
	}

	CBaseStandaloneFile* pFile = nullptr;

	if (pITriggerConnection != nullptr)
	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CProgrammerSoundFile");
		pFile = new CProgrammerSoundFile(filePath, static_cast<CTrigger const* const>(pITriggerConnection)->GetGuid(), standaloneFile);
	}
	else
	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CStandaloneFile");
		pFile = new CStandaloneFile(filePath, standaloneFile);
	}

	return static_cast<IStandaloneFileConnection*>(pFile);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructStandaloneFileConnection(IStandaloneFileConnection const* const pIStandaloneFileConnection)
{
	CRY_ASSERT_MESSAGE(pIStandaloneFileConnection != nullptr, "pIStandaloneFileConnection is nullptr during %s", __FUNCTION__);
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
		stack_string path(s_szEventPrefix);
		path += pRootNode->getAttr(s_szNameAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			EActionType actionType = EActionType::Start;
			char const* const szEventType = pRootNode->getAttr(s_szTypeAttribute);

			if ((szEventType != nullptr) && (szEventType[0] != '\0'))
			{
				if (_stricmp(szEventType, s_szStopValue) == 0)
				{
					actionType = EActionType::Stop;
				}
				else if (_stricmp(szEventType, s_szPauseValue) == 0)
				{
					actionType = EActionType::Pause;
				}
				else if (_stricmp(szEventType, s_szResumeValue) == 0)
				{
					actionType = EActionType::Resume;
				}
			}

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CTrigger");
			pTrigger = new CTrigger(StringToId(path.c_str()), actionType, nullptr, guid);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pTrigger->SetName(pRootNode->getAttr(s_szNameAttribute));
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod event: %s", path.c_str());
		}
	}
	else if (_stricmp(szTag, s_szKeyTag) == 0)
	{
		stack_string path(s_szEventPrefix);
		path += pRootNode->getAttr(s_szEventAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			char const* const szKey = pRootNode->getAttr(s_szNameAttribute);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CTrigger");
			pTrigger = new CTrigger(StringToId(path.c_str()), EActionType::Start, nullptr, guid, true, szKey);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pTrigger->SetName(szKey);
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod event: %s", path.c_str());
		}
	}
	else if (_stricmp(szTag, s_szSnapshotTag) == 0)
	{
		stack_string path(s_szSnapshotPrefix);
		path += pRootNode->getAttr(s_szNameAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			EActionType actionType = EActionType::Start;
			char const* const szFmodEventType = pRootNode->getAttr(s_szTypeAttribute);

			if ((szFmodEventType != nullptr) && (szFmodEventType[0] != '\0') && (_stricmp(szFmodEventType, s_szStopValue) == 0))
			{
				actionType = EActionType::Stop;
			}

			FMOD::Studio::EventDescription* pEventDescription = nullptr;
			m_pSystem->getEventByID(&guid, &pEventDescription);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CTrigger");
			pTrigger = new CTrigger(StringToId(path.c_str()), actionType, pEventDescription, guid);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pTrigger->SetName(pRootNode->getAttr(s_szNameAttribute));
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod snapshot: %s", path.c_str());
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}

	return static_cast<ITriggerConnection*>(pTrigger);
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	ITriggerConnection* pITriggerConnection = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		stack_string path(s_szEventPrefix);
		path += pTriggerInfo->name.c_str();
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CTrigger");
			auto const pTrigger = new CTrigger(StringToId(path.c_str()), EActionType::Start, nullptr, guid);

	#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pTrigger->SetName(pTriggerInfo->name.c_str());
	#endif    // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

			pITriggerConnection = static_cast<ITriggerConnection*>(pTrigger);
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod event: %s", path.c_str());
		}
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	auto const pTrigger = static_cast<CTrigger const*>(pITriggerConnection);
	g_triggerToParameterIndexes.erase(pTrigger);
	delete pTrigger;
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	IParameterConnection* pIParameterConnection = nullptr;
	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szParameterTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		float multiplier = s_defaultParamMultiplier;
		float shift = s_defaultParamShift;
		pRootNode->getAttr(s_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(s_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CParameter");

		auto const pParameter = new CParameter(StringToId(szName), multiplier, shift);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
		pParameter->SetName(szName);
#endif    // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

		pIParameterConnection = static_cast<IParameterConnection*>(pParameter);
	}
	else if (_stricmp(szTag, s_szVcaTag) == 0)
	{
		stack_string fullName(s_szVcaPrefix);
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		fullName += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(fullName.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::VCA* pVca = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getVCAByID(&guid, &pVca);
			ASSERT_FMOD_OK;

			float multiplier = s_defaultParamMultiplier;
			float shift = s_defaultParamShift;
			pRootNode->getAttr(s_szMutiplierAttribute, multiplier);
			pRootNode->getAttr(s_szShiftAttribute, shift);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CVcaParameter");
			auto const pVcaParameter = new CVcaParameter(StringToId(fullName.c_str()), multiplier, shift, pVca);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pVcaParameter->SetName(szName);
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

			pIParameterConnection = static_cast<IParameterConnection*>(pVcaParameter);
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod VCA: %s", fullName.c_str());
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}

	return pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	auto const pParameter = static_cast<CBaseParameter const*>(pIParameterConnection);

	for (auto const pObject : g_constructedObjects)
	{
		pObject->RemoveParameter(pParameter);
	}

	delete pParameter;
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	ISwitchStateConnection* pISwitchStateConnection = nullptr;
	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szParameterTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		char const* const szValue = pRootNode->getAttr(s_szValueAttribute);
		auto const value = static_cast<float>(atof(szValue));

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CSwitchState");
		auto const pSwitchState = new CSwitchState(StringToId(szName), value);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
		pSwitchState->SetName(szName);
#endif    // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

		pISwitchStateConnection = static_cast<ISwitchStateConnection*>(pSwitchState);
	}
	else if (_stricmp(szTag, s_szVcaTag) == 0)
	{
		stack_string fullName(s_szVcaPrefix);
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		fullName += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(fullName.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::VCA* pVca = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getVCAByID(&guid, &pVca);
			ASSERT_FMOD_OK;

			char const* const szValue = pRootNode->getAttr(s_szValueAttribute);
			auto const value = static_cast<float>(atof(szValue));

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CVcaState");
			auto const pVcaState = new CVcaState(StringToId(fullName.c_str()), value, pVca);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pVcaState->SetName(szName);
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

			pISwitchStateConnection = static_cast<ISwitchStateConnection*>(pVcaState);
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod VCA: %s", fullName.c_str());
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}

	return pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	auto const pSwitchState = static_cast<CBaseSwitchState const*>(pISwitchStateConnection);

	for (auto const pObject : g_constructedObjects)
	{
		pObject->RemoveSwitch(pSwitchState);
	}

	delete pSwitchState;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	CEnvironment* pEnvironment = nullptr;
	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szBusTag) == 0)
	{
		stack_string path(s_szBusPrefix);
		path += pRootNode->getAttr(s_szNameAttribute);
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::Bus* pBus = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getBusByID(&guid, &pBus);
			ASSERT_FMOD_OK;

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CEnvironmentBus");
			pEnvironment = new CEnvironmentBus(nullptr, pBus);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			pEnvironment->SetName(pRootNode->getAttr(s_szNameAttribute));
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod bus: %s", path.c_str());
		}
	}
	else if (_stricmp(szTag, s_szParameterTag) == 0)
	{

		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		float multiplier = s_defaultParamMultiplier;
		float shift = s_defaultParamShift;
		pRootNode->getAttr(s_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(s_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CEnvironmentParameter");
		pEnvironment = new CEnvironmentParameter(StringToId(szName), multiplier, shift);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
		pEnvironment->SetName(szName);
#endif    // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}

	return static_cast<IEnvironmentConnection*>(pEnvironment);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
	auto const pEnvironment = static_cast<CEnvironment const*>(pIEnvironmentConnection);

	for (auto const pObject : g_constructedObjects)
	{
		pObject->RemoveEnvironment(pEnvironment);
	}

	delete pEnvironment;
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CSetting");
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
	UnloadMasterBanks();
	LoadMasterBanks();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLanguage(char const* const szLanguage)
{
	if (szLanguage != nullptr)
	{
		m_language = szLanguage;
		m_localizedSoundBankFolder = PathUtil::GetLocalizationFolder().c_str();
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += m_language.c_str();
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += AUDIO_SYSTEM_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szAssetsFolderName;
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::CreateVersionString(CryFixedStringT<MaxInfoStringLength>& string) const
{
	// Remove the leading zeros on the upper 16 bit and inject the 2 dots between the 3 groups
	size_t const stringLength = string.size();
	for (size_t i = 0; i < stringLength; ++i)
	{
		if (string.c_str()[0] == '0')
		{
			string.erase(0, 1);
		}
		else
		{
			if (i < 4)
			{
				string.insert(4 - i, '.'); // First dot
				string.insert(7 - i, '.'); // Second dot
				break;
			}
			else
			{
				// This shouldn't happen therefore clear the string and back out
				string.clear();
				return;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK FmodFileOpenCallback(const char* szName, unsigned int* pFileSize, void** pHandle, void* pUserData)
{
	FMOD_RESULT fmodResult = FMOD_ERR_FILE_NOTFOUND;

	char const* szFileName = (char const*)pUserData;
	FILE* const pFile = gEnv->pCryPak->FOpen(szFileName, "rbx", ICryPak::FOPEN_HINT_DIRECT_OPERATION);

	if (pFile != nullptr)
	{
		gEnv->pCryPak->FSeek(pFile, 0, SEEK_END);
		auto const fileSize = static_cast<size_t>(gEnv->pCryPak->FTell(pFile));
		gEnv->pCryPak->FSeek(pFile, 0, SEEK_SET);

		if (fileSize >= s_maxFileSize)
		{
			gEnv->pCryPak->FClose(pFile);
			fmodResult = FMOD_ERR_FILE_BAD;
		}
		else
		{
			*pFileSize = static_cast<unsigned int>(fileSize);
			*pHandle = pFile;
			fmodResult = FMOD_OK;
		}
	}

	return fmodResult;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK FmodFileCloseCallback(void* pHandle, void* pUserData)
{
	FMOD_RESULT fmodResult = FMOD_ERR_FILE_NOTFOUND;
	auto const pFile = static_cast<FILE*>(pHandle);

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
	auto const pFile = static_cast<FILE*>(pHandle);
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
	auto const pFile = static_cast<FILE*>(pHandle);

	if (gEnv->pCryPak->FSeek(pFile, static_cast<long>(pos), 0) == 0)
	{
		fmodResult = FMOD_OK;
	}

	return fmodResult;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT CImpl::LoadBankCustom(char const* const szFileName, FMOD::Studio::Bank** ppBank)
{
	FMOD_STUDIO_BANK_INFO bankInfo;
	ZeroStruct(bankInfo);
	bankInfo.opencallback = &FmodFileOpenCallback;
	bankInfo.closecallback = &FmodFileCloseCallback;
	bankInfo.readcallback = &FmodFileReadCallback;
	bankInfo.seekcallback = &FmodFileSeekCallback;
	bankInfo.size = sizeof(bankInfo);
	bankInfo.userdata = reinterpret_cast<void*>(const_cast<char*>(szFileName));

	return m_pSystem->loadBankCustom(&bankInfo, FMOD_STUDIO_LOAD_BANK_NORMAL, ppBank);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::LoadMasterBanks()
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
	m_masterBankPath.clear();
	m_masterAssetsBankPath.clear();
	m_masterStreamsBankPath.clear();
	m_masterStringsBankPath.clear();
	CryFixedStringT<MaxFilePathLength + MaxFileNameLength> search(m_regularSoundBankFolder + "/*.bank");
	_finddata_t fd;
	intptr_t const handle = gEnv->pCryPak->FindFirst(search.c_str(), &fd);

	if (handle != -1)
	{
		do
		{
			m_masterStringsBankPath = fd.name;
			size_t const substrPos = m_masterStringsBankPath.find(".strings.bank");

			if (substrPos != m_masterStringsBankPath.npos)
			{
				m_masterBankPath = m_regularSoundBankFolder.c_str();
				m_masterBankPath += "/";
				m_masterBankPath += m_masterStringsBankPath.substr(0, substrPos);
				m_masterAssetsBankPath = m_masterBankPath;
				m_masterStreamsBankPath = m_masterBankPath;
				m_masterBankPath += ".bank";
				m_masterAssetsBankPath += ".assets.bank";
				m_masterStreamsBankPath += ".streams.bank";
				m_masterStringsBankPath.insert(0, "/");
				m_masterStringsBankPath.insert(0, m_regularSoundBankFolder.c_str());
				break;
			}

		}
		while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

		gEnv->pCryPak->FindClose(handle);
	}

	if (!m_masterBankPath.empty() && !m_masterStringsBankPath.empty())
	{
		size_t const masterBankFileSize = gEnv->pCryPak->FGetSize(m_masterBankPath.c_str());
		CRY_ASSERT_MESSAGE(masterBankFileSize > 0, "Master bank doesn't exist during %s", __FUNCTION__);
		size_t const masterBankStringsFileSize = gEnv->pCryPak->FGetSize(m_masterStringsBankPath.c_str());
		CRY_ASSERT_MESSAGE(masterBankStringsFileSize > 0, "Master strings bank doesn't exist during %s", __FUNCTION__);

		if (masterBankFileSize > 0 && masterBankStringsFileSize > 0)
		{
			fmodResult = LoadBankCustom(m_masterBankPath.c_str(), &m_pMasterBank);
			ASSERT_FMOD_OK;

			fmodResult = LoadBankCustom(m_masterStringsBankPath.c_str(), &m_pMasterStringsBank);
			ASSERT_FMOD_OK;

			if (m_pMasterBank != nullptr)
			{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
				m_masterBankSize = masterBankFileSize;
				m_masterStringsBankSize = masterBankStringsFileSize;
#endif        // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

				int numBuses = 0;
				fmodResult = m_pMasterBank->getBusCount(&numBuses);
				ASSERT_FMOD_OK;

				if (numBuses > 0)
				{
					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "FMOD::Studio::Bus*[]");
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

			size_t const masterBankAssetsFileSize = gEnv->pCryPak->FGetSize(m_masterAssetsBankPath.c_str());

			if (masterBankAssetsFileSize > 0)
			{
				fmodResult = LoadBankCustom(m_masterAssetsBankPath.c_str(), &m_pMasterAssetsBank);
				ASSERT_FMOD_OK;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
				m_masterAssetsBankSize = masterBankAssetsFileSize;
#endif        // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
			}

			size_t const masterBankStreamsFileSize = gEnv->pCryPak->FGetSize(m_masterStreamsBankPath.c_str());

			if (masterBankStreamsFileSize > 0)
			{
				fmodResult = LoadBankCustom(m_masterStreamsBankPath.c_str(), &m_pMasterStreamsBank);
				ASSERT_FMOD_OK;
			}
		}
	}
	else
	{
		// This does not qualify for a fallback to the NULL implementation!
		// Still notify the user about this failure!
		Cry::Audio::Log(ELogType::Error, "Fmod failed to load master banks during %", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnloadMasterBanks()
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	if (m_pMasterStringsBank != nullptr)
	{
		fmodResult = m_pMasterStringsBank->unload();
		ASSERT_FMOD_OK;
		m_pMasterStringsBank = nullptr;
	}

	if (m_pMasterStreamsBank != nullptr)
	{
		fmodResult = m_pMasterStreamsBank->unload();
		ASSERT_FMOD_OK;
		m_pMasterStreamsBank = nullptr;
	}

	if (m_pMasterAssetsBank != nullptr)
	{
		fmodResult = m_pMasterAssetsBank->unload();
		ASSERT_FMOD_OK;
		m_pMasterAssetsBank = nullptr;
	}

	if (m_pMasterBank != nullptr)
	{
		fmodResult = m_pMasterBank->unload();
		ASSERT_FMOD_OK;
		m_pMasterBank = nullptr;
	}

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	m_masterBankSize = 0;
	m_masterStringsBankSize = 0;
	m_masterAssetsBankSize = 0;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::MuteMasterBus(bool const shouldMute)
{
	FMOD::Studio::Bus* pMasterBus = nullptr;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	if (m_pSystem->getBus(s_szBusPrefix, &pMasterBus) != FMOD_OK)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to get master bus during %s", __FUNCTION__);
	}
#else
	m_pSystem->getBus(s_szBusPrefix, &pMasterBus);
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	if (pMasterBus != nullptr)
	{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
		if (pMasterBus->setMute(shouldMute) != FMOD_OK)
		{
			char const* const szMuteUnmute = shouldMute ? "mute" : "unmute";
			Cry::Audio::Log(ELogType::Error, "Failed to %s master bus during %s", szMuteUnmute, __FUNCTION__);
		}
#else
		pMasterBus->setMute(shouldMute);
#endif    // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::PauseMasterBus(bool const shouldPause)
{
	FMOD::Studio::Bus* pMasterBus = nullptr;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	if (m_pSystem->getBus(s_szBusPrefix, &pMasterBus) != FMOD_OK)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to get master bus during %s", __FUNCTION__);
	}
#else
	m_pSystem->getBus(s_szBusPrefix, &pMasterBus);
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	if (pMasterBus != nullptr)
	{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
		if (pMasterBus->setPaused(shouldPause) != FMOD_OK)
		{
			char const* const szPauseResume = shouldPause ? "pause" : "resume";
			Cry::Audio::Log(ELogType::Error, "Failed to %s master bus during %s", szPauseResume, __FUNCTION__);
		}
#else
		pMasterBus->setPaused(shouldPause);
#endif    // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
CEvent* CImpl::ConstructEvent(TriggerInstanceId const triggerInstanceId)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CEvent");
	auto const pEvent = new CEvent(triggerInstanceId);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	g_constructedEvents.push_back(pEvent);
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	return pEvent;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructEvent(CEvent const* const pEvent)
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
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
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	delete pEvent;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetFileData(char const* const szName, SFileData& fileData) const
{
	FMOD::Sound* pSound = nullptr;
	FMOD_CREATESOUNDEXINFO info;
	ZeroStruct(info);
	info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	info.decodebuffersize = 1;
	FMOD_RESULT const fmodResult = m_pLowLevelSystem->createStream(szName, FMOD_OPENONLY, &info, &pSound);
	ASSERT_FMOD_OK;

	if (pSound != nullptr)
	{
		unsigned int length = 0;
		pSound->getLength(&length, FMOD_TIMEUNIT_MS);
		fileData.duration = length / 1000.0f; // convert to seconds
	}
}

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
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
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugMemoryInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
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
		posY += Debug::s_systemLineHeight;

		if (m_pMasterAssetsBank != nullptr)
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::s_systemFontSize, Debug::s_systemColorTextPrimary, false, "Master Bank: %uKiB | Master Strings Bank: %uKiB | Master Assets Bank: %uKiB",
			                    static_cast<uint32>(m_masterBankSize / 1024),
			                    static_cast<uint32>(m_masterStringsBankSize / 1024),
			                    static_cast<uint32>(m_masterAssetsBankSize / 1024));
		}
		else
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::s_systemFontSize, Debug::s_systemColorTextPrimary, false, "Master Bank: %uKiB | Master Strings Bank: %uKiB",
			                    static_cast<uint32>(m_masterBankSize / 1024),
			                    static_cast<uint32>(m_masterStringsBankSize / 1024));
		}

		{
			auto& allocator = CObject::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects", g_objectPoolSize);
		}

		{
			auto& allocator = CEvent::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Events", g_eventPoolSize);
		}

		if (g_debugPoolSizes.triggers > 0)
		{
			auto& allocator = CTrigger::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Triggers", g_poolSizes.triggers);
		}

		if (g_debugPoolSizes.parameters > 0)
		{
			auto& allocator = CParameter::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters", g_poolSizes.parameters);
		}

		if (g_debugPoolSizes.switchStates > 0)
		{
			auto& allocator = CSwitchState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "SwitchStates", g_poolSizes.switchStates);
		}

		if (g_debugPoolSizes.envBuses > 0)
		{
			auto& allocator = CEnvironmentBus::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "EnvironmentBuses", g_poolSizes.envBuses);
		}

		if (g_debugPoolSizes.envParameters > 0)
		{
			auto& allocator = CEnvironmentParameter::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "EnvironmentParameters", g_poolSizes.envParameters);
		}

		if (g_debugPoolSizes.vcaParameters > 0)
		{
			auto& allocator = CVcaParameter::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "VCAParameters", g_poolSizes.vcaParameters);
		}

		if (g_debugPoolSizes.vcaStates > 0)
		{
			auto& allocator = CVcaState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "VCAStates", g_poolSizes.vcaStates);
		}

		if (g_debugPoolSizes.files > 0)
		{
			auto& allocator = CFile::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Files", g_poolSizes.files);
		}
	}

	size_t const numEvents = g_constructedEvents.size();

	posY += Debug::s_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::s_systemFontSize, Debug::s_systemColorTextSecondary, false, "Events: %3" PRISIZE_T " | Objects with Doppler: %u",
	                    numEvents, g_numObjectsWithDoppler);

	Vec3 const& listenerPosition = g_pListener->GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	float const listenerVelocity = g_pListener->GetVelocity().GetLength();
	char const* const szName = g_pListener->GetName();

	posY += Debug::s_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::s_systemFontSize, Debug::s_systemColorListenerActive, false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f | Velocity: %.2f m/s",
	                    szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z, listenerVelocity);
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfoList(IRenderAuxGeom& auxGeom, float& posX, float posY, float const debugDistance, char const* const szTextFilter) const
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(szTextFilter);
	lowerCaseSearchString.MakeLower();

	auxGeom.Draw2dLabel(posX, posY, Debug::s_listHeaderFontSize, Debug::s_globalColorHeader, false, "Fmod Studio Events [%" PRISIZE_T "]", g_constructedEvents.size());
	posY += Debug::s_listHeaderLineHeight;

	for (auto const pEvent : g_constructedEvents)
	{
		Vec3 const& position = pEvent->GetObject()->GetTransformation().GetPosition();
		float const distance = position.GetDistance(g_pListener->GetPosition());

		if ((debugDistance <= 0.0f) || ((debugDistance > 0.0f) && (distance < debugDistance)))
		{
			char const* const szTriggerName = pEvent->GetName();
			CryFixedStringT<MaxControlNameLength> lowerCaseTriggerName(szTriggerName);
			lowerCaseTriggerName.MakeLower();
			bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseTriggerName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

			if (draw)
			{
				ColorF color = Debug::s_globalColorInactive;

				switch (pEvent->GetState())
				{
				case EEventState::Playing:
					{
						color = Debug::s_listColorItemActive;
						break;
					}
				case EEventState::Virtual:
					{
						color = Debug::s_globalColorVirtual;
						break;
					}
				default:
					break;
				}

				auxGeom.Draw2dLabel(posX, posY, Debug::s_listFontSize, color, false, "%s on %s", szTriggerName, pEvent->GetObject()->GetName());

				posY += Debug::s_listLineHeight;
			}
		}
	}

	posX += 600.0f;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
