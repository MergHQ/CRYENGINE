// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CryMovie.h"
#include "Movie.h"
#include <CryCore/CrtDebugStats.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ClassWeaver.h>

#undef GetClassName

class CEngineModule_CryMovie : public IEngineModule
{
	CRYINTERFACE_SIMPLE(IEngineModule)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_CryMovie, "EngineModule_CryMovie", 0xdce26beebdc6400f, 0xa0e9b42839f2dd5b)

	virtual const char* GetName() override { return "CryMovie"; };
	virtual const char* GetCategory() override { return "CryEngine"; };

	virtual bool        Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override
	{
		env.pMovieSystem = new CMovieSystem(env.pSystem);
		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryMovie)

CEngineModule_CryMovie::CEngineModule_CryMovie()
{

};

CEngineModule_CryMovie::~CEngineModule_CryMovie()
{
};
