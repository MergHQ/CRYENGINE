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
			namespace Detail
			{
				struct SUserIdentifierResolver : public Serialization::IArchive
				{
					SUserIdentifierResolver()
						: Serialization::IArchive(Serialization::IArchive::INPUT)
						, steamId(k_steamIDNil)
					{
					}

					// Serialization::IArchive

					using Serialization::IArchive::operator ();

					virtual bool operator()(uint64& value, const char* szName = "", const char* szLabel = nullptr) override
					{
						steamId = value;
						return true;
					}


					virtual bool operator () (const Serialization::SStruct& value, const char* szName = "", const char* szLabel = nullptr) override
					{
						value(*this);
						return true;
					}

					// ~Serialization::IArchive

					CSteamID steamId;
				};
			}			

			inline CSteamID ExtractSteamID(const UserIdentifier& userId)
			{
				Detail::SUserIdentifierResolver userSer;
				userSer(userId.GetAccount(SteamServiceID));

				return userSer.steamId;
			}

			inline CSteamID ExtractSteamID(const AccountIdentifier& accountId)
			{
				Detail::SUserIdentifierResolver userSer;
				userSer(accountId);

				return userSer.steamId;
			}

			inline CSteamID ExtractSteamID(const LobbyIdentifier& lobbyId)
			{
				Detail::SUserIdentifierResolver userSer;
				userSer(lobbyId);

				return userSer.steamId;
			}
		}
	}
}