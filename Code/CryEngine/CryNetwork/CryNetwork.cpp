// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

//
//	File: crynetwork.cpp
//  Description: dll entry point
//
//	History:
//	-July 25,2001:Created by Alberto Demichelis
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Network.h"
// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>

#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

//////////////////////////////////////////////////////////////////////////

CRYNETWORK_API INetwork* CreateNetwork(ISystem* pSystem, int ncpu)
{
	ModuleInitISystem(pSystem, "CryNetwork");

	CNetwork* pNetwork = new CNetwork;

	if (!pNetwork->Init(ncpu))
	{
		pNetwork->Release();
		return NULL;
	}
	return pNetwork;
}

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryNetwork : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryNetwork, "EngineModule_CryNetwork", 0x7dc5c3b8bb374063, 0xa29ac2d6dd718e0f)

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName() override { return "CryNetwork"; };
	virtual const char* GetCategory()  override { return "CryEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		ISystem* pSystem = env.pSystem;

		CNetwork* pNetwork = new CNetwork;

		int ncpu = env.pi.numCoresAvailableToProcess;
		if (!pNetwork->Init(ncpu))
		{
			pNetwork->Release();
			return false;
		}
		env.pNetwork = pNetwork;

		CryLogAlways("[Network Version]: "
#if defined(_RELEASE)
		             "RELEASE "
#elif defined(_PROFILE)
		             "PROFILE "
#else
		             "DEBUG "
#endif

#if defined(PURE_CLIENT)
		             "PURE CLIENT"
#elif (DEDICATED_SERVER)
		             "DEDICATED SERVER"
#else
		             "DEVELOPMENT BUILD"
#endif
		             );

#if defined(DEDICATED_SERVER) && defined(PURE_CLIENT)
		CryFatalError("[Network]: Invalid build configuration - aborting");
#endif

		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryNetwork)

CEngineModule_CryNetwork::CEngineModule_CryNetwork()
{

};

CEngineModule_CryNetwork::~CEngineModule_CryNetwork()
{
};

#include <CryCore/CrtDebugStats.h>
