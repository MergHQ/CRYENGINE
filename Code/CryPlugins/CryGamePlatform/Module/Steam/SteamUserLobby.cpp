// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "SteamUserLobby.h"
#include "SteamService.h"
#include "SteamUserIdentifier.h"

#include <CryGame/IGameFramework.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			/* Convert the character string in "ip" into an unsigned integer.

			   This assumes that an unsigned integer contains at least 32 bits.*/
			unsigned int ip_to_int(const char*  ip)
			{
				/* The return value.*/
				unsigned v = 0;
				/* The count of the number of bytes processed.*/
				int i;
				/* A pointer to the next digit to process.*/
				const char*  start;

				start = ip;
				for (i = 0; i < 4; i++) {
					/* The digit being processed.*/
					char c;
					/* The value of this byte.*/
					int n = 0;
					while (1) {
						c =* start;
						start++;
						if (c >= '0' && c <= '9') {
							n *= 10;
							n += c - '0';
						}
						/* We insist on stopping at "." if we are still parsing
						   the first, second, or third numbers. If we have reached
						   the end of the numbers, we will allow any character.*/
						else if ((i < 3 && c == '.') || i == 3) {
							break;
						}
						else {
							return -1;
						}
					}
					if (n >= 256) {
						return -1;
					}
					v *= 256;
					v += n;
				}
				return v;
			}

			CUserLobby::CUserLobby(CService& steamService, const LobbyIdentifier& lobbyId)
				: m_service(steamService)
				, m_steamLobbyId(ExtractSteamID(lobbyId))
				, m_callbackChatDataUpdate(this, &CUserLobby::OnLobbyChatUpdate)
				, m_callbackDataUpdate(this, &CUserLobby::OnLobbyDataUpdate)
				, m_callbackChatMessage(this, &CUserLobby::OnLobbyChatMessage)
				, m_callbackGameCreated(this, &CUserLobby::OnLobbyGameCreated)
				, m_serverIP(0)
				, m_serverPort(0)
				, m_serverId(0)
			{
			}

			CUserLobby::~CUserLobby()
			{
				Leave();

				// Just in case loading gets aborted
				gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
			}

			void CUserLobby::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
			{
				if (event != ESYSTEM_EVENT_LEVEL_LOAD_END)
					return;

				if (gEnv->bMultiplayer)
				{
					CServer* pPlatformServer = m_service.GetLocalServer();
					ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
					if (pPlatformServer == nullptr || pSteamMatchmaking == nullptr)
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Steam] Failed to create server!");
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

					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Steam] Created server at %s:%i", ipAddressFinal, m_serverPort);

					if (pSteamMatchmaking)
					{
						pSteamMatchmaking->SetLobbyGameServer(m_steamLobbyId, m_serverIP, m_serverPort, CSteamID(pPlatformServer->GetIdentifier()));

						string ipAddressInt;
						ipAddressInt.Format("%i", m_serverIP);

						pSteamMatchmaking->SetLobbyData(m_steamLobbyId, "ipaddress", ipAddressInt);

						string port;
						port.Format("%i", m_serverPort);

						pSteamMatchmaking->SetLobbyData(m_steamLobbyId, "port", port);

						string serverIdString;
						serverIdString.Format("%i", m_serverId);

						pSteamMatchmaking->SetLobbyData(m_steamLobbyId, "serverid", serverIdString);
					}
					else
					{
						CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Steam matchmaking service not available");
					}

					gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
				}
			}

			bool CUserLobby::SetData(const char* key, const char* value)
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					return pSteamMatchmaking->SetLobbyData(m_steamLobbyId, key, value);
				}
				return false;
			}

			const char* CUserLobby::GetData(const char* key) const
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					return pSteamMatchmaking->GetLobbyData(m_steamLobbyId, key);
				}
				return nullptr;
			}

			const char* CUserLobby::GetMemberData(const AccountIdentifier& userId, const char* szKey)
			{
				const CSteamID steamUserId = ExtractSteamID(userId);

				const char* szMemberData = nullptr;

				// https://partner.steamgames.com/doc/api/ISteamMatchmaking#LobbyDataUpdate_t
				if (m_steamLobbyId != steamUserId)
				{
					ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
					if (pSteamMatchmaking)
					{
						szMemberData = pSteamMatchmaking->GetLobbyMemberData(m_steamLobbyId, steamUserId, szKey);
					}
				}
				else
				{
					szMemberData = GetData(szKey);
				}

				// https://partner.steamgames.com/doc/api/ISteamMatchmaking#GetLobbyMemberData
				if (szMemberData != nullptr && szMemberData[0] != '\0')
				{
					return szMemberData;
				}
				return nullptr;
			}

			void CUserLobby::SetMemberData(const char* szKey, const char* szValue)
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					pSteamMatchmaking->SetLobbyMemberData(m_steamLobbyId, szKey, szValue);
				}
			}

			bool CUserLobby::HostServer(const char* szLevel, bool isLocal)
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (!pSteamMatchmaking)
				{
					return false;
				}
				CSteamID ownerId = pSteamMatchmaking->GetLobbyOwner(m_steamLobbyId);
				// Only owner can decide to host
				ISteamUser* pSteamUser = SteamUser();
				if (!pSteamUser || ownerId != pSteamUser->GetSteamID())
					return false;

				CServer* pPlatformServer = m_service.CreateServer(isLocal);
				if (pPlatformServer == nullptr)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to create server!");
					return false;
				}

				gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "SteamUserLobby");

				gEnv->pConsole->ExecuteString(string().Format("map %s s", szLevel));

				return true;
			}

			int CUserLobby::GetMemberLimit() const
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					return pSteamMatchmaking->GetLobbyMemberLimit(m_steamLobbyId);
				}
				return 0;
			}

			int CUserLobby::GetNumMembers() const
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					return pSteamMatchmaking->GetNumLobbyMembers(m_steamLobbyId);
				}
				return 0;
			}

			AccountIdentifier CUserLobby::GetMemberAtIndex(int index) const
			{
				CSteamID result = k_steamIDNil;
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					result = pSteamMatchmaking->GetLobbyMemberByIndex(m_steamLobbyId, index);
				}

				return CreateAccountIdentifier(result);
			}

			void CUserLobby::Leave()
			{
				const CSteamID invalidLobby = k_steamIDNil;
				if (m_steamLobbyId != invalidLobby)
				{
					for (IListener* pListener : m_listeners)
					{
						pListener->OnLeave();
					}

					ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
					const bool isShuttingDown = pSteamMatchmaking == nullptr;
					if (!isShuttingDown)
					{
						pSteamMatchmaking->LeaveLobby(m_steamLobbyId);
					}
					m_steamLobbyId = invalidLobby;

					ISteamUser* pSteamUser = SteamUser();
					if (pSteamUser && m_serverIP != 0)
					{
						pSteamUser->TerminateGameConnection(m_serverIP, m_serverPort);
					}

					if (!isShuttingDown)
					{
						if (CMatchmaking* pMatchmaking = m_service.GetMatchmaking())
						{
							pMatchmaking->OnLobbyRemoved(this);
						}
					}
				}
			}

			AccountIdentifier CUserLobby::GetOwnerId() const
			{
				CSteamID result = k_steamIDNil;
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					result = pSteamMatchmaking->GetLobbyOwner(m_steamLobbyId);
				}

				return CreateAccountIdentifier(result);
			}

			bool CUserLobby::SendChatMessage(const char* message) const
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					return pSteamMatchmaking->SendLobbyChatMsg(m_steamLobbyId, message, strlen(message) + 1);
				}
				return false;
			}

			void CUserLobby::ShowInviteDialog() const
			{
				if (ISteamFriends* pSteamFriends = SteamFriends())
				{
					pSteamFriends->ActivateGameOverlayInviteDialog(m_steamLobbyId);
				}
			}

			void CUserLobby::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback)
			{
				if (CSteamID(pCallback->m_ulSteamIDLobby) != m_steamLobbyId)
					return;

				switch (pCallback->m_rgfChatMemberStateChange)
				{
					case k_EChatMemberStateChangeEntered:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerEntered(CreateAccountIdentifier(pCallback->m_ulSteamIDUserChanged));
						}
					}
					break;
					case k_EChatMemberStateChangeLeft:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerLeft(CreateAccountIdentifier(pCallback->m_ulSteamIDUserChanged));
						}
					}
					break;
					case k_EChatMemberStateChangeDisconnected:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerDisconnected(CreateAccountIdentifier(pCallback->m_ulSteamIDUserChanged));
						}
					}
					break;
					case k_EChatMemberStateChangeKicked:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerKicked(CreateAccountIdentifier(pCallback->m_ulSteamIDUserChanged),
							                          CreateAccountIdentifier(pCallback->m_ulSteamIDMakingChange));
						}
					}
					break;
					case k_EChatMemberStateChangeBanned:
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnPlayerBanned(CreateAccountIdentifier(pCallback->m_ulSteamIDUserChanged),
							                          CreateAccountIdentifier(pCallback->m_ulSteamIDMakingChange));
						}
					}
					break;
				}
			}

			void CUserLobby::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback)
			{
				if (!pCallback->m_bSuccess)
					return;

				if (m_serverIP == 0 || m_serverPort == 0 || m_serverId == 0)
				{
					const char* ipString = GetData("ipaddress");
					const char* portString = GetData("port");
					const char* serverIdString = GetData("serverid");

					if (ipString && strlen(ipString) != 0 && portString && strlen(portString) != 0 && serverIdString && strlen(serverIdString) != 0)
					{
						ConnectToServer(atoi(ipString), atoi(portString), atoi(serverIdString));
					}
				}

				if (pCallback->m_ulSteamIDLobby == pCallback->m_ulSteamIDMember)
				{
					for (IListener* pListener : m_listeners)
					{
						pListener->OnLobbyDataUpdate(CreateLobbyIdentifier(pCallback->m_ulSteamIDLobby));
					}
				}
				else
				{
					for (IListener* pListener : m_listeners)
					{
						pListener->OnUserDataUpdate(CreateAccountIdentifier(pCallback->m_ulSteamIDMember));
					}
				}
			}

			void CUserLobby::OnLobbyChatMessage(LobbyChatMsg_t* pCallback)
			{
				if (CSteamID(pCallback->m_ulSteamIDLobby) != m_steamLobbyId)
					return;

				CSteamID userId;
				EChatEntryType entryType;
				char data[2048];
				int cubData = sizeof(data);

				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (!pSteamMatchmaking)
				{
					return;
				}
				int numBytes = pSteamMatchmaking->GetLobbyChatEntry(m_steamLobbyId, pCallback->m_iChatID, &userId, data, cubData, &entryType);
				if (entryType == k_EChatEntryTypeChatMsg)
				{
					string message(data, numBytes);

					if (message.size() > 0)
					{
						for (IListener* pListener : m_listeners)
						{
							pListener->OnChatMessage(CreateAccountIdentifier(userId), message);
						}
					}
				}
			}

			void CUserLobby::OnLobbyGameCreated(LobbyGameCreated_t* pCallback)
			{
				ISteamUser* pSteamUser = SteamUser();
				if (pSteamUser)
				{
					for (IListener* pListener : m_listeners)
					{
						pListener->OnGameCreated(pCallback->m_ulSteamIDGameServer, pCallback->m_unIP, pCallback->m_usPort, CreateAccountIdentifier(pSteamUser->GetSteamID()) == GetOwnerId());
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Steam user service not available");
				}

				if (!gEnv->bServer)
				{
					ConnectToServer(pCallback->m_unIP, pCallback->m_usPort, pCallback->m_ulSteamIDGameServer);
				}
			}

			void CUserLobby::ConnectToServer(uint32 ip, uint16 port, IServer::Identifier serverId)
			{
				if (gEnv->bServer)
					return;

				m_serverIP = ip;
				m_serverPort = port;
				m_serverId = serverId;

				// Ignore the local server
				if (CServer* pServer = m_service.GetLocalServer())
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
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Steam] Lobby game created, connecting to %s:%i", ipAddressFinal, m_serverPort);
					gEnv->pConsole->ExecuteString(string().Format("connect %s %i", ipAddressFinal, m_serverPort));
				}
			}
		}
	}
}