// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EntitySystemsComponent.h"

#include <Schematyc/Entity/EntityClasses.h>
#include <Schematyc/Entity/EntityUtils.h>

#include "AutoRegister.h"
#include "STDModules.h"

//#include <CryFlowGraph/IFlowSystem.h>
#include <CryGame/IGameTokens.h>
#include <CryGame/IGameFramework.h>

namespace Schematyc
{
	CEntitySystemsComponent::CEntitySystemsComponent()
	{
	}

	bool CEntitySystemsComponent::Init()
	{		
		IGameTokenSystem* pGTS = gEnv->pGameFramework->GetIGameTokenSystem();
		if (pGTS)
		{
			pGTS->RegisterListener(this);
		}

		return true;
	}

	void CEntitySystemsComponent::Run(ESimulationMode simulationMode)
	{
	}

	void CEntitySystemsComponent::Shutdown()
	{
		IGameTokenSystem* pGTS = gEnv->pGameFramework->GetIGameTokenSystem();
		if (pGTS)
		{
			pGTS->UnregisterListener(this);
		}
	}

	void CEntitySystemsComponent::ReflectType(CTypeDesc<CEntitySystemsComponent>& desc)
	{
		desc.SetGUID("0e44b095-66bc-49a5-aeeb-184de76cc2c2"_schematyc_guid);
		desc.SetLabel("CrySystems");
		desc.SetDescription("Entity systems component");
		desc.SetIcon("icons:schematyc/element_class.ico");
	}

	void CEntitySystemsComponent::Register(IEnvRegistrar& registrar)
	{
		CEnvRegistrationScope scope = registrar.Scope(g_entityClassGUID);
		{
			CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEntitySystemsComponent));
			// Functions
			{
				auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntitySystemsComponent::SetGametoken, "5003114b-ae6b-4654-9dc3-c2ea9b0b756b"_schematyc_guid, "SetGametoken");
				pFunction->SetDescription("Sets a Gametoken to the specific (string) value");
				pFunction->SetFlags({ EEnvFunctionFlags::Member, EEnvFunctionFlags::Const });
				pFunction->BindInput(1, 'tokn', "TokenName");
				pFunction->BindInput(2, 'valu', "TokenValue");
				pFunction->BindInput(3, 'crea', "CreateIfNotExisting");
				componentScope.Register(pFunction);
			}
			// Signals
			{
				componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(SGametokenChangedSignal));
			}
		}
	}

	void CEntitySystemsComponent::SetGametoken(CSharedString tokenName, CSharedString valueToSet, bool bCreateIfNotExisting)
	{
		IGameTokenSystem* pTokenSystem = gEnv->pGameFramework->GetIGameTokenSystem();
		CRY_ASSERT(pTokenSystem);
		if (bCreateIfNotExisting)
		{
			pTokenSystem->SetOrCreateToken(tokenName.c_str(), valueToSet.c_str());
		}
		else
		{
			IGameToken* pToken = pTokenSystem->FindToken(tokenName.c_str());
			if (pToken)
			{
				pToken->SetValueAsString(valueToSet.c_str());
			}
			else
			{
				//CryWarning()
			}

		}
		return;
	}

	void CEntitySystemsComponent::OnGameTokenEvent(EGameTokenEvent event, IGameToken* pGameToken)
	{
		//todo: filtering based on gameTokenFilter property
		if (event == EGAMETOKEN_EVENT_CHANGE)
		{
			OutputSignal(SGametokenChangedSignal(pGameToken->GetName(), pGameToken->GetValueAsString()));
		}
	}

	void CEntitySystemsComponent::SGametokenChangedSignal::ReflectType(CTypeDesc<SGametokenChangedSignal>& desc)
	{		
		desc.SetGUID("81b00d58-7c1a-4cfd-bb61-776e1d5e964b"_schematyc_guid);
		desc.SetLabel("GametokenChangedSignal");
		desc.AddMember(&SGametokenChangedSignal::m_tokenname, 'name', "tokenName", "TokenName", "Name of the gametoken that has changed");
		desc.AddMember(&SGametokenChangedSignal::m_value, 'val', "value", "Value", "new value of the changed gametoken");
	}

} // Schematyc

SCHEMATYC_AUTO_REGISTER(&Schematyc::CEntitySystemsComponent::Register)
