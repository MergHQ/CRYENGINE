// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
				AccountIdentifierValue tmpVal;
				if (accountId.GetAsUint64(tmpVal))
				{
					return tmpVal;
				}

				return k_steamIDNil;
			}

			inline CSteamID ExtractSteamID(const LobbyIdentifier& lobbyId)
			{
				LobbyIdentifierValue tmpVal;
				if (lobbyId.GetAsUint64(tmpVal))
				{
					return tmpVal;
				}

				return k_steamIDNil;
			}

			inline AppId_t ExtractSteamID(const ApplicationIdentifier& appId)
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

				return k_uAppIdInvalid;
			}
		}
	}
}
