// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformLobby.h"
#include "RailTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			class CUserLobby
				: public IUserLobby
				, public ISystemEventListener
				, public rail::IRailEvent
			{
			public:
				CUserLobby(CService& railService, rail::IRailRoom& room);
				~CUserLobby();

				// IUserLobby
				virtual void AddListener(IListener& listener) override;
				virtual void RemoveListener(IListener& listener) override;

				virtual bool HostServer(const char* szLevel, bool isLocal) override;

				virtual int GetMemberLimit() const override;
				virtual int GetNumMembers() const override;
				virtual AccountIdentifier GetMemberAtIndex(int index) const override;

				virtual bool IsInServer() const override { return m_serverIP != 0; }
				virtual void Leave() override;

				virtual AccountIdentifier GetOwnerId() const override;
				virtual LobbyIdentifier GetIdentifier() const override;

				virtual bool SendChatMessage(const char* message) const override;

				virtual void ShowInviteDialog() const override;

				virtual bool SetData(const char* key, const char* value) override;
				virtual const char* GetData(const char* szKey) const override;
				virtual const char* GetMemberData(const AccountIdentifier& userId, const char* szKey) override;
				virtual void SetMemberData(const char* szKey, const char* szValue) override;
				// ~IUserLobby

				// ISystemEventListener
				virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
				// ~ISystemEventListener

				// IRailEvent
				virtual void OnRailEvent(rail::RAIL_EVENT_ID eventId, rail::EventBase* pEvent) override;
				// ~IRailEvent

				rail::IRailRoom& GetRoom() const { return m_room; }

			private:
				using KeyValueArray = rail::RailArray<rail::RailKeyValue>;
				void ConnectToServer(uint32 ip, uint16 port, IServer::Identifier serverId);

				static size_t FindIndexForKey(const KeyValueArray& kva, const char* szKey);
				static const char* FindValueForKey(const KeyValueArray& kva, const char* szKey);
				static void SetValueForKey(KeyValueArray& kva, const char* szKey, const char* szValue);

				void OnUsersInviteJoinGameResult(const rail::rail_event::RailUsersInviteJoinGameResult* pResult);
				void OnMemberKicked(const rail::rail_event::NotifyRoomMemberKicked* pKicked);
				void OnMemberChange(const rail::rail_event::NotifyRoomMemberChange* pChange);
				void OnFriendsMetadataChanged(const rail::rail_event::RailFriendsMetadataChanged* pMetadata);
				void OnDataReceived(const rail::rail_event::RoomDataReceived* pData);
				void OnSetRoomMetadata(const rail::rail_event::SetRoomMetadataResult* pResult);
				void OnGetRoomMetadata(const rail::rail_event::GetRoomMetadataResult* pMetadata);
				void OnSetMemberMetadata(const rail::rail_event::SetMemberMetadataResult* pResult);
				void OnGetMemberMetadata(const rail::rail_event::GetMemberMetadataResult* pMetadata);
				void OnRoomMetadataChange(const rail::rail_event::NotifyMetadataChange* pMetadata);
				void OnRoomGetAllDataResult(const rail::rail_event::GetAllRoomDataResult* pRoomData);
				void OnUsersNotifyInviter(const rail::rail_event::RailUsersNotifyInviter* pInvitation);
			private:
				CService& m_service;
				rail::IRailRoom& m_room;
				KeyValueArray m_roomDataCache;
				std::unordered_map<uint64_t, KeyValueArray> m_memberDataCache;

				std::vector<IListener*> m_listeners;

				uint32 m_serverIP = 0;
				uint16 m_serverPort = 0;

				IServer::Identifier m_serverId = 0;
			};
		}
	}
}