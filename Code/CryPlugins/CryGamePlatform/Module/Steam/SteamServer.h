// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformServer.h"
#include "SteamTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			class CServer
				: public IServer
			{
			public:
				explicit CServer(bool bLocal);
				virtual ~CServer();

				// IServer
				virtual IServer::Identifier GetIdentifier() const override;

				virtual uint32 GetPublicIP() const override;
				virtual const char* GetPublicIPString() const override;

				virtual uint16 GetPort() const override;

				virtual bool IsLocal() const override { return m_bLocal; }

				virtual bool AuthenticateUser(uint32 clientIP, char* authData, int authDataLength, AccountIdentifier &userId) override;
				virtual void SendUserDisconnect(const AccountIdentifier& userId) override;
				// ~IServer

			protected:
				bool m_bLocal;
			};
		}
	}
}