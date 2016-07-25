// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGameFramework.h>
#include <CryFlowGraph/IFlowBaseNode.h>

template<ENodeCloneType CLONE_TYPE>
class CFlowGameSDKBaseNode : public CFlowBaseNode<CLONE_TYPE>
{
protected:
	bool InputEntityIsLocalPlayer(const IFlowNode::SActivationInfo* const pActInfo) const
	{
		bool bRet = true;

		if (pActInfo->pEntity)
		{
			IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pActInfo->pEntity->GetId());
			if (pActor != gEnv->pGame->GetIGameFramework()->GetClientActor())
			{
				bRet = false;
			}
		}
		else if (gEnv->bMultiplayer)
		{
			bRet = false;
		}

		return bRet;
	}

	//-------------------------------------------------------------
	// returns the actor associated with the input entity. In single player, it returns the local player if that actor does not exists.
	IActor* GetInputActor(const IFlowNode::SActivationInfo* const pActInfo) const
	{
		IActor* pActor = pActInfo->pEntity ? gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pActInfo->pEntity->GetId()) : nullptr;
		if (!pActor && !gEnv->bMultiplayer)
		{
			pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
		}

		return pActor;
	}
};
