// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "AudioImplCVars.h"
#include <CryAudio/IAudioSystem.h>
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ClassWeaver.h>

#if CRY_PLATFORM_DURANGO
	#include <apu.h>
	#include <shapexmacontext.h>
#endif // CRY_PLATFORM_DURANGO

using namespace CryAudio;
using namespace CryAudio::Impl::Wwise;

// Define global objects.
CAudioLogger g_audioImplLogger;
CAudioImplCVars CryAudio::Impl::Wwise::g_audioImplCVars;

#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
MemoryPoolReferenced g_audioImplMemoryPoolSecondary;
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioImplWwise : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioImplWwise, "EngineModule_AudioImpl", 0xb4971e5dd02442c5, 0xb34a9ac0b4abfffd)

	CEngineModule_CryAudioImplWwise();
	virtual ~CEngineModule_CryAudioImplWwise() {}

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() const override     { return "CryAudioImplWwise"; }
	virtual const char* GetCategory() const override { return "CryAudio"; }

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
#if defined(PROVIDE_WWISE_IMPL_SECONDARY_POOL)
		size_t secondarySize = 0;
		void* pSecondaryMemory = nullptr;

	#if CRY_PLATFORM_DURANGO
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Wwise Implementation Audio Pool Secondary");
		secondarySize = g_audioImplCVars.m_secondaryMemoryPoolSize << 10;

		APU_ADDRESS temp;
		HRESULT const result = ApuAlloc(&pSecondaryMemory, &temp, secondarySize, SHAPE_XMA_INPUT_BUFFER_ALIGNMENT);
		CRY_ASSERT(result == S_OK);
	#endif // CRY_PLATFORM_DURANGO

		g_audioImplMemoryPoolSecondary.InitMem(secondarySize, (uint8*)pSecondaryMemory);
#endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL

		gEnv->pAudioSystem->AddRequestListener(&CEngineModule_CryAudioImplWwise::OnAudioEvent, nullptr, eSystemEvent_ImplSet);
		SRequestUserData const data(eRequestFlags_ExecuteBlocking | eRequestFlags_SyncCallback);
		gEnv->pAudioSystem->SetImpl(new CAudioImpl, data);
		gEnv->pAudioSystem->RemoveRequestListener(&CEngineModule_CryAudioImplWwise::OnAudioEvent, nullptr);

		if (m_bSuccess)
		{
			g_audioImplLogger.Log(eAudioLogType_Always, "CryAudioImplWwise loaded");
		}
		else
		{
			g_audioImplLogger.Log(eAudioLogType_Error, "CryAudioImplWwise failed to load");
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

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioImplWwise)
bool CEngineModule_CryAudioImplWwise::m_bSuccess = false;

CEngineModule_CryAudioImplWwise::CEngineModule_CryAudioImplWwise()
{
	g_audioImplCVars.RegisterVariables();
}

#include <CryCore/CrtDebugStats.h>
