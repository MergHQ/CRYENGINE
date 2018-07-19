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
			namespace Detail
			{
				struct SUserIdentifierResolver : public Serialization::IArchive
				{
					SUserIdentifierResolver()
						: Serialization::IArchive(Serialization::IArchive::INPUT)
					{
					}

					// Serialization::IArchive

					using Serialization::IArchive::operator ();

					virtual bool operator()(yasli::StringInterface& value, const char* name = "", const char* label = nullptr) override
					{
						if (strlen(value.get()) < discordId.MAX_SIZE)
						{
							discordId = value.get();
							return true;
						}

						return false;
					}

					virtual bool operator()(yasli::u64& value, const char* name = "", const char* label = nullptr) override
					{
						return false;
					}

					virtual bool operator () (const Serialization::SStruct& value, const char* szName = "", const char* szLabel = nullptr) override
					{
						value(*this);
						return true;
					}

					// ~Serialization::IArchive

					AccountIdentifierValue discordId;
				};
			}

			inline AccountIdentifierValue ExtractDiscordID(const AccountIdentifier& accountId)
			{
				Detail::SUserIdentifierResolver userSer;
				userSer(accountId);

				return userSer.discordId;
			}
		}
	}
}