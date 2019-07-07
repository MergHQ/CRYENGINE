// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "RailMatchmaking.h"
#include "RailService.h"
#include "IPlatformUser.h"
#include "CrySchematyc2/Utils/StringUtils.h"
#include "CrySystem/CryVersion.h"
#include "RailHelper.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			/*static*/ rail::EnumRoomType CMatchmaking::ToRailRoomType(IUserLobby::EVisibility visibility)
			{
				switch (visibility)
				{
				case IUserLobby::EVisibility::Private:     return rail::kRailRoomTypePrivate;
				case IUserLobby::EVisibility::FriendsOnly: return rail::kRailRoomTypeWithFriends;
				case IUserLobby::EVisibility::Public:      return rail::kRailRoomTypePublic;
				case IUserLobby::EVisibility::Invisible:   return rail::kRailRoomTypeHidden;
				}

				CRY_ASSERT(false, "[Rail][Matchmaking] Unrecognized IUserLobby::EVisibility enum value! Defaulting to 'Public'.");
				return rail::kRailRoomTypePublic;
			}

			/*static*/ rail::EnumRailComparisonType CMatchmaking::ToRailComparisonType(IUserLobby::EComparison comparison)
			{
				switch (comparison)
				{
				case IUserLobby::EComparison::Equal:                return rail::kRailComparisonTypeEqual;
				case IUserLobby::EComparison::EqualToOrGreaterThan: return rail::kRailComparisonTypeEqualToOrGreaterThan;
				case IUserLobby::EComparison::EqualToOrLess:        return rail::kRailComparisonTypeEqualToOrLessThan;
				case IUserLobby::EComparison::GreaterThan:          return rail::kRailComparisonTypeGreaterThan;
				case IUserLobby::EComparison::NotEqual:             return rail::kRailComparisonTypeNotEqual;
				}

				CRY_ASSERT(false, "[Rail][Matchmaking] Unrecognized IUserLobby::EComparison enum value! Defaulting to 'Equal'.");
				return rail::kRailComparisonTypeEqual;
			}

			CMatchmaking::CMatchmaking(CService& service)
				: m_service(service)
			{
				Helper::RegisterEvent(rail::kRailEventRoomCreated, *this);
				Helper::RegisterEvent(rail::kRailEventRoomGetRoomListResult, *this);
				Helper::RegisterEvent(rail::kRailEventRoomJoinRoomResult, *this);
				Helper::RegisterEvent(rail::kRailEventUsersRespondInvitation, *this);
				Helper::RegisterEvent(rail::kRailEventRoomLeaveRoomResult, *this);
				Helper::RegisterEvent(rail::kRailEventUsersGetInviteDetailResult, *this);

				ProcessLaunchParameters();
			}

			CMatchmaking::~CMatchmaking()
			{
				Helper::UnregisterEvent(rail::kRailEventRoomCreated, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomGetRoomListResult, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomJoinRoomResult, *this);
				Helper::UnregisterEvent(rail::kRailEventUsersRespondInvitation, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomLeaveRoomResult, *this);
				Helper::UnregisterEvent(rail::kRailEventUsersGetInviteDetailResult, *this);

				m_listeners.clear();
				m_lobbies.clear();

				for (rail::IRailRoom* pRoom : m_roomInstances)
				{
					pRoom->Release();
				}
			}

			void CMatchmaking::CreateLobby(IUserLobby::EVisibility visibility, int maxMemberCount)
			{
				if (rail::IRailRoomHelper* pRailMatchmaking = Helper::RoomHelper())
				{
					rail::RoomOptions options;
					options.max_members = maxMemberCount;
					options.type = ToRailRoomType(visibility);
					options.enable_team_voice = false;

					// Actual creation of Lobby is in callback: OnCreateLobby()
					if (rail::IRailRoom* pRoom = pRailMatchmaking->AsyncCreateRoom(options, "", ""))
					{
						AddRoomInstance(pRoom);
					}
					else
					{
						CRY_ASSERT(false, "[Rail][Matchmaking] AsyncCreateRoom() returned NULL!");
					}
				}
			}

			CUserLobby* CMatchmaking::GetUserLobby(const AccountIdentifier& user) const
			{
				for (const std::unique_ptr<CUserLobby>& pLobby : m_lobbies)
				{
					const int numMembers = pLobby->GetNumMembers();
					for (int i = 0; i < numMembers; i++)
					{
						const AccountIdentifier memberId = pLobby->GetMemberAtIndex(i);
						if (memberId == user)
						{
							return pLobby.get();
						}
					}
				}

				return nullptr;
			}

			CUserLobby* CMatchmaking::GetUserLobby(const IUser& user) const
			{
				if (IAccount* pAccount = user.GetAccount(RailServiceID))
				{
					return GetUserLobby(pAccount->GetIdentifier());
				}

				return nullptr;
			}

			CUserLobby* CMatchmaking::GetLobbyById(const LobbyIdentifier& id)
			{
				for (const std::unique_ptr<CUserLobby>& pLobby : m_lobbies)
				{
					if (pLobby->GetIdentifier() == id)
					{
						return pLobby.get();
					}
				}

				return nullptr;
			}

			void CMatchmaking::QueryLobbies()
			{
				const CryFixedStringT<32> versionString = gEnv->pSystem->GetProductVersion().ToString();

				// Always query lobbies for this version only
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] [Lobby] Looking for lobby with version: %s", versionString.c_str());
#ifdef RELEASE
				AddLobbyQueryFilterString("version", versionString.c_str(), IUserLobby::EComparison::Equal);
#endif

				if (rail::IRailRoomHelper* pRooms = Helper::RoomHelper())
				{
					const uint32_t start_index = 0;
					const uint32_t end_index = 1;
					const rail::RailArray<rail::RoomInfoListSorter> sorter;
					rail::RailArray<rail::RoomInfoListFilter> filter;
					filter.push_back(m_lobbyFilter);

					const rail::RailResult res = pRooms->AsyncGetRoomList(start_index, end_index, sorter, filter, "");
					if (res == rail::kSuccess)
					{
						return;
					}

					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] AsyncGetRoomListInZone() failed : %s", Helper::ErrorString(res));
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] Matchmaking service not available");
				}

				rail::rail_event::GetRoomListResult temp;
				temp.total_room_num = 0;
				OnLobbyMatchList(&temp);
			}

			LobbyIdentifier CMatchmaking::GetQueriedLobbyIdByIndex(int index) const
			{
				if (index < m_lastQueriedRoomInfo.size())
				{
					return CreateLobbyIdentifier(m_lastQueriedRoomInfo[index].room_id);
				}

				return LobbyIdentifier();
			}

			void CMatchmaking::AddLobbyQueryFilterString(const char* key, const char* value, IUserLobby::EComparison comparison)
			{
				rail::RoomInfoListFilterKey newFilter;
				newFilter.key_name = key;
				newFilter.filter_value = value;
				newFilter.value_type = rail::kRailPropertyValueTypeString;
				newFilter.comparison_type = ToRailComparisonType(comparison);

				for (size_t fIdx = 0; fIdx < m_lobbyFilter.key_filters.size(); ++fIdx)
				{
					if (m_lobbyFilter.key_filters[fIdx].key_name == key)
					{
						m_lobbyFilter.key_filters[fIdx] = newFilter;
						return;
					}
				}

				m_lobbyFilter.key_filters.push_back(newFilter);
			}

			void CMatchmaking::JoinLobby(const LobbyIdentifier& lobbyId)
			{
				JoinLobby(ExtractRoomID(lobbyId), "");
			}

			void CMatchmaking::AddRoomInstance(rail::IRailRoom* pRoom)
			{
				if(stl::find(m_roomInstances, pRoom))
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] Room instance '%" PRIu64 "' was already in the list!?", pRoom->GetRoomID());
					return;
				}

				m_roomInstances.push_back(pRoom);
			}

			void CMatchmaking::JoinLobby(uint64_t lobbyId, const char* szPassword)
			{
				rail::IRailRoomHelper* pRailMatchmaking = Helper::RoomHelper();
				if (pRailMatchmaking)
				{
					rail::RailResult res = rail::kSuccess;
					if (rail::IRailRoom* pRoom = pRailMatchmaking->OpenRoom(lobbyId, &res))
					{
						CRY_ASSERT(res == rail::kSuccess, "[Rail][Matchmaking] OpenRoom() succeeded but returned an error : %s", Helper::ErrorString(res));
						// Actual creation of Lobby is in callback: OnLocalJoin()
						res = pRoom->AsyncJoinRoom(szPassword, "");
						if (res == rail::kSuccess)
						{
							AddRoomInstance(pRoom);
							return;
						}

						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] AsyncJoinRoom() failed : %s", Helper::ErrorString(res));
						pRoom->Release();
					}
					else
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] OpenRoom() failed : %s", Helper::ErrorString(res));
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] Matchmaking service not available");
				}

				rail::rail_event::JoinRoomResult info;
				info.result = rail::kFailure;

				OnLocalJoin(&info);
			}

			void CMatchmaking::ProcessLaunchParameters()
			{
				ICmdLine* const pCmdLine = gEnv->pSystem->GetICmdLine();

				// Check if we were launched from invite. Args are set in CUserLobby.
				const ICmdLineArg* const pConnectRoomArg = pCmdLine->FindArg(eCLAT_Post, "connect_rail_lobby");

				if (pConnectRoomArg)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] Received pre-launch join request, loading lobby");

					const uint64_t roomId = Schematyc2::StringUtils::UInt64FromString(pConnectRoomArg->GetValue(), 0);

					JoinLobby(roomId, "");
				}
			}

			CUserLobby* CMatchmaking::AllocateUserLobby(uint64_t roomId)
			{
				size_t idx = 0;
				for (; idx < m_roomInstances.size(); ++idx)
				{
					if (m_roomInstances[idx]->GetRoomID() == roomId)
					{
						break;
					}
				}

				if (idx >= m_roomInstances.size())
				{
					CRY_ASSERT(false, "[Rail][Matchmaking] couldn't find a match for provided room id: '%" PRIu64 "'!", roomId);
					return nullptr;
				}

				if (idx >= m_lobbies.size())
				{
					m_lobbies.resize(idx + 1);
				}

				CRY_ASSERT(!m_lobbies[idx], "[Rail][Matchmaking] A user lobby instance was already created for room id: '%" PRIu64 "'. It will be destroyed!", roomId);

				m_lobbies[idx] = stl::make_unique<CUserLobby>(m_service, *m_roomInstances[idx]);

				CUserLobby* const pLobby = m_lobbies[idx].get();

				CryFixedStringT<32> versionString = gEnv->pSystem->GetProductVersion().ToString();

				pLobby->SetData("version", versionString.c_str());

				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] [Lobby] Created lobby using room id %" PRIu64 " with game version: %s", roomId, versionString.c_str());

				return pLobby;
			}

			// Callback for AsyncCreateRoom()
			void CMatchmaking::OnCreateLobby(const rail::rail_event::CreateRoomResult* pRoomInfo)
			{
				if (pRoomInfo->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Create lobby failure : %s", Helper::ErrorString(pRoomInfo->result));
					return;
				}

				if (CUserLobby* const pLobby = AllocateUserLobby(pRoomInfo->room_id))
				{
					for (IListener* pListener : m_listeners)
					{
						pListener->OnCreatedLobby(pLobby);
						pListener->OnJoinedLobby(pLobby);
					}
				}
			}

			// Callback for AsyncGetRoomListInZone()
			void CMatchmaking::OnLobbyMatchList(const rail::rail_event::GetRoomListResult* pRoomList)
			{
				if (pRoomList->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Retrieving lobby list failure : %s", Helper::ErrorString(pRoomList->result));
					return;
				}

				m_lastQueriedRoomInfo = pRoomList->room_infos;

				for (IListener* pListener : m_listeners)
				{
					pListener->OnLobbyQueryComplete(m_lastQueriedRoomInfo.size());
				}
			}

			// Callback for AsyncJoinRoom()
			void CMatchmaking::OnLocalJoin(const rail::rail_event::JoinRoomResult* pJoinInfo)
			{
				if (pJoinInfo->result == rail::kSuccess)
				{
					if (CUserLobby* const pLobby = AllocateUserLobby(pJoinInfo->room_id))
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnJoinedLobby(pLobby);
						}
					}
					else
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnLobbyJoinFailed(CreateLobbyIdentifier(pJoinInfo->room_id));
						}
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Join Lobby failure : %s", Helper::ErrorString(pJoinInfo->result));

					for (IListener* pListener : m_listeners)
					{
						pListener->OnLobbyJoinFailed(CreateLobbyIdentifier(pJoinInfo->room_id));
					}
				}
			}

			// Triggered when local player responds to a friend invite
 			void CMatchmaking::OnInvited(const rail::rail_event::RailUsersRespondInvitation* pInvite)
 			{
				if (pInvite->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Friend invite failure : %s", Helper::ErrorString(pInvite->result));
					return;
				}

				if (pInvite->response != rail::kRailInviteResponseTypeAccepted)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] Friend invite failure : response was '%s' (expected: '%s')",
						Helper::EnumString(pInvite->response),
						Helper::EnumString(rail::kRailInviteResponseTypeAccepted));

					return;
				}

				if (pInvite->game_id != Helper::GameID() || pInvite->original_invite_option.invite_type != rail::kRailUsersInviteTypeRoom)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][Matchmaking] Received unexpected invite: game_id='%" PRIu64 "', invite_type='%s'. Expected: '%" PRIu64 "', '%s'.",
						pInvite->game_id.get_id(),
						Helper::EnumString(pInvite->original_invite_option.invite_type),
						Helper::GameID().get_id(),
						Helper::EnumString(rail::kRailUsersInviteTypeRoom));
				}

				if (rail::IRailUsersHelper* pUsers = Helper::UsersHelper())
				{
					CryLogAlways("[Rail][Matchmaking] Invite of type '%s' received from '%" PRIu64 "'",
						Helper::EnumString(pInvite->original_invite_option.invite_type),
						pInvite->inviter_id.get_id());

					// Response is received in callback: OnJoinRequested()
					const rail::RailResult res = pUsers->AsyncGetInviteDetail(pInvite->inviter_id, pInvite->original_invite_option.invite_type, "");
					if (res != rail::kSuccess)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] AsyncGetInviteDetail failure : %s", Helper::ErrorString(res));
					}
				}
			}

			// Callback for AsyncGetInviteDetail()
			void CMatchmaking::OnJoinRequested(const rail::rail_event::RailUsersGetInviteDetailResult* pInvite)
			{
				if (pInvite->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Get invite details failure : %s", Helper::ErrorString(pInvite->result));
					return;
				}

				if (pInvite->invite_type != rail::kRailUsersInviteTypeRoom)
				{
					CRY_ASSERT(false, "[Rail][Matchmaking] Only 'Room' invite type is supported!");
					return;
				}

				CryLogAlways("[Rail][Matchmaking] OnJoinRequested() with command line: '%s'", pInvite->command_line.c_str());

				stack_string cmdLine(pInvite->command_line.c_str());

				const char* const szArgSeparators = " -\"=+";
				const char* const szValSeparators = " =";
				int curPos = 0;
				stack_string roomId;
				stack_string password;

				stack_string arg = cmdLine.Tokenize(szArgSeparators, curPos);
				while (!arg.empty())
				{
					if (arg == "rail_connect_to_roomid")
					{
						roomId = cmdLine.Tokenize(szValSeparators, curPos);
					}
					else if (arg == "rail_room_password")
					{
						password = cmdLine.Tokenize(szValSeparators, curPos);
					}

					arg = cmdLine.Tokenize(szArgSeparators, curPos);
				}

				CRY_ASSERT(!roomId.empty(), "[Rail][Matchmaking] Command line parsing failed!");
				const uint64_t railRoomId = Schematyc2::StringUtils::UInt64FromString(roomId.c_str(), 0);
				JoinLobby(railRoomId, password.c_str());
			}

			void CMatchmaking::OnRoomLeft(const rail::rail_event::LeaveRoomResult* pLeaveInfo)
			{
				auto pos = std::find_if(m_roomInstances.begin(), m_roomInstances.end(), [pLeaveInfo](rail::IRailRoom* pRoom) { return pRoom->GetRoomID() == pLeaveInfo->room_id; });
				if (pos != m_roomInstances.end())
				{
					rail::IRailRoom* pRoomToRelease = *pos;
					m_roomInstances.erase(pos);

					for (auto it = m_lobbies.begin(); it != m_lobbies.end(); ++it)
					{
						if (&((*it)->GetRoom()) == pRoomToRelease)
						{
							CRY_ASSERT(false, "[Rail][Matchmaking] Found a User Lobby still associated with room '%" PRIu64 "' that we have already left!", pLeaveInfo->room_id);
							m_lobbies.erase(it);
							break;
						}
					}

					pRoomToRelease->Release();
				}
			}

			void CMatchmaking::OnRailEvent(rail::RAIL_EVENT_ID eventId, rail::EventBase* pEvent)
			{
				if (pEvent->game_id != Helper::GameID())
				{
					return;
				}

				switch (eventId)
				{
					case rail::kRailEventRoomCreated:
						OnCreateLobby(static_cast<const rail::rail_event::CreateRoomResult*>(pEvent));
						break;
					case rail::kRailEventRoomGetRoomListResult:
						OnLobbyMatchList(static_cast<const rail::rail_event::GetRoomListResult*>(pEvent));
						break;
					case rail::kRailEventRoomJoinRoomResult:
						OnLocalJoin(static_cast<const rail::rail_event::JoinRoomResult*>(pEvent));
						break;
					case rail::kRailEventUsersRespondInvitation:
						OnInvited(static_cast<const rail::rail_event::RailUsersRespondInvitation*>(pEvent));
						break;
					case rail::kRailEventRoomLeaveRoomResult:
						OnRoomLeft(static_cast<const rail::rail_event::LeaveRoomResult*>(pEvent));
						break;
					case rail::kRailEventUsersGetInviteDetailResult:
						OnJoinRequested(static_cast<const rail::rail_event::RailUsersGetInviteDetailResult*>(pEvent));
						break;
				}
			}
		}
	}
}