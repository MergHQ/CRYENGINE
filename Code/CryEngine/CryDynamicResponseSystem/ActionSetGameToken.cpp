// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ActionSetGameToken.h"
#include "ResponseInstance.h"
#include "ResponseSystem.h"

#include <CryGame/IGameTokens.h>
#include <CryFlowGraph/IFlowSystem.h>
#include <CrySystem/ISystem.h>
#include <CryGame/IGame.h>
#include <CryGame/IGameFramework.h>

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
void CActionSetGameToken::Serialize(Serialization::IArchive& ar)
{
	ar(m_tokenName, "tokenname", "^TokenName");
	ar(m_valueToSet, "stringValue", "^ Value");
	ar(m_bCreateTokenIfNotExisting, "create", "^ Create");

	if (ar.isEdit() && !m_bCreateTokenIfNotExisting)
	{
		IGameTokenSystem* pTokenSystem = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();
		if (!pTokenSystem->FindToken(m_tokenName.c_str()))
		{
			ar.warning(m_tokenName, "Token not existing");
		}
	}
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionSetGameToken::Execute(DRS::IResponseInstance* pResponseInstance)
{
	IGameTokenSystem* pTokenSystem = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();
	if (m_bCreateTokenIfNotExisting)
	{
		pTokenSystem->SetOrCreateToken(m_tokenName.c_str(), TFlowInputData(m_valueToSet, true));
	}
	else
	{
		IGameToken* pToken = pTokenSystem->FindToken(m_tokenName.c_str());
		if (pToken)
		{
			pToken->SetValueAsString(m_valueToSet.c_str());
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Could not find game token with name '%s'", m_tokenName.c_str());
		}

	}
	return nullptr;
}

REGISTER_DRS_ACTION(CActionSetGameToken, "SetGameToken", DEFAULT_DRS_ACTION_COLOR);
