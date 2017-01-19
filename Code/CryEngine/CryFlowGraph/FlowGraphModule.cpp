// CryDB.cpp : Defines the exported functions for the DLL application.
//

#include "StdAfx.h"

// Included only once here
#include <CryCore//Platform/platform_impl.inl>
#include <CrySystem/IEngineModule.h>

#include "FlowSystem/FlowSystem.h"
#include "FlowSystem/Modules/ModuleManager.h"
#include "GameTokens/GameTokenSystem.h"



// IEngineModule -----------------------------------

class CEngineModule_FlowGraph : public ICryPlugin
{
	CRYINTERFACE_SIMPLE(ICryPlugin)
	CRYGENERATE_SINGLETONCLASS(CEngineModule_FlowGraph, "EngineModule_FlowGraph", 0x8D22D250CBF24DBA, 0xADCCA656C06752D7)

	CEngineModule_FlowGraph();
	~CEngineModule_FlowGraph();

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() const override { return "CryFlowGraph"; };
	virtual const char *GetCategory() const override { return "CryEngine"; };

	virtual void OnPluginUpdate(EPluginUpdateType updateType) override {};

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize( SSystemGlobalEnvironment &env,const SSystemInitParams &initParams ) override
	{
		ISystem* pSystem = env.pSystem;
		ModuleInitISystem(pSystem,"CryFlowGraph");

		CFlowSystem* pFlowSystem = new CFlowSystem();
		env.pFlowSystem = pFlowSystem;
		pFlowSystem->PreInit();
		pFlowSystem->Init();

		CryRegisterFlowNodes();
		return true;
	}
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_FlowGraph)

CEngineModule_FlowGraph::CEngineModule_FlowGraph()
{
}

CEngineModule_FlowGraph::~CEngineModule_FlowGraph()
{
	if (gEnv->pFlowSystem)
	{
		gEnv->pFlowSystem->Release();
		gEnv->pFlowSystem = nullptr;
	}
}

