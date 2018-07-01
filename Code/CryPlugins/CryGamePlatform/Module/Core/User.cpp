// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "User.h"
#include "IPlatformAccount.h"

namespace Cry
{
	namespace GamePlatform
	{
		CUser::CUser(IAccount& account, const DynArray<IAccount*>& connectedAccounts)
			: m_mainAccount(account)
			, m_connectedAccounts(connectedAccounts)
		{
		}

		CUser::~CUser()
		{
		}

		const char* CUser::GetNickname() const
		{
			return m_mainAccount.GetNickname();
		}

		UserIdentifier CUser::GetIdentifier() const
		{
			return UserIdentifier(m_mainAccount.GetIdentifier());
		}

		void CUser::SetStatus(const char* status)
		{
			m_mainAccount.SetStatus(status);

			for (IAccount* pAccount : m_connectedAccounts)
			{
				pAccount->SetStatus(status);
			}
		}

		const char* CUser::GetStatus() const
		{
			return m_mainAccount.GetStatus();
		}

		ITexture* CUser::GetAvatar(EAvatarSize size) const
		{
			return m_mainAccount.GetAvatar(size);
		}

		IAccount* CUser::GetAccount(const ServiceIdentifier& svcId) const
		{
			if (m_mainAccount.GetIdentifier().Service() == svcId)
			{
				return &m_mainAccount;
			}

			return nullptr;
		}
	}
}