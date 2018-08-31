// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformUserIdentifier.h"
#include "DiscordTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Discord
		{
			inline AccountIdentifierValue ExtractDiscordID(const AccountIdentifier& accountId)
			{
				AccountIdentifierValue tmpStr;
				accountId.GetAsString(tmpStr);

				return AccountIdentifierValue(tmpStr.c_str());
			}
		}
	}
}