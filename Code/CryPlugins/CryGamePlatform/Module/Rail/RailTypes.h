// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformTypes.h"
#include <rail/sdk/rail_api.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			class CMatchmaking;
			class CNetworking;
			class CUserLobby;
			class CAccount;
			class CService;

			using AccountIdentifierValue     = Cry::GamePlatform::Detail::NumericIdentifierValue;
			using LobbyIdentifierValue       = Cry::GamePlatform::Detail::NumericIdentifierValue;
			using ApplicationIdentifierValue = Cry::GamePlatform::Detail::NumericIdentifierValue;

			inline AccountIdentifier     CreateAccountIdentifier(AccountIdentifierValue rawRailId)         { return AccountIdentifier(RailServiceID, rawRailId); }
			inline LobbyIdentifier       CreateLobbyIdentifier(LobbyIdentifierValue rawRailId)             { return LobbyIdentifier(RailServiceID, rawRailId); }
			inline ApplicationIdentifier CreateApplicationIdentifier(ApplicationIdentifierValue rawRailId) { return ApplicationIdentifier(RailServiceID, rawRailId); }

			inline AccountIdentifier     CreateAccountIdentifier(const rail::RailID& railId)               { return CreateAccountIdentifier(railId.get_id()); }
			inline ApplicationIdentifier CreateApplicationIdentifier(const rail::RailGameID& railId)       { return CreateApplicationIdentifier(railId.get_id()); }

			inline rail::RailID ExtractRailID(const AccountIdentifier& accountId)
			{
				if (accountId.Service() == RailServiceID)
				{
					AccountIdentifierValue tmpVal;
					if (accountId.GetAsUint64(tmpVal))
					{
						return tmpVal;
					}
				}

				CRY_ASSERT(false, "[Rail] AccountIdentifier '%s' does not seem to contain a valid Rail ID", accountId.ToDebugString());
				return rail::kInvalidRailId;
			}

			inline LobbyIdentifierValue ExtractRoomID(const LobbyIdentifier& lobbyId)
			{
				if (lobbyId.Service() == RailServiceID)
				{
					LobbyIdentifierValue tmpVal;
					if (lobbyId.GetAsUint64(tmpVal))
					{
						return tmpVal;
					}
				}

				CRY_ASSERT(false, "[Rail] LobbyIdentifier '%s' does not seem to contain a valid Rail ID", lobbyId.ToDebugString());
				return 0;
			}

			inline rail::RailGameID ExtractGameID(const ApplicationIdentifier& appId)
			{
				if (appId.Service() == RailServiceID)
				{
					ApplicationIdentifierValue tmpVal;
					if (appId.GetAsUint64(tmpVal))
					{
						return tmpVal;
					}
				}

				CRY_ASSERT(false, "[Rail] ApplicationIdentifier '%s' does not seem to contain a valid Rail ID", appId.ToDebugString());
				return 0;
			}
		}
	}
}
