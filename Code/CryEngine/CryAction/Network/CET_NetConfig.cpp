// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CET_NetConfig.h"
#include "GameContext.h"
#include "GameClientChannel.h"
#include "GameClientNub.h"
#include "GameServerChannel.h"
#include "GameServerNub.h"
#include <CryNetwork/NetHelpers.h>



//! Signals the begin of context state establishment
class CCET_StartedEstablishingContext final : public CCET_Base
{
public:
	CCET_StartedEstablishingContext(const int token) : m_token(token) {}

	// IContextEstablishTask
	virtual const char*                 GetName() override { return "StartedEstablishingContext"; }

	virtual EContextEstablishTaskResult OnStep(SContextEstablishState& /*state*/) override
	{
		if (CGameContext* pGameContext = CCryAction::GetCryAction()->GetGameContext())
		{
			INetContext* pNetContext = pGameContext->GetNetContext();
			CRY_ASSERT(pNetContext, "Task should never be updated without NetContext");
			pNetContext->StartedEstablishingContext(m_token);
		}
		return eCETR_Ok;
	}
	// ~IContextEstablishTask

private:
	const int m_token;
};

void AddStartedEstablishingContext(IContextEstablisher* pEst, EContextViewState state, int token)
{
	pEst->AddTask(state, new CCET_StartedEstablishingContext(token));
}


//! Signals that context is established
class CCET_EstablishedContext : public CCET_Base
{
public:
	CCET_EstablishedContext(const int token) : m_token(token) {}

	// IContextEstablishTask
	virtual const char*                 GetName() override { return "EstablishedContext"; }

	virtual EContextEstablishTaskResult OnStep(SContextEstablishState& /*state*/) override
	{
		if (CGameContext* pGameContext = CCryAction::GetCryAction()->GetGameContext())
		{
			INetContext* pNetContext = pGameContext->GetNetContext();
			CRY_ASSERT(pNetContext, "Task should never be updated without NetContext");
			pNetContext->EstablishedContext(m_token);
		}
		return eCETR_Ok;
	}
	// ~IContextEstablishTask

private:
	const int m_token;
};

void AddEstablishedContext(IContextEstablisher* pEst, EContextViewState state, int token)
{
	pEst->AddTask(state, new CCET_EstablishedContext(token));
}


//! Declare witness
class CCET_DeclareWitness : public CCET_Base
{
public:
	// IContextEstablishTask
	virtual const char*                 GetName() override { return "DeclareWitness"; }

	virtual EContextEstablishTaskResult OnStep(SContextEstablishState& /*state*/) override
	{
		if (CGameClientNub* pNub = CCryAction::GetCryAction()->GetGameClientNub())
		{
			if (CGameClientChannel* pChannel = pNub->GetGameClientChannel())
			{
				if (IActor* pActor = CCryAction::GetCryAction()->GetClientActor())
				{
					pChannel->GetNetChannel()->DeclareWitness(pActor->GetEntityId());
					return eCETR_Ok;
				}
			}
		}
		return eCETR_Failed;
	}
	// ~IContextEstablishTask
};

void AddDeclareWitness(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_DeclareWitness);
}


//! Delegate authority to player
class CCET_DelegateAuthority_ToClientActor : public CCET_Base
{
public:
	// IContextEstablishTask
	virtual const char* GetName() override { return "DelegateAuthorityToClientActor"; }

	virtual EContextEstablishTaskResult OnStep(SContextEstablishState& state) override
	{
		EntityId entityId = GetEntity(state);
		if (!entityId || !gEnv->pEntitySystem->GetEntity(entityId))
			return eCETR_Ok; // Proceed even if there is no Actor
		if (CGameContext* pGameContext = CCryAction::GetCryAction()->GetGameContext())
		{
			INetContext* pNetContext = pGameContext->GetNetContext();
			CRY_ASSERT(pNetContext, "Task should never be updated without NetContext");
			pNetContext->DelegateAuthority(entityId, state.pSender);
		}
		return eCETR_Ok;
	}
	// ~IContextEstablishTask

private:
	EntityId GetEntity(SContextEstablishState& state)
	{
		if (!state.pSender)
			return 0;
		CGameChannel* pGameChannel = static_cast<CGameChannel*>(state.pSender->GetGameChannel());
		if (pGameChannel)
			return pGameChannel->GetPlayerId();
		return 0;
	}
};

void AddDelegateAuthorityToClientActor(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_DelegateAuthority_ToClientActor());
}


//! Clear player ids
class CCET_ClearPlayerIds : public CCET_Base
{
public:
	// IContextEstablishTask
	virtual const char*                 GetName() override { return "ClearPlayerIds"; }

	virtual EContextEstablishTaskResult OnStep(SContextEstablishState& state) override
	{
		if (CGameServerNub* pNub = CCryAction::GetCryAction()->GetGameServerNub())
		{
			TServerChannelMap* pMap = pNub->GetServerChannelMap();
			for (TServerChannelMap::iterator it = pMap->begin(); it != pMap->end(); ++it)
				it->second->ResetPlayerId();
		}
		if (CGameClientNub* pCNub = CCryAction::GetCryAction()->GetGameClientNub())
		{
			if (CGameChannel* pChannel = pCNub->GetGameClientChannel())
				pChannel->ResetPlayerId();
		}
		return eCETR_Ok;
	}
	// ~IContextEstablishTask
};

void AddClearPlayerIds(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_ClearPlayerIds());
}
