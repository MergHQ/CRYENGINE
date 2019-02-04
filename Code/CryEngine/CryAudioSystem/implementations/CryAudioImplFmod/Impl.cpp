// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "Common.h"
#include "CVars.h"
#include "Bank.h"
#include "Event.h"
#include "EventInstance.h"
#include "Listener.h"
#include "GlobalObject.h"
#include "Object.h"
#include "Parameter.h"
#include "ParameterEnvironment.h"
#include "ParameterState.h"
#include "ProgrammerSoundFile.h"
#include "Return.h"
#include "Snapshot.h"
#include "StandaloneFile.h"
#include "Vca.h"
#include "VcaState.h"
#include "GlobalData.h"

#include <ISettingConnection.h>
#include <FileInfo.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	#include <Logger.h>
	#include <DebugStyle.h>
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
SPoolSizes g_poolSizes;
SPoolSizes g_poolSizesLevelSpecific;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
SPoolSizes g_debugPoolSizes;
EventInstances g_constructedEventInstances;
uint16 g_objectPoolSize = 0;
uint16 g_eventPoolSize = 0;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

constexpr char const* g_szEventPrefix = "event:/";
constexpr char const* g_szSnapshotPrefix = "snapshot:/";
constexpr char const* g_szBusPrefix = "bus:/";
constexpr char const* g_szVcaPrefix = "vca:/";

constexpr size_t g_maxFileSize = size_t(1) << size_t(31);

//////////////////////////////////////////////////////////////////////////
void CountPoolSizes(XmlNodeRef const pNode, SPoolSizes& poolSizes)
{
	uint16 numEvents = 0;
	pNode->getAttr(g_szEventsAttribute, numEvents);
	poolSizes.events += numEvents;

	uint16 numParameters = 0;
	pNode->getAttr(g_szParametersAttribute, numParameters);
	poolSizes.parameters += numParameters;

	uint16 numParameterEnvironments = 0;
	pNode->getAttr(g_szParameterEnvironmentsAttribute, numParameterEnvironments);
	poolSizes.parameters += numParameterEnvironments;

	uint16 numParameterStates = 0;
	pNode->getAttr(g_szParameterStatesAttribute, numParameterStates);
	poolSizes.parameterStates += numParameterStates;

	uint16 numSnapshots = 0;
	pNode->getAttr(g_szSnapshotsAttribute, numSnapshots);
	poolSizes.parameterStates += numSnapshots;

	uint16 numReturns = 0;
	pNode->getAttr(g_szReturnsAttribute, numReturns);
	poolSizes.returns += numReturns;

	uint16 numVcas = 0;
	pNode->getAttr(g_szVcasAttribute, numVcas);
	poolSizes.vcas += numVcas;

	uint16 numVcaStates = 0;
	pNode->getAttr(g_szVcaStatesAttribute, numVcaStates);
	poolSizes.vcaStates += numVcaStates;

	uint16 numFiles = 0;
	pNode->getAttr(g_szBanksAttribute, numFiles);
	poolSizes.banks += numFiles;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	CObject::CreateAllocator(objectPoolSize);
	CEventInstance::CreateAllocator(eventPoolSize);
	CEvent::CreateAllocator(g_poolSizes.events);
	CParameter::CreateAllocator(g_poolSizes.parameters);
	CParameterEnvironment::CreateAllocator(g_poolSizes.parameterEnvironments);
	CParameterState::CreateAllocator(g_poolSizes.parameterStates);
	CSnapshot::CreateAllocator(g_poolSizes.snapshots);
	CReturn::CreateAllocator(g_poolSizes.returns);
	CVca::CreateAllocator(g_poolSizes.vcas);
	CVcaState::CreateAllocator(g_poolSizes.vcaStates);
	CBank::CreateAllocator(g_poolSizes.banks);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CObject::FreeMemoryPool();
	CEventInstance::FreeMemoryPool();
	CEvent::FreeMemoryPool();
	CParameter::FreeMemoryPool();
	CParameterEnvironment::FreeMemoryPool();
	CParameterState::FreeMemoryPool();
	CSnapshot::FreeMemoryPool();
	CReturn::FreeMemoryPool();
	CVca::FreeMemoryPool();
	CVcaState::FreeMemoryPool();
	CBank::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void ClearActiveSnapshots()
{
	for (auto const& snapshotInstancePair : g_activeSnapshots)
	{
		snapshotInstancePair.second->release();
	}

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	g_activeSnapshotNames.clear();
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
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
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize)
{
	if (g_cvars.m_eventPoolSize < 1)
	{
		g_cvars.m_eventPoolSize = 1;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, R"(Event pool size must be at least 1. Forcing the cvar "s_FmodEventPoolSize" to 1!)");
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}

	g_constructedObjects.reserve(static_cast<size_t>(objectPoolSize));
	AllocateMemoryPools(objectPoolSize, static_cast<uint16>(g_cvars.m_eventPoolSize));

	m_regularSoundBankFolder = CRY_AUDIO_DATA_ROOT;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += g_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += g_szAssetsFolderName;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

	FMOD_RESULT fmodResult = FMOD::Studio::System::create(&m_pSystem);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
	fmodResult = m_pSystem->getLowLevelSystem(&m_pLowLevelSystem);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	g_constructedEventInstances.reserve(static_cast<size_t>(g_cvars.m_eventPoolSize));

	g_objectPoolSize = objectPoolSize;
	g_eventPoolSize = static_cast<uint16>(g_cvars.m_eventPoolSize);

	uint32 version = 0;
	fmodResult = m_pLowLevelSystem->getVersion(&version);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	CryFixedStringT<MaxInfoStringLength> systemVersion;
	systemVersion.Format("%08x", version);
	CryFixedStringT<MaxInfoStringLength> headerVersion;
	headerVersion.Format("%08x", FMOD_VERSION);
	CreateVersionString(systemVersion);
	CreateVersionString(headerVersion);

	m_name = CRY_AUDIO_IMPL_FMOD_INFO_STRING;
	m_name += "System: ";
	m_name += systemVersion;
	m_name += " - Header: ";
	m_name += headerVersion;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	int sampleRate = 0;
	int numRawSpeakers = 0;
	FMOD_SPEAKERMODE speakerMode = FMOD_SPEAKERMODE_DEFAULT;
	fmodResult = m_pLowLevelSystem->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
	fmodResult = m_pLowLevelSystem->setSoftwareFormat(sampleRate, speakerMode, numRawSpeakers);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	fmodResult = m_pLowLevelSystem->set3DSettings(g_cvars.m_dopplerScale, g_cvars.m_distanceFactor, g_cvars.m_rolloffScale);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	void* pExtraDriverData = nullptr;
	int initFlags = FMOD_INIT_NORMAL | FMOD_INIT_VOL0_BECOMES_VIRTUAL;
	int studioInitFlags = FMOD_STUDIO_INIT_NORMAL;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	if (g_cvars.m_enableLiveUpdate > 0)
	{
		studioInitFlags |= FMOD_STUDIO_INIT_LIVEUPDATE;
	}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	if (g_cvars.m_enableSynchronousUpdate > 0)
	{
		studioInitFlags |= FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE;
	}

	fmodResult = m_pSystem->initialize(g_cvars.m_maxChannels, studioInitFlags, initFlags, pExtraDriverData);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	LoadMasterBanks();

	FMOD_3D_ATTRIBUTES attributes = {
		{ 0 } };
	attributes.forward.z = 1.0f;
	attributes.up.y = 1.0f;
	fmodResult = m_pSystem->setListenerAttributes(0, &attributes);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

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
		ClearActiveSnapshots();
		UnloadMasterBanks();

		FMOD_RESULT const fmodResult = m_pSystem->release();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
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

		g_poolSizesLevelSpecific.events = std::max(g_poolSizesLevelSpecific.events, levelPoolSizes.events);
		g_poolSizesLevelSpecific.parameters = std::max(g_poolSizesLevelSpecific.parameters, levelPoolSizes.parameters);
		g_poolSizesLevelSpecific.parameterEnvironments = std::max(g_poolSizesLevelSpecific.parameterEnvironments, levelPoolSizes.parameterEnvironments);
		g_poolSizesLevelSpecific.parameterStates = std::max(g_poolSizesLevelSpecific.parameterStates, levelPoolSizes.parameterStates);
		g_poolSizesLevelSpecific.snapshots = std::max(g_poolSizesLevelSpecific.snapshots, levelPoolSizes.snapshots);
		g_poolSizesLevelSpecific.returns = std::max(g_poolSizesLevelSpecific.returns, levelPoolSizes.returns);
		g_poolSizesLevelSpecific.vcas = std::max(g_poolSizesLevelSpecific.vcas, levelPoolSizes.vcas);
		g_poolSizesLevelSpecific.vcaStates = std::max(g_poolSizesLevelSpecific.vcaStates, levelPoolSizes.vcaStates);
		g_poolSizesLevelSpecific.banks = std::max(g_poolSizesLevelSpecific.banks, levelPoolSizes.banks);
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

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	ZeroStruct(g_debugPoolSizes);
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_poolSizes.events += g_poolSizesLevelSpecific.events;
	g_poolSizes.parameters += g_poolSizesLevelSpecific.parameters;
	g_poolSizes.parameterEnvironments += g_poolSizesLevelSpecific.parameterEnvironments;
	g_poolSizes.parameterStates += g_poolSizesLevelSpecific.parameterStates;
	g_poolSizes.snapshots += g_poolSizesLevelSpecific.snapshots;
	g_poolSizes.returns += g_poolSizesLevelSpecific.returns;
	g_poolSizes.vcas += g_poolSizesLevelSpecific.vcas;
	g_poolSizes.vcaStates += g_poolSizesLevelSpecific.vcaStates;
	g_poolSizes.banks += g_poolSizesLevelSpecific.banks;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	g_poolSizes.events = std::max<uint16>(1, g_poolSizes.events);
	g_poolSizes.parameters = std::max<uint16>(1, g_poolSizes.parameters);
	g_poolSizes.parameterEnvironments = std::max<uint16>(1, g_poolSizes.parameterEnvironments);
	g_poolSizes.parameterStates = std::max<uint16>(1, g_poolSizes.parameterStates);
	g_poolSizes.snapshots = std::max<uint16>(1, g_poolSizes.snapshots);
	g_poolSizes.returns = std::max<uint16>(1, g_poolSizes.returns);
	g_poolSizes.vcas = std::max<uint16>(1, g_poolSizes.vcas);
	g_poolSizes.vcaStates = std::max<uint16>(1, g_poolSizes.vcaStates);
	g_poolSizes.banks = std::max<uint16>(1, g_poolSizes.banks);
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

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	if (m_pSystem->getBus(g_szBusPrefix, &pMasterBus) != FMOD_OK)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to get master bus during %s", __FUNCTION__);
	}
#else
	m_pSystem->getBus(g_szBusPrefix, &pMasterBus);
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	if (pMasterBus != nullptr)
	{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		if (pMasterBus->stopAllEvents(FMOD_STUDIO_STOP_IMMEDIATE) != FMOD_OK)
		{
			Cry::Audio::Log(ELogType::Error, "Failed to stop all events during %s", __FUNCTION__);
		}
#else
		pMasterBus->stopAllEvents(FMOD_STUDIO_STOP_IMMEDIATE);
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CBank*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			// loadBankMemory requires 32-byte alignment when using FMOD_STUDIO_LOAD_MEMORY_POINT
			if ((reinterpret_cast<uintptr_t>(pFileInfo->pFileData) & (32 - 1)) > 0)
			{
				CryFatalError("<Audio>: allocation not %d byte aligned!", 32);
			}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

			FMOD_RESULT fmodResult = m_pSystem->loadBankMemory(static_cast<char*>(pFileInfo->pFileData), static_cast<int>(pFileInfo->size), FMOD_STUDIO_LOAD_MEMORY_POINT, FMOD_STUDIO_LOAD_BANK_NORMAL, &pFileData->pBank);
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			CryFixedStringT<MaxFilePathLength> streamsBankPath = pFileInfo->szFilePath;
			PathUtil::RemoveExtension(streamsBankPath);
			streamsBankPath += ".streams.bank";

			size_t const streamsBankFileSize = gEnv->pCryPak->FGetSize(streamsBankPath.c_str());

			if (streamsBankFileSize > 0)
			{
				pFileData->m_streamsBankPath = streamsBankPath;
				fmodResult = LoadBankCustom(pFileData->m_streamsBankPath.c_str(), &pFileData->pStreamsBank);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
			}
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Fmod implementation of %s", __FUNCTION__);
		}
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CBank*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			FMOD_RESULT fmodResult = pFileData->pBank->unload();
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			FMOD_STUDIO_LOADING_STATE loadingState;

			do
			{
				fmodResult = m_pSystem->update();
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
				fmodResult = pFileData->pBank->getLoadingState(&loadingState);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK_OR_INVALID_HANDLE;
			}
			while (loadingState == FMOD_STUDIO_LOADING_STATE_UNLOADING);

			pFileData->pBank = nullptr;

			if (pFileData->pStreamsBank != nullptr)
			{
				fmodResult = pFileData->pStreamsBank->unload();
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
				pFileData->pStreamsBank = nullptr;
			}
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Fmod implementation of %s", __FUNCTION__);
		}
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ConstructFile(XmlNodeRef const pRootNode, SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if ((_stricmp(pRootNode->getTag(), g_szFileTag) == 0) && (pFileInfo != nullptr))
	{
		char const* const szFileName = pRootNode->getAttr(g_szNameAttribute);

		if (szFileName != nullptr && szFileName[0] != '\0')
		{
			char const* const szLocalized = pRootNode->getAttr(g_szLocalizedAttribute);
			pFileInfo->bLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, g_szTrueValue) == 0);
			pFileInfo->szFileName = szFileName;

			// FMOD Studio always uses 32 byte alignment for preloaded banks regardless of the platform.
			pFileInfo->memoryBlockAlignment = 32;

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CFile");
			pFileInfo->pImplData = new CBank();

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
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	implInfo.folderName = g_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CGlobalObject");
	new CGlobalObject;

	if (!stl::push_back_unique(g_constructedObjects, static_cast<CBaseObject*>(g_pObject)))
	{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}

	return static_cast<IObject*>(g_pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CObject");
	CBaseObject* const pObject = new CObject(transformation);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	if (szName != nullptr)
	{
		pObject->SetName(szName);
	}

	if (!stl::push_back_unique(g_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}
#else
	stl::push_back_unique(g_constructedObjects, pObject);
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

	if (!stl::find_and_erase(g_constructedObjects, pBaseObject))
	{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, "Trying to delete a non-existing object.");
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}

	delete pBaseObject;
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	static int id = 0;

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CListener");
	g_pListener = new CListener(transformation, id++);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	if (szName != nullptr)
	{
		g_pListener->SetName(szName);
	}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

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
		pFile = new CProgrammerSoundFile(filePath, static_cast<CEvent const* const>(pITriggerConnection)->GetGuid(), standaloneFile);
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
	ITriggerConnection* pITriggerConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szEventTag) == 0)
	{
		stack_string path(g_szEventPrefix);
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		path += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::EventDescription* pEventDescription = nullptr;
			m_pSystem->getEventByID(&guid, &pEventDescription);

			CEvent::EActionType actionType = CEvent::EActionType::Start;
			char const* const szEventType = pRootNode->getAttr(g_szTypeAttribute);

			if ((szEventType != nullptr) && (szEventType[0] != '\0'))
			{
				if (_stricmp(szEventType, g_szStopValue) == 0)
				{
					actionType = CEvent::EActionType::Stop;
				}
				else if (_stricmp(szEventType, g_szPauseValue) == 0)
				{
					actionType = CEvent::EActionType::Pause;
				}
				else if (_stricmp(szEventType, g_szResumeValue) == 0)
				{
					actionType = CEvent::EActionType::Resume;
				}
			}

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CEvent");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(StringToId(path.c_str()), actionType, guid, szName));
#else
			pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(StringToId(path.c_str()), actionType, guid));
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod event: %s", path.c_str());
		}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
	else if (_stricmp(szTag, g_szKeyTag) == 0)
	{
		stack_string path(g_szEventPrefix);
		char const* const szEventName = pRootNode->getAttr(g_szEventAttribute);
		path += szEventName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			char const* const szKey = pRootNode->getAttr(g_szNameAttribute);

			FMOD::Studio::EventDescription* pEventDescription = nullptr;
			m_pSystem->getEventByID(&guid, &pEventDescription);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CEvent");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(StringToId(path.c_str()), guid, szKey, szEventName));
#else
			pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(StringToId(path.c_str()), guid, szKey));
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod event: %s", path.c_str());
		}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
	else if (_stricmp(szTag, g_szSnapshotTag) == 0)
	{
		stack_string path(g_szSnapshotPrefix);
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		path += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			CSnapshot::EActionType actionType = CSnapshot::EActionType::Start;
			char const* const szActionType = pRootNode->getAttr(g_szTypeAttribute);

			if ((szActionType != nullptr) && (szActionType[0] != '\0') && (_stricmp(szActionType, g_szStopValue) == 0))
			{
				actionType = CSnapshot::EActionType::Stop;
			}

			FMOD::Studio::EventDescription* pEventDescription = nullptr;
			m_pSystem->getEventByID(&guid, &pEventDescription);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CSnapshot");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			pITriggerConnection = static_cast<ITriggerConnection*>(new CSnapshot(StringToId(path.c_str()), actionType, pEventDescription, szName));
#else
			pITriggerConnection = static_cast<ITriggerConnection*>(new CSnapshot(StringToId(path.c_str()), actionType, pEventDescription));
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod snapshot: %s", path.c_str());
		}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	return pITriggerConnection;
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	ITriggerConnection* pITriggerConnection = nullptr;

	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		stack_string path(g_szEventPrefix);
		char const* const szName = pTriggerInfo->name.c_str();
		path += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CEvent");
			pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(StringToId(path.c_str()), CEvent::EActionType::Start, guid, szName));
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod event: %s", path.c_str());
		}
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	delete pITriggerConnection;
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	IParameterConnection* pIParameterConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szParameterTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float multiplier = g_defaultParamMultiplier;
		float shift = g_defaultParamShift;
		pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(g_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CParameter");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		pIParameterConnection = static_cast<IParameterConnection*>(new CParameter(StringToId(szName), multiplier, shift, szName));
#else
		pIParameterConnection = static_cast<IParameterConnection*>(new CParameter(StringToId(szName), multiplier, shift));
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
	else if (_stricmp(szTag, g_szVcaTag) == 0)
	{
		stack_string fullName(g_szVcaPrefix);
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		fullName += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(fullName.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::VCA* pVca = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getVCAByID(&guid, &pVca);
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			float multiplier = g_defaultParamMultiplier;
			float shift = g_defaultParamShift;
			pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
			pRootNode->getAttr(g_szShiftAttribute, shift);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CVca");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			pIParameterConnection = static_cast<IParameterConnection*>(new CVca(pVca, multiplier, shift, szName));
#else
			pIParameterConnection = static_cast<IParameterConnection*>(new CVca(pVca, multiplier, shift));
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod VCA: %s", fullName.c_str());
		}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	return pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	delete pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	ISwitchStateConnection* pISwitchStateConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szParameterTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		char const* const szValue = pRootNode->getAttr(g_szValueAttribute);
		auto const value = static_cast<float>(atof(szValue));

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CParameterState");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CParameterState(StringToId(szName), value, szName));
#else
		pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CParameterState(StringToId(szName), value));
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
	else if (_stricmp(szTag, g_szVcaTag) == 0)
	{
		stack_string fullName(g_szVcaPrefix);
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		fullName += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(fullName.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::VCA* pVca = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getVCAByID(&guid, &pVca);
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			char const* const szValue = pRootNode->getAttr(g_szValueAttribute);
			auto const value = static_cast<float>(atof(szValue));

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CVcaState");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CVcaState(pVca, value, szName));
#else
			pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CVcaState(pVca, value));
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod VCA: %s", fullName.c_str());
		}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	return pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	delete pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	IEnvironmentConnection* pIEnvironmentConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szBusTag) == 0)
	{
		stack_string path(g_szBusPrefix);
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		path += szName;
		FMOD_GUID guid = { 0 };

		if (m_pSystem->lookupID(path.c_str(), &guid) == FMOD_OK)
		{
			FMOD::Studio::Bus* pBus = nullptr;
			FMOD_RESULT const fmodResult = m_pSystem->getBusByID(&guid, &pBus);
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CReturn");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CReturn(pBus, szName));
#else
			pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CReturn(pBus));
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Unknown Fmod bus: %s", path.c_str());
		}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
	else if (_stricmp(szTag, g_szParameterTag) == 0)
	{

		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float multiplier = g_defaultParamMultiplier;
		float shift = g_defaultParamShift;
		pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(g_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CParameterEnvironment");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CParameterEnvironment(StringToId(szName), multiplier, shift, szName));
#else
		pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CParameterEnvironment(StringToId(szName), multiplier, shift));
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Fmod tag: %s", szTag);
	}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	return pIEnvironmentConnection;
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
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	g_vcaValues.clear();
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	ClearActiveSnapshots();
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
		m_localizedSoundBankFolder += CRY_AUDIO_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szAssetsFolderName;
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

		if (fileSize >= g_maxFileSize)
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
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			fmodResult = LoadBankCustom(m_masterStringsBankPath.c_str(), &m_pMasterStringsBank);
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			if (m_pMasterBank != nullptr)
			{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
				m_masterBankSize = masterBankFileSize;
				m_masterStringsBankSize = masterBankStringsFileSize;
#endif        // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

				int numBuses = 0;
				fmodResult = m_pMasterBank->getBusCount(&numBuses);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

				if (numBuses > 0)
				{
					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "FMOD::Studio::Bus*[]");
					FMOD::Studio::Bus** pBuses = new FMOD::Studio::Bus*[numBuses];
					int numRetrievedBuses = 0;
					fmodResult = m_pMasterBank->getBusList(pBuses, numBuses, &numRetrievedBuses);
					CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
					CRY_ASSERT(numBuses == numRetrievedBuses);

					for (int i = 0; i < numRetrievedBuses; ++i)
					{
						fmodResult = pBuses[i]->lockChannelGroup();
						CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
					}

					delete[] pBuses;
				}
			}

			size_t const masterBankAssetsFileSize = gEnv->pCryPak->FGetSize(m_masterAssetsBankPath.c_str());

			if (masterBankAssetsFileSize > 0)
			{
				fmodResult = LoadBankCustom(m_masterAssetsBankPath.c_str(), &m_pMasterAssetsBank);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
				m_masterAssetsBankSize = masterBankAssetsFileSize;
#endif        // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
			}

			size_t const masterBankStreamsFileSize = gEnv->pCryPak->FGetSize(m_masterStreamsBankPath.c_str());

			if (masterBankStreamsFileSize > 0)
			{
				fmodResult = LoadBankCustom(m_masterStreamsBankPath.c_str(), &m_pMasterStreamsBank);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
			}
		}
	}
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	else
	{
		// This does not qualify for a fallback to the NULL implementation!
		// Still notify the user about this failure!
		Cry::Audio::Log(ELogType::Error, "Fmod failed to load master banks during %s", __FUNCTION__);
	}
#endif        // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnloadMasterBanks()
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	if (m_pMasterStringsBank != nullptr)
	{
		fmodResult = m_pMasterStringsBank->unload();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
		m_pMasterStringsBank = nullptr;
	}

	if (m_pMasterStreamsBank != nullptr)
	{
		fmodResult = m_pMasterStreamsBank->unload();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
		m_pMasterStreamsBank = nullptr;
	}

	if (m_pMasterAssetsBank != nullptr)
	{
		fmodResult = m_pMasterAssetsBank->unload();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
		m_pMasterAssetsBank = nullptr;
	}

	if (m_pMasterBank != nullptr)
	{
		fmodResult = m_pMasterBank->unload();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
		m_pMasterBank = nullptr;
	}

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	m_masterBankSize = 0;
	m_masterStringsBankSize = 0;
	m_masterAssetsBankSize = 0;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::MuteMasterBus(bool const shouldMute)
{
	FMOD::Studio::Bus* pMasterBus = nullptr;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	if (m_pSystem->getBus(g_szBusPrefix, &pMasterBus) != FMOD_OK)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to get master bus during %s", __FUNCTION__);
	}
#else
	m_pSystem->getBus(g_szBusPrefix, &pMasterBus);
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	if (pMasterBus != nullptr)
	{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		if (pMasterBus->setMute(shouldMute) != FMOD_OK)
		{
			char const* const szMuteUnmute = shouldMute ? "mute" : "unmute";
			Cry::Audio::Log(ELogType::Error, "Failed to %s master bus during %s", szMuteUnmute, __FUNCTION__);
		}
#else
		pMasterBus->setMute(shouldMute);
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::PauseMasterBus(bool const shouldPause)
{
	FMOD::Studio::Bus* pMasterBus = nullptr;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	if (m_pSystem->getBus(g_szBusPrefix, &pMasterBus) != FMOD_OK)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to get master bus during %s", __FUNCTION__);
	}
#else
	m_pSystem->getBus(g_szBusPrefix, &pMasterBus);
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	if (pMasterBus != nullptr)
	{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		if (pMasterBus->setPaused(shouldPause) != FMOD_OK)
		{
			char const* const szPauseResume = shouldPause ? "pause" : "resume";
			Cry::Audio::Log(ELogType::Error, "Failed to %s master bus during %s", szPauseResume, __FUNCTION__);
		}
#else
		pMasterBus->setPaused(shouldPause);
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
CEventInstance* CImpl::ConstructEventInstance(
	TriggerInstanceId const triggerInstanceId,
	uint32 const eventId,
	CEvent const* const pEvent,
	CBaseObject const* const pBaseObject /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Fmod::CEventInstance");

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	auto const pEventInstance = new CEventInstance(triggerInstanceId, eventId, pEvent, pBaseObject);
	g_constructedEventInstances.push_back(pEventInstance);
#else
	auto const pEventInstance = new CEventInstance(triggerInstanceId, eventId, pEvent);
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	return pEventInstance;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructEventInstance(CEventInstance const* const pEventInstance)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
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
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

	delete pEventInstance;
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
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	if (pSound != nullptr)
	{
		unsigned int length = 0;
		pSound->getLength(&length, FMOD_TIMEUNIT_MS);
		fileData.duration = length / 1000.0f; // convert to seconds
	}
}

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
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

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, color, false,
	                    "[%s] Constructed: %" PRISIZE_T " (%s) | Allocated: %" PRISIZE_T " (%s) | Pool Size: %u",
	                    szType, pool.nUsed, memUsedString.c_str(), pool.nAlloc, memAllocString.c_str(), poolSize);
}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugMemoryInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
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

	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemHeaderFontSize, Debug::s_globalColorHeader, false, memInfoString.c_str());
	posY += Debug::g_systemHeaderLineSpacerHeight;

	if (showDetailedInfo)
	{
		posY += Debug::g_systemLineHeight;

		if (m_pMasterAssetsBank != nullptr)
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextPrimary, false, "Master Bank: %uKiB | Master Strings Bank: %uKiB | Master Assets Bank: %uKiB",
			                    static_cast<uint32>(m_masterBankSize / 1024),
			                    static_cast<uint32>(m_masterStringsBankSize / 1024),
			                    static_cast<uint32>(m_masterAssetsBankSize / 1024));
		}
		else
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextPrimary, false, "Master Bank: %uKiB | Master Strings Bank: %uKiB",
			                    static_cast<uint32>(m_masterBankSize / 1024),
			                    static_cast<uint32>(m_masterStringsBankSize / 1024));
		}

		{
			auto& allocator = CObject::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects", g_objectPoolSize);
		}

		{
			auto& allocator = CEventInstance::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Event Instances", g_eventPoolSize);
		}

		if (g_debugPoolSizes.events > 0)
		{
			auto& allocator = CEvent::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Events", g_poolSizes.events);
		}

		if (g_debugPoolSizes.parameters > 0)
		{
			auto& allocator = CParameter::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters", g_poolSizes.parameters);
		}

		if (g_debugPoolSizes.parameterEnvironments > 0)
		{
			auto& allocator = CParameterEnvironment::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameter Environments", g_poolSizes.parameterEnvironments);
		}

		if (g_debugPoolSizes.parameterStates > 0)
		{
			auto& allocator = CParameterState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameter States", g_poolSizes.parameterStates);
		}

		if (g_debugPoolSizes.snapshots > 0)
		{
			auto& allocator = CSnapshot::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Snapshots", g_poolSizes.snapshots);
		}

		if (g_debugPoolSizes.returns > 0)
		{
			auto& allocator = CReturn::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Returns", g_poolSizes.returns);
		}

		if (g_debugPoolSizes.vcas > 0)
		{
			auto& allocator = CVca::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "VCAs", g_poolSizes.vcas);
		}

		if (g_debugPoolSizes.vcaStates > 0)
		{
			auto& allocator = CVcaState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "VCA States", g_poolSizes.vcaStates);
		}

		if (g_debugPoolSizes.banks > 0)
		{
			auto& allocator = CBank::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Banks", g_poolSizes.banks);
		}
	}

	size_t const numEvents = g_constructedEventInstances.size();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextSecondary, false, "Active Events: %3" PRISIZE_T " | Objects with Doppler: %u",
	                    numEvents, g_numObjectsWithDoppler);

	Vec3 const& listenerPosition = g_pListener->GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	float const listenerVelocity = g_pListener->GetVelocity().GetLength();
	char const* const szName = g_pListener->GetName();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorListenerActive, false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f | Velocity: %.2f m/s",
	                    szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z, listenerVelocity);
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfoList(IRenderAuxGeom& auxGeom, float& posX, float posY, float const debugDistance, char const* const szTextFilter) const
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	if ((g_cvars.m_debugListFilter & g_debugListMask) != 0)
	{
		CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(szTextFilter);
		lowerCaseSearchString.MakeLower();

		float const initialPosY = posY;

		if ((g_cvars.m_debugListFilter & EDebugListFilter::EventInstances) != 0)
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Fmod Events [%" PRISIZE_T "]", g_constructedEventInstances.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const pEventInstance : g_constructedEventInstances)
			{
				Vec3 const& position = pEventInstance->GetObject()->GetTransformation().GetPosition();
				float const distance = position.GetDistance(g_pListener->GetPosition());

				if ((debugDistance <= 0.0f) || ((debugDistance > 0.0f) && (distance < debugDistance)))
				{
					char const* const szEventName = pEventInstance->GetEvent()->GetName();
					CryFixedStringT<MaxControlNameLength> lowerCaseEventName(szEventName);
					lowerCaseEventName.MakeLower();
					bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseEventName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

					if (draw)
					{
						EEventState const state = pEventInstance->GetState();
						ColorF const color = (state == EEventState::Pending) ? Debug::s_listColorItemLoading : ((state == EEventState::Virtual) ? Debug::s_globalColorVirtual : Debug::s_listColorItemActive);
						auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, color, false, "%s on %s", szEventName, pEventInstance->GetObject()->GetName());

						posY += Debug::g_listLineHeight;
					}
				}
			}

			posX += 600.0f;
		}

		if ((g_cvars.m_debugListFilter & EDebugListFilter::Snapshots) != 0)
		{
			posY = initialPosY;

			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Fmod Snapshots [%" PRISIZE_T "]", g_activeSnapshotNames.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const& snapshotName : g_activeSnapshotNames)
			{
				CryFixedStringT<MaxControlNameLength> lowerCaseSnapshotName(snapshotName);
				lowerCaseSnapshotName.MakeLower();
				bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseSnapshotName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

				if (draw)
				{
					auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, Debug::s_listColorItemActive, false, "%s", snapshotName.c_str());

					posY += Debug::g_listLineHeight;
				}
			}

			posX += 300.0f;
		}

		if ((g_cvars.m_debugListFilter & EDebugListFilter::Vcas) != 0)
		{
			posY = initialPosY;

			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Fmod VCAs [%" PRISIZE_T "]", g_vcaValues.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const& vcaPair : g_vcaValues)
			{
				char const* const szVcaName = vcaPair.first.c_str();
				CryFixedStringT<MaxControlNameLength> lowerCaseVcaName(szVcaName);
				lowerCaseVcaName.MakeLower();
				bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseVcaName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

				if (draw)
				{
					auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, Debug::s_listColorItemActive, false, "%s : %3.2f", szVcaName, vcaPair.second);

					posY += Debug::g_listLineHeight;
				}
			}

			posX += 300.0f;
		}
	}
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
