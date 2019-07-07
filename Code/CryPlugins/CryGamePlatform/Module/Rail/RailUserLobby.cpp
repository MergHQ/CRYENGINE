// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "RailUserLobby.h"
#include "RailService.h"
#include "RailHelper.h"

#include <CryGame/IGameFramework.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			CUserLobby::CUserLobby(CService& railService, rail::IRailRoom& room)
				: m_service(railService)
				, m_room(room)
			{
				Helper::RegisterEvent(rail::kRailEventRoomNotifyRoomDataReceived, *this);
				Helper::RegisterEvent(rail::kRailEventRoomNotifyMetadataChanged, *this);
				Helper::RegisterEvent(rail::kRailEventRoomGetAllDataResult, *this);
				Helper::RegisterEvent(rail::kRailEventRoomNotifyMemberChanged, *this);
				Helper::RegisterEvent(rail::kRailEventRoomNotifyMemberkicked, *this);
				Helper::RegisterEvent(rail::kRailEventFriendsMetadataChanged, *this);
				Helper::RegisterEvent(rail::kRailEventUsersNotifyInviter, *this);
				Helper::RegisterEvent(rail::kRailEventUsersInviteJoinGameResult, *this);
				Helper::RegisterEvent(rail::kRailEventRoomSetRoomMetadataResult, *this);
				Helper::RegisterEvent(rail::kRailEventRoomGetRoomMetadataResult, *this);
				Helper::RegisterEvent(rail::kRailEventRoomGetMemberMetadataResult, *this);
				Helper::RegisterEvent(rail::kRailEventRoomSetMemberMetadataResult, *this);

				if (rail::IRailFriends* pFriends = Helper::Friends())
				{
					stack_string cmdLine;
					cmdLine.Format("--rail_connect_to_roomid=%" PRIu64 " --rail_room_password=\"\" +connect_lobby %" PRIu64, m_room.GetRoomID(), m_room.GetRoomID());

					const rail::RailResult res = pFriends->AsyncSetInviteCommandLine(cmdLine.c_str(), "");
					if (res != rail::kSuccess)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncSetInviteCommandLine() failed : %s", Helper::ErrorString(res));
					}
				}
			}

			CUserLobby::~CUserLobby()
			{
				Helper::UnregisterEvent(rail::kRailEventRoomNotifyRoomDataReceived, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomNotifyMetadataChanged, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomGetAllDataResult, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomNotifyMemberChanged, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomNotifyMemberkicked, *this);
				Helper::UnregisterEvent(rail::kRailEventFriendsMetadataChanged, *this);
				Helper::UnregisterEvent(rail::kRailEventUsersNotifyInviter, *this);
				Helper::UnregisterEvent(rail::kRailEventUsersInviteJoinGameResult, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomSetRoomMetadataResult, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomGetRoomMetadataResult, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomGetMemberMetadataResult, *this);
				Helper::UnregisterEvent(rail::kRailEventRoomSetMemberMetadataResult, *this);

				// Just in case loading gets aborted
				gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

				Leave();
			}

			void CUserLobby::RemoveListener(IListener& listener)
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_COMMENT, "[Rail][UserLobby] Removed listener %X", &listener);
				stl::find_and_erase(m_listeners, &listener);
			}

			void CUserLobby::AddListener(IListener& listener)
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_COMMENT, "[Rail][UserLobby] Added listener %X", &listener);
				m_listeners.push_back(&listener);
			}

			void CUserLobby::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
			{
				if (event != ESYSTEM_EVENT_LEVEL_LOAD_END)
					return;

				if (gEnv->bMultiplayer)
				{
					IServer* pPlatformServer = m_service.GetLocalServer();
					if (pPlatformServer == nullptr)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Failed to create server!");
						return;
					}

					if (!pPlatformServer->IsLocal())
						m_serverIP = pPlatformServer->GetPublicIP();

					m_serverPort = pPlatformServer->GetPort();
					m_serverId = pPlatformServer->GetIdentifier();

					const int NBYTES = 4;
					uint8 octet[NBYTES];
					char ipAddressFinal[15];
					for (int i = 0; i < NBYTES; i++)
					{
						octet[i] = m_serverIP >> (i*  8);
					}
					cry_sprintf(ipAddressFinal, 15, "%d.%d.%d.%d", octet[3], octet[2], octet[1], octet[0]);

					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][UserLobby] Created server at %s:%i", ipAddressFinal, m_serverPort);

					stack_string tmpStr;

					tmpStr.Format("%i", m_serverIP);
					SetValueForKey(m_roomDataCache, "ipaddress", tmpStr.c_str());

					tmpStr.Format("%i", m_serverPort);
					SetValueForKey(m_roomDataCache, "port", tmpStr.c_str());

					tmpStr.Format("%i", m_serverId);
					SetValueForKey(m_roomDataCache, "serverid", tmpStr.c_str());

					const rail::RailResult res = m_room.AsyncSetRoomMetadata(m_roomDataCache, "");
					if (res != rail::kSuccess)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Failed to set room metadata : %s", Helper::ErrorString(res));
					}

					gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
				}
			}

			/*static*/ size_t CUserLobby::FindIndexForKey(const KeyValueArray& kva, const char* szKey)
			{
				const size_t count = kva.size();

				for (size_t idx = 0; idx < count; ++idx)
				{
					const rail::RailKeyValue& kvp = kva[idx];
					if (kvp.key == szKey)
					{
						return idx;
					}
				}
				
				return count;
			}

			/*static*/ const char* CUserLobby::FindValueForKey(const KeyValueArray& kva, const char* szKey)
			{
				const size_t idx = FindIndexForKey(kva, szKey);
				if (idx < kva.size())
				{
					return kva[idx].value.c_str();
				}

				return "";
			}

			/*static*/ void CUserLobby::SetValueForKey(KeyValueArray& kva, const char* szKey, const char* szValue)
			{
				const size_t idx = FindIndexForKey(kva, szKey);
				if (idx < kva.size())
				{
					kva[idx].value = szValue;
				}
				else
				{
					kva.push_back(rail::RailKeyValue{ szKey, szValue });
				}
			}

			bool CUserLobby::SetData(const char* key, const char* value)
			{
				SetValueForKey(m_roomDataCache, key, value);

				const rail::RailResult res = m_room.AsyncSetRoomMetadata(m_roomDataCache, "");
				if (res != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Failed to set room metadata : %s", Helper::ErrorString(res));
					return false;
				}

				return true;
			}

			const char* CUserLobby::GetData(const char* szKey) const
			{
				const char* szData = nullptr;

				const size_t idx = FindIndexForKey(m_roomDataCache, szKey);
				if (idx < m_roomDataCache.size())
				{
					szData = m_roomDataCache[idx].value.c_str();
				}
				
				rail::RailArray<rail::RailString> keys;
				keys.push_back(szKey);
				const rail::RailResult res = m_room.AsyncGetRoomMetadata(keys, "");
				if (res != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Failed to get room metadata : %s", Helper::ErrorString(res));
				}

				return szData;
			}

			const char* CUserLobby::GetMemberData(const AccountIdentifier& userId, const char* szKey)
			{
				const char* szData = nullptr;
				
				const rail::RailID railUserId = ExtractRailID(userId);
				KeyValueArray& memberData = m_memberDataCache[railUserId.get_id()];

				const size_t idx = FindIndexForKey(memberData, szKey);
				if (idx < memberData.size())
				{
					szData = memberData[idx].value.c_str();
				}
				
				rail::RailArray<rail::RailString> keys;
				keys.push_back(szKey);

				const rail::RailResult res = m_room.AsyncGetMemberMetadata(railUserId, keys, "");
				if (res != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Failed to get room member metadata : %s", Helper::ErrorString(res));
				}

				return szData;
			}

			void CUserLobby::SetMemberData(const char* szKey, const char* szValue)
			{
				if (CAccount* pLocalAccount = m_service.GetLocalAccount())
				{
					const rail::RailID railUserId = pLocalAccount->GetRailID();
					KeyValueArray& memberData = m_memberDataCache[railUserId.get_id()];
					SetValueForKey(memberData, szKey, szValue);

					const rail::RailResult res = m_room.AsyncSetMemberMetadata(railUserId, memberData, "");
					if (res != rail::kSuccess)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Failed to set member metadata %s='%s'", szKey, szValue);
					}
				}
			}

			bool CUserLobby::HostServer(const char* szLevel, bool isLocal)
			{
				const rail::RailID ownerId = m_room.GetOwnerID();

				// Only owner can decide to host
				CAccount* pLocalAccount = m_service.GetLocalAccount();
				if (!pLocalAccount || ownerId != pLocalAccount->GetRailID())
				{
					return false;
				}

				IServer* pPlatformServer = m_service.CreateServer(isLocal);
				if (pPlatformServer == nullptr)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Failed to create server!");
					return false;
				}

				gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "RailUserLobby");

				gEnv->pConsole->ExecuteString(string().Format("map %s s", szLevel));

				return true;
			}

			int CUserLobby::GetMemberLimit() const
			{
				return m_room.GetMaxMembers();
			}

			int CUserLobby::GetNumMembers() const
			{
				return m_room.GetMembers();
			}

			AccountIdentifier CUserLobby::GetMemberAtIndex(int index) const
			{
				return CreateAccountIdentifier(m_room.GetMemberByIndex(index));
			}

			void CUserLobby::Leave()
			{
				for (IListener* pListener : m_listeners)
				{
					pListener->OnLeave();
				}

				// CMatchmaking is listening for response and will later release the room
				m_room.Leave();
			}

			AccountIdentifier CUserLobby::GetOwnerId() const
			{
				return CreateAccountIdentifier(m_room.GetOwnerID());
			}

			Cry::GamePlatform::LobbyIdentifier CUserLobby::GetIdentifier() const
			{
				return CreateLobbyIdentifier(m_room.GetRoomID());
			}

			bool CUserLobby::SendChatMessage(const char* szMessage) const
			{
				constexpr size_t dataLimit = 1200;
				const size_t dataLen = std::min(strlen(szMessage) + 1, dataLimit);
				const rail::RailResult res = m_room.SendDataToMember(0, szMessage, dataLen);
				if (res == rail::kSuccess)
				{
					return true;
				}

				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] SendChatMessage() failed : %s", Helper::ErrorString(res));
				return false;
			}

			void CUserLobby::ShowInviteDialog() const
			{
				if (rail::IRailFriends* pFriends = Helper::Friends())
				{
					stack_string cmdLine;
					cmdLine.Format("--rail_connect_to_roomid=%" PRIu64 " --rail_room_password=\"\" +connect_rail_lobby %" PRIu64,
						m_room.GetRoomID(),
						m_room.GetRoomID());

					const rail::RailResult res = pFriends->AsyncSetInviteCommandLine(cmdLine.c_str(), "");
					if (res != rail::kSuccess)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncSetInviteCommandLine('%s') failed : %s", cmdLine.c_str(), Helper::ErrorString(res));
					}
				}

				if(rail::IRailFloatingWindow* pFloatingWindow = Helper::FloatingWindow())
				{
					if (!pFloatingWindow->IsFloatingWindowAvailable())
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Floating window is not available!");
						return;
					}

					const rail::RailResult res = pFloatingWindow->AsyncShowRailFloatingWindow(rail::kRailWindowFriendList, "");
					if (res != rail::kSuccess)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncShowRailFloatingWindow() failed : %s", Helper::ErrorString(res));
					}
				}
			}

			// Triggered after friend responded to our invite.
			// NOTE: As of version 1.9.8.0 of the SDK, only triggered if invite is accepted.
			void CUserLobby::OnUsersInviteJoinGameResult(const rail::rail_event::RailUsersInviteJoinGameResult* pResult)
			{
				if (pResult->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Friend invite join failure : %s", Helper::ErrorString(pResult->result));
					return;
				}

				CryComment("[Rail][Matchmaking] Invite Friend Result: invitee_id='%" PRIu64 "' invite_type='%s' response_value='%s'",
					pResult->invitee_id.get_id(),
					Helper::EnumString(pResult->invite_type),
					Helper::EnumString(pResult->response_value));
			}

			void CUserLobby::OnMemberKicked(const rail::rail_event::NotifyRoomMemberKicked* pKicked)
			{
				if (pKicked->room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pKicked->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Member kick failure : %s", Helper::ErrorString(pKicked->result));
					return;
				}

				for (IListener* pListener : m_listeners)
				{
					pListener->OnPlayerKicked(CreateAccountIdentifier(pKicked->kicked_id), CreateAccountIdentifier(pKicked->id_for_making_kick));
				}
			}

			void CUserLobby::OnMemberChange(const rail::rail_event::NotifyRoomMemberChange* pChange)
			{
				if (pChange->room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pChange->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Room member change failure : %s", Helper::ErrorString(pChange->result));
					return;
				}

				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_COMMENT, "[Rail][UserLobby] %s : user %" PRIu64 " %s", __func__, pChange->changer_id, Helper::EnumString(pChange->state_change));

				switch (pChange->state_change)
				{
					case rail::kMemberEnteredRoom:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerEntered(CreateAccountIdentifier(pChange->changer_id));
						}
					}
					break;
					case rail::kMemberLeftRoom:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerLeft(CreateAccountIdentifier(pChange->changer_id));
						}
					}
					break;
					case rail::kMemberDisconnectServer:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerDisconnected(CreateAccountIdentifier(pChange->changer_id));
						}
					}
					break;
				}
			}

			// Response to AsyncSetRoomMetadata()
			void CUserLobby::OnSetRoomMetadata(const rail::rail_event::SetRoomMetadataResult* pResult)
			{
				if (pResult->room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pResult->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncSetRoomMetadata() returned failure : %s", Helper::ErrorString(pResult->result));
				}
			}

			// Response to AsyncGetRoomMetadata()
			void CUserLobby::OnGetRoomMetadata(const rail::rail_event::GetRoomMetadataResult* pMetadata)
			{
				if (pMetadata->room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pMetadata->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncGetRoomMetadata() returned failure : %s", Helper::ErrorString(pMetadata->result));
					return;
				}

				// Returned metadata can (and usually will be) a subset
				for (size_t idx = 0; idx < pMetadata->key_value.size(); ++idx)
				{
					SetValueForKey(m_roomDataCache, pMetadata->key_value[idx].key.c_str(), pMetadata->key_value[idx].value.c_str());
				}

				for (IListener* pListener : m_listeners)
				{
					pListener->OnLobbyDataUpdate(CreateLobbyIdentifier(pMetadata->room_id));
				}
			}

			// Response to AsyncSetMemberMetadata()
			void CUserLobby::OnSetMemberMetadata(const rail::rail_event::SetMemberMetadataResult* pResult)
			{
				if (pResult->room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pResult->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncSetMemberMetadata() returned failure : %s", Helper::ErrorString(pResult->result));
				}
			}

			// // Response to AsyncGetMemberMetadata()
			void CUserLobby::OnGetMemberMetadata(const rail::rail_event::GetMemberMetadataResult* pMetadata)
			{
				if (pMetadata->room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pMetadata->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncGetMemberMetadata() returned failure : %s", Helper::ErrorString(pMetadata->result));
					return;
				}

				KeyValueArray& memberData = m_memberDataCache[pMetadata->member_id.get_id()];

				// Returned metadata can (and usually will be) a subset
				for (size_t idx = 0; idx < pMetadata->key_value.size(); ++idx)
				{
					SetValueForKey(memberData, pMetadata->key_value[idx].key.c_str(), pMetadata->key_value[idx].value.c_str());
				}

				for (IListener* pListener : m_listeners)
				{
					pListener->OnUserDataUpdate(CreateAccountIdentifier(pMetadata->member_id));
				}
			}

			void CUserLobby::OnRoomMetadataChange(const rail::rail_event::NotifyMetadataChange* pMetadata)
			{
				if (pMetadata->room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pMetadata->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Room metadata change failure : %s", Helper::ErrorString(pMetadata->result));
					return;
				}

				CryComment("[Rail][UserLobby] Room %" PRIu64 " metadata changed by %" PRIu64 "", pMetadata->room_id, pMetadata->changer_id);

				const rail::RailResult res = m_room.AsyncGetAllRoomData("");
				if (res != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncGetAllRoomData() return failure : %s", Helper::ErrorString(res));
				}
			}

			void CUserLobby::OnFriendsMetadataChanged(const rail::rail_event::RailFriendsMetadataChanged* pMetadata)
			{
				if (pMetadata->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Friends metadata change failure : %s", Helper::ErrorString(pMetadata->result));
					return;
				}

				const rail::RailArray<rail::RailFriendMetadata>& changedData = pMetadata->friends_changed_metadata;

				for (size_t uIdx = 0; uIdx < changedData.size(); ++uIdx)
				{
					const rail::RailID friendId = changedData[uIdx].friend_rail_id;
					KeyValueArray& memberData = m_memberDataCache[friendId.get_id()];

					const KeyValueArray& changedKvp = changedData[uIdx].metadatas;
					for (size_t dataIdx = 0; dataIdx < changedKvp.size(); ++dataIdx)
					{
						SetValueForKey(memberData, changedKvp[dataIdx].key.c_str(), changedKvp[dataIdx].value.c_str());
					}

					const AccountIdentifier accountId = CreateAccountIdentifier(friendId);
					for (IListener* pListener : m_listeners)
					{
						pListener->OnUserDataUpdate(accountId);
					}
				}
			}

			void CUserLobby::OnDataReceived(const rail::rail_event::RoomDataReceived* pData)
			{
				if (pData->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] Room chat message receive failure : %s", Helper::ErrorString(pData->result));
					return;
				}

				for (IListener* pListener : m_listeners)
				{
					pListener->OnChatMessage(CreateAccountIdentifier(pData->remote_peer), pData->data_buf.c_str());
				}
			}

			// Response to AsyncGetAllRoomData()
			void CUserLobby::OnRoomGetAllDataResult(const rail::rail_event::GetAllRoomDataResult* pRoomData)
			{
				if (pRoomData->room_info.room_id != m_room.GetRoomID())
				{
					return;
				}

				if (pRoomData->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][UserLobby] AsyncGetAllRoomData() returned failure : %s", Helper::ErrorString(pRoomData->result));
					return;
				}

				CryComment("[Rail][UserLobby] Room '%" PRIu64 "' metadata FULL dump:", pRoomData->room_info.room_id);
				CryComment("[Rail][UserLobby] 'name' = '%s'", pRoomData->room_info.room_name.c_str());
				CryComment("[Rail][UserLobby] 'current_members' = '%u'", pRoomData->room_info.current_members);
				CryComment("[Rail][UserLobby] 'max_members' = '%u'", pRoomData->room_info.max_members);
				CryComment("[Rail][UserLobby] 'game_server_rail_id' = '%" PRIu64 "'", pRoomData->room_info.game_server_rail_id);
				CryComment("[Rail][UserLobby] 'is_joinable' = '%s'", pRoomData->room_info.is_joinable ? "true" : "false");
				CryComment("[Rail][UserLobby] 'type' = '%s'", Helper::EnumString(pRoomData->room_info.type));

				for (size_t kvIdx = 0; kvIdx < pRoomData->room_info.room_kvs.size(); ++kvIdx)
				{
					const rail::RailKeyValue& keyValue = pRoomData->room_info.room_kvs[kvIdx];

					CryComment("[Rail][UserLobby] '%s' = '%s'", keyValue.key.c_str(), keyValue.value.c_str());
				}

				for (size_t idx = 0; idx < pRoomData->room_info.room_kvs.size(); ++idx)
				{
					SetValueForKey(m_roomDataCache, pRoomData->room_info.room_kvs[idx].key.c_str(), pRoomData->room_info.room_kvs[idx].value.c_str());
				}

				if (m_serverIP == 0 || m_serverPort == 0 || m_serverId == 0)
				{
					const char* ipString = FindValueForKey(m_roomDataCache, "ipaddress");
					const char* portString = FindValueForKey(m_roomDataCache, "port");
					const char* serverIdString = FindValueForKey(m_roomDataCache, "serverid");

					if (ipString && strlen(ipString) != 0 && portString && strlen(portString) != 0 && serverIdString && strlen(serverIdString) != 0)
					{
						ConnectToServer(atoi(ipString), atoi(portString), atoi(serverIdString));
					}
				}

				for (IListener* pListener : m_listeners)
				{
					pListener->OnLobbyDataUpdate(CreateLobbyIdentifier(pRoomData->room_info.room_id));
				}
			}

			// Triggered right after a friend has been invited (does not include response!)
			void CUserLobby::OnUsersNotifyInviter(const rail::rail_event::RailUsersNotifyInviter* pInvitation)
			{
				if (pInvitation->result != rail::kSuccess)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Rail][Matchmaking] Invitation failure : %s", Helper::ErrorString(pInvitation->result));
					return;
				}

				CryComment("[Rail][Matchmaking] Invitation to friend '%" PRIu64 "' sent.", pInvitation->invitee_id.get_id());
			}

			void CUserLobby::ConnectToServer(uint32 ip, uint16 port, IServer::Identifier serverId)
			{
				if (gEnv->bServer)
					return;

				m_serverIP = ip;
				m_serverPort = port;
				m_serverId = serverId;

				// Ignore the local server
				if (IServer* pServer = m_service.GetLocalServer())
				{
					if (pServer->GetIdentifier() == serverId)
						return;
				}

				char conReq[2];
				conReq[0] = 'C';
				conReq[1] = 'T';
				m_service.GetNetworking()->SendPacket(GetOwnerId(), &conReq, 2);

				IGameFramework* pGameFramework = gEnv->pGameFramework;
				pGameFramework->EndGameContext();

				const int NBYTES = 4;
				uint8 octet[NBYTES];
				char ipAddressFinal[15];
				for (int i = 0; i < NBYTES; i++)
				{
					octet[i] = m_serverIP >> (i*  8);
				}
				cry_sprintf(ipAddressFinal, 15, "%d.%d.%d.%d", octet[3], octet[2], octet[1], octet[0]);

				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Rail][UserLobby] Lobby game created, connecting to %s:%i", ipAddressFinal, m_serverPort);
					gEnv->pConsole->ExecuteString(string().Format("connect %s %i", ipAddressFinal, m_serverPort));
				}
			}

			void CUserLobby::OnRailEvent(rail::RAIL_EVENT_ID eventId, rail::EventBase* pEvent)
			{
				if (pEvent->game_id != Helper::GameID())
				{
					return;
				}

				switch (eventId)
				{
					case rail::kRailEventRoomNotifyRoomDataReceived:
						OnDataReceived(static_cast<const rail::rail_event::RoomDataReceived*>(pEvent));
						break;
					case rail::kRailEventRoomNotifyMetadataChanged:
						OnRoomMetadataChange(static_cast<const rail::rail_event::NotifyMetadataChange*>(pEvent));
						break;
					case rail::kRailEventRoomGetAllDataResult:
						OnRoomGetAllDataResult(static_cast<const rail::rail_event::GetAllRoomDataResult*>(pEvent));
						break;
					case rail::kRailEventRoomNotifyMemberChanged:
						OnMemberChange(static_cast<const rail::rail_event::NotifyRoomMemberChange*>(pEvent));
						break;
					case rail::kRailEventRoomNotifyMemberkicked:
						OnMemberKicked(static_cast<const rail::rail_event::NotifyRoomMemberKicked*>(pEvent));
						break;
					case rail::kRailEventFriendsMetadataChanged:
						OnFriendsMetadataChanged(static_cast<const rail::rail_event::RailFriendsMetadataChanged*>(pEvent));
						break;
					case rail::kRailEventUsersNotifyInviter:
						OnUsersNotifyInviter(static_cast<const rail::rail_event::RailUsersNotifyInviter*>(pEvent));
						break;
					case rail::kRailEventUsersInviteJoinGameResult:
						OnUsersInviteJoinGameResult(static_cast<const rail::rail_event::RailUsersInviteJoinGameResult*>(pEvent));
						break;
					case rail::kRailEventRoomSetRoomMetadataResult:
						OnSetRoomMetadata(static_cast<const rail::rail_event::SetRoomMetadataResult*>(pEvent));
						break;
					case rail::kRailEventRoomGetRoomMetadataResult:
						OnGetRoomMetadata(static_cast<const rail::rail_event::GetRoomMetadataResult*>(pEvent));
						break;
					case rail::kRailEventRoomSetMemberMetadataResult:
						OnSetMemberMetadata(static_cast<const rail::rail_event::SetMemberMetadataResult*>(pEvent));
						break;
					case rail::kRailEventRoomGetMemberMetadataResult:
						OnGetMemberMetadata(static_cast<const rail::rail_event::GetMemberMetadataResult*>(pEvent));
						break;
				}
			}
		}
}
}