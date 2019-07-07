// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "RailNetworking.h"
#include "RailHelper.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			CNetworking::CNetworking()
			{
				Helper::RegisterEvent(rail::kRailEventNetworkCreateSessionRequest, *this);
				Helper::RegisterEvent(rail::kRailEventNetworkCreateSessionFailed, *this);
			}

			CNetworking::~CNetworking()
			{
				Helper::UnregisterEvent(rail::kRailEventNetworkCreateSessionRequest, *this);
				Helper::UnregisterEvent(rail::kRailEventNetworkCreateSessionFailed, *this);
			}

			bool CNetworking::SendPacket(const AccountIdentifier& remoteUser, void* pData, uint32 dataLength)
			{
				if (rail::IRailNetwork* pNetwork = Helper::Network())
				{
					const rail::RailResult res = pNetwork->SendReliableData(Helper::PlayerID(), ExtractRailID(remoteUser), pData, dataLength);
					if (res == rail::kSuccess)
					{
						return true;
					}

					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Networking] SendReliableData('%" PRIu64 "', '%s', pData[%u]) failed : %s",
						Helper::PlayerID().get_id(),
						remoteUser.ToDebugString(),
						dataLength,
						Helper::ErrorString(res));
				}

				return false;
			}

			bool CNetworking::CloseSession(const AccountIdentifier& remoteUser)
			{
				if (rail::IRailNetwork* pNetwork = Helper::Network())
				{
					const rail::RailResult res = pNetwork->CloseSession(Helper::PlayerID(), ExtractRailID(remoteUser));
					if (res == rail::kSuccess)
					{
						return true;
					}

					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Networking] CloseSession() failed : %s", Helper::ErrorString(res));
				}

				return false;
			}

			bool CNetworking::IsPacketAvailable(uint32* pPacketSizeOut) const
			{
				if (rail::IRailNetwork* pNetwork = Helper::Network())
				{
					rail::RailID localPeer;
					return pNetwork->IsDataReady(&localPeer, pPacketSizeOut);
				}

				return false;
			}

			bool CNetworking::ReadPacket(void* pDest, uint32 destLength, uint32* pMessageSizeOut, AccountIdentifier* pRemoteIdOut)
			{
				// We don't have this info, unfortunately
				if (pMessageSizeOut)
				{
					*pMessageSizeOut = 0;
				}

				if (rail::IRailNetwork* pNetwork = Helper::Network())
				{
					rail::RailID remotePeer;
					const rail::RailResult res = pNetwork->ReadData(Helper::PlayerID(), &remotePeer, pDest, destLength);
					if (res == rail::kSuccess)
					{
						if (pRemoteIdOut)
						{
							*pRemoteIdOut = CreateAccountIdentifier(remotePeer);
						}

						return true;
					}

					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Networking] ReadData('%" PRIu64 "') failed : %s", Helper::PlayerID().get_id(), Helper::ErrorString(res));
				}

				return false;
			}

			void CNetworking::OnSessionRequest(const rail::rail_event::CreateSessionRequest* pP2PSessionRequest)
			{
				if (pP2PSessionRequest->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Networking] %s error : %s", __func__, Helper::ErrorString(pP2PSessionRequest->result));
					return;
				}

				if (rail::IRailNetwork* pNetwork = Helper::Network())
				{
					const rail::RailResult res = pNetwork->AcceptSessionRequest(pP2PSessionRequest->local_peer, pP2PSessionRequest->remote_peer);
					if (res != rail::kSuccess)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Networking] Local peer '%" PRIu64 "' was unable to establish session with remote peer '%" PRIu64 "' : %s",
							pP2PSessionRequest->local_peer.get_id(),
							pP2PSessionRequest->remote_peer.get_id(),
							Helper::ErrorString(res));
					}
				}
			}

			void CNetworking::OnSessionConnectFail(const rail::rail_event::CreateSessionFailed* pP2PSessionConnectFail)
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Networking] Local peer '%" PRIu64 "' was unable to establish session with remote peer '%" PRIu64 "' : %s",
					pP2PSessionConnectFail->local_peer.get_id(),
					pP2PSessionConnectFail->remote_peer.get_id(),
					Helper::ErrorString(pP2PSessionConnectFail->result));
			}

			void CNetworking::OnRailEvent(rail::RAIL_EVENT_ID eventId, rail::EventBase* pEvent)
			{
				if (pEvent->game_id != Helper::GameID())
				{
					return;
				}

				if (eventId == rail::kRailEventNetworkCreateSessionRequest)
				{
					OnSessionRequest(static_cast<const rail::rail_event::CreateSessionRequest*>(pEvent));
				}
				else if (eventId == rail::kRailEventNetworkCreateSessionFailed)
				{
					OnSessionConnectFail(static_cast<const rail::rail_event::CreateSessionFailed*>(pEvent));
				}
			}
		}
	}
}