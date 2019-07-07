// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformTypes.h"
#include <discord_rpc.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Discord
		{
			using AccountIdentifierValue = Cry::GamePlatform::Detail::StringIdentifierValue;
			using ApplicationIdentifierValue = Cry::GamePlatform::Detail::StringIdentifierValue;

			inline AccountIdentifier     CreateAccountIdentifier(const AccountIdentifierValue& discordId)         { return AccountIdentifier(DiscordServiceID, discordId); }
			inline ApplicationIdentifier CreateApplicationIdentifier(const ApplicationIdentifierValue& discordId) { return ApplicationIdentifier(DiscordServiceID, discordId); }
		}
	}
}
