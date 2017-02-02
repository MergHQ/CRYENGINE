// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "AudioImplCVars.h"
#include "FileIOHandler.h"
#include "Common.h"
#include <SharedAudioData.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryThreading/IThreadManager.h>
#include <CryThreading/IThreadConfigManager.h>

#include <AK/SoundEngine/Common/AkSoundEngine.h>     // Sound engine
#include <AK/MotionEngine/Common/AkMotionEngine.h>   // Motion Engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>     // Music Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>       // Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>          // Default memory and stream managers
#include <AK/SoundEngine/Common/AkStreamMgrModule.h> // AkStreamMgrModule
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkCallback.h>

#include <AK/Plugin/AllPluginsRegistrationHelpers.h>

#if defined(WWISE_USE_OCULUS)
	#include <OculusSpatializer.h>
	#include <CryCore/Platform/CryLibrary.h>
	#define OCULUS_SPATIALIZER_DLL "OculusSpatializerWwise.dll"
#endif // WWISE_USE_OCULUS

#if !defined(WWISE_FOR_RELEASE)
	#include <AK/Comm/AkCommunication.h> // Communication between Wwise and the game (excluded in release build)
	#include <AK/Tools/Common/AkMonitorError.h>
	#include <AK/Tools/Common/AkPlatformFuncs.h>
#endif // WWISE_FOR_RELEASE

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::Wwise;

char const* const CAudioImpl::s_szWwiseEventTag = "WwiseEvent";
char const* const CAudioImpl::s_szWwiseRtpcTag = "WwiseRtpc";
char const* const CAudioImpl::s_szWwiseSwitchTag = "WwiseSwitch";
char const* const CAudioImpl::s_szWwiseStateTag = "WwiseState";
char const* const CAudioImpl::s_szWwiseRtpcSwitchTag = "WwiseRtpc";
char const* const CAudioImpl::s_szWwiseFileTag = "WwiseFile";
char const* const CAudioImpl::s_szWwiseAuxBusTag = "WwiseAuxBus";
char const* const CAudioImpl::s_szWwiseValueTag = "WwiseValue";
char const* const CAudioImpl::s_szWwiseNameAttribute = "wwise_name";
char const* const CAudioImpl::s_szWwiseValueAttribute = "wwise_value";
char const* const CAudioImpl::s_szWwiseMutiplierAttribute = "wwise_value_multiplier";
char const* const CAudioImpl::s_szWwiseShiftAttribute = "wwise_value_shift";
char const* const CAudioImpl::s_szWwiseLocalisedAttribute = "wwise_localised";

class CAuxWwiseAudioThread : public IThread
{
public:

	CAuxWwiseAudioThread()
		: m_pImpl(nullptr)
		, m_bQuit(false)
		, m_threadState(eAuxWwiseAudioThreadState_Wait)
	{}

	~CAuxWwiseAudioThread() {}

	enum EAuxWwiseAudioThreadState : int
	{
		eAuxWwiseAudioThreadState_Wait = 0,
		eAuxWwiseAudioThreadState_Start,
		eAuxWwiseAudioThreadState_Stop,
	};

	void Init(CAudioImpl* const pImpl)
	{
		m_pImpl = pImpl;

		if (!gEnv->pThreadManager->SpawnThread(this, "AuxWwiseAudioThread"))
		{
			CryFatalError("Error spawning \"AuxWwiseAudioThread\" thread.");
		}
	}

	void ThreadEntry()
	{
		while (!m_bQuit)
		{
			m_lock.Lock();

			if (m_threadState == eAuxWwiseAudioThreadState_Stop)
			{
				m_threadState = eAuxWwiseAudioThreadState_Wait;
				m_sem.Notify();
			}

			while (m_threadState == eAuxWwiseAudioThreadState_Wait)
			{
				m_sem.Wait(m_lock);
			}

			m_lock.Unlock();

			if (m_bQuit)
			{
				break;
			}

			m_pImpl->Update(0.0f);
		}
	}

	void SignalStopWork()
	{
		m_bQuit = true;
		m_threadState = eAuxWwiseAudioThreadState_Start;
		m_sem.Notify();
		gEnv->pThreadManager->JoinThread(this, eJM_Join);
	}

	bool IsActive()
	{
		// JoinThread returns true if thread is not running.
		// JoinThread returns false if thread is still running
		return !gEnv->pThreadManager->JoinThread(this, eJM_TryJoin);
	}

	CAudioImpl*                        m_pImpl;
	volatile bool                      m_bQuit;
	volatile EAuxWwiseAudioThreadState m_threadState;
	CryMutex                           m_lock;
	CryConditionVariable               m_sem;
};

CAuxWwiseAudioThread g_auxAudioThread;

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
	size_t const nSecondSize = g_audioImplMemoryPoolSecondary.MemSize();

	if (nSecondSize > 0)
	{
		size_t const nAllocHandle = g_audioImplMemoryPoolSecondary.Allocate<size_t>(in_size, in_alignment);

		if (nAllocHandle > 0)
		{
			pAlloc = g_audioImplMemoryPoolSecondary.Resolve<void*>(nAllocHandle);
			g_audioImplMemoryPoolSecondary.Item(nAllocHandle)->Lock();
		}
	}
	#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

	CRY_ASSERT(pAlloc != nullptr);
	return pAlloc;
}

void APUFreeHook(void* in_pMemAddress)
{
	#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
	size_t const nAllocHandle = g_audioImplMemoryPoolSecondary.AddressToHandle(in_pMemAddress);
	//size_t const nOldSize = g_MemoryPoolSoundSecondary.Size(nAllocHandle);
	g_audioImplMemoryPoolSecondary.Free(nAllocHandle);
	#else
	CryFatalError("%s", "<Audio>: Called APUFreeHook without secondary pool");
	#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL
}
#endif // CRY_PLATFORM_DURANGO
}

#if !defined(WWISE_FOR_RELEASE)
static void ErrorMonitorCallback(
  AK::Monitor::ErrorCode in_eErrorCode,   ///< Error code number value
  const AkOSChar* in_pszError,            ///< Message or error string to be displayed
  AK::Monitor::ErrorLevel in_eErrorLevel, ///< Specifies whether it should be displayed as a message or an error
  AkPlayingID in_playingID,               ///< Related Playing ID if applicable, AK_INVALID_PLAYING_ID otherwise
  AkGameObjectID in_gameObjID             ///< Related Game Object ID if applicable, AK_INVALID_GAME_OBJECT otherwise
  )
{
	char* sTemp = nullptr;
	CONVERT_OSCHAR_TO_CHAR(in_pszError, sTemp);
	g_audioImplLogger.Log(
	  ((in_eErrorLevel& AK::Monitor::ErrorLevel_Error) != 0) ? eAudioLogType_Error : eAudioLogType_Comment,
	  "<Wwise> %s ErrorCode: %d PlayingID: %u GameObjID: %" PRISIZE_T, sTemp, in_eErrorCode, in_playingID, in_gameObjID);
}
#endif // WWISE_FOR_RELEASE

///////////////////////////////////////////////////////////////////////////
CAudioImpl::CAudioImpl()
	: m_initBankId(AK_INVALID_BANK_ID)
#if !defined(WWISE_FOR_RELEASE)
	, m_bCommSystemInitialized(false)
#endif // !WWISE_FOR_RELEASE
#if defined(WWISE_USE_OCULUS)
	, m_pOculusSpatializerLibrary(nullptr)
#endif // WWISE_USE_OCULUS
{
	char const* const szAssetDirectory = gEnv->pSystem->GetIProjectManager()->GetCurrentAssetDirectoryRelative();
	if (strlen(szAssetDirectory) == 0)
	{
		CryFatalError("<Audio - Wwise>: Needs a valid asset folder to proceed!");
	}

	m_regularSoundBankFolder = szAssetDirectory;
	m_regularSoundBankFolder += CRY_NATIVE_PATH_SEPSTR WWISE_IMPL_DATA_ROOT;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	m_fullImplString.Format("%s (Build: %d) (%s%s)", WWISE_IMPL_INFO_STRING, AK_WWISESDK_VERSION_BUILD, szAssetDirectory, CRY_NATIVE_PATH_SEPSTR WWISE_IMPL_DATA_ROOT);
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::Update(float const deltaTime)
{
	if (AK::SoundEngine::IsInitialized())
	{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
		AKRESULT wwiseResult = AK_Fail;
		static int enableOutputCapture = 0;

		if (g_audioImplCVars.m_enableOutputCapture == 1 && enableOutputCapture == 0)
		{
			AkOSChar const* pOutputCaptureFileName = nullptr;
			CONVERT_CHAR_TO_OSCHAR("../wwise_audio_capture.wav", pOutputCaptureFileName);
			wwiseResult = AK::SoundEngine::StartOutputCapture(pOutputCaptureFileName);
			AKASSERT((wwiseResult == AK_Success) || !"StartOutputCapture failed!");
			enableOutputCapture = g_audioImplCVars.m_enableOutputCapture;
		}
		else if (g_audioImplCVars.m_enableOutputCapture == 0 && enableOutputCapture == 1)
		{
			wwiseResult = AK::SoundEngine::StopOutputCapture();
			AKASSERT((wwiseResult == AK_Success) || !"StopOutputCapture failed!");
			enableOutputCapture = g_audioImplCVars.m_enableOutputCapture;
		}
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

		AK::SoundEngine::RenderAudio();
	}
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize)
{
	// If something fails so severely during initialization that we need to fall back to the NULL implementation
	// we will need to shut down what has been initialized so far. Therefore make sure to call Shutdown() before returning eARS_FAILURE!

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Wwise Audio Object Pool");
	CAudioObject::CreateAllocator(audioObjectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Wwise Audio Event Pool");
	SAudioEvent::CreateAllocator(eventPoolSize);

	AkMemSettings memSettings;
	memSettings.uMaxNumPools = 20;
	AKRESULT wwiseResult = AK::MemoryMgr::Init(&memSettings);

	if (wwiseResult != AK_Success)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "AK::MemoryMgr::Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return eRequestStatus_Failure;
	}

	AkMemPoolId const prepareMemPoolId = AK::MemoryMgr::CreatePool(nullptr, g_audioImplCVars.m_prepareEventMemoryPoolSize << 10, 16, AkMalloc, 16);

	if (prepareMemPoolId == AK_INVALID_POOL_ID)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "AK::MemoryMgr::CreatePool() PrepareEventMemoryPool failed!\n");
		ShutDown();

		return eRequestStatus_Failure;
	}

	wwiseResult = AK::MemoryMgr::SetPoolName(prepareMemPoolId, "PrepareEventMemoryPool");

	if (wwiseResult != AK_Success)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "AK::MemoryMgr::SetPoolName() could not set name of event prepare memory pool!\n");
		ShutDown();

		return eRequestStatus_Failure;
	}

	AkStreamMgrSettings streamSettings;
	AK::StreamMgr::GetDefaultSettings(streamSettings);
	streamSettings.uMemorySize = g_audioImplCVars.m_streamManagerMemoryPoolSize << 10; // 64 KiB is the default value!
	if (AK::StreamMgr::Create(streamSettings) == nullptr)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "AK::StreamMgr::Create() failed!\n");
		ShutDown();

		return eRequestStatus_Failure;
	}

	IThreadConfigManager* pThreadConfigMngr = gEnv->pThreadManager->GetThreadConfigManager();
	const SThreadConfig* pDeviceThread = pThreadConfigMngr->GetThreadConfig("Wwise_Device");
	const SThreadConfig* pBankManger = pThreadConfigMngr->GetThreadConfig("Wwise_BankManager");
	const SThreadConfig* pLEngineThread = pThreadConfigMngr->GetThreadConfig("Wwise_LEngine");
	const SThreadConfig* pMonitorThread = pThreadConfigMngr->GetThreadConfig("Wwise_Monitor");

	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);
	deviceSettings.uIOMemorySize = g_audioImplCVars.m_streamDeviceMemoryPoolSize << 10; // 2 MiB is the default value!

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
		g_audioImplLogger.Log(eAudioLogType_Error, "m_fileIOHandler.Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return eRequestStatus_Failure;
	}

	CryFixedStringT<AK_MAX_PATH> temp(PathUtil::GetGameFolder().c_str());
	temp.append(CRY_NATIVE_PATH_SEPSTR WWISE_IMPL_DATA_ROOT CRY_NATIVE_PATH_SEPSTR);
	AkOSChar const* szTemp = nullptr;
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);
	m_fileIOHandler.SetBankPath(szTemp);

	AkInitSettings initSettings;
	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	initSettings.uDefaultPoolSize = g_audioImplCVars.m_soundEngineDefaultMemoryPoolSize << 10;
	initSettings.uCommandQueueSize = g_audioImplCVars.m_commandQueueMemoryPoolSize << 10;
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	initSettings.uMonitorPoolSize = g_audioImplCVars.m_monitorMemoryPoolSize << 10;
	initSettings.uMonitorQueuePoolSize = g_audioImplCVars.m_monitorQueueMemoryPoolSize << 10;
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
	initSettings.uPrepareEventMemoryPoolID = prepareMemPoolId;
	initSettings.bEnableGameSyncPreparation = false;//TODO: ???

	initSettings.bUseLEngineThread = g_audioImplCVars.m_enableEventManagerThread > 0;
	initSettings.bUseSoundBankMgrThread = g_audioImplCVars.m_enableSoundBankManagerThread > 0;

	// We need this additional thread during bank unloading if the user decided to run Wwise without the EventManager thread.
	if (g_audioImplCVars.m_enableEventManagerThread == 0)
	{
		g_auxAudioThread.Init(this);
	}

	AkPlatformInitSettings platformInitSettings;
	AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);
	platformInitSettings.uLEngineDefaultPoolSize = g_audioImplCVars.m_lowerEngineDefaultPoolSize << 10;

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
		g_audioImplLogger.Log(eAudioLogType_Error, "AK::SoundEngine::Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return eRequestStatus_Failure;
	}

	AkMusicSettings musicSettings;
	AK::MusicEngine::GetDefaultInitSettings(musicSettings);

	wwiseResult = AK::MusicEngine::Init(&musicSettings);

	if (wwiseResult != AK_Success)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "AK::MusicEngine::Init() returned AKRESULT %d", wwiseResult);
		ShutDown();

		return eRequestStatus_Failure;
	}

#if !defined(WWISE_FOR_RELEASE)
	if (g_audioImplCVars.m_enableCommSystem == 1)
	{
		m_bCommSystemInitialized = true;
		AkCommSettings commSettings;
		AK::Comm::GetDefaultInitSettings(commSettings);

		wwiseResult = AK::Comm::Init(commSettings);

		if (wwiseResult != AK_Success)
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "AK::Comm::Init() returned AKRESULT %d. Communication between the Wwise authoring application and the game will not be possible\n", wwiseResult);
			m_bCommSystemInitialized = false;
		}

		wwiseResult = AK::Monitor::SetLocalOutput(AK::Monitor::ErrorLevel_All, ErrorMonitorCallback);

		if (wwiseResult != AK_Success)
		{
			AK::Comm::Term();
			g_audioImplLogger.Log(eAudioLogType_Error, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", wwiseResult);
			m_bCommSystemInitialized = false;
		}
	}
#endif // !WWISE_FOR_RELEASE

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
					g_audioImplLogger.Log(eAudioLogType_Error, "Failed to register OculusSpatializer plugin.");
				}
			}
			else
			{
				g_audioImplLogger.Log(eAudioLogType_Error, "Failed call to AkGetSoundEngineCallbacks in " OCULUS_SPATIALIZER_DLL);
			}

			// Register plugin attachment (for data attachment on individual sounds, like frequency hints etc.)
			if (AkGetSoundEngineCallbacks(AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER_ATTACHMENT, CreateOculusFX, CreateOculusFXParams))
			{
				wwiseResult = AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKEFFECTID_OCULUS, AKEFFECTID_OCULUS_SPATIALIZER_ATTACHMENT, nullptr, CreateOculusFXParams);

				if (wwiseResult != AK_Success)
				{
					g_audioImplLogger.Log(eAudioLogType_Error, "Failed to register OculusSpatializer attachment.");
				}
			}
			else
			{
				g_audioImplLogger.Log(eAudioLogType_Error, "Failed call to AkGetSoundEngineCallbacks in " OCULUS_SPATIALIZER_DLL);
			}
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Failed to load functions AkGetSoundEngineCallbacks in " OCULUS_SPATIALIZER_DLL);
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Failed to load " OCULUS_SPATIALIZER_DLL);
	}
#endif // WWISE_USE_OCULUS

	REINST("Register Global Callback")

	//wwiseResult = AK::SoundEngine::RegisterGlobalCallback(GlobalCallback);
	//if (wwiseResult != AK_Success)
	//{
	//	g_audioImplLogger.Log(eALT_WARNING, "AK::SoundEngine::RegisterGlobalCallback() returned AKRESULT %d", wwiseResult);
	//}

	// Register the DummyGameObject used for the events that don't need a location in the game world
	wwiseResult = AK::SoundEngine::RegisterGameObj(CAudioObject::s_dummyGameObjectId, "DummyObject");

	if (wwiseResult != AK_Success)
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "AK::SoundEngine::RegisterGameObject() failed for the Dummyobject with AKRESULT %d", wwiseResult);
	}

	// Load Init.bnk before making the system available to the users
	temp = "Init.bnk";
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);

	wwiseResult = AK::SoundEngine::LoadBank(szTemp, AK_DEFAULT_POOL_ID, m_initBankId);

	if (wwiseResult != AK_Success)
	{
		// This does not qualify for a fallback to the NULL implementation!
		// Still notify the user about this failure!
		g_audioImplLogger.Log(eAudioLogType_Error, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", wwiseResult);
		m_initBankId = AK_INVALID_BANK_ID;
	}

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ShutDown()
{
	AKRESULT wwiseResult = AK_Fail;

#if !defined(WWISE_FOR_RELEASE)
	if (m_bCommSystemInitialized)
	{
		AK::Comm::Term();

		wwiseResult = AK::Monitor::SetLocalOutput(0, nullptr);

		if (wwiseResult != AK_Success)
		{
			g_audioImplLogger.Log(eAudioLogType_Warning, "AK::Monitor::SetLocalOutput() returned AKRESULT %d", wwiseResult);
		}

		m_bCommSystemInitialized = false;
	}
#endif // !WWISE_FOR_RELEASE

	AK::MusicEngine::Term();

	if (AK::SoundEngine::IsInitialized())
	{
		// UnRegister the DummyGameObject
		wwiseResult = AK::SoundEngine::UnregisterGameObj(CAudioObject::s_dummyGameObjectId);

		if (wwiseResult != AK_Success)
		{
			g_audioImplLogger.Log(eAudioLogType_Warning, "AK::SoundEngine::UnregisterGameObject() failed for the Dummyobject with AKRESULT %d", wwiseResult);
		}

		if (g_audioImplCVars.m_enableEventManagerThread > 0)
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
			g_audioImplLogger.Log(eAudioLogType_Error, "Failed to clear banks\n");
		}

		REINST("Unregister global callback")
		//wwiseResult = AK::SoundEngine::UnregisterGlobalCallback(GlobalCallback);
		//ASSERT_WWISE_OK(wwiseResult);

		//if (wwiseResult != AK_Success)
		//{
		//	g_audioImplLogger.Log(eALT_WARNING, "AK::SoundEngine::UnregisterGlobalCallback() returned AKRESULT %d", wwiseResult);
		//}

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
#endif // WWISE_USE_OCULUS

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Release()
{
	g_audioImplCVars.UnregisterVariables();

	delete this;

	CAudioObject::FreeMemoryPool();
	SAudioEvent::FreeMemoryPool();

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnLoseFocus()
{
	// With Wwise we drive this via events.
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnGetFocus()
{
	// With Wwise we drive this via events.
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::MuteAll()
{
	// With Wwise we drive this via events.
	// Note: Still, make sure to return eARS_SUCCESS to signal the ATL.
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnmuteAll()
{
	// With Wwise we drive this via events.
	// Note: Still, make sure to return eARS_SUCCESS to signal the ATL.
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::StopAllSounds()
{
	AK::SoundEngine::StopAll();

	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::RegisterInMemoryFile(SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus result = eRequestStatus_Failure;

	if (pFileEntryInfo != nullptr)
	{
		SAudioFileEntry* const pFileDataWwise = static_cast<SAudioFileEntry*>(pFileEntryInfo->pImplData);

		if (pFileDataWwise != nullptr)
		{
			AkBankID bankId = AK_INVALID_BANK_ID;

			AKRESULT const wwiseResult = AK::SoundEngine::LoadBank(
			  pFileEntryInfo->pFileData,
			  static_cast<AkUInt32>(pFileEntryInfo->size),
			  bankId);

			if (IS_WWISE_OK(wwiseResult))
			{
				pFileDataWwise->bankId = bankId;
				result = eRequestStatus_Success;
			}
			else
			{
				pFileDataWwise->bankId = AK_INVALID_BANK_ID;
				g_audioImplLogger.Log(eAudioLogType_Error, "Failed to load file %s\n", pFileEntryInfo->szFileName);
			}
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the Wwise implementation of RegisterInMemoryFile");
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnregisterInMemoryFile(SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus result = eRequestStatus_Failure;

	if (pFileEntryInfo != nullptr)
	{
		SAudioFileEntry* const pFileDataWwise = static_cast<SAudioFileEntry*>(pFileEntryInfo->pImplData);
		if (pFileDataWwise != nullptr)
		{
			AKRESULT wwiseResult = AK_Fail;

			// If the EventManager thread has been disabled the synchronous UnloadBank version will get stuck.
			if (g_audioImplCVars.m_enableEventManagerThread > 0)
			{
				wwiseResult = AK::SoundEngine::UnloadBank(pFileDataWwise->bankId, pFileEntryInfo->pFileData);
			}
			else
			{
				SignalAuxAudioThread();
				wwiseResult = AK::SoundEngine::UnloadBank(pFileDataWwise->bankId, pFileEntryInfo->pFileData);
				WaitForAuxAudioThread();
			}
			if (IS_WWISE_OK(wwiseResult))
			{
				result = eRequestStatus_Success;
			}
			else
			{
				g_audioImplLogger.Log(eAudioLogType_Error, "Wwise Failed to unregister in memory file %s\n", pFileEntryInfo->szFileName);
			}
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioFileEntryData passed to the Wwise implementation of UnregisterInMemoryFile");
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ParseAudioFileEntry(XmlNodeRef const pAudioFileEntryNode, SAudioFileEntryInfo* const pFileEntryInfo)
{
	ERequestStatus result = eRequestStatus_Failure;

	if ((_stricmp(pAudioFileEntryNode->getTag(), s_szWwiseFileTag) == 0) && (pFileEntryInfo != nullptr))
	{
		char const* const szWwiseAudioFileEntryName = pAudioFileEntryNode->getAttr(s_szWwiseNameAttribute);

		if (szWwiseAudioFileEntryName != nullptr && szWwiseAudioFileEntryName[0] != '\0')
		{
			char const* const szWwiseLocalized = pAudioFileEntryNode->getAttr(s_szWwiseLocalisedAttribute);
			pFileEntryInfo->bLocalized = (szWwiseLocalized != nullptr) && (_stricmp(szWwiseLocalized, "true") == 0);
			pFileEntryInfo->szFileName = szWwiseAudioFileEntryName;
			pFileEntryInfo->memoryBlockAlignment = AK_BANK_PLATFORM_DATA_ALIGNMENT;

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

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioFileEntry(IAudioFileEntry* const pIFileEntry)
{
	delete pIFileEntry;
}

//////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetAudioFileLocation(SAudioFileEntryInfo* const pFileEntryInfo)
{
	char const* szResult = nullptr;

	if (pFileEntryInfo != nullptr)
	{
		szResult = pFileEntryInfo->bLocalized ? m_localizedSoundBankFolder.c_str() : m_regularSoundBankFolder.c_str();
	}

	return szResult;
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructGlobalAudioObject()
{
	return new CAudioObject(AK_INVALID_GAME_OBJECT);
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructAudioObject(char const* const szAudioObjectName)
{
	static AkGameObjectID objectIDCounter = 1;
	CRY_ASSERT(objectIDCounter != AK_INVALID_GAME_OBJECT);

	AkGameObjectID const id = objectIDCounter++;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	AKRESULT const wwiseResult = AK::SoundEngine::RegisterGameObj(id, szAudioObjectName);

	if (!IS_WWISE_OK(wwiseResult))
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Wwise RegisterGameObj failed with AKRESULT: %d", wwiseResult);
	}
#else
	AK::SoundEngine::RegisterGameObj(id);
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	return static_cast<IAudioObject*>(new CAudioObject(id));
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioObject(IAudioObject const* const pAudioObject)
{
	CAudioObject const* pWwiseAudioObject = static_cast<CAudioObject const*>(pAudioObject);
	AKRESULT const wwiseResult = AK::SoundEngine::UnregisterGameObj(pWwiseAudioObject->m_id);
	if (!IS_WWISE_OK(wwiseResult))
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Wwise UnregisterGameObj failed with AKRESULT: %d", wwiseResult);
	}

	delete pWwiseAudioObject;
}

///////////////////////////////////////////////////////////////////////////
CryAudio::Impl::IAudioListener* CAudioImpl::ConstructAudioListener()
{
	static AkUniqueID id = 0;
	SAudioListener* pListener = new SAudioListener(id++);
	return pListener;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioListener(CryAudio::Impl::IAudioListener* const pIAudioListener)
{
	delete pIAudioListener;
}

//////////////////////////////////////////////////////////////////////////
IAudioEvent* CAudioImpl::ConstructAudioEvent(CATLEvent& audioEvent)
{
	return new SAudioEvent(audioEvent);
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioEvent(IAudioEvent const* const pAudioEvent)
{
	delete pAudioEvent;
}

//////////////////////////////////////////////////////////////////////////
IAudioStandaloneFile* CAudioImpl::ConstructAudioStandaloneFile(CATLStandaloneFile& atlStandaloneFile, char const* const szFile, bool const bLocalized, IAudioTrigger const* pTrigger /*= nullptr*/)
{
	return new SAudioStandaloneFile();
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioStandaloneFile(IAudioStandaloneFile const* const pIAudioStandaloneFile)
{
	delete pIAudioStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadConnected(TAudioGamepadUniqueID const deviceUniqueID)
{
#if defined(AK_MOTION)
	AudioInputDevices::const_iterator Iter(m_mapInputDevices.find(deviceUniqueID));

	if (Iter == m_mapInputDevices.end())
	{
		AkUInt8 const deviceID = static_cast<AkUInt8>(m_mapInputDevices.size());
		Iter = m_mapInputDevices.insert(std::make_pair(deviceUniqueID, deviceID)).first;
		CRY_ASSERT(m_mapInputDevices.size() < 5); // Wwise does not allow for more than 4 motion devices.
	}

	AkUInt8 const deviceID = Iter->second;
	AKRESULT wwiseResult = AK::MotionEngine::AddPlayerMotionDevice(deviceID, AKCOMPANYID_AUDIOKINETIC, AKMOTIONDEVICEID_RUMBLE);

	if (IS_WWISE_OK(wwiseResult))
	{
		AK::MotionEngine::SetPlayerListener(deviceID, deviceID);
		wwiseResult = AK::SoundEngine::SetListenerPipeline(static_cast<AkUInt32>(deviceID), true, true);

		if (!IS_WWISE_OK(wwiseResult))
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "SetListenerPipeline failed in GamepadConnected! (%u : %u)", deviceUniqueID, deviceID);
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "AddPlayerMotionDevice failed! (%u : %u)", deviceUniqueID, deviceID);
	}
#endif // AK_MOTION
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadDisconnected(TAudioGamepadUniqueID const deviceUniqueID)
{
#if defined(AK_MOTION)
	AudioInputDevices::const_iterator const Iter(m_mapInputDevices.find(deviceUniqueID));

	if (Iter != m_mapInputDevices.end())
	{
		AkUInt8 const deviceID = Iter->second;
		AK::MotionEngine::RemovePlayerMotionDevice(deviceID, AKCOMPANYID_AUDIOKINETIC, AKMOTIONDEVICEID_RUMBLE);
		AKRESULT const wwiseResult = AK::SoundEngine::SetListenerPipeline(static_cast<AkUInt32>(deviceID), true, false);

		if (!IS_WWISE_OK(wwiseResult))
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "SetListenerPipeline failed in GamepadDisconnected! (%u : %u)", deviceUniqueID, deviceID);
		}
	}
	else
	{
		CRY_ASSERT(m_mapInputDevices.empty());
	}
#endif // AK_MOTION
}

///////////////////////////////////////////////////////////////////////////
IAudioTrigger const* CAudioImpl::NewAudioTrigger(XmlNodeRef const pAudioTriggerNode)
{
	IAudioTrigger* pIAudioTrigger = nullptr;

	if (_stricmp(pAudioTriggerNode->getTag(), s_szWwiseEventTag) == 0)
	{
		char const* const szWwiseEventName = pAudioTriggerNode->getAttr(s_szWwiseNameAttribute);
		AkUniqueID const uniqueId = AK::SoundEngine::GetIDFromString(szWwiseEventName);//Does not check if the string represents an event!!!!

		if (uniqueId != AK_INVALID_UNIQUE_ID)
		{
			pIAudioTrigger = new SAudioTrigger(uniqueId);
		}
		else
		{
			CRY_ASSERT(false);
		}
	}
	return pIAudioTrigger;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioTrigger(IAudioTrigger const* const pIAudioTrigger)
{
	delete pIAudioTrigger;
}

///////////////////////////////////////////////////////////////////////////
IParameter const* CAudioImpl::NewAudioParameter(XmlNodeRef const pAudioRtpcNode)
{
	IParameter* pAudioRtpc = nullptr;

	AkRtpcID rtpcId = AK_INVALID_RTPC_ID;
	float multiplier = 1.0f;
	float shift = 0.0f;

	ParseRtpcImpl(pAudioRtpcNode, rtpcId, multiplier, shift);

	if (rtpcId != AK_INVALID_RTPC_ID)
	{
		pAudioRtpc = new SAudioRtpc(rtpcId, multiplier, shift);
	}

	return pAudioRtpc;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioParameter(IParameter const* const pIParameter)
{
	delete pIParameter;
}

///////////////////////////////////////////////////////////////////////////
IAudioSwitchState const* CAudioImpl::NewAudioSwitchState(XmlNodeRef const pAudioSwitchNode)
{
	char const* const szTag = pAudioSwitchNode->getTag();
	IAudioSwitchState const* pAudioSwitchState = nullptr;

	if (_stricmp(szTag, s_szWwiseSwitchTag) == 0)
	{
		pAudioSwitchState = ParseWwiseSwitchOrState(pAudioSwitchNode, eWwiseSwitchType_Switch);
	}
	else if (_stricmp(szTag, s_szWwiseStateTag) == 0)
	{
		pAudioSwitchState = ParseWwiseSwitchOrState(pAudioSwitchNode, eWwiseSwitchType_State);
	}
	else if (_stricmp(szTag, s_szWwiseRtpcSwitchTag) == 0)
	{
		pAudioSwitchState = ParseWwiseRtpcSwitch(pAudioSwitchNode);
	}
	else
	{
		// Unknown Wwise switch tag!
		g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Wwise switch tag! (%s)", szTag);
	}

	return pAudioSwitchState;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioSwitchState(IAudioSwitchState const* const pISwitchState)
{
	delete pISwitchState;
}

///////////////////////////////////////////////////////////////////////////
IAudioEnvironment const* CAudioImpl::NewAudioEnvironment(XmlNodeRef const pAudioEnvironmentNode)
{
	IAudioEnvironment* pAudioEnvironment = nullptr;
	if (_stricmp(pAudioEnvironmentNode->getTag(), s_szWwiseAuxBusTag) == 0)
	{
		char const* const szWwiseAuxBusName = pAudioEnvironmentNode->getAttr(s_szWwiseNameAttribute);
		AkUniqueID const busId = AK::SoundEngine::GetIDFromString(szWwiseAuxBusName);//Does not check if the string represents an event!!!!

		if (busId != AK_INVALID_AUX_ID)
		{
			pAudioEnvironment = new SAudioEnvironment(eWwiseAudioEnvironmentType_AuxBus, static_cast<AkAuxBusID>(busId));
		}
		else
		{
			CRY_ASSERT(false);// unknown Aux Bus
		}
	}
	else if (_stricmp(pAudioEnvironmentNode->getTag(), s_szWwiseRtpcTag) == 0)
	{
		AkRtpcID rtpcId = AK_INVALID_RTPC_ID;
		float multiplier = 1.0f;
		float shift = 0.0f;

		ParseRtpcImpl(pAudioEnvironmentNode, rtpcId, multiplier, shift);

		if (rtpcId != AK_INVALID_RTPC_ID)
		{
			pAudioEnvironment = new SAudioEnvironment(eWwiseAudioEnvironmentType_Rtpc, rtpcId, multiplier, shift);
		}
		else
		{
			CRY_ASSERT(false); // Unknown RTPC
		}
	}

	return pAudioEnvironment;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioEnvironment(IAudioEnvironment const* const pIEnvironment)
{
	delete pIEnvironment;
}

///////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetImplementationNameString() const
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	return m_fullImplString.c_str();
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const
{
	CryModuleMemoryInfo memInfo;
	ZeroStruct(memInfo);
	CryGetMemoryInfoForModule(&memInfo);

	memoryInfo.totalMemory = static_cast<size_t>(memInfo.allocated - memInfo.freed);

#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
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
		auto& allocator = SAudioEvent::GetAllocator();
		auto mem = allocator.GetTotalMemory();
		auto pool = allocator.GetCounts();
		memoryInfo.poolUsedObjects += pool.nUsed;
		memoryInfo.poolConstructedObjects += pool.nAlloc;
		memoryInfo.poolUsedMemory += mem.nUsed;
		memoryInfo.poolAllocatedMemory += mem.nAlloc;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const
{}

//////////////////////////////////////////////////////////////////////////
SAudioSwitchState const* CAudioImpl::ParseWwiseSwitchOrState(
  XmlNodeRef const pNode,
  EWwiseSwitchType const switchType)
{
	SAudioSwitchState* pSwitchStateImpl = nullptr;

	char const* const szWwiseSwitchNodeName = pNode->getAttr(s_szWwiseNameAttribute);

	if ((szWwiseSwitchNodeName != nullptr) && (szWwiseSwitchNodeName[0] != 0) && (pNode->getChildCount() == 1))
	{
		XmlNodeRef const pValueNode(pNode->getChild(0));

		if (pValueNode && _stricmp(pValueNode->getTag(), s_szWwiseValueTag) == 0)
		{
			char const* const szWwiseSwitchStateName = pValueNode->getAttr(s_szWwiseNameAttribute);

			if ((szWwiseSwitchStateName != nullptr) && (szWwiseSwitchStateName[0] != 0))
			{
				AkUniqueID const switchId = AK::SoundEngine::GetIDFromString(szWwiseSwitchNodeName);
				AkUniqueID const switchStateId = AK::SoundEngine::GetIDFromString(szWwiseSwitchStateName);
				pSwitchStateImpl = new SAudioSwitchState(switchType, switchId, switchStateId);
			}
		}
	}
	else
	{
		g_audioImplLogger.Log(
		  eAudioLogType_Warning,
		  "A Wwise Switch or State %s inside ATLSwitchState needs to have exactly one WwiseValue.",
		  szWwiseSwitchNodeName);
	}

	return pSwitchStateImpl;
}

///////////////////////////////////////////////////////////////////////////
SAudioSwitchState const* CAudioImpl::ParseWwiseRtpcSwitch(XmlNodeRef const pNode)
{
	SAudioSwitchState* pSwitchStateImpl = nullptr;

	char const* const szWwiseRtpcNodeName = pNode->getAttr(s_szWwiseNameAttribute);

	if ((szWwiseRtpcNodeName != nullptr) && (szWwiseRtpcNodeName[0] != '\0'))
	{
		float rtpcValue = 0.0f;

		if (pNode->getAttr(s_szWwiseValueAttribute, rtpcValue))
		{
			AkUniqueID const rtpcId = AK::SoundEngine::GetIDFromString(szWwiseRtpcNodeName);
			pSwitchStateImpl = new SAudioSwitchState(eWwiseSwitchType_Rtpc, rtpcId, rtpcId, rtpcValue);
		}
	}
	else
	{
		g_audioImplLogger.Log(
		  eAudioLogType_Warning,
		  "The Wwise Rtpc %s inside ATLSwitchState does not have a valid name.",
		  szWwiseRtpcNodeName);
	}

	return pSwitchStateImpl;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::ParseRtpcImpl(XmlNodeRef const pNode, AkRtpcID& rtpcId, float& multiplier, float& shift)
{
	if (_stricmp(pNode->getTag(), s_szWwiseRtpcTag) == 0)
	{
		char const* const szRtpcName = pNode->getAttr(s_szWwiseNameAttribute);
		rtpcId = static_cast<AkRtpcID>(AK::SoundEngine::GetIDFromString(szRtpcName));

		if (rtpcId != AK_INVALID_RTPC_ID)
		{
			//the Wwise name is supplied
			pNode->getAttr(s_szWwiseMutiplierAttribute, multiplier);
			pNode->getAttr(s_szWwiseShiftAttribute, shift);
		}
		else
		{
			// Invalid Wwise RTPC name!
			g_audioImplLogger.Log(eAudioLogType_Warning, "Invalid Wwise RTPC name %s", szRtpcName);
		}
	}
	else
	{
		// Unknown Wwise RTPC tag!
		g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown Wwise RTPC tag %s", pNode->getTag());

	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::SignalAuxAudioThread()
{
	g_auxAudioThread.m_lock.Lock();
	g_auxAudioThread.m_threadState = CAuxWwiseAudioThread::eAuxWwiseAudioThreadState_Start;
	g_auxAudioThread.m_lock.Unlock();
	g_auxAudioThread.m_sem.Notify();
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::WaitForAuxAudioThread()
{
	g_auxAudioThread.m_lock.Lock();
	g_auxAudioThread.m_threadState = CAuxWwiseAudioThread::eAuxWwiseAudioThreadState_Stop;

	// Wait until the AuxWwiseAudioThread is actually waiting again and not processing any work.
	while (g_auxAudioThread.m_threadState != CAuxWwiseAudioThread::eAuxWwiseAudioThreadState_Wait)
	{
		g_auxAudioThread.m_sem.Wait(g_auxAudioThread.m_lock);
	}

	g_auxAudioThread.m_lock.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::OnAudioSystemRefresh()
{
	AKRESULT wwiseResult = AK_Fail;

	if (m_initBankId != AK_INVALID_BANK_ID)
	{
		if (g_audioImplCVars.m_enableEventManagerThread > 0)
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
			g_audioImplLogger.Log(eAudioLogType_Error, "Wwise failed to unload Init.bnk, returned the AKRESULT: %d", wwiseResult);
			CRY_ASSERT(false);
		}
	}

	CryFixedStringT<MaxFilePathLength> const temp("Init.bnk");
	AkOSChar const* szTemp = nullptr;
	CONVERT_CHAR_TO_OSCHAR(temp.c_str(), szTemp);

	wwiseResult = AK::SoundEngine::LoadBank(szTemp, AK_DEFAULT_POOL_ID, m_initBankId);
	if (wwiseResult != AK_Success)
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Wwise failed to load Init.bnk, returned the AKRESULT: %d", wwiseResult);
		m_initBankId = AK_INVALID_BANK_ID;
		CRY_ASSERT(false);
	}
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
		m_localizedSoundBankFolder += WWISE_IMPL_DATA_ROOT;

		CryFixedStringT<MaxFilePathLength> temp(m_localizedSoundBankFolder);
		temp += CRY_NATIVE_PATH_SEPSTR;
		AkOSChar const* pTemp = nullptr;
		CONVERT_CHAR_TO_OSCHAR(temp.c_str(), pTemp);
		m_fileIOHandler.SetLanguageFolder(pTemp);
	}
}
