// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "AudioImplCVars.h"
#include <CryAudio/IAudioSystem.h>
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ClassWeaver.h>

using namespace CryAudio;
using namespace CryAudio::Impl::SDL_mixer;

// Define global objects.
CLogger g_implLogger;
CAudioImplCVars CryAudio::Impl::SDL_mixer::g_audioImplCVars;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioImplSDLMixer : public CryAudio::IImplModule
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(Cry::IDefaultModule)
		CRYINTERFACE_ADD(CryAudio::IImplModule)
	CRYINTERFACE_END()
	
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioImplSDLMixer, "EngineModule_AudioImpl", 0x8030c0d1905b4031, 0xa3785a8b53125f3f)

	CEngineModule_CryAudioImplSDLMixer();
	virtual ~CEngineModule_CryAudioImplSDLMixer() {}

	//////////////////////////////////////////////////////////////////////////
	virtual char const* GetName() const override     { return "CryAudioImplSDLMixer"; }
	virtual char const* GetCategory() const override { return "CryAudio"; }

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		gEnv->pAudioSystem->AddRequestListener(&CEngineModule_CryAudioImplSDLMixer::OnAudioEvent, nullptr, ESystemEvents::ImplSet);
		SRequestUserData const data(ERequestFlags::ExecuteBlocking | ERequestFlags::CallbackOnExternalOrCallingThread);
		gEnv->pAudioSystem->SetImpl(new CAudioImpl, data);
		gEnv->pAudioSystem->RemoveRequestListener(&CEngineModule_CryAudioImplSDLMixer::OnAudioEvent, nullptr);

		if (m_bSuccess)
		{
			g_implLogger.Log(ELogType::Always, "CryAudioImplSDLMixer loaded");
		}
		else
		{
			g_implLogger.Log(ELogType::Error, "CryAudioImplSDLMixer failed to load");
		}

		return m_bSuccess;
	}

	//////////////////////////////////////////////////////////////////////////
	static void OnAudioEvent(SRequestInfo const* const pAudioRequestInfo)
	{
		m_bSuccess = pAudioRequestInfo->requestResult == ERequestResult::Success;
	}

	static bool m_bSuccess;
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioImplSDLMixer)
bool CEngineModule_CryAudioImplSDLMixer::m_bSuccess = false;

CEngineModule_CryAudioImplSDLMixer::CEngineModule_CryAudioImplSDLMixer()
{
	g_audioImplCVars.RegisterVariables();
}

#include <CryCore/CrtDebugStats.h>
