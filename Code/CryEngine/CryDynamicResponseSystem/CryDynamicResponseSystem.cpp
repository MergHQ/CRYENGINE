// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryMemory/BucketAllocatorImpl.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

#include "ResponseSystem.h"
#include "VariableCollection.h"

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryDynamicResponseSystem : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryDynamicResponseSystem, "EngineModule_CryDynamicResponseSystem", 0xd1ed34dda44c4c17, 0x959a46df79af5db3)

	virtual ~CEngineModule_CryDynamicResponseSystem()
	{
		SAFE_DELETE(gEnv->pDynamicResponseSystem);
	}

	virtual const char* GetName() const override { return "CryDynamicResponseSystem"; }
	virtual const char* GetCategory() const override { return "CryEngine"; }

	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		CryDRS::CResponseSystem* pResponseSystem = new CryDRS::CResponseSystem();
		env.pDynamicResponseSystem = pResponseSystem;

		pResponseSystem->CreateVariableCollection(CryDRS::CVariableCollection::s_globalCollectionName);
		pResponseSystem->CreateVariableCollection(CryDRS::CVariableCollection::s_localCollectionName);
		pResponseSystem->CreateVariableCollection(CryDRS::CVariableCollection::s_contextCollectionName);

		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryDynamicResponseSystem)

#include <CryCore/CrtDebugStats.h>
