// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "CVars.h"
#include "FileIOHandler.h"
#include "AuxThread.h"
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
#include <SharedData.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

#include <AK/SoundEngine/Common/AkSoundEngine.h>     // Sound engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>     // Music Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>       // Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>          // Default memory and stream managers
#include <AK/SoundEngine/Common/AkStreamMgrModule.h> // AkStreamMgrModule
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkCallback.h>

#include <AK/Plugin/AllPluginsRegistrationHelpers.h>
#include <AK/Plugin/AkConvolutionReverbFXFactory.h>

#if defined(WWISE_USE_OCULUS)
	#include <OculusSpatializer.h>
	#include <CryCore/Platform/CryLibrary.h>
	#define OCULUS_SPATIALIZER_DLL "OculusSpatializerWwise.dll"
#endif // WWISE_USE_OCULUS

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include <AK/Comm/AkCommunication.h> // Communication between Wwise and the game (excluded in release build)
	#include <AK/Tools/Common/AkMonitorError.h>
	#include <AK/Tools/Common/AkPlatformFuncs.h>
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

/////////////////////////////////////////////////////////////////////////////////
//                              MEMORY HOOKS SETUP
//
//                             ##### IMPORTANT #####
//
// These custom alloc/free functions are declared as "extern" in AkMemoryMgr.h
// and MUST be defined by the game developer.
/////////////////////////////////////////////////////////////////////////////////

namespace AK
{
void* AllocHook(size_t in_size)
{
	return CryModuleMalloc(in_size);
}

void FreeHook(void* in_ptr)
{
	CryModuleFree(in_ptr);
}

void* VirtualAllocHook(void* in_pMemAddress, size_t in_size, DWORD in_dwAllocationType, DWORD in_dwProtect)
{
	return AllocHook(in_size);
}

void VirtualFreeHook(void* in_pMemAddress, size_t in_size, DWORD in_dwFreeType)
{
	FreeHook(in_pMemAddress);
}

#if CRY_PLATFORM_DURANGO
void* APUAllocHook(size_t in_size, unsigned int in_alignment)
{
	void* pAlloc = nullptr;

	#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
	size_t const nSecondSize = CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.MemSize();

	if (nSecondSize > 0)
	{
		size_t const nAllocHandle = CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.Allocate<size_t>(in_size, in_alignment);

		if (nAllocHandle > 0)
		{
			pAlloc = CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.Resolve<void*>(nAllocHandle);
			CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.Item(nAllocHandle)->Lock();
		}
	}
	#endif  // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

	CRY_ASSERT(pAlloc != nullptr);
	return pAlloc;
}

void APUFreeHook(void* in_pMemAddress)
{
	#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
	size_t const nAllocHandle = CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.AddressToHandle(in_pMemAddress);
	CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.Free(nAllocHandle);
	#else
	CryFatalError("%s", "<Audio>: Called APUFreeHook without secondary pool");
	#endif  // PROVIDE_AUDIO_IMPL_SECONDARY_POOL
}
#endif  // CRY_PLATFORM_DURANGO
}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
static void ErrorMonitorCallback(
	AK::Monitor::ErrorCode in_eErrorCode,   ///< Error code number value
	const AkOSChar* in_pszError,            ///< Message or error string to be displayed
	AK::Monitor::ErrorLevel in_eErrorLevel, ///< Specifies whether it should be displayed as a message or an error
	AkPlayingID in_playingID,               ///< Related Playing ID if applicable, AK_INVALID_PLAYING_ID otherwise
	AkGameObjectID in_gameObjID             ///< Related Game Object ID if applicable, AK_INVALID_GAME_OBJECT otherwise
	)
{
	char* szTemp = nullptr;
	CONVERT_OSCHAR_TO_CHAR(in_pszError, szTemp);

	CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
	CryAudio::Impl::Wwise::CEvent const* const pEvent = stl::find_in_map(CryAudio::Impl::Wwise::g_playingIds, in_playingID, nullptr);
	CryAudio::Impl::Wwise::CObject const* const pObject = stl::find_in_map(CryAudio::Impl::Wwise::g_gameObjectIds, in_gameObjID, nullptr);
	char const* const szEventName = (pEvent != nullptr) ? pEvent->GetName() : "Unknown PlayingID";
	char const* const szObjectName = (pObject != nullptr) ? pObject->GetName() : "Unknown GameObjID";
	Cry::Audio::Log(
		((in_eErrorLevel& AK::Monitor::ErrorLevel_Error) != 0) ? CryAudio::ELogType::Error : CryAudio::ELogType::Comment,
		"<Wwise> %s | ErrorCode: %d | PlayingID: %u (%s) | GameObjID: %" PRISIZE_T " (%s)", szTemp, in_eErrorCode, in_playingID, szEventName, in_gameObjID, szObjectName);
}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
CAuxThread g_auxAudioThread;
std::map<AkUniqueID, float> g_maxAttenuations;

SPoolSizes g_poolSizes;
SPoolSizes g_poolSizesLevelSpecific;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
SPoolSizes g_debugPoolSizes;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

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

	uint16 numEnvironments = 0;
	pNode->getAttr(s_szEnvironmentsAttribute, numEnvironments);
	poolSizes.environments += numEnvironments;

	uint16 numFiles = 0;
	pNode->getAttr(s_szFilesAttribute, numFiles);
	poolSizes.files += numFiles;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Wwise Object Pool");
	CObject::CreateAllocator(objectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Wwise Event Pool");
	CEvent::CreateAllocator(eventPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Wwise Trigger Pool");
	CTrigger::CreateAllocator(g_poolSizes.triggers);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Wwise Parameter Pool");
	CParameter::CreateAllocator(g_poolSizes.parameters);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Wwise Switch State Pool");
	CSwitchState::CreateAllocator(g_poolSizes.switchStates);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Wwise Environment Pool");
	CEnvironment::CreateAllocator(g_poolSizes.environments);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Wwise File Pool");
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
	CEnvironment::FreeMemoryPool();
	CFile::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void LoadEventsMaxAttenuations(const string& soundbanksPath)
{
	g_maxAttenuations.clear();
	string const bankInfoPath = soundbanksPath + "/SoundbanksInfo.xml";
	XmlNodeRef const pRootNode = GetISystem()->LoadXmlFromFile(bankInfoPath.c_str());

	if (pRootNode != nullptr)
	{
		XmlNodeRef const pSoundBanksNode = pRootNode->findChild("SoundBanks");

		if (pSoundBanksNode != nullptr)
		{
			int const numSoundBankNodes = pSoundBanksNode->getChildCount();

			for (int i = 0; i < numSoundBankNodes; ++i)
			{
				XmlNodeRef const pSoundBankNode = pSoundBanksNode->getChild(i);

				if (pSoundBankNode != nullptr)
				{
					XmlNodeRef const pIncludedEventsNode = pSoundBankNode->findChild("IncludedEvents");

					if (pIncludedEventsNode != nullptr)
					{
						int const numEventNodes = pIncludedEventsNode->getChildCount();

						for (int j = 0; j < numEventNodes; ++j)
						{
							XmlNodeRef const pEventNode = pIncludedEventsNode->getChild(j);

							if ((pEventNode != nullptr) && pEventNode->haveAttr("MaxAttenuation"))
							{
								float maxAttenuation = 0.0f;
								pEventNode->getAttr("MaxAttenuation", maxAttenuation);

								uint32 id = 0;
								pEventNode->getAttr("Id", id);

								g_maxAttenuations[static_cast<AkUniqueID>(id)] = maxAttenuation;
							}
						}
					}
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////
CSwitchState* ParseWwiseRtpcSwitch(XmlNodeRef const pNode)
{
	CSwitchState* pSwitchStateImpl = nullptr;

	char const* const szName = pNode->getAttr(s_szNameAttribute);

	if ((szName != nullptr) && (szName[0] != '\0'))
	{
		float value = s_defaultStateValue;

		if (pNode->getAttr(s_szValueAttribute, value))
		{
			AkUniqueID const rtpcId = AK::SoundEngine::GetIDFromString(szName);
			pSwitchStateImpl = new CSwitchState(ESwitchType::Rtpc, rtpcId, rtpcId, value);
		}
	}
	else
	{
		Cry::Audio::Log(
			ELogType::Warning,
			"The Wwise Rtpc %s inside SwitchState does not have a valid name.",
			szName);
	}

	return pSwitchStateImpl;
}

///////////////////////////////////////////////////////////////////////////
void ParseRtpcImpl(XmlNodeRef const pNode, AkRtpcID& rtpcId, float& multiplier, float& shift)
{
	if (_stricmp(pNode->getTag(), s_szParameterTag) == 0)
	{
		char const* const szName = pNode->getAttr(s_szNameAttribute);
		rtpcId = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(szName));

		if (rtpcId != AK_INVALID_RTPC_ID)
		{
			//the Wwise name is supplied
			pNode->getAttr(s_szMutiplierAttribute, multiplier);
			pNode->getAttr(s_szShiftAttribute, shift);
		}
		else
		{
			// Invalid Wwise RTPC name!
			Cry::Audio::Log(ELogType::Warning, "Invalid Wwise parameter name %s", szName);
		}
	}
	else
	{
		// Unknown Wwise RTPC tag!
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag %s", pNode->getTag());
	}
}

//////////////////////////////////////////////////////////////////////////
bool ParseSwitchOrState(XmlNodeRef const pNode, AkUInt32& stateOrSwitchGroupId, AkUInt32& stateOrSwitchId)
{
	bool bSuccess = false;
	char const* const szStateOrSwitchGroupName = pNode->getAttr(s_szNameAttribute);

	if ((szStateOrSwitchGroupName != nullptr) && (szStateOrSwitchGroupName[0] != 0) && (pNode->getChildCount() == 1))
	{
		XmlNodeRef const pValueNode(pNode->getChild(0));

		if (pValueNode && _stricmp(pValueNode->getTag(), s_szValueTag) == 0)
		{
			char const* const szStateOrSwitchName = pValueNode->getAttr(s_szNameAttribute);

			if ((szStateOrSwitchName != nullptr) && (szStateOrSwitchName[0] != 0))
			{
				stateOrSwitchGroupId = AK::SoundEngine::GetIDFromString(szStateOrSwitchGroupName);
				stateOrSwitchId = AK::SoundEngine::GetIDFromString(szStateOrSwitchName);
				bSuccess = true;
			}
		}
	}
	else
	{
		Cry::Audio::Log(
			ELogType::Warning,
			"A Wwise SwitchGroup or StateGroup %s inside SwitchState needs to have exactly one WwiseValue.",
			szStateOrSwitchGroupName);
	}

	return bSuccess;
}

///////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_gameObjectId(2) // Start with id 2, because id 1 would get ignored when setting a parameter on all constructed objects.
	, m_initBankId(AK_INVALID_BANK_ID)
	, m_toBeReleased(false)
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	, m_bCommSystemInitialized(false)
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
#if defined(WWISE_USE_OCULUS)
	, m_pOculusSpatializerLibrary(nullptr)
#endif // WWISE_USE_OCULUS
{
	g_pImpl = this;
	m_regularSoundBankFolder = AUDIO_SYSTEM_DATA_ROOT "/";
	m_regularSoundBankFolder += s_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szAssetsFolderName;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	m_name.Format(
		"%s (Build: %d)",
		WWISE_IMPL_INFO_STRING,
		AK_WWISESDK_VERSION_BUILD);
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::Update()
{
	if (AK::SoundEngine::IsInitialized())
	{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
		AKRESULT wwiseResult = AK_Fail;
		static int enableOutputCapture = 0;

		if (g_cvars.m_enableOutputCapture == 1 && enableOutputCapture == 0)
		{
			AkOSChar const* pOutputCaptureFileName = nullptr;
			CONVERT_CHAR_TO_OSCHAR("../wwise_audio_capture.wav", pOutputCaptureFileName);
			wwiseResult = AK::SoundEngine::StartOutputCapture(pOutputCaptureFileName);
			AKASSERT((wwiseResult == AK_Success) || !"StartOutputCapture failed!");
			enableOutputCapture = g_cvars.m_enableOutputCapture;
		}
		else if (g_cvars.m_enableOutputCapture == 0 && enableOutputCapture == 1)
		{
			wwiseResult = AK::SoundEngine::StopOutputCapture();
			AKASSERT((wwiseResult == AK_Success) || !"StopOutputCapture failed!");
			enableOutputCapture = g_cvars.m_enableOutputCapture;
		}
#endif    // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

		AK::SoundEngine::RenderAudio();
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	// If something fails so severely during initialization that we need to fall back to the NULL implementation
	// we will need to shut down what has been initialized so far. Therefore make sure to call Shutdown() before returning eARS_FAILURE!

	AllocateMemoryPools(objectPoolSize, eventPoolSize);

	AkMemSettings memSettings;
	memSettings.uMaxNumPools = 20;
	AKRESULT wwiseResult = AK::MemoryMgr::Init(&memSettings);

	if (wwiseResult != AK_Success)
	{
		Cry::Audio::Log(ELogType::Error, "AK::MemoryMgr::Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return ERequestStatus::Failure;
	}

	AkMemPoolId const prepareMemPoolId = AK::MemoryMgr::CreatePool(nullptr, g_cvars.m_prepareEventMemoryPoolSize << 10, 16, AkMalloc, 16);

	if (prepareMemPoolId == AK_INVALID_POOL_ID)
	{
		Cry::Audio::Log(ELogType::Error, "AK::MemoryMgr::CreatePool() PrepareEventMemoryPool failed!\n");
		ShutDown();

		return ERequestStatus::Failure;
	}

	wwiseResult = AK::MemoryMgr::SetPoolName(prepareMemPoolId, "PrepareEventMemoryPool");

	if (wwiseResult != AK_Success)
	{
		Cry::Audio::Log(ELogType::Error, "AK::MemoryMgr::SetPoolName() could not set name of event prepare memory pool!\n");
		ShutDown();

		return ERequestStatus::Failure;
	}

	AkStreamMgrSettings streamSettings;
	AK::StreamMgr::GetDefaultSettings(streamSettings);
	streamSettings.uMemorySize = g_cvars.m_streamManagerMemoryPoolSize << 10; // 64 KiB is the default value!
	if (AK::StreamMgr::Create(streamSettings) == nullptr)
	{
		Cry::Audio::Log(ELogType::Error, "AK::StreamMgr::Create() failed!\n");
		ShutDown();

		return ERequestStatus::Failure;
	}

	IThreadConfigManager* pThreadConfigMngr = gEnv->pThreadManager->GetThreadConfigManager();
	const SThreadConfig* pDeviceThread = pThreadConfigMngr->GetThreadConfig("Wwise_Device");
	const SThreadConfig* pBankManger = pThreadConfigMngr->GetThreadConfig("Wwise_BankManager");
	const SThreadConfig* pLEngineThread = pThreadConfigMngr->GetThreadConfig("Wwise_LEngine");
	const SThreadConfig* pMonitorThread = pThreadConfigMngr->GetThreadConfig("Wwise_Monitor");

	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);
	deviceSettings.uIOMemorySize = g_cvars.m_streamDeviceMemoryPoolSize << 10; // 2 MiB is the default value!

	// Device thread settings
	if (pDeviceThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_Affinity)
		deviceSettings.threadProperties.dwAffinityMask = pDeviceThread->affinityFlag;
	if (pDeviceThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_Priority)
#if defined(CRY_PLATFORM_POSIX)
		if (!(pDeviceThread->priority > 0 && pDeviceThread->priority <= 99))
			CryWarning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, "Thread 'Wwise_Device' requested priority %d when allowed range is 1 to 99. Applying default value of %d.", pDeviceThread->priority, deviceSettings.threadProperties.nPriority);
		else
#endif
		deviceSettings.threadProperties.nPriority = pDeviceThread->priority;

	if (pDeviceThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_StackSize)
		deviceSettings.threadProperties.uStackSize = pDeviceThread->stackSizeBytes;

	wwiseResult = m_fileIOHandler.Init(deviceSettings);

	if (wwiseResult != AK_Success)
	{
		Cry::Audio::Log(ELogType::Error, "m_fileIOHandler.Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return ERequestStatus::Failure;
	}

	CryFixedStringT<AK_MAX_PATH> temp = AUDIO_SYSTEM_DATA_ROOT "/";
	temp.append(s_szImplFolderName);
	temp.append("/");
	temp.append(s_szAssetsFolderName);
	temp.append("/");
	AkOSChar const* szTemp = nullptr;
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);
	m_fileIOHandler.SetBankPath(szTemp);

	AkInitSettings initSettings;
	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	initSettings.uDefaultPoolSize = g_cvars.m_soundEngineDefaultMemoryPoolSize << 10;
	initSettings.uCommandQueueSize = g_cvars.m_commandQueueMemoryPoolSize << 10;
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	initSettings.uMonitorPoolSize = g_cvars.m_monitorMemoryPoolSize << 10;
	initSettings.uMonitorQueuePoolSize = g_cvars.m_monitorQueueMemoryPoolSize << 10;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
	initSettings.uPrepareEventMemoryPoolID = prepareMemPoolId;
	initSettings.bEnableGameSyncPreparation = false;//TODO: ???
	g_cvars.m_panningRule = crymath::clamp(g_cvars.m_panningRule, 0, 1);
	initSettings.settingsMainOutput.ePanningRule = static_cast<AkPanningRule>(g_cvars.m_panningRule);

	initSettings.bUseLEngineThread = g_cvars.m_enableEventManagerThread > 0;
	initSettings.bUseSoundBankMgrThread = g_cvars.m_enableSoundBankManagerThread > 0;
	initSettings.uNumSamplesPerFrame = static_cast<AkUInt32>(g_cvars.m_numSamplesPerFrame);

	// We need this additional thread during bank unloading if the user decided to run Wwise without the EventManager thread.
	if (g_cvars.m_enableEventManagerThread == 0)
	{
		g_auxAudioThread.Init();
	}

	AkPlatformInitSettings platformInitSettings;
	AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);
	platformInitSettings.uLEngineDefaultPoolSize = g_cvars.m_lowerEngineDefaultPoolSize << 10;
	platformInitSettings.uNumRefillsInVoice = static_cast<AkUInt16>(g_cvars.m_numRefillsInVoice);

	// Bank Manager thread settings
	if (pBankManger->paramActivityFlag & SThreadConfig::eThreadParamFlag_Affinity)
		platformInitSettings.threadBankManager.dwAffinityMask = pBankManger->affinityFlag;

	if (pBankManger->paramActivityFlag & SThreadConfig::eThreadParamFlag_Priority)
#if defined(CRY_PLATFORM_POSIX)
		if (!(pBankManger->priority > 0 && pBankManger->priority <= 99))
			CryWarning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, "Thread 'Wwise_BankManager' requested priority %d when allowed range is 1 to 99. Applying default value of %d.", pBankManger->priority, platformInitSettings.threadBankManager.nPriority);
		else
#endif
		platformInitSettings.threadBankManager.nPriority = pBankManger->priority;

	if (pBankManger->paramActivityFlag & SThreadConfig::eThreadParamFlag_StackSize)
		platformInitSettings.threadBankManager.uStackSize = pBankManger->stackSizeBytes;

	// LEngine thread settings
	if (pLEngineThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_Affinity)
		platformInitSettings.threadLEngine.dwAffinityMask = pLEngineThread->affinityFlag;

	if (pLEngineThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_Priority)
#if defined(CRY_PLATFORM_POSIX)
		if (!(pLEngineThread->priority > 0 && pLEngineThread->priority <= 99))
			CryWarning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, "Thread 'Wwise_LEngine' requested priority %d when allowed range is 1 to 99. Applying default value of %d.", pLEngineThread->priority, platformInitSettings.threadLEngine.nPriority);
		else
#endif
		platformInitSettings.threadLEngine.nPriority = pLEngineThread->priority;

	if (pLEngineThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_StackSize)
		platformInitSettings.threadLEngine.uStackSize = pLEngineThread->stackSizeBytes;

	// Monitor thread settings
	if (pMonitorThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_Affinity)
		platformInitSettings.threadMonitor.dwAffinityMask = pMonitorThread->affinityFlag;

	if (pMonitorThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_Priority)
#if defined(CRY_PLATFORM_POSIX)
		if (!(pMonitorThread->priority > 0 && pMonitorThread->priority <= 99))
			CryWarning(VALIDATOR_MODULE_AUDIO, VALIDATOR_WARNING, "Thread 'Wwise_Monitor' requested priority %d when allowed range is 1 to 99. Applying default value of %d.", pMonitorThread->priority, platformInitSettings.threadMonitor.nPriority);
		else
#endif
		platformInitSettings.threadMonitor.nPriority = pMonitorThread->priority;

	if (pMonitorThread->paramActivityFlag & SThreadConfig::eThreadParamFlag_StackSize)
		platformInitSettings.threadMonitor.uStackSize = pMonitorThread->stackSizeBytes;

	wwiseResult = AK::SoundEngine::Init(&initSettings, &platformInitSettings);

	if (wwiseResult != AK_Success)
	{
		Cry::Audio::Log(ELogType::Error, "AK::SoundEngine::Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return ERequestStatus::Failure;
	}

	AkMusicSettings musicSettings;
	AK::MusicEngine::GetDefaultInitSettings(musicSettings);

	wwiseResult = AK::MusicEngine::Init(&musicSettings);

	if (wwiseResult != AK_Success)
	{
		Cry::Audio::Log(ELogType::Error, "AK::MusicEngine::Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return ERequestStatus::Failure;
	}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	if (g_cvars.m_enableCommSystem == 1)
	{
		m_bCommSystemInitialized = true;
		AkCommSettings commSettings;
		AK::Comm::GetDefaultInitSettings(commSettings);

		wwiseResult = AK::Comm::Init(commSettings);

		if (wwiseResult != AK_Success)
		{
			Cry::Audio::Log(ELogType::Error, "AK::Comm::Init() returned AKRESULT %d. Communication between the Wwise authoring application and the game will not be possible\n", wwiseResult);
			m_bCommSystemInitialized = false;
		}

		wwiseResult = AK::Monitor::SetLocalOutput(AK::Monitor::ErrorLevel_All, ErrorMonitorCallback);

		if (wwiseResult != AK_Success)
		{
			AK::Comm::Term();
			Cry::Audio::Log(ELogType::Error, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", wwiseResult);
			m_bCommSystemInitialized = false;
		}
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

#if defined(WWISE_USE_OCULUS)
	m_pOculusSpatializerLibrary = CryLoadLibrary(OCULUS_SPATIALIZER_DLL);

	if (m_pOculusSpatializerLibrary != nullptr)
	{
		typedef bool (__stdcall * AkGetSoundEngineCallbacksType)
		  (unsigned short in_usCompanyID,
		  unsigned short in_usPluginID,
		  AkCreatePluginCallback& out_funcEffect,
		  AkCreateParamCallback& out_funcParam);

		AkGetSoundEngineCallbacksType AkGetSoundEngineCallbacks =
			(AkGetSoundEngineCallbacksType)CryGetProcAddress(m_pOculusSpatializerLibrary, "AkGetSoundEngineCallbacks");

		if (AkGetSoundEngineCallbacks != nullptr)
		{
			AkCreatePluginCallback CreateOculusFX;
			AkCreateParamCallback CreateOculusFXParams;

			// Register plugin effect
			if (AkGetSoundEngineCallbacks(AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER, CreateOculusFX, CreateOculusFXParams))
			{
				wwiseResult = AK::SoundEngine::RegisterPlugin(AkPluginTypeMixer, AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER, CreateOculusFX, CreateOculusFXParams);

				if (wwiseResult != AK_Success)
				{
					Cry::Audio::Log(ELogType::Error, "Failed to register OculusSpatializer plugin.");
				}
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, "Failed call to AkGetSoundEngineCallbacks in " OCULUS_SPATIALIZER_DLL);
			}

			// Register plugin attachment (for data attachment on individual sounds, like frequency hints etc.)
			if (AkGetSoundEngineCallbacks(AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER_ATTACHMENT, CreateOculusFX, CreateOculusFXParams))
			{
				wwiseResult = AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER_ATTACHMENT, nullptr, CreateOculusFXParams);

				if (wwiseResult != AK_Success)
				{
					Cry::Audio::Log(ELogType::Error, "Failed to register OculusSpatializer attachment.");
				}
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, "Failed call to AkGetSoundEngineCallbacks in " OCULUS_SPATIALIZER_DLL);
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Failed to load functions AkGetSoundEngineCallbacks in " OCULUS_SPATIALIZER_DLL);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Failed to load " OCULUS_SPATIALIZER_DLL);
	}
#endif  // WWISE_USE_OCULUS

	REINST("Register Global Callback")

	//wwiseResult = AK::SoundEngine::RegisterGlobalCallback(GlobalCallback);
	//if (wwiseResult != AK_Success)
	//{
	//	Cry::Audio::Log(eALT_WARNING, "AK::SoundEngine::RegisterGlobalCallback() returned AKRESULT %d", wwiseResult);
	//}

	// Load Init.bnk before making the system available to the users
	temp = "Init.bnk";
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);

	wwiseResult = AK::SoundEngine::LoadBank(szTemp, AK_DEFAULT_POOL_ID, m_initBankId);

	if (wwiseResult != AK_Success)
	{
		// This does not qualify for a fallback to the NULL implementation!
		// Still notify the user about this failure!
		Cry::Audio::Log(ELogType::Error, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", wwiseResult);
		m_initBankId = AK_INVALID_BANK_ID;
	}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	GetInitBankSize();
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	LoadEventsMaxAttenuations(m_regularSoundBankFolder);

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
	AKRESULT wwiseResult = AK_Fail;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	if (m_bCommSystemInitialized)
	{
		AK::Comm::Term();

		wwiseResult = AK::Monitor::SetLocalOutput(0, nullptr);

		if (wwiseResult != AK_Success)
		{
			Cry::Audio::Log(ELogType::Warning, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", wwiseResult);
		}

		m_bCommSystemInitialized = false;
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	AK::MusicEngine::Term();

	if (AK::SoundEngine::IsInitialized())
	{
		if (g_cvars.m_enableEventManagerThread > 0)
		{
			wwiseResult = AK::SoundEngine::ClearBanks();
		}
		else
		{
			SignalAuxAudioThread();
			wwiseResult = AK::SoundEngine::ClearBanks();
			g_auxAudioThread.SignalStopWork();
		}

		if (wwiseResult != AK_Success)
		{
			Cry::Audio::Log(ELogType::Error, "Failed to clear banks\n");
		}

		REINST("Unregister global callback")
		//wwiseResult = AK::SoundEngine::UnregisterGlobalCallback(GlobalCallback);
		//ASSERT_WWISE_OK(wwiseResult);

		//if (wwiseResult != AK_Success)
		//{
		//	Cry::Audio::Log(eALT_WARNING, "AK::SoundEngine::UnregisterGlobalCallback() returned AKRESULT %d", wwiseResult);
		//}

		AK::SoundEngine::Query::AkGameObjectsList objectList;
		AK::SoundEngine::Query::GetActiveGameObjects(objectList);
		AkUInt32 const length = objectList.Length();

		for (AkUInt32 i = 0; i < length; ++i)
		{
			// This call requires at least Wwise v2017.1.0.
			AK::SoundEngine::CancelEventCallbackGameObject(objectList[i]);
		}

		objectList.Term();
		AK::SoundEngine::Term();
	}

	// Terminate the streaming device and streaming manager
	// CAkFilePackageLowLevelIOBlocking::Term() destroys its associated streaming device
	// that lives in the Stream Manager, and unregisters itself as the File Location Resolver.
	if (AK::IAkStreamMgr::Get())
	{
		m_fileIOHandler.ShutDown();
		AK::IAkStreamMgr::Get()->Destroy();
	}

	// Terminate the Memory Manager
	if (AK::MemoryMgr::IsInitialized())
	{
		wwiseResult = AK::MemoryMgr::DestroyPool(0);
		AK::MemoryMgr::Term();
	}

#if defined(WWISE_USE_OCULUS)
	if (m_pOculusSpatializerLibrary != nullptr)
	{
		CryFreeLibrary(m_pOculusSpatializerLibrary);
		m_pOculusSpatializerLibrary = nullptr;
	}
#endif  // WWISE_USE_OCULUS
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeRelease()
{
	m_toBeReleased = true;
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
		g_poolSizesLevelSpecific.environments = std::max(g_poolSizesLevelSpecific.environments, levelPoolSizes.environments);
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

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	ZeroStruct(g_debugPoolSizes);
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_poolSizes.triggers += g_poolSizesLevelSpecific.triggers;
	g_poolSizes.parameters += g_poolSizesLevelSpecific.parameters;
	g_poolSizes.switchStates += g_poolSizesLevelSpecific.switchStates;
	g_poolSizes.environments += g_poolSizesLevelSpecific.environments;
	g_poolSizes.files += g_poolSizesLevelSpecific.files;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	g_poolSizes.triggers = std::max<uint16>(1, g_poolSizes.triggers);
	g_poolSizes.parameters = std::max<uint16>(1, g_poolSizes.parameters);
	g_poolSizes.switchStates = std::max<uint16>(1, g_poolSizes.switchStates);
	g_poolSizes.environments = std::max<uint16>(1, g_poolSizes.environments);
	g_poolSizes.files = std::max<uint16>(1, g_poolSizes.files);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnLoseFocus()
{
	// With Wwise we drive this via events.
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnGetFocus()
{
	// With Wwise we drive this via events.
}

///////////////////////////////////////////////////////////////////////////
void CImpl::MuteAll()
{
	// With Wwise we drive this via events.
}

///////////////////////////////////////////////////////////////////////////
void CImpl::UnmuteAll()
{
	// With Wwise we drive this via events.
}

///////////////////////////////////////////////////////////////////////////
void CImpl::PauseAll()
{
	// With Wwise we drive this via events.
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ResumeAll()
{
	// With Wwise we drive this via events.
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::StopAllSounds()
{
	AK::SoundEngine::StopAll();

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalParameter(IParameterConnection* const pIParameterConnection, float const value)
{
	if (pIParameterConnection != nullptr)
	{
		auto const pParameter = static_cast<CParameter const*>(pIParameterConnection);
		auto const rtpcValue = static_cast<AkRtpcValue>(pParameter->GetMultiplier() * value + pParameter->GetShift());

		AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(pParameter->GetId(), rtpcValue, AK_INVALID_GAME_OBJECT);

		if (!IS_WWISE_OK(wwiseResult))
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"Wwise - failed to set the Rtpc %" PRISIZE_T " globally to value %f",
				pParameter->GetId(),
				static_cast<AkRtpcValue>(value));
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid RtpcData passed to the Wwise implementation of %s", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalSwitchState(ISwitchStateConnection* const pISwitchStateConnection)
{
	if (pISwitchStateConnection != nullptr)
	{
		auto const pSwitchState = static_cast<CSwitchState const*>(pISwitchStateConnection);

		switch (pSwitchState->GetType())
		{
		case ESwitchType::StateGroup:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetState(
					pSwitchState->GetStateOrSwitchGroupId(),
					pSwitchState->GetStateOrSwitchId());

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise failed to set the StateGroup %" PRISIZE_T "to state %" PRISIZE_T,
						pSwitchState->GetStateOrSwitchGroupId(),
						pSwitchState->GetStateOrSwitchId());
				}

				break;
			}
		case ESwitchType::SwitchGroup:
			{
				Cry::Audio::Log(ELogType::Warning, "Wwise - SwitchGroups cannot get set globally!");

				break;
			}
		case ESwitchType::Rtpc:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(
					pSwitchState->GetStateOrSwitchGroupId(),
					static_cast<AkRtpcValue>(pSwitchState->GetRtpcValue()),
					AK_INVALID_GAME_OBJECT);

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the Rtpc %" PRISIZE_T " globally to value %f",
						pSwitchState->GetStateOrSwitchGroupId(),
						static_cast<AkRtpcValue>(pSwitchState->GetRtpcValue()));
				}

				break;
			}
		case ESwitchType::None:
			{
				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Wwise - Unknown ESwitchType: %" PRISIZE_T, pSwitchState->GetType());

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid SwitchState passed to the Wwise implementation of %s", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			AkBankID bankId = AK_INVALID_BANK_ID;

			AKRESULT const wwiseResult = AK::SoundEngine::LoadBank(
				pFileInfo->pFileData,
				static_cast<AkUInt32>(pFileInfo->size),
				bankId);

			if (IS_WWISE_OK(wwiseResult))
			{
				pFileData->bankId = bankId;
				result = ERequestStatus::Success;
			}
			else
			{
				pFileData->bankId = AK_INVALID_BANK_ID;
				Cry::Audio::Log(ELogType::Error, "Failed to load file %s\n", pFileInfo->szFileName);
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the Wwise implementation of RegisterInMemoryFile");
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			AKRESULT wwiseResult = AK_Fail;

			// If the EventManager thread has been disabled the synchronous UnloadBank version will get stuck.
			if (g_cvars.m_enableEventManagerThread > 0)
			{
				wwiseResult = AK::SoundEngine::UnloadBank(pFileData->bankId, pFileInfo->pFileData);
			}
			else
			{
				SignalAuxAudioThread();
				wwiseResult = AK::SoundEngine::UnloadBank(pFileData->bankId, pFileInfo->pFileData);
				WaitForAuxAudioThread();
			}

			if (IS_WWISE_OK(wwiseResult))
			{
				result = ERequestStatus::Success;
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, "Wwise Failed to unregister in memory file %s\n", pFileInfo->szFileName);
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the Wwise implementation of UnregisterInMemoryFile");
		}
	}

	return result;
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
			pFileInfo->memoryBlockAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;
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
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
	implInfo.folderName = s_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	g_globalObjectId = m_gameObjectId++;
	char const* const szName = "GlobalObject";

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	AKRESULT const wwiseResult = AK::SoundEngine::RegisterGameObj(g_globalObjectId, szName);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise ConstructGlobalObject failed with AKRESULT: %d", wwiseResult);
	}
#else
	AK::SoundEngine::RegisterGameObj(g_globalObjectId);
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	CTransformation transformation;
	ZeroStruct(transformation);
	g_pObject = new CObject(g_globalObjectId, transformation, szName);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_gameObjectIds[g_globalObjectId] = g_pObject;
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	return static_cast<IObject*>(g_pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	AKRESULT const wwiseResult = AK::SoundEngine::RegisterGameObj(m_gameObjectId, szName);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise ConstructObject failed with AKRESULT: %d", wwiseResult);
	}
#else
	AK::SoundEngine::RegisterGameObj(m_gameObjectId);
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	auto const pObject = new CObject(m_gameObjectId++, transformation, szName);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_gameObjectIds[pObject->GetId()] = pObject;
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pObject = static_cast<CObject const*>(pIObject);
	AkGameObjectID const objectID = pObject->GetId() == AK_INVALID_GAME_OBJECT ? g_globalObjectId : pObject->GetId();
	AKRESULT const wwiseResult = AK::SoundEngine::UnregisterGameObj(objectID);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise DestructObject failed with AKRESULT: %d", wwiseResult);
	}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_gameObjectIds.erase(pObject->GetId());
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	delete pObject;

	if (objectID == g_globalObjectId)
	{
		g_pObject = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	IListener* pIListener = nullptr;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	AKRESULT wwiseResult = AK::SoundEngine::RegisterGameObj(m_gameObjectId, szName);

	if (IS_WWISE_OK(wwiseResult))
	{
		wwiseResult = AK::SoundEngine::SetDefaultListeners(&m_gameObjectId, 1);

		if (IS_WWISE_OK(wwiseResult))
		{
			g_pListener = new CListener(transformation, m_gameObjectId);

	#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
			if (szName != nullptr)
			{
				g_pListener->SetName(szName);
			}
	#endif    // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

			pIListener = static_cast<IListener*>(g_pListener);
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "SetDefaultListeners failed with AKRESULT: %d", wwiseResult);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "RegisterGameObj in ConstructListener failed with AKRESULT: %d", wwiseResult);
	}
#else
	AK::SoundEngine::RegisterGameObj(m_gameObjectId);
	AK::SoundEngine::SetDefaultListeners(&m_gameObjectId, 1);
	g_pListener = new CListener(transformation, m_gameObjectId);
	pIListener = static_cast<IListener*>(g_pListener);
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	g_listenerId = m_gameObjectId++;

	return pIListener;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructListener(IListener* const pIListener)
{
	CRY_ASSERT_MESSAGE(pIListener == g_pListener, "pIListener is not g_pListener during %s", __FUNCTION__);
	AKRESULT const wwiseResult = AK::SoundEngine::UnregisterGameObj(g_pListener->GetId());

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise DestructListener failed with AKRESULT: %d", wwiseResult);
	}

	delete g_pListener;
	g_pListener = nullptr;
}

//////////////////////////////////////////////////////////////////////////
IEvent* CImpl::ConstructEvent(CryAudio::CEvent& event)
{
	return static_cast<IEvent*>(new CEvent(event));
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEvent(IEvent const* const pIEvent)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	auto const pEvent = static_cast<CEvent const*>(pIEvent);

	if (pEvent != nullptr)
	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_playingIds.erase(pEvent->m_id);
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	delete pIEvent;
}

//////////////////////////////////////////////////////////////////////////
IStandaloneFileConnection* CImpl::ConstructStandaloneFileConnection(CryAudio::CStandaloneFile& standaloneFile, char const* const szFile, bool const bLocalized, ITriggerConnection const* pITriggerConnection /*= nullptr*/)
{
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
	CRY_ASSERT(m_mapInputDevices.find(deviceUniqueID) == m_mapInputDevices.end()); // Mustn't exist yet!
	AkOutputSettings settings("Wwise_Motion", static_cast<AkUniqueID>(deviceUniqueID));
	AkOutputDeviceID deviceID = AK_INVALID_OUTPUT_DEVICE_ID;
	AKRESULT const wwiseResult = AK::SoundEngine::AddOutput(settings, &deviceID, &g_listenerId, 1);

	if (IS_WWISE_OK(wwiseResult))
	{
		m_mapInputDevices[deviceUniqueID] = { true, deviceID };
	}
	else
	{
		m_mapInputDevices[deviceUniqueID] = { false, deviceID };
		Cry::Audio::Log(ELogType::Error, "AK::SoundEngine::AddOutput failed! (%u : %u)", deviceUniqueID, deviceID);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadDisconnected(DeviceId const deviceUniqueID)
{
	AudioInputDevices::const_iterator const Iter(m_mapInputDevices.find(deviceUniqueID));

	// If AK::SoundEngine::AddOutput failed, then the device is not in the list
	if (Iter != m_mapInputDevices.end())
	{
		if (Iter->second.akIsActiveOutputDevice)
		{
			AkOutputDeviceID const deviceID = Iter->second.akOutputDeviceID;
			AKRESULT const wwiseResult = AK::SoundEngine::RemoveOutput(deviceID);

			if (!IS_WWISE_OK(wwiseResult))
			{
				Cry::Audio::Log(ELogType::Error, "AK::SoundEngine::RemoveOutput failed in GamepadDisconnected! (%u : %u)", deviceUniqueID, deviceID);
			}
		}
		m_mapInputDevices.erase(Iter);
	}
}

///////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(XmlNodeRef const pRootNode, float& radius)
{
	CTrigger* pTrigger = nullptr;

	if (_stricmp(pRootNode->getTag(), s_szEventTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		AkUniqueID const uniqueId = AK::SoundEngine::GetIDFromString(szName); // Does not check if the string represents an event!

		if (uniqueId != AK_INVALID_UNIQUE_ID)
		{
			float maxAttenuation = 0.0f;
			auto const attenuationPair = g_maxAttenuations.find(uniqueId);

			if (attenuationPair != g_maxAttenuations.end())
			{
				maxAttenuation = attenuationPair->second;
			}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
			pTrigger = new CTrigger(uniqueId, maxAttenuation, szName);
			radius = maxAttenuation;
#else
			pTrigger = new CTrigger(uniqueId, maxAttenuation);
#endif      // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
		}
		else
		{
			CRY_ASSERT(false);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag: %s", pRootNode->getTag());
	}

	return static_cast<ITriggerConnection*>(pTrigger);
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	ITriggerConnection* pITriggerConnection = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		char const* const szName = pTriggerInfo->name.c_str();
		AkUniqueID const uniqueId = AK::SoundEngine::GetIDFromString(szName);

		pITriggerConnection = static_cast<ITriggerConnection*>(new CTrigger(uniqueId, 0.0f, szName));
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	delete pITriggerConnection;
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	CParameter* pParameter = nullptr;

	AkRtpcID rtpcId = AK_INVALID_RTPC_ID;
	float multiplier = s_defaultParamMultiplier;
	float shift = s_defaultParamShift;

	ParseRtpcImpl(pRootNode, rtpcId, multiplier, shift);

	if (rtpcId != AK_INVALID_RTPC_ID)
	{
		pParameter = new CParameter(rtpcId, multiplier, shift);
	}

	return static_cast<IParameterConnection*>(pParameter);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	delete pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	char const* const szTag = pRootNode->getTag();
	CSwitchState* pSwitchState = nullptr;

	if (_stricmp(szTag, s_szStateGroupTag) == 0)
	{
		AkUInt32 stateOrSwitchGroupId = AK_INVALID_UNIQUE_ID;
		AkUInt32 stateOrSwitchId = AK_INVALID_UNIQUE_ID;

		if (ParseSwitchOrState(pRootNode, stateOrSwitchGroupId, stateOrSwitchId))
		{
			pSwitchState = new CSwitchState(ESwitchType::StateGroup, stateOrSwitchGroupId, stateOrSwitchId);
		}
	}
	else if (_stricmp(szTag, s_szSwitchGroupTag) == 0)
	{
		AkUInt32 stateOrSwitchGroupId = AK_INVALID_UNIQUE_ID;
		AkUInt32 stateOrSwitchId = AK_INVALID_UNIQUE_ID;

		if (ParseSwitchOrState(pRootNode, stateOrSwitchGroupId, stateOrSwitchId))
		{
			pSwitchState = new CSwitchState(ESwitchType::SwitchGroup, stateOrSwitchGroupId, stateOrSwitchId);
		}
	}
	else if (_stricmp(szTag, s_szParameterTag) == 0)
	{
		pSwitchState = ParseWwiseRtpcSwitch(pRootNode);
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag: %s", szTag);
	}

	return static_cast<ISwitchStateConnection*>(pSwitchState);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	delete pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	char const* const szTag = pRootNode->getTag();
	CEnvironment* pEnvironment = nullptr;

	if (_stricmp(szTag, s_szAuxBusTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		AkUniqueID const busId = AK::SoundEngine::GetIDFromString(szName);

		if (busId != AK_INVALID_AUX_ID)
		{
			pEnvironment = new CEnvironment(EEnvironmentType::AuxBus, static_cast<AkAuxBusID>(busId));
		}
		else
		{
			CRY_ASSERT(false); // Unknown AuxBus
		}
	}
	else if (_stricmp(szTag, s_szParameterTag) == 0)
	{
		AkRtpcID rtpcId = AK_INVALID_RTPC_ID;
		float multiplier = s_defaultParamMultiplier;
		float shift = s_defaultParamShift;

		ParseRtpcImpl(pRootNode, rtpcId, multiplier, shift);

		if (rtpcId != AK_INVALID_RTPC_ID)
		{
			pEnvironment = new CEnvironment(EEnvironmentType::Rtpc, rtpcId, multiplier, shift);
		}
		else
		{
			CRY_ASSERT(false); // Unknown parameter
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag: %s", szTag);
	}

	return static_cast<IEnvironmentConnection*>(pEnvironment);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
	delete pIEnvironmentConnection;
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	return static_cast<ISettingConnection*>(new CSetting);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructSettingConnection(ISettingConnection const* const pISettingConnection)
{
	delete pISettingConnection;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetFileData(char const* const szName, SFileData& fileData) const
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SignalAuxAudioThread()
{
	g_auxAudioThread.m_lock.Lock();
	g_auxAudioThread.m_threadState = CAuxThread::EAuxThreadState::Start;
	g_auxAudioThread.m_lock.Unlock();
	g_auxAudioThread.m_sem.Notify();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::WaitForAuxAudioThread()
{
	g_auxAudioThread.m_lock.Lock();
	g_auxAudioThread.m_threadState = CAuxThread::EAuxThreadState::Stop;

	// Wait until the AuxWwiseAudioThread is actually waiting again and not processing any work.
	while (g_auxAudioThread.m_threadState != CAuxThread::EAuxThreadState::Wait)
	{
		g_auxAudioThread.m_sem.Wait(g_auxAudioThread.m_lock);
	}

	g_auxAudioThread.m_lock.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnRefresh()
{
	AKRESULT wwiseResult = AK_Fail;

	if (m_initBankId != AK_INVALID_BANK_ID)
	{
		if (g_cvars.m_enableEventManagerThread > 0)
		{
			wwiseResult = AK::SoundEngine::UnloadBank(m_initBankId, nullptr);
		}
		else
		{
			SignalAuxAudioThread();
			wwiseResult = AK::SoundEngine::UnloadBank(m_initBankId, nullptr);
			WaitForAuxAudioThread();
		}

		if (wwiseResult != AK_Success)
		{
			Cry::Audio::Log(ELogType::Error, "Wwise failed to unload Init.bnk, returned the AKRESULT: %d", wwiseResult);
		}
	}

	CryFixedStringT<MaxFilePathLength> const temp("Init.bnk");
	AkOSChar const* szTemp = nullptr;
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);

	wwiseResult = AK::SoundEngine::LoadBank(szTemp, AK_DEFAULT_POOL_ID, m_initBankId);

	if (wwiseResult != AK_Success)
	{
		Cry::Audio::Log(ELogType::Error, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", wwiseResult);
		m_initBankId = AK_INVALID_BANK_ID;
	}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	GetInitBankSize();
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	LoadEventsMaxAttenuations(m_regularSoundBankFolder);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLanguage(char const* const szLanguage)
{
	if (szLanguage != nullptr)
	{
		m_localizedSoundBankFolder = PathUtil::GetLocalizationFolder().c_str();
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += szLanguage;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += AUDIO_SYSTEM_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szAssetsFolderName;

		CryFixedStringT<MaxFilePathLength> temp(m_localizedSoundBankFolder);
		temp += "/";
		AkOSChar const* pTemp = nullptr;
		CONVERT_CHAR_TO_OSCHAR(temp.c_str(), pTemp);
		m_fileIOHandler.SetLanguageFolder(pTemp);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetPanningRule(int const panningRule)
{
	AK::SoundEngine::SetPanningRule(static_cast<AkPanningRule>(panningRule));
}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
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

//////////////////////////////////////////////////////////////////////////
void CImpl::GetInitBankSize()
{
	string const initBankPath = m_regularSoundBankFolder + "/Init.bnk";
	m_initBankSize = gEnv->pCryPak->FGetSize(initBankPath.c_str());
}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
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
		posY += g_debugSystemLineHeight;
		auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false, "Init.bnk: %uKiB",
		                    static_cast<uint32>(m_initBankSize / 1024));

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

		if (g_debugPoolSizes.environments > 0)
		{
			auto& allocator = CEnvironment::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Environments");
		}

		if (g_debugPoolSizes.files > 0)
		{
			auto& allocator = CFile::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Files");
		}
	}

	if (g_numObjectsWithRelativeVelocity > 0)
	{
		posY += g_debugSystemLineHeight;
		auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false, "Objects with relative velocity calculation: %u", g_numObjectsWithRelativeVelocity);
	}

	Vec3 const& listenerPosition = g_pListener->GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	float const listenerVelocity = g_pListener->GetVelocity().GetLength();
	char const* const szName = g_pListener->GetName();

	posY += g_debugSystemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorListenerActive.data(), false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f | Velocity: %.2f m/s",
	                    szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z, listenerVelocity);

#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
