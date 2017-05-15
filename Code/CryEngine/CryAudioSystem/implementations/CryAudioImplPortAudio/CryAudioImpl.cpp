// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"
#include "AudioImplCVars.h"
#include <CryAudio/IAudioSystem.h>
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ClassWeaver.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
// Define global objects.
CLogger g_implLogger;
CCVars g_cvars;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryAudioImplPortAudio : public IImplModule
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(Cry::IDefaultModule)
	CRYINTERFACE_ADD(IImplModule)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryAudioImplPortAudio, "EngineModule_AudioImpl", 0xaa6a039a0ce5bbab, 0x33e0aad69f3136f4);

	CEngineModule_CryAudioImplPortAudio();

	//////////////////////////////////////////////////////////////////////////
	virtual char const* GetName()  const override    { return "CryAudioImplPortAudio"; }
	virtual char const* GetCategory() const override { return "CryAudio"; }

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		gEnv->pAudioSystem->AddRequestListener(&CEngineModule_CryAudioImplPortAudio::OnEvent, nullptr, ESystemEvents::ImplSet);
		SRequestUserData const data(ERequestFlags::ExecuteBlocking | ERequestFlags::CallbackOnExternalOrCallingThread);
		gEnv->pAudioSystem->SetImpl(new CImpl, data);
		gEnv->pAudioSystem->RemoveRequestListener(&CEngineModule_CryAudioImplPortAudio::OnEvent, nullptr);

		if (m_bSuccess)
		{
			g_implLogger.Log(ELogType::Always, "CryAudioImplPortAudio loaded");
		}
		else
		{
			g_implLogger.Log(ELogType::Error, "CryAudioImplPortAudio failed to load");
		}

		return m_bSuccess;
	}

	//////////////////////////////////////////////////////////////////////////
	static void OnEvent(SRequestInfo const* const pRequestInfo)
	{
		m_bSuccess = pRequestInfo->requestResult == ERequestResult::Success;
	}

	static bool m_bSuccess;
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryAudioImplPortAudio)
bool CEngineModule_CryAudioImplPortAudio::m_bSuccess = false;

CEngineModule_CryAudioImplPortAudio::CEngineModule_CryAudioImplPortAudio()
{
	g_cvars.RegisterVariables();
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
#include <CryCore/CrtDebugStats.h>
