// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformMatchmaking.h"
#include "RailTypes.h"
#include "RailUserLobby.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			class CMatchmaking
				: public IMatchmaking
				, public rail::IRailEvent
			{
			public:
				explicit CMatchmaking(CService& service);
				~CMatchmaking();

				// IMatchmaking
				virtual void AddListener(IMatchmaking::IListener& listener) override { stl::push_back_unique(m_listeners, &listener); }
				virtual void RemoveListener(IMatchmaking::IListener& listener) override { stl::find_and_erase(m_listeners, &listener); }

				virtual void CreateLobby(IUserLobby::EVisibility visibility, int maxMemberCount) override;
				virtual CUserLobby* GetUserLobby(const AccountIdentifier& user) const override;
				virtual CUserLobby* GetUserLobby(const IUser& user) const override;
				virtual CUserLobby* GetLobbyById(const LobbyIdentifier& id) override;

				virtual void QueryLobbies() override;
				virtual LobbyIdentifier GetQueriedLobbyIdByIndex(int index) const override;

				virtual void AddLobbyQueryFilterString(const char* key, const char* value, IUserLobby::EComparison comparison) override;

				virtual void JoinLobby(const LobbyIdentifier& lobbyId) override;
				// ~IMatchmaking

				// IRailEvent
				virtual void OnRailEvent(rail::RAIL_EVENT_ID eventId, rail::EventBase* pEvent) override;
				// ~IRailEvent

			private:
				static rail::EnumRoomType ToRailRoomType(IUserLobby::EVisibility visibility);
				static rail::EnumRailComparisonType ToRailComparisonType(IUserLobby::EComparison comparison);

				void AddRoomInstance(rail::IRailRoom* pRoom);
				void JoinLobby(uint64_t lobbyId, const char* szPassword);
				void ProcessLaunchParameters();
				CUserLobby* AllocateUserLobby(uint64_t roomId);

				void OnCreateLobby(const rail::rail_event::CreateRoomResult* pRoomInfo);
				void OnLobbyMatchList(const rail::rail_event::GetRoomListResult* pRoomList);
				void OnJoinRequested(const rail::rail_event::RailUsersGetInviteDetailResult* pInvite);
				void OnLocalJoin(const rail::rail_event::JoinRoomResult* pJoinInfo);
				void OnInvited(const rail::rail_event::RailUsersRespondInvitation* pInvite);
				void OnRoomLeft(const rail::rail_event::LeaveRoomResult* pLeaveInfo);

			private:
				CService & m_service;
				std::vector<IMatchmaking::IListener*> m_listeners;
				std::vector<std::unique_ptr<CUserLobby>> m_lobbies;
				std::vector<rail::IRailRoom*> m_roomInstances;

				rail::RoomInfoListFilter m_lobbyFilter;
				rail::RailArray<rail::RoomInfo> m_lastQueriedRoomInfo;
			};
		}
	}
}