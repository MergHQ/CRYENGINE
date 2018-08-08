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
		env.pUDR = this;
		return true;
	}
	// ~Cry::IDefaultModule

	// Cry::IUDR
	virtual Cry::UDR::IHub& GetHub() override
	{
		return m_cHub;
	}
	// ~Cry::IUDR

private:
	Cry::UDR::CHub m_cHub;
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryUDR)

#include <CryCore/CrtDebugStats.h>
