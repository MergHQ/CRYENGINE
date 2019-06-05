// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>
#include <CryUDR/InterfaceIncludes.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryUDR : public Cry::UDR::IUDR
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(Cry::IDefaultModule)
	CRYINTERFACE_ADD(Cry::UDR::IUDR)
	CRYINTERFACE_END()

	CRYGENERATE_SINGLETONCLASS_GUID(CEngineModule_CryUDR, "EngineModule_CryUDR", "52eb3317-1abe-412a-9173-5d8d8277c3e4"_cry_guid)

	virtual ~CEngineModule_CryUDR() override
	{
		gEnv->pUDR = nullptr;
	}

	// Cry::IDefaultModule
	virtual const char* GetName() const override     { return "CryUDR"; }
	virtual const char* GetCategory() const override { return "CryEngine"; }
	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		if (env.pTimer)
		{
			env.pUDR = this;
			m_baseTimestamp = env.pTimer->GetAsyncTime();
			return true;
		}
		CRY_ASSERT_MESSAGE(env.pTimer, "Global Env Timer must be initialized before UDR.");
		return false;
	}
	// ~Cry::IDefaultModule

	// Cry::IUDR
	virtual bool SetConfig(const SConfig& config) override
	{
		m_config = config;
		return true;
	}

	virtual const SConfig& GetConfig() const override
	{
		return m_config;
	}

	// TODO: This function may be called from Sandbox Editor to manually clear UDR data. In that case, the config should be ignored. Maybe we need a different function or force parameter?
	virtual void Reset() override
	{
		if (m_config.resetDataOnSwitchToGameMode)
		{
			m_baseTimestamp = gEnv->pTimer->GetAsyncTime();
			m_cHub.ClearTree(Cry::UDR::ITreeManager::ETreeIndex::Live);
		}
	}

	virtual CTimeValue GetElapsedTime() const override
	{
		return gEnv->pTimer->GetAsyncTime() - m_baseTimestamp;
	}

	virtual Cry::UDR::IHub& GetHub() override
	{
		return m_cHub;
	}
	// ~Cry::IUDR

private:

	Cry::UDR::CHub          m_cHub;
	CTimeValue              m_baseTimestamp;
	// TODO: Add a frame counter that gets updated in the Update function.
	Cry::UDR::IUDR::SConfig m_config;
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryUDR)

#include <CryCore/CrtDebugStats.h>
