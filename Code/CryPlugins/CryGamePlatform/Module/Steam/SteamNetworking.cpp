// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "SteamNetworking.h"
#include "SteamUserIdentifier.h"

#include <steam/steam_gameserver.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			CNetworking::CNetworking()
				: m_callbackP2PSessionRequest(this, &CNetworking::OnSessionRequest)
				, m_callbackP2PSessionConnectFail(this, &CNetworking::OnSessionConnectFail)
			{
			}

			ISteamNetworking* CNetworking::GetSteamNetworking() const
			{
#ifdef DEDICATED_SERVER
				return SteamGameServerNetworking();
#else
				return SteamNetworking();
#endif
			}

			bool CNetworking::SendPacket(const AccountIdentifier& remoteUser, void* pData, uint32 dataLength)
			{
				ISteamNetworking* pSteamNetworking = GetSteamNetworking();
				if (!pSteamNetworking)
				{
					return false;
				}
				return pSteamNetworking->SendP2PPacket(ExtractSteamID(remoteUser), pData, dataLength, k_EP2PSendReliable);
			}

			bool CNetworking::CloseSession(const AccountIdentifier& remoteUser)
			{
				ISteamNetworking* pSteamNetworking = GetSteamNetworking();
				if (!pSteamNetworking)
				{
					return false;
				}
				return pSteamNetworking->CloseP2PSessionWithUser(ExtractSteamID(remoteUser));
			}

			bool CNetworking::IsPacketAvailable(uint32* pPacketSizeOut) const
			{
				ISteamNetworking* pSteamNetworking = GetSteamNetworking();
				if (!pSteamNetworking)
				{
					return false;
				}
				return pSteamNetworking->IsP2PPacketAvailable(pPacketSizeOut);
			}

			bool CNetworking::ReadPacket(void* pDest, uint32 destLength, uint32* pMessageSizeOut, AccountIdentifier* pRemoteIdOut)
			{
				ISteamNetworking* pSteamNetworking = GetSteamNetworking();
				if (!pSteamNetworking)
				{
					return false;
				}
				return pSteamNetworking->ReadP2PPacket(pDest, destLength, pMessageSizeOut, reinterpret_cast<CSteamID*>(pRemoteIdOut));
			}

			// Steam callbacks
			void CNetworking::OnSessionRequest(P2PSessionRequest_t* pP2PSessionRequest)
			{
				// always accept packets
				ISteamNetworking* pSteamNetworking = GetSteamNetworking();
				if (!pSteamNetworking)
				{
					return;
				}
				pSteamNetworking->AcceptP2PSessionWithUser(pP2PSessionRequest->m_steamIDRemote);
			}

			void CNetworking::OnSessionConnectFail(P2PSessionConnectFail_t* pP2PSessionConnectFail)
			{
			}
		}
	}
}