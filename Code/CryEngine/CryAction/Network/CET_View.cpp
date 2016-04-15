// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CET_View.h"
#include <CryNetwork/NetHelpers.h>
#include "ViewSystem/ViewSystem.h"

class CCET_InitView : public CCET_Base
{
public:
	EContextEstablishTaskResult OnStep(SContextEstablishState& state)
	{
		IGameObject* pGO = GetGameObject();
		if (!pGO)
		{
			GameWarning("Nothing to link view to");
			return eCETR_Failed;
		}

		CViewSystem* pVS = (CViewSystem*) CCryAction::GetCryAction()->GetIViewSystem();
		IView* pView = pVS->CreateView();
		pView->LinkTo(pGO);
		pVS->SetActiveView(pView);
		pVS->Update(0.0f);
		return eCETR_Ok;
	}

private:
	virtual IGameObject* GetGameObject() = 0;
};

class CCET_InitView_ClientActor : public CCET_InitView
{
private:
	const char*  GetName() { return "InitView.ClientActor"; }

	IGameObject* GetGameObject()
	{
		IActor* pActor = CCryAction::GetCryAction()->GetClientActor();
		if (!pActor)
			return 0;
		return pActor->GetGameObject();
	}
};

class CCET_InitView_CurrentSpectator : public CCET_InitView
{
private:
	const char*  GetName() { return "InitView.CurrentSpectator"; }

	IGameObject* GetGameObject()
	{
		IActorSystem* pActorSys = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
		IActor* pActor = pActorSys->GetCurrentDemoSpectator();
		if (!pActor)
			return 0;
		return pActor->GetGameObject();
	}
};

void AddInitView_ClientActor(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_InitView_ClientActor);
}

void AddInitView_CurrentSpectator(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_InitView_CurrentSpectator);
}

class CCET_ClearViews : public CCET_Base
{
public:
	EContextEstablishTaskResult OnStep(SContextEstablishState& state)
	{
		CViewSystem* pViewSystem = static_cast<CViewSystem*>(CCryAction::GetCryAction()->GetIViewSystem());
		pViewSystem->ClearAllViews();

		return eCETR_Ok;
	}

private:
	const char* GetName() { return "InitView.ClearViews"; }
};

void AddClearViews(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_ClearViews);
}
