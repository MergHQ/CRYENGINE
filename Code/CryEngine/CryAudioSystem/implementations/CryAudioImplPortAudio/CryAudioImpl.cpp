// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "AudioImplCVars.h"
#include <CryAudio/IAudioSystem.h>
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ClassWeaver.h>

using namespace CryAudio;
using namespace CryAudio::Impl::PortAudio;

// Define global objects.
CAudioLogger g_audioImplLogger;
CAudioImplCVars CryAudio::Impl::PortAudio::g_audioImplCVars;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioImplPortAudio : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule);
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioImplPortAudio, "EngineModule_AudioImpl", 0xef1271ff25c24461, 0x6cea190c93444190);

	CEngineModule_CryAudioImplPortAudio();
	virtual ~CEngineModule_CryAudioImplPortAudio() {}

	//////////////////////////////////////////////////////////////////////////
	virtual char const* GetName()     { return "CryAudioImplPortAudio"; }
	virtual char const* GetCategory() { return "CryAudio"; }

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
	{
		gEnv->pAudioSystem->AddRequestListener(&CEngineModule_CryAudioImplPortAudio::OnAudioEvent, nullptr, eSystemEvent_ImplSet);
		SRequestUserData const data(eRequestFlags_PriorityHigh | eRequestFlags_ExecuteBlocking | eRequestFlags_SyncCallback);
		gEnv->pAudioSystem->SetImpl(new CAudioImpl, data);
		gEnv->pAudioSystem->RemoveRequestListener(&CEngineModule_CryAudioImplPortAudio::OnAudioEvent, nullptr);

		if (m_bSuccess)
		{
			g_audioImplLogger.Log(eAudioLogType_Always, "CryAudioImplPortAudio loaded");
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "CryAudioImplPortAudio failed to load");
		}

		return m_bSuccess;
	}

	//////////////////////////////////////////////////////////////////////////
	static void OnAudioEvent(SRequestInfo const* const pAudioRequestInfo)
	{
		m_bSuccess = pAudioRequestInfo->requestResult == eRequestResult_Success;
	}

	static bool m_bSuccess;
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioImplPortAudio)
bool CEngineModule_CryAudioImplPortAudio::m_bSuccess = false;

CEngineModule_CryAudioImplPortAudio::CEngineModule_CryAudioImplPortAudio()
{
	g_audioImplCVars.RegisterVariables();
}

#include <CryCore/CrtDebugStats.h>
