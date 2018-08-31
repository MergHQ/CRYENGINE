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
				AccountIdentifierValue tmpVal;
				if (lobbyId.GetAsUint64(tmpVal))
				{
					return tmpVal;
				}

				return k_steamIDNil;
			}
		}
	}
}
