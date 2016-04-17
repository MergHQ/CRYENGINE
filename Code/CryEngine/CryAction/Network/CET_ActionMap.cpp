// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CET_ActionMap.h"
#include "IPlayerProfiles.h"
#include <CryNetwork/NetHelpers.h>

/*
 * disable the action map
 */

class CCET_DisableActionMap : public CCET_Base
{
public:
	const char*                 GetName() { return "DisableActionMap"; }

	EContextEstablishTaskResult OnStep(SContextEstablishState& state)
	{
		IActionMapManager* pActionMapMan = CCryAction::GetCryAction()->GetIActionMapManager();
		pActionMapMan->Enable(false);
		pActionMapMan->EnableActionMap("player", true);
		pActionMapMan->EnableFilter(0, false); // disable all actionfilters
		return eCETR_Ok;
	}
};

void AddDisableActionMap(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_DisableActionMap);
}

/*
 * action map initialization
 */

class CCET_InitActionMap : public CCET_Base
{
public:
	EContextEstablishTaskResult OnStep(SContextEstablishState& state)
	{
		IActionMapManager* pActionMapMan = CCryAction::GetCryAction()->GetIActionMapManager();
		CRY_ASSERT(pActionMapMan);

		IActionMap* pDefaultActionMap = NULL;
		IActionMap* pDebugActionMap = NULL;
		IActionMap* pPlayerActionMap = NULL;
		IActionMap* pPlayerGamemodeActionMap = NULL;

		const char* disableGamemodeActionMapName = "player_mp";
		const char* gamemodeActionMapName = "player_sp";

		if (gEnv->bMultiplayer)
		{
			disableGamemodeActionMapName = "player_sp";
			gamemodeActionMapName = "player_mp";
		}

		if (true)
		{
			IPlayerProfileManager* pPPMgr = CCryAction::GetCryAction()->GetIPlayerProfileManager();
			if (pPPMgr)
			{
				int userCount = pPPMgr->GetUserCount();

				IPlayerProfile* pProfile = NULL;
				const char* userId = "UNKNOWN";
				if (userCount == 0)
				{
					if (gEnv->pSystem->IsDevMode())
					{
#ifndef _RELEASE
						//In devmode and not release get the default user if no users are signed in e.g. autotesting, map on the command line
						pProfile = pPPMgr->GetDefaultProfile();
						if (pProfile)
						{
							userId = pProfile->GetUserId();
						}
#endif      // #ifndef _RELEASE
					}
					else
					{
						CryFatalError("[PlayerProfiles] CGameContext::StartGame: No users logged in");
						return eCETR_Failed;
					}
				}

				if (userCount > 0)
				{
					IPlayerProfileManager::SUserInfo info;
					pPPMgr->GetUserInfo(0, info);
					pProfile = pPPMgr->GetCurrentProfile(info.userId);
					userId = info.userId;
				}
				if (pProfile)
				{
					pDefaultActionMap = pProfile->GetActionMap("default");
					pDebugActionMap = pProfile->GetActionMap("debug");
					pPlayerActionMap = pProfile->GetActionMap("player");

					if (pDefaultActionMap == 0 && pPlayerActionMap == 0)
					{
						CryFatalError("[PlayerProfiles] CGameContext::StartGame: User '%s' has no actionmap 'default'!", userId);
						return eCETR_Failed;
					}
				}
				else
				{
					CryFatalError("[PlayerProfiles] CGameContext::StartGame: User '%s' has no active profile!", userId);
					return eCETR_Failed;
				}
			}
			else
			{
				CryFatalError("[PlayerProfiles] CGameContext::StartGame: No player profile manager!");
				return eCETR_Failed;
			}
		}

		if (pDefaultActionMap == 0)
		{
			// use action map without any profile stuff
			pActionMapMan->EnableActionMap("default", true);
			pDefaultActionMap = pActionMapMan->GetActionMap("default");
			CRY_ASSERT_MESSAGE(pDefaultActionMap, "'default' action map not found!");
		}

		if (pDebugActionMap == 0)
		{
			// use action map without any profile stuff
			pActionMapMan->EnableActionMap("debug", true);
			pDebugActionMap = pActionMapMan->GetActionMap("debug");
		}

		if (pPlayerActionMap == 0)
		{
			pActionMapMan->EnableActionMap("player", true);
			pPlayerActionMap = pActionMapMan->GetActionMap("player");
		}

		if (!pDefaultActionMap)
			return eCETR_Failed;

		EntityId actorId = GetListener();
		if (!actorId)
			return eCETR_Wait;

		pActionMapMan->SetDefaultActionEntity(actorId);
		pActionMapMan->EnableActionMap(disableGamemodeActionMapName, false);
		pActionMapMan->EnableActionMap(gamemodeActionMapName, true);
		pPlayerGamemodeActionMap = pActionMapMan->GetActionMap(gamemodeActionMapName);

		if (pPlayerGamemodeActionMap)
		{
			pPlayerGamemodeActionMap->SetActionListener(actorId);
		}

		CCryAction::GetCryAction()->GetIActionMapManager()->Enable(true);

		return eCETR_Ok;
	}

private:
	virtual EntityId GetListener() = 0;
};

class CCET_InitActionMap_ClientActor : public CCET_InitActionMap
{
private:
	const char* GetName() { return "InitActionMap.ClientActor"; }

	EntityId    GetListener()
	{
		if (IActor* pActor = CCryAction::GetCryAction()->GetClientActor())
			return pActor->GetEntityId();
		return 0;
	}
};

class CCET_InitActionMap_OriginalSpectator : public CCET_InitActionMap
{
private:
	const char* GetName() { return "InitActionMap.OriginalSpectator"; }

	EntityId    GetListener()
	{
		if (IActor* pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetOriginalDemoSpectator())
			return pActor->GetEntityId();
		return 0;
	}
};

void AddInitActionMap_ClientActor(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_InitActionMap_ClientActor());
}

void AddInitActionMap_OriginalSpectator(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_InitActionMap_OriginalSpectator());
}

class CCET_DisableKeyboardMouse : public CCET_Base
{
public:
	const char*                 GetName() { return "DisableKeyboardMouse"; }

	EContextEstablishTaskResult OnStep(SContextEstablishState& state)
	{
		if (ICVar* cvar = gEnv->pConsole->GetCVar("sv_requireinputdevice"))
		{
			if (0 == strcmpi(cvar->GetString(), "gamepad"))
			{
				IInput* pInput = gEnv->pInput;
				pInput->EnableDevice(eIDT_Keyboard, false);
				pInput->EnableDevice(eIDT_Mouse, false);
			}
		}

		return eCETR_Ok;
	}
};

void AddDisableKeyboardMouse(IContextEstablisher* pEst, EContextViewState state)
{
	pEst->AddTask(state, new CCET_DisableKeyboardMouse());
}
