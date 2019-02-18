// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"
#include "CVars.h"
#include "FileIOHandler.h"
#include "GlobalData.h"
#include "AuxThread.h"
#include "Common.h"
#include "AuxBus.h"
#include "Event.h"
#include "EventInstance.h"
#include "Parameter.h"
#include "ParameterEnvironment.h"
#include "ParameterState.h"
#include "Listener.h"
#include "Object.h"
#include "GlobalObject.h"
#include "SoundBank.h"
#include "State.h"
#include "Switch.h"

#include <ISettingConnection.h>
#include <FileInfo.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	#include <Logger.h>
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	#include <DebugStyle.h>
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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
	#define CRY_AUDIO_IMPL_WWISE_OCULUS_SPATIALIZER_DLL "OculusSpatializerWwise.dll"
#endif // WWISE_USE_OCULUS

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	#include <AK/Comm/AkCommunication.h> // Communication between Wwise and the game (excluded in release build)
	#include <AK/Tools/Common/AkMonitorError.h>
	#include <AK/Tools/Common/AkPlatformFuncs.h>
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "AK::AllocHook");
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

	#if defined(CRY_AUDIO_IMPL_WWISE_PROVIDE_SECONDARY_POOL)
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
	#if defined(CRY_AUDIO_IMPL_WWISE_PROVIDE_SECONDARY_POOL)
	size_t const nAllocHandle = CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.AddressToHandle(in_pMemAddress);
	CryAudio::Impl::Wwise::g_audioImplMemoryPoolSecondary.Free(nAllocHandle);
	#else
	CryFatalError("%s", "<Audio>: Called APUFreeHook without secondary pool");
	#endif  // PROVIDE_AUDIO_IMPL_SECONDARY_POOL
}
#endif  // CRY_PLATFORM_DURANGO
}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
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
	CryAudio::Impl::Wwise::CEventInstance const* const pEventInstance = stl::find_in_map(CryAudio::Impl::Wwise::g_playingIds, in_playingID, nullptr);
	CryAudio::Impl::Wwise::CBaseObject const* const pBaseObject = stl::find_in_map(CryAudio::Impl::Wwise::g_gameObjectIds, in_gameObjID, nullptr);
	char const* const szEventName = (pEventInstance != nullptr) ? pEventInstance->GetEvent().GetName() : "Unknown PlayingID";
	char const* const szObjectName = (pBaseObject != nullptr) ? pBaseObject->GetName() : "Unknown GameObjID";
	Cry::Audio::Log(
		((in_eErrorLevel& AK::Monitor::ErrorLevel_Error) != 0) ? CryAudio::ELogType::Error : CryAudio::ELogType::Comment,
		"<Wwise> %s | ErrorCode: %d | PlayingID: %u (%s) | GameObjID: %" PRISIZE_T " (%s)", szTemp, in_eErrorCode, in_playingID, szEventName, in_gameObjID, szObjectName);
}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
SPoolSizes g_debugPoolSizes;
EventInstances g_constructedEventInstances;
uint16 g_objectPoolSize = 0;
uint16 g_eventPoolSize = 0;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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
	pNode->getAttr(g_szParametersAttribute, numParameterEnvironments);
	poolSizes.parameterEnvironments += numParameterEnvironments;

	uint16 numParameterStates = 0;
	pNode->getAttr(g_szParameterStatesAttribute, numParameterStates);
	poolSizes.parameterStates += numParameterStates;

	uint16 numStates = 0;
	pNode->getAttr(g_szStatesAttribute, numStates);
	poolSizes.states += numStates;

	uint16 numSwitches = 0;
	pNode->getAttr(g_szSwitchesAttribute, numSwitches);
	poolSizes.switches += numSwitches;

	uint16 numAuxBuses = 0;
	pNode->getAttr(g_szAuxBusesAttribute, numAuxBuses);
	poolSizes.auxBuses += numAuxBuses;

	uint16 numSoundBanks = 0;
	pNode->getAttr(g_szSoundBanksAttribute, numSoundBanks);
	poolSizes.soundBanks += numSoundBanks;
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
	CState::CreateAllocator(g_poolSizes.states);
	CSwitch::CreateAllocator(g_poolSizes.switches);
	CAuxBus::CreateAllocator(g_poolSizes.auxBuses);
	CSoundBank::CreateAllocator(g_poolSizes.soundBanks);
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
	CState::FreeMemoryPool();
	CSwitch::FreeMemoryPool();
	CAuxBus::FreeMemoryPool();
	CSoundBank::FreeMemoryPool();
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

//////////////////////////////////////////////////////////////////////////
AkUInt32 IntToAkSpeakerSetup(int const value)
{
	AkUInt32 speakerSetup = 0;

	switch (value)
	{
	case 10:
		{
			speakerSetup = AK_SPEAKER_SETUP_MONO;
			break;
		}
	case 11:
		{
			speakerSetup = AK_SPEAKER_SETUP_1POINT1;
			break;
		}
	case 20:
		{
			speakerSetup = AK_SPEAKER_SETUP_STEREO;
			break;
		}
	case 21:
		{
			speakerSetup = AK_SPEAKER_SETUP_2POINT1;
			break;
		}
	case 30:
		{
			speakerSetup = AK_SPEAKER_SETUP_3STEREO;
			break;
		}
	case 31:
		{
			speakerSetup = AK_SPEAKER_SETUP_3POINT1;
			break;
		}
	case 40:
		{
			speakerSetup = AK_SPEAKER_SETUP_4;
			break;
		}
	case 41:
		{
			speakerSetup = AK_SPEAKER_SETUP_4POINT1;
			break;
		}
	case 50:
		{
			speakerSetup = AK_SPEAKER_SETUP_5;
			break;
		}
	case 51:
		{
			speakerSetup = AK_SPEAKER_SETUP_5POINT1;
			break;
		}
	case 60:
		{
			speakerSetup = AK_SPEAKER_SETUP_6;
			break;
		}
	case 61:
		{
			speakerSetup = AK_SPEAKER_SETUP_6POINT1;
			break;
		}
	case 70:
		{
			speakerSetup = AK_SPEAKER_SETUP_7;
			break;
		}
	case 71:
		{
			speakerSetup = AK_SPEAKER_SETUP_7POINT1;
			break;
		}
	default:
		{
			break;
		}
	}

	return speakerSetup;
}

///////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_gameObjectId(2) // Start with id 2, because id 1 would get ignored when setting a parameter on all constructed objects.
	, m_initBankId(AK_INVALID_BANK_ID)
	, m_toBeReleased(false)
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	, m_bCommSystemInitialized(false)
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
#if defined(WWISE_USE_OCULUS)
	, m_pOculusSpatializerLibrary(nullptr)
#endif // WWISE_USE_OCULUS
{
	g_pImpl = this;
	m_regularSoundBankFolder = CRY_AUDIO_DATA_ROOT "/";
	m_regularSoundBankFolder += g_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += g_szAssetsFolderName;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	m_name.Format(
		"%s (Build: %d)",
		CRY_AUDIO_IMPL_WWISE_INFO_STRING,
		AK_WWISESDK_VERSION_BUILD);
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::Update()
{
	if (AK::SoundEngine::IsInitialized())
	{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
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
#endif    // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

		AK::SoundEngine::RenderAudio();
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize)
{
	// If something fails so severely during initialization that we need to fall back to the NULL implementation
	// we will need to shut down what has been initialized so far. Therefore make sure to call Shutdown() before returning eARS_FAILURE!

	if (g_cvars.m_eventPoolSize < 1)
	{
		g_cvars.m_eventPoolSize = 1;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, R"(Event pool size must be at least 1. Forcing the cvar "s_WwiseEventPoolSize" to 1!)");
#endif    // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	g_constructedEventInstances.reserve(static_cast<size_t>(g_cvars.m_eventPoolSize));
	g_objectPoolSize = objectPoolSize;
	g_eventPoolSize = static_cast<uint16>(g_cvars.m_eventPoolSize);
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	AllocateMemoryPools(objectPoolSize, static_cast<uint16>(g_cvars.m_eventPoolSize));

	AkMemSettings memSettings;
	memSettings.uMaxNumPools = 20;
	AKRESULT wwiseResult = AK::MemoryMgr::Init(&memSettings);

	if (wwiseResult != AK_Success)
	{
		ShutDown();

		return ERequestStatus::Failure;
	}

	AkMemPoolId const prepareMemPoolId = AK::MemoryMgr::CreatePool(nullptr, g_cvars.m_prepareEventMemoryPoolSize << 10, 16, AkMalloc, 16);

	if (prepareMemPoolId == AK_INVALID_POOL_ID)
	{
		ShutDown();

		return ERequestStatus::Failure;
	}

	wwiseResult = AK::MemoryMgr::SetPoolName(prepareMemPoolId, "PrepareEventMemoryPool");

	if (wwiseResult != AK_Success)
	{
		ShutDown();

		return ERequestStatus::Failure;
	}

	AkStreamMgrSettings streamSettings;
	AK::StreamMgr::GetDefaultSettings(streamSettings);
	streamSettings.uMemorySize = g_cvars.m_streamManagerMemoryPoolSize << 10; // 64 KiB is the default value!

	if (AK::StreamMgr::Create(streamSettings) == nullptr)
	{
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
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Error, "m_fileIOHandler.Init() returned AKRESULT %d", wwiseResult);
#endif    // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

		ShutDown();

		return ERequestStatus::Failure;
	}

	CryFixedStringT<AK_MAX_PATH> temp = CRY_AUDIO_DATA_ROOT "/";
	temp.append(g_szImplFolderName);
	temp.append("/");
	temp.append(g_szAssetsFolderName);
	temp.append("/");
	AkOSChar const* szTemp = nullptr;
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);
	m_fileIOHandler.SetBankPath(szTemp);

	AkInitSettings initSettings;
	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	initSettings.uDefaultPoolSize = g_cvars.m_soundEngineDefaultMemoryPoolSize << 10;
	initSettings.uCommandQueueSize = g_cvars.m_commandQueueMemoryPoolSize << 10;
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	initSettings.uMonitorPoolSize = g_cvars.m_monitorMemoryPoolSize << 10;
	initSettings.uMonitorQueuePoolSize = g_cvars.m_monitorQueueMemoryPoolSize << 10;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	initSettings.uPrepareEventMemoryPoolID = prepareMemPoolId;
	initSettings.bEnableGameSyncPreparation = false;//TODO: ???
	g_cvars.m_panningRule = crymath::clamp(g_cvars.m_panningRule, 0, 1);
	initSettings.settingsMainOutput.ePanningRule = static_cast<AkPanningRule>(g_cvars.m_panningRule);
	initSettings.settingsMainOutput.channelConfig.SetStandard(IntToAkSpeakerSetup(g_cvars.m_channelConfig));

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
		ShutDown();

		return ERequestStatus::Failure;
	}

	AkMusicSettings musicSettings;
	AK::MusicEngine::GetDefaultInitSettings(musicSettings);

	wwiseResult = AK::MusicEngine::Init(&musicSettings);

	if (wwiseResult != AK_Success)
	{
		ShutDown();

		return ERequestStatus::Failure;
	}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	if (g_cvars.m_enableCommSystem == 1)
	{
		m_bCommSystemInitialized = true;
		AkCommSettings commSettings;
		AK::Comm::GetDefaultInitSettings(commSettings);

		wwiseResult = AK::Comm::Init(commSettings);

		if (wwiseResult != AK_Success)
		{
			m_bCommSystemInitialized = false;
		}

		wwiseResult = AK::Monitor::SetLocalOutput(AK::Monitor::ErrorLevel_All, ErrorMonitorCallback);

		if (wwiseResult != AK_Success)
		{
			AK::Comm::Term();
			m_bCommSystemInitialized = false;
		}
	}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

#if defined(WWISE_USE_OCULUS)
	m_pOculusSpatializerLibrary = CryLoadLibrary(CRY_AUDIO_IMPL_WWISE_OCULUS_SPATIALIZER_DLL);

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
				AK::SoundEngine::RegisterPlugin(AkPluginTypeMixer, AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER, CreateOculusFX, CreateOculusFXParams);
			}
	#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, "Failed call to AkGetSoundEngineCallbacks in " CRY_AUDIO_IMPL_WWISE_OCULUS_SPATIALIZER_DLL);
			}
	#endif    // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

			// Register plugin attachment (for data attachment on individual sounds, like frequency hints etc.)
			if (AkGetSoundEngineCallbacks(AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER_ATTACHMENT, CreateOculusFX, CreateOculusFXParams))
			{
				AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER_ATTACHMENT, nullptr, CreateOculusFXParams);
			}
	#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, "Failed call to AkGetSoundEngineCallbacks in " CRY_AUDIO_IMPL_WWISE_OCULUS_SPATIALIZER_DLL);
			}
	#endif    // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
		}
	#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Failed to load functions AkGetSoundEngineCallbacks in " CRY_AUDIO_IMPL_WWISE_OCULUS_SPATIALIZER_DLL);
		}
	#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
	#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Failed to load " CRY_AUDIO_IMPL_WWISE_OCULUS_SPATIALIZER_DLL);
	}
	#endif // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
#endif   // WWISE_USE_OCULUS

	REINST("Register Global Callback")

	// AK::SoundEngine::RegisterGlobalCallback(GlobalCallback);
	// Load Init.bnk before making the system available to the users
	temp = "Init.bnk";
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);

	wwiseResult = AK::SoundEngine::LoadBank(szTemp, AK_DEFAULT_POOL_ID, m_initBankId);

	if (wwiseResult != AK_Success)
	{
		// This does not qualify for a fallback to the NULL implementation!
		m_initBankId = AK_INVALID_BANK_ID;
	}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	GetInitBankSize();
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	LoadEventsMaxAttenuations(m_regularSoundBankFolder);

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	if (m_bCommSystemInitialized)
	{
		AK::Comm::Term();

		AK::Monitor::SetLocalOutput(0, nullptr);
		m_bCommSystemInitialized = false;
	}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	AK::MusicEngine::Term();

	if (AK::SoundEngine::IsInitialized())
	{
		if (g_cvars.m_enableEventManagerThread > 0)
		{
			AK::SoundEngine::ClearBanks();
		}
		else
		{
			SignalAuxAudioThread();
			AK::SoundEngine::ClearBanks();
			g_auxAudioThread.SignalStopWork();
		}

		REINST("Unregister global callback")
		// AK::SoundEngine::UnregisterGlobalCallback(GlobalCallback);

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
		AK::MemoryMgr::DestroyPool(0);
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

		g_poolSizesLevelSpecific.events = std::max(g_poolSizesLevelSpecific.events, levelPoolSizes.events);
		g_poolSizesLevelSpecific.parameters = std::max(g_poolSizesLevelSpecific.parameters, levelPoolSizes.parameters);
		g_poolSizesLevelSpecific.parameterEnvironments = std::max(g_poolSizesLevelSpecific.parameterEnvironments, levelPoolSizes.parameterEnvironments);
		g_poolSizesLevelSpecific.parameterStates = std::max(g_poolSizesLevelSpecific.parameterStates, levelPoolSizes.parameterStates);
		g_poolSizesLevelSpecific.states = std::max(g_poolSizesLevelSpecific.states, levelPoolSizes.states);
		g_poolSizesLevelSpecific.switches = std::max(g_poolSizesLevelSpecific.switches, levelPoolSizes.switches);
		g_poolSizesLevelSpecific.auxBuses = std::max(g_poolSizesLevelSpecific.auxBuses, levelPoolSizes.auxBuses);
		g_poolSizesLevelSpecific.soundBanks = std::max(g_poolSizesLevelSpecific.soundBanks, levelPoolSizes.soundBanks);
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

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	ZeroStruct(g_debugPoolSizes);
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_poolSizes.events += g_poolSizesLevelSpecific.events;
	g_poolSizes.parameters += g_poolSizesLevelSpecific.parameters;
	g_poolSizes.parameterEnvironments += g_poolSizesLevelSpecific.parameterEnvironments;
	g_poolSizes.parameterStates += g_poolSizesLevelSpecific.parameterStates;
	g_poolSizes.states += g_poolSizesLevelSpecific.states;
	g_poolSizes.switches += g_poolSizesLevelSpecific.switches;
	g_poolSizes.auxBuses += g_poolSizesLevelSpecific.auxBuses;
	g_poolSizes.soundBanks += g_poolSizesLevelSpecific.soundBanks;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	g_poolSizes.events = std::max<uint16>(1, g_poolSizes.events);
	g_poolSizes.parameters = std::max<uint16>(1, g_poolSizes.parameters);
	g_poolSizes.parameterEnvironments = std::max<uint16>(1, g_poolSizes.parameterEnvironments);
	g_poolSizes.parameterStates = std::max<uint16>(1, g_poolSizes.parameterStates);
	g_poolSizes.states = std::max<uint16>(1, g_poolSizes.states);
	g_poolSizes.switches = std::max<uint16>(1, g_poolSizes.switches);
	g_poolSizes.auxBuses = std::max<uint16>(1, g_poolSizes.auxBuses);
	g_poolSizes.soundBanks = std::max<uint16>(1, g_poolSizes.soundBanks);
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
void CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CSoundBank*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			AkBankID bankId = AK_INVALID_BANK_ID;

			AKRESULT const wwiseResult = AK::SoundEngine::LoadBank(
				pFileInfo->pFileData,
				static_cast<AkUInt32>(pFileInfo->size),
				bankId);

			if (CRY_AUDIO_IMPL_WWISE_IS_OK(wwiseResult))
			{
				pFileData->bankId = bankId;
			}
			else
			{
				pFileData->bankId = AK_INVALID_BANK_ID;
			}
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the Wwise implementation of %s", __FUNCTION__);
		}
#endif    // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CSoundBank*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			// If the EventManager thread has been disabled the synchronous UnloadBank version will get stuck.
			if (g_cvars.m_enableEventManagerThread > 0)
			{
				AK::SoundEngine::UnloadBank(pFileData->bankId, pFileInfo->pFileData);
			}
			else
			{
				SignalAuxAudioThread();
				AK::SoundEngine::UnloadBank(pFileData->bankId, pFileInfo->pFileData);
				WaitForAuxAudioThread();
			}
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid AudioFileEntryData passed to the Wwise implementation of %s", __FUNCTION__);
		}
#endif    // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
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
			pFileInfo->memoryBlockAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CSoundBank");
			pFileInfo->pImplData = new CSoundBank();
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
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	implInfo.folderName = g_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	g_globalObjectId = m_gameObjectId++;

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	char const* const szName = "GlobalObject";
	AK::SoundEngine::RegisterGameObj(g_globalObjectId, szName);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CGlobalObject");
	g_pObject = new CGlobalObject(g_globalObjectId, szName);

	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_gameObjectIds[g_globalObjectId] = g_pObject;
	}
#else
	AK::SoundEngine::RegisterGameObj(g_globalObjectId);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CGlobalObject");
	g_pObject = new CGlobalObject(g_globalObjectId);
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	return static_cast<IObject*>(g_pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	AK::SoundEngine::RegisterGameObj(m_gameObjectId, szName);
#else
	AK::SoundEngine::RegisterGameObj(m_gameObjectId);
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CObject");
	auto const pObject = new CObject(m_gameObjectId++, transformation, szName);

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_gameObjectIds[pObject->GetId()] = pObject;
	}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);
	AkGameObjectID const objectID = pBaseObject->GetId();
	AK::SoundEngine::UnregisterGameObj(objectID);

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_gameObjectIds.erase(pBaseObject->GetId());
	}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	delete pBaseObject;

	if (objectID == g_globalObjectId)
	{
		g_pObject = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	IListener* pIListener = nullptr;

	AK::SoundEngine::RegisterGameObj(m_gameObjectId);
	AK::SoundEngine::SetDefaultListeners(&m_gameObjectId, 1);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CListener");
	g_pListener = new CListener(transformation, m_gameObjectId);

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	g_pListener->SetName(szName);
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	pIListener = static_cast<IListener*>(g_pListener);
	g_listenerId = m_gameObjectId++;

	return pIListener;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructListener(IListener* const pIListener)
{
	CRY_ASSERT_MESSAGE(pIListener == g_pListener, "pIListener is not g_pListener during %s", __FUNCTION__);

	AK::SoundEngine::UnregisterGameObj(g_pListener->GetId());

	delete g_pListener;
	g_pListener = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadConnected(DeviceId const deviceUniqueID)
{
	CRY_ASSERT(m_mapInputDevices.find(deviceUniqueID) == m_mapInputDevices.end()); // Mustn't exist yet!
	AkOutputSettings settings("Wwise_Motion", static_cast<AkUniqueID>(deviceUniqueID));
	AkOutputDeviceID deviceID = AK_INVALID_OUTPUT_DEVICE_ID;
	AKRESULT const wwiseResult = AK::SoundEngine::AddOutput(settings, &deviceID, &g_listenerId, 1);

	if (CRY_AUDIO_IMPL_WWISE_IS_OK(wwiseResult))
	{
		m_mapInputDevices[deviceUniqueID] = { true, deviceID };
	}
	else
	{
		m_mapInputDevices[deviceUniqueID] = { false, deviceID };
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
			AK::SoundEngine::RemoveOutput(deviceID);
		}
		m_mapInputDevices.erase(Iter);
	}
}

///////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(XmlNodeRef const pRootNode, float& radius)
{
	ITriggerConnection* pITriggerConnection = nullptr;

	if (_stricmp(pRootNode->getTag(), g_szEventTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		AkUniqueID const uniqueId = AK::SoundEngine::GetIDFromString(szName); // Does not check if the string represents an event!

		if (uniqueId != AK_INVALID_UNIQUE_ID)
		{
			float maxAttenuation = 0.0f;
			auto const attenuationPair = g_maxAttenuations.find(uniqueId);

			if (attenuationPair != g_maxAttenuations.end())
			{
				maxAttenuation = attenuationPair->second;
			}

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CEvent");
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
			pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(uniqueId, maxAttenuation, szName));
			radius = maxAttenuation;
#else
			pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(uniqueId, maxAttenuation));
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Invalid Wwise event name %s", szName);
		}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag: %s", pRootNode->getTag());
	}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	return pITriggerConnection;
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	ITriggerConnection* pITriggerConnection = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		char const* const szName = pTriggerInfo->name.c_str();
		AkUniqueID const uniqueId = AK::SoundEngine::GetIDFromString(szName);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CEvent");
		pITriggerConnection = static_cast<ITriggerConnection*>(new CEvent(uniqueId, 0.0f, szName));
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
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
	IParameterConnection* pIParameterConnection = nullptr;

	if (_stricmp(pRootNode->getTag(), g_szParameterTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		auto const rtpcId = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(szName));

		if (rtpcId != AK_INVALID_RTPC_ID)
		{
			float multiplier = g_defaultParamMultiplier;
			float shift = g_defaultParamShift;

			pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
			pRootNode->getAttr(g_szShiftAttribute, shift);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CParameter");

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
			pIParameterConnection = static_cast<IParameterConnection*>(new CParameter(rtpcId, multiplier, shift, szName));
#else
			pIParameterConnection = static_cast<IParameterConnection*>(new CParameter(rtpcId, multiplier, shift));
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Invalid Wwise parameter name %s", szName);
		}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag %s", pRootNode->getTag());
	}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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

	if (_stricmp(szTag, g_szStateGroupTag) == 0)
	{
		char const* const szStateGroupName = pRootNode->getAttr(g_szNameAttribute);

		if ((szStateGroupName != nullptr) && (szStateGroupName[0] != 0) && (pRootNode->getChildCount() == 1))
		{
			XmlNodeRef const pValueNode(pRootNode->getChild(0));

			if (pValueNode && _stricmp(pValueNode->getTag(), g_szValueTag) == 0)
			{
				char const* const szStateName = pValueNode->getAttr(g_szNameAttribute);

				if ((szStateName != nullptr) && (szStateName[0] != 0))
				{
					AkUInt32 const stateGroupId = AK::SoundEngine::GetIDFromString(szStateGroupName);
					AkUInt32 const stateId = AK::SoundEngine::GetIDFromString(szStateName);

					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CState");

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
					pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CState(stateGroupId, stateId, szStateGroupName, szStateName));
#else
					pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CState(stateGroupId, stateId));
#endif          // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
				}
			}
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"A Wwise StateGroup %s inside SwitchState needs to have exactly one WwiseValue.",
				szStateGroupName);
		}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
	else if (_stricmp(szTag, g_szSwitchGroupTag) == 0)
	{
		char const* const szSwitchGroupName = pRootNode->getAttr(g_szNameAttribute);

		if ((szSwitchGroupName != nullptr) && (szSwitchGroupName[0] != 0) && (pRootNode->getChildCount() == 1))
		{
			XmlNodeRef const pValueNode(pRootNode->getChild(0));

			if (pValueNode && _stricmp(pValueNode->getTag(), g_szValueTag) == 0)
			{
				char const* const szSwitchName = pValueNode->getAttr(g_szNameAttribute);

				if ((szSwitchName != nullptr) && (szSwitchName[0] != 0))
				{
					AkUInt32 const switchGroupId = AK::SoundEngine::GetIDFromString(szSwitchGroupName);
					AkUInt32 const switchId = AK::SoundEngine::GetIDFromString(szSwitchName);

					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CSwitch");

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
					pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CSwitch(switchGroupId, switchId, szSwitchGroupName, szSwitchName));
#else
					pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CSwitch(switchGroupId, switchId));
#endif          // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
				}
			}
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"A Wwise SwitchGroup %s inside SwitchState needs to have exactly one WwiseValue.",
				szSwitchGroupName);
		}
#endif          // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
	else if (_stricmp(szTag, g_szParameterTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		auto const rtpcId = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(szName));

		if (rtpcId != AK_INVALID_RTPC_ID)
		{
			float value = g_defaultStateValue;
			pRootNode->getAttr(g_szValueAttribute, value);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CParameterState");

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
			pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CParameterState(rtpcId, value, szName));
#else
			pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CParameterState(rtpcId, value));
#endif        // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Invalid Wwise parameter name %s", szName);
		}
#endif          // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag: %s", szTag);
	}
#endif          // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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

	if (_stricmp(szTag, g_szAuxBusTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		AkUniqueID const busId = AK::SoundEngine::GetIDFromString(szName);

		if (busId != AK_INVALID_AUX_ID)
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CAuxBus");

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
			pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CAuxBus(static_cast<AkAuxBusID>(busId), szName));
#else
			pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CAuxBus(static_cast<AkAuxBusID>(busId)));
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
		}
		else
		{
			CRY_ASSERT(false); // Unknown AuxBus
		}
	}
	else if (_stricmp(szTag, g_szParameterTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		auto const rtpcId = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(szName));

		if (rtpcId != AK_INVALID_RTPC_ID)
		{
			float multiplier = g_defaultParamMultiplier;
			float shift = g_defaultParamShift;

			pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
			pRootNode->getAttr(g_szShiftAttribute, shift);

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CParameterEnvironment");

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
			pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CParameterEnvironment(rtpcId, multiplier, shift, szName));
#else
			pIEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CParameterEnvironment(rtpcId, multiplier, shift));
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Invalid Wwise parameter name %s", szName);
		}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Wwise tag: %s", szTag);
	}
#endif      // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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
	if (m_initBankId != AK_INVALID_BANK_ID)
	{
		if (g_cvars.m_enableEventManagerThread > 0)
		{
			AK::SoundEngine::UnloadBank(m_initBankId, nullptr);
		}
		else
		{
			SignalAuxAudioThread();
			AK::SoundEngine::UnloadBank(m_initBankId, nullptr);
			WaitForAuxAudioThread();
		}
	}

	CryFixedStringT<MaxFilePathLength> const temp("Init.bnk");
	AkOSChar const* szTemp = nullptr;
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);

	AKRESULT const wwiseResult = AK::SoundEngine::LoadBank(szTemp, AK_DEFAULT_POOL_ID, m_initBankId);

	if (wwiseResult != AK_Success)
	{
		m_initBankId = AK_INVALID_BANK_ID;
	}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	GetInitBankSize();
	g_debugStates.clear();
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

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
		m_localizedSoundBankFolder += CRY_AUDIO_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szAssetsFolderName;

		CryFixedStringT<MaxFilePathLength> temp(m_localizedSoundBankFolder);
		temp += "/";
		AkOSChar const* pTemp = nullptr;
		CONVERT_CHAR_TO_OSCHAR(temp.c_str(), pTemp);
		m_fileIOHandler.SetLanguageFolder(pTemp);
	}
}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
CEventInstance* CImpl::ConstructEventInstance(TriggerInstanceId const triggerInstanceId, CEvent& event, CBaseObject const& baseObject)
{
	event.IncrementNumInstances();

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CEventInstance");
	auto const pEventInstance = new CEventInstance(triggerInstanceId, event, baseObject);
	g_constructedEventInstances.push_back(pEventInstance);

	return pEventInstance;
}
#else
//////////////////////////////////////////////////////////////////////////
CEventInstance* CImpl::ConstructEventInstance(TriggerInstanceId const triggerInstanceId, CEvent& event)
{
	event.IncrementNumInstances();

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Wwise::CEventInstance");
	return new CEventInstance(triggerInstanceId, event);
}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructEventInstance(CEventInstance const* const pEventInstance)
{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
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

	{
		CryAutoLock<CryCriticalSection> const lock(CryAudio::Impl::Wwise::g_cs);
		g_playingIds.erase(pEventInstance->GetPlayingId());
	}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

	CEvent* const pEvent = &pEventInstance->GetEvent();
	delete pEventInstance;

	pEvent->DecrementNumInstances();

	if (pEvent->CanBeDestructed())
	{
		delete pEvent;
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetPanningRule(int const panningRule)
{
	AK::SoundEngine::SetPanningRule(static_cast<AkPanningRule>(panningRule));
}

#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
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

//////////////////////////////////////////////////////////////////////////
void CImpl::GetInitBankSize()
{
	string const initBankPath = m_regularSoundBankFolder + "/Init.bnk";
	m_initBankSize = gEnv->pCryPak->FGetSize(initBankPath.c_str());
}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugMemoryInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
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
		auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextPrimary, false, "Init.bnk: %uKiB",
		                    static_cast<uint32>(m_initBankSize / 1024));

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
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters on Environments", g_poolSizes.parameterEnvironments);
		}

		if (g_debugPoolSizes.parameterStates > 0)
		{
			auto& allocator = CParameterState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters on States", g_poolSizes.parameterStates);
		}

		if (g_debugPoolSizes.states > 0)
		{
			auto& allocator = CState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "States", g_poolSizes.states);
		}

		if (g_debugPoolSizes.switches > 0)
		{
			auto& allocator = CSwitch::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Switches", g_poolSizes.switches);
		}

		if (g_debugPoolSizes.auxBuses > 0)
		{
			auto& allocator = CAuxBus::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Aux Buses", g_poolSizes.auxBuses);
		}

		if (g_debugPoolSizes.soundBanks > 0)
		{
			auto& allocator = CSoundBank::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "SoundBanks", g_poolSizes.soundBanks);
		}
	}

	size_t const numEvents = g_constructedEventInstances.size();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextSecondary, false, "Active Events: %3" PRISIZE_T " | Objects with relative velocity calculation: %u",
	                    numEvents, g_numObjectsWithRelativeVelocity);

	Vec3 const& listenerPosition = g_pListener->GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	float const listenerVelocity = g_pListener->GetVelocity().GetLength();
	char const* const szName = g_pListener->GetName();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorListenerActive, false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f | Velocity: %.2f m/s",
	                    szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z, listenerVelocity);

#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfoList(IRenderAuxGeom& auxGeom, float& posX, float posY, float const debugDistance, char const* const szTextFilter) const
{
#if defined(CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE)
	if ((g_cvars.m_debugListFilter & g_debugListMask) != 0)
	{
		CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(szTextFilter);
		lowerCaseSearchString.MakeLower();

		float const initialPosY = posY;

		if ((g_cvars.m_debugListFilter & EDebugListFilter::EventInstances) != 0)
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Wwise Events [%" PRISIZE_T "]", g_constructedEventInstances.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const pEventInstance : g_constructedEventInstances)
			{
				Vec3 const& position = pEventInstance->GetObject().GetTransformation().GetPosition();
				float const distance = position.GetDistance(g_pListener->GetPosition());

				if ((debugDistance <= 0.0f) || ((debugDistance > 0.0f) && (distance < debugDistance)))
				{
					char const* const szEventName = pEventInstance->GetEvent().GetName();
					CryFixedStringT<MaxControlNameLength> lowerCaseEventName(szEventName);
					lowerCaseEventName.MakeLower();
					bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseEventName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

					if (draw)
					{
						ColorF const& color = (pEventInstance->GetState() == EEventInstanceState::Virtual) ? Debug::s_globalColorVirtual : Debug::s_listColorItemActive;
						auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, color, false, "%s on %s", szEventName, pEventInstance->GetObject().GetName());

						posY += Debug::g_listLineHeight;
					}
				}
			}

			posX += 600.0f;
		}

		if ((g_cvars.m_debugListFilter & EDebugListFilter::States) != 0)
		{
			posY = initialPosY;

			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Wwise States [%" PRISIZE_T "]", g_debugStates.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const& statePair : g_debugStates)
			{
				char const* const szStateGroupName = statePair.first.c_str();
				CryFixedStringT<MaxControlNameLength> lowerCaseStateGroupName(szStateGroupName);
				lowerCaseStateGroupName.MakeLower();

				char const* const szStateName = statePair.second.c_str();
				CryFixedStringT<MaxControlNameLength> lowerCaseStateName(szStateName);
				lowerCaseStateName.MakeLower();

				bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) ||
				                   (lowerCaseStateGroupName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos) ||
				                   (lowerCaseStateName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

				if (draw)
				{
					auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, Debug::s_listColorItemActive, false, "%s : %s", szStateGroupName, szStateName);

					posY += Debug::g_listLineHeight;
				}
			}

			posX += 300.0f;
		}
	}
#endif  // CRY_AUDIO_IMPL_WWISE_USE_PRODUCTION_CODE
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
