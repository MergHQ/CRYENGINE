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

			inline AccountIdentifier CreateAccountIdentifier(const CSteamID& steamId) { return AccountIdentifier(SteamServiceID, steamId.ConvertToUint64()); }
			inline AccountIdentifier CreateAccountIdentifier(uint64 rawSteamId)       { return AccountIdentifier(SteamServiceID, rawSteamId); }
			inline LobbyIdentifier   CreateLobbyIdentifier(const CSteamID& steamId)   { return LobbyIdentifier(SteamServiceID, steamId.ConvertToUint64()); }
			inline LobbyIdentifier   CreateLobbyIdentifier(uint64 rawSteamId)         { return LobbyIdentifier(SteamServiceID, rawSteamId); }
		}
	}
}
