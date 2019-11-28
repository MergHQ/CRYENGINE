// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#if CRY_PLATFORM_DURANGO
#include "XboxLiveGameEvents.h"

#include <rpc.h>
#include <CryCore/Platform/IPlatformOS.h>
#include <ILevelSystem.h>

#include "GameRules.h"
#include "PlaylistManager.h"

#include <CryCore/Platform/CryWindows.h>
#include <Objbase.h>

namespace XboxLiveGameEvents
{
	const GUID InvalidGUID = {0};
	extern const wchar_t* InvalideCorrelationId = L"";

	EventParamId::EventParamId()
	{
		m_eventParamId = 0;//CNamesTable::InvalidId;

#if XBOXLIVE_GAME_EVENT_TRACKING
		m_debugName = "InvalidId";
#endif
	}

	EventParamId::EventParamId( const char* name )
	{
		Set(name);
	}

	bool EventParamId::Valid() const
	{
		return m_eventParamId != 0;//NamesTable::InvalidId;
	}

	void EventParamId::Set( const char* name )
	{
		m_eventParamId = 0;//CNamesTable::Get().GetIdForName( name );		

#if XBOXLIVE_GAME_EVENT_TRACKING
		m_debugName = name;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	void Init()
	{
#if XBOXLIVE_GAME_EVENTS_ENABLED
		EventRegisterCGBH_06BFF3BF();
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	void Shutdown()
	{
#if XBOXLIVE_GAME_EVENTS_ENABLED
		EventUnregisterCGBH_06BFF3BF();
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	bool CreateGUID(GUID& newGUID)
	{
		HRESULT hr = CoCreateGuid(&newGUID);
		if (SUCCEEDED(hr))
		{
			return true;
		}

		// init anyway to avoid messing database
		newGUID = InvalidGUID;

		CRY_ASSERT(SUCCEEDED(hr), "CreateGUID failed");

		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	void GetUserXUID(SUserXUID & userXUID, unsigned int userIdx /* = 0 */)
	{
		CRY_PROFILE_FUNCTION(PROFILE_GAME);

#if XBOXLIVE_GAME_EVENTS_ENABLED
		gEnv->pSystem->GetPlatformOS()->UserGetXUID(userIdx, userXUID);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	bool GetXDPLevelId( XboxLiveGameEvents::EventParamId& outParam )
	{
		CRY_PROFILE_FUNCTION(PROFILE_GAME);

		ILevelSystem* pLevelSystem = gEnv->pGameFramework->GetILevelSystem();
		if(pLevelSystem == NULL)
			return false;

		ILevelInfo* pInfo = pLevelSystem->GetCurrentLevel();

		if(pInfo == NULL)
			return false;

		CryFixedStringT<64> levelName;
		levelName.Format("Levels.%s", pInfo->GetName());

		outParam = XboxLiveGameEvents::EventParamId(levelName.c_str());

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	bool GetXDPGameModeId( XboxLiveGameEvents::EventParamId& outParam )
	{
		CRY_PROFILE_FUNCTION(PROFILE_GAME);

		if ( g_pGame )
		{
			if ( CGameRules* pGameRules = g_pGame->GetGameRules() )
			{
				if ( IEntity* pEntity = pGameRules->GetEntity() )
				{
					CryFixedStringT<64> gameModeName;
					gameModeName.Format("GameModes.%s", pEntity->GetClass()->GetName());

					outParam = XboxLiveGameEvents::EventParamId(gameModeName.c_str());
					return true;
				}
			}
		}

		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	void GetXDPAttackContextId( const char* szAttackContext, XboxLiveGameEvents::EventParamId& outParam )
	{
		CRY_PROFILE_FUNCTION(PROFILE_GAME);

		CryFixedStringT<64> attackContextName;
		attackContextName.Format("AttackContext.%s", szAttackContext ? szAttackContext : "");

		outParam = XboxLiveGameEvents::EventParamId(attackContextName.c_str());
	}

	//////////////////////////////////////////////////////////////////////////
	SMultiplayerParamIds::SMultiplayerParamIds()
		: MPCorrelationId(NULL)
		, pPlaylistId(NULL)
		, pMatchGuid(NULL)
		, pRoundGuid(NULL)
	{
	}

	//////////////////////////////////////////////////////////////////////////
	void QueryMultiplayerParamIds(SMultiplayerParamIds& params, uint32 filter /*= eMPIT_ALL*/, EntityId entityId /*= 0*/)
	{
		CRY_PROFILE_FUNCTION(PROFILE_GAME);

		CryFixedStringT<64> qualifiedName;

		if (entityId == 0)
		{
			entityId = g_pGame->GetIGameFramework()->GetClientEntityId();
		}

		//const CPlaylistManager* pPlaylistManager = g_pGame->GetPlaylistManager();
		//CRY_ASSERT(pPlaylistManager, "CPlaylistManager hasn't been created");

		if (filter & eMPIT_MP_CORRELATION_ID)
		{
			params.MPCorrelationId = InvalideCorrelationId;
		}
		/*
		if (filter & eMPIT_PLAYLIST_ID)
		{
			params.pPlaylistId = pPlaylistManager ? &pPlaylistManager->GetSelectedPlaylistId().guid : &CPlaylistManager::INVALID_PLAYLIST_ID;
		}
		if (filter & eMPIT_MATCH_GUID)
		{
			params.pMatchGuid = pPlaylistManager ? pPlaylistManager->GetPlaylistSessionId() : &InvalidGUID;
		}
		if (filter & eMPIT_ROUND_GUID)
		{
			params.pRoundGuid = pPlaylistManager ? pPlaylistManager->GetRoundSessionId() : &InvalidGUID;
		}
		if (filter & eMPIT_GOD_ID)
		{
			const Progression::CGladiatorRoster* pGladiatorRoster = g_pGame->GetProgressionServices().GetGladiatorRoster();
			const char* chosenGodName = pGladiatorRoster->GetChosenGodName(entityId);
			qualifiedName.Format("Gods.%s", chosenGodName ? chosenGodName : "");

			params.godId.Set(qualifiedName);
		}
		if (filter & eMPIT_TILESET_ID)
		{
			const char* tilesetName = pPlaylistManager ? pPlaylistManager->GetCurrentTilesetName() : NULL;
			qualifiedName.Format("Tilesets.%s", tilesetName ? tilesetName : "");

			params.tilesetId.Set(qualifiedName);
		}
		*/
	}
}

#endif