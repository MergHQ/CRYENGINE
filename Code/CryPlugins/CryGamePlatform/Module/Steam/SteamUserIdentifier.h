// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformUserIdentifier.h"
#include "SteamTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			inline CSteamID ExtractSteamID(const AccountIdentifier& accountId)
			{
				if (accountId.Service() == SteamServiceID)
				{
					AccountIdentifierValue tmpVal;
					if (accountId.GetAsUint64(tmpVal))
					{
						return tmpVal;
					}
				}

				CRY_ASSERT(false, "[Steam] AccountIdentifier '%s' does not seem to contain a valid Steam ID", accountId.ToDebugString());
				return k_steamIDNil;
			}

			inline CSteamID ExtractSteamID(const LobbyIdentifier& lobbyId)
			{
				if (lobbyId.Service() == SteamServiceID)
				{
					LobbyIdentifierValue tmpVal;
					if (lobbyId.GetAsUint64(tmpVal))
					{
						return tmpVal;
					}
				}

				CRY_ASSERT(false, "[Steam] LobbyIdentifier '%s' does not seem to contain a valid Steam ID", lobbyId.ToDebugString());
				return k_steamIDNil;
			}

			inline AppId_t ExtractSteamID(const ApplicationIdentifier& appId)
			{
				if (appId.Service() == SteamServiceID)
				{
					ApplicationIdentifierValue tmpVal;
					if (appId.GetAsUint64(tmpVal))
					{
						if (tmpVal <= std::numeric_limits<AppId_t>::max())
						{
							return static_cast<AppId_t>(tmpVal);
						}

						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_COMMENT, "[GamePlatform] %s: Unable to convert '%s' to AppId_t.", __func__, appId.ToDebugString());
					}
				}

				CRY_ASSERT(false, "[Steam] ApplicationIdentifier '%s' does not seem to contain a valid Steam ID", appId.ToDebugString());
				return k_uAppIdInvalid;
			}
		}
	}
}
