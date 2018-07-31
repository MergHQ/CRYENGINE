// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "DiscordAccount.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Discord
		{
			CAccount::CAccount(const DiscordUser& user)
			{
				SetDiscordUser(user);
			}

			CAccount::~CAccount()
			{
			}

			const char* CAccount::GetNickname() const
			{
				return m_nickname.c_str();
			}

			AccountIdentifier CAccount::GetIdentifier() const
			{
				return AccountIdentifier(DiscordServiceID, m_id);
			}

			ServiceIdentifier CAccount::GetServiceIdentifier() const
			{
				return DiscordServiceID;
			}

			void CAccount::SetStatus(const char* status)
			{
				m_status = status;
				
				DiscordRichPresence discordPresence;
				memset(&discordPresence, 0, sizeof(discordPresence));
				discordPresence.state = "Playing";
				discordPresence.details = m_status.c_str();
				discordPresence.endTimestamp = time(0) + 5 * 60;
				discordPresence.largeImageKey = "canary-large";
				discordPresence.smallImageKey = "ptb-small";

				CryComment("[Discord] Setting rich presence '%s'", status);

				Discord_UpdatePresence(&discordPresence);
			}

			const char* CAccount::GetStatus() const
			{
				return m_status.c_str();
			}

			const DynArray<Cry::GamePlatform::AccountIdentifier>& CAccount::GetConnectedAccounts() const
			{
				return m_connectedAccounts;
			}

			bool CAccount::IsLocal() const
			{
				// luca 7/4/2018: As of now, Discord SDK doesn't provide account information for anybody else
				return true;
			}

			ITexture* CAccount::GetAvatar(EAvatarSize size) const
			{
				// luca 7/3/2018: Need a web request to get the avatar https://discordapp.com/developers/docs/reference#image-formatting
				return nullptr;
			}

			void CAccount::SetDiscordUser(const DiscordUser& user)
			{
				CRY_ASSERT_MESSAGE(strlen(user.userId) < m_id.MAX_SIZE);

				m_id = user.userId;
				m_nickname = user.username;
				m_avatarHash = user.avatar;
			}
		}
	}
}