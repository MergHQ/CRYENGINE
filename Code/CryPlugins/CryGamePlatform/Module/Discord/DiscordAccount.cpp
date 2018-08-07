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

			void CAccount::SetStatus(const char* szStatus)
			{
				SRichPresence presence;
				presence.status = szStatus;

				SetPresence(presence);
			}

			void CAccount::SetPresence(const SRichPresence& presence)
			{
				m_presence = presence;

				// Note: Should be zero-initialized. Keep that in mind in case 
				// this struct somehow changes in future versions of the SDK
				DiscordRichPresence discordPresence = {};
				
				discordPresence.state = m_presence.status.c_str();
				discordPresence.details = m_presence.details.c_str();
				discordPresence.partySize = m_presence.partySize;
				discordPresence.partyMax = m_presence.partyMax;

				if(m_presence.countdownTimer == SRichPresence::ETimer::Remaining)
				{
					discordPresence.endTimestamp = time(0) + m_presence.seconds;
				}
				else if(m_presence.countdownTimer == SRichPresence::ETimer::Elapsed)
				{
					discordPresence.startTimestamp = time(0) + m_presence.seconds;
				}

				CryComment("[Discord] Setting rich presence:\n%s\n%s", m_presence.status.c_str(), m_presence.details.c_str());

				Discord_UpdatePresence(&discordPresence);
			}

			void CAccount::GetPresence(SRichPresence& presence) const
			{
				presence = m_presence;
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