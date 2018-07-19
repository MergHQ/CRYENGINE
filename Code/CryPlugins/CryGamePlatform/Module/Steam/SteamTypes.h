// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformTypes.h"
#include <steam/steam_api.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			class CMatchmaking;
			class CNetworking;
			class CServer;
			class CUserLobby;
			class CAccount;
			class CAchievement;
			class CLeaderboards;
			class CStatistics;
			class CUser;
			class CRemoteFile;
			class CRemoteStorage;
			class CSharedRemoteFile;
			class CUserGeneratedContent;
			class CUserGeneratedContentManager;
			class CService;

			using AccountIdentifierValue = Cry::GamePlatform::Detail::NumericIdentifierValue;
			
			inline AccountIdentifier CreateAccountIdentifier(AccountIdentifierValue rawSteamId) { return AccountIdentifier(SteamServiceID, rawSteamId); }
			inline LobbyIdentifier   CreateLobbyIdentifier(AccountIdentifierValue rawSteamId)   { return LobbyIdentifier(SteamServiceID, rawSteamId); }

			inline AccountIdentifier CreateAccountIdentifier(const CSteamID& steamId)           { return CreateAccountIdentifier(steamId.ConvertToUint64()); }
			inline LobbyIdentifier   CreateLobbyIdentifier(const CSteamID& steamId)             { return CreateLobbyIdentifier(steamId.ConvertToUint64()); }
		}
	}
}
