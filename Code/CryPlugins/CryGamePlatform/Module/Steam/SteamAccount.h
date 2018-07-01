// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformAccount.h"
#include "SteamTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			class CAccount
				: public IAccount
			{
			public:
				CAccount(CSteamID id);
				virtual ~CAccount();

				// IAccount
				virtual const char* GetNickname() const override;
				virtual AccountIdentifier GetIdentifier() const override;

				virtual void SetStatus(const char* status) override;
				virtual const char* GetStatus() const override;

				virtual const DynArray<AccountIdentifier>& GetConnectedAccounts() const override;

				virtual ITexture* GetAvatar(EAvatarSize size) const override;
				// ~IAccount

				const CSteamID& GetSteamID() const { return m_id; }

			protected:
				CSteamID m_id;
				DynArray<AccountIdentifier> m_connectedAccounts;
			};
		}
	}
}