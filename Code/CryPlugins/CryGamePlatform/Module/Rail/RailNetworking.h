// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformNetworking.h"
#include "RailTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			class CNetworking
				: public INetworking
				, public rail::IRailEvent
			{
			public:
				CNetworking();
				~CNetworking();

				// INetworking
				virtual bool SendPacket(const AccountIdentifier& remoteUser, void* pData, uint32 dataLength) override;
				virtual bool CloseSession(const AccountIdentifier& remoteUser) override;
				virtual bool IsPacketAvailable(uint32* pPacketSizeOut) const override;
				virtual bool ReadPacket(void* pDest, uint32 destLength, uint32* pMessageSizeOut, AccountIdentifier* pRemoteIdOut) override;
				// ~INetworking

				// rail::IRailEvent
				virtual void OnRailEvent(rail::RAIL_EVENT_ID eventId, rail::EventBase* pEvent) override;
				// ~rail::IRailEvent

			private:
				void OnSessionRequest(const rail::rail_event::CreateSessionRequest* pP2PSessionRequest);
				void OnSessionConnectFail(const rail::rail_event::CreateSessionFailed* pP2PSessionConnectFail);
			};
		}
	}
}