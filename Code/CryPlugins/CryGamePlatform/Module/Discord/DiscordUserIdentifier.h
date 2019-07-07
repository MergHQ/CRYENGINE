// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DiscordTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Discord
		{
			inline AccountIdentifierValue ExtractDiscordID(const AccountIdentifier& accountId)
			{
				if (accountId.Service() == DiscordServiceID)
				{
					AccountIdentifierValue tmpStr;
					accountId.GetAsString(tmpStr);

					return AccountIdentifierValue(tmpStr.c_str());
				}

				CRY_ASSERT(false, "[Discord] AccountIdentifier '%s' does not seem to contain a valid Discord ID", accountId.ToDebugString());
				return AccountIdentifierValue();
			}
		}
	}
}