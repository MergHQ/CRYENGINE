// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "User.h"
#include "IPlatformAccount.h"

namespace Cry
{
	namespace GamePlatform
	{
		CUser::CUser(DynArray<IAccount*> accounts)
		{
			SetAccounts(std::move(accounts));
		}

		CUser::~CUser()
		{
		}

		const char* CUser::GetNickname() const
		{
			return m_accounts.empty() ? m_accounts[0]->GetNickname() : "?Account?";
		}

		UserIdentifier CUser::GetIdentifier() const
		{
			return m_accounts.empty() ? UserIdentifier() : UserIdentifier(m_accounts[0]->GetIdentifier());
		}

		void CUser::SetStatus(const char* status)
		{
			for (IAccount* pAccount : m_accounts)
			{
				pAccount->SetStatus(status);
			}
		}

		const char* CUser::GetStatus() const
		{
			return m_accounts.empty() ? "?Account?" : m_accounts[0]->GetStatus();
		}

		ITexture* CUser::GetAvatar(EAvatarSize size) const
		{
			return m_accounts.empty() ? nullptr : m_accounts[0]->GetAvatar(size);
		}

		IAccount* CUser::GetAccount(const ServiceIdentifier& svcId) const
		{
			auto pred = [&svcId](IAccount* pAcc) 
			{ 
				return pAcc->GetIdentifier().Service() == svcId; 
			};

			auto accPos = std::find_if(m_accounts.begin(), m_accounts.end(), pred);
			return accPos != m_accounts.end() ? *accPos : nullptr;
		}

		bool CUser::HasAccount(const IAccount& account) const
		{
			return std::find(m_accounts.begin(), m_accounts.end(), &account) != m_accounts.end();
		}

		void CUser::SetAccounts(DynArray<IAccount*> accounts)
		{
			m_accounts.swap(accounts);
		}

		void CUser::RemoveAccount(const IAccount& account)
		{
			stl::find_and_erase_all(m_accounts, &account);
		}

		const char* CUser::ToDebugString() const
		{
			return GetIdentifier().ToDebugString();
		}
	}
}