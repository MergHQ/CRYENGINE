// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformNetworking.h"
#include "SteamTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			class CNetworking
				: public INetworking
			{
			public:
				CNetworking();
				virtual ~CNetworking() = default;

				// INetworking
				virtual bool SendPacket(const AccountIdentifier& remoteUser, void* pData, uint32 dataLength);
				virtual bool CloseSession(const AccountIdentifier& remoteUser);

				virtual bool IsPacketAvailable(uint32* pPacketSizeOut) const;
				virtual bool ReadPacket(void* pDest, uint32 destLength, uint32* pMessageSizeOut, AccountIdentifier* pRemoteIdOut);
				// ~INetworking

				ISteamNetworking* GetSteamNetworking() const;

				STEAM_CALLBACK(CNetworking, OnSessionRequest, P2PSessionRequest_t, m_callbackP2PSessionRequest);
				STEAM_CALLBACK(CNetworking, OnSessionConnectFail, P2PSessionConnectFail_t, m_callbackP2PSessionConnectFail);
			};
		}
	}
}