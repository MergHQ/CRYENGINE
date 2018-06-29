// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "SteamMatchmaking.h"
#include "SteamService.h"
#include "SteamUserIdentifier.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			CMatchmaking::CMatchmaking(CService& steamService)
				: m_service(steamService)
				, m_callbackJoinRequested(this, &CMatchmaking::OnJoinRequested)
				, m_callbackGameServerChangeRequested(this, &CMatchmaking::OnGameServerChangeRequested)
				, m_callbackInvited(this, &CMatchmaking::OnInvited)
			{
			}

			void CMatchmaking::CreateLobby(IUserLobby::EVisbility visibility, int maxMemberCount)
			{
				if (ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking())
				{
					SteamAPICall_t hSteamAPICall = pSteamMatchmaking->CreateLobby((ELobbyType)visibility, maxMemberCount);
					m_callResultCreateLobby.Set(hSteamAPICall, this, &CMatchmaking::OnCreateLobby);

					m_service.SetAwaitingCallback(1);
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

			CUserLobby* CMatchmaking::GetLobbyById(const LobbyIdentifier& id)
			{
				return TryGetLobby(id);
			}

			void CMatchmaking::QueryLobbies()
			{
				CryFixedStringT<32> versionString = gEnv->pSystem->GetProductVersion().ToString();

				// Always query lobbies for this version only
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Steam] [Lobby] Looking for lobby with version: %s", versionString.c_str());
#ifdef RELEASE
				AddLobbyQueryFilterString("version", versionString.c_str(), IUserLobby::EComparison::Equal);
#endif
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					SteamAPICall_t hSteamAPICall = pSteamMatchmaking->RequestLobbyList();
					m_callResultLobbyMatchList.Set(hSteamAPICall, this, &CMatchmaking::OnLobbyMatchList);

					m_service.SetAwaitingCallback(1);
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Steam matchmaking service not available");
					LobbyMatchList_t temp;
					temp.m_nLobbiesMatching = 0;
					m_service.SetAwaitingCallback(1); // since OnLobbyMatchList will expect to be used as a callback
					OnLobbyMatchList(&temp, true);
				}
			}

			LobbyIdentifier CMatchmaking::GetQueriedLobbyIdByIndex(int index) const
			{
				CSteamID result = k_steamIDNil;
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					result = pSteamMatchmaking->GetLobbyByIndex(index);
				}
				return CreateLobbyIdentifier(result);
			}

			void CMatchmaking::AddLobbyQueryFilterString(const char* key, const char* value, IUserLobby::EComparison comparison)
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					pSteamMatchmaking->AddRequestLobbyListStringFilter(key, value, static_cast<ELobbyComparison>(comparison));
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Steam matchmaking service not available");
				}
			}

			void CMatchmaking::JoinLobby(const LobbyIdentifier& lobbyId)
			{
				ISteamMatchmaking* pSteamMatchmaking = SteamMatchmaking();
				if (pSteamMatchmaking)
				{
					SteamAPICall_t hSteamAPICall = pSteamMatchmaking->JoinLobby(ExtractSteamID(lobbyId));
					m_callResultLobbyEntered.Set(hSteamAPICall, this, &CMatchmaking::OnJoin);

					m_service.SetAwaitingCallback(1);
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Steam matchmaking service not available");
					LobbyEnter_t temp;
					temp.m_EChatRoomEnterResponse = k_EChatRoomEnterResponseError;
					temp.m_ulSteamIDLobby = k_steamIDNil.ConvertToUint64();
					m_service.SetAwaitingCallback(1); // since onJoin will expect to be used as a callback
					OnJoin(&temp, true);
				}
			}

			void CMatchmaking::OnLobbyRemoved(CUserLobby* pLobby)
			{
				for (auto it = m_lobbies.begin(); it != m_lobbies.end(); ++it)
				{
					if (it->get() == pLobby)
					{
						m_lobbies.erase(it);
						return;
					}
				}
			}

			CUserLobby* CMatchmaking::TryGetLobby(const LobbyIdentifier& id)
			{
				for (const std::unique_ptr<CUserLobby>& pLobby : m_lobbies)
				{
					if (pLobby->GetIdentifier() == id)
					{
						return pLobby.get();
					}
				}

				m_lobbies.emplace_back(stl::make_unique<CUserLobby>(m_service, id));
				return m_lobbies.back().get();
			}

			CUserLobby* CMatchmaking::TryGetLobby(const CSteamID& id)
			{
				return TryGetLobby(CreateLobbyIdentifier(id));
			}

			// Steam callbacks
			void CMatchmaking::OnCreateLobby(LobbyCreated_t* pResult, bool bIOFailure)
			{
				if (pResult->m_eResult == k_EResultOK)
				{
					CryFixedStringT<32> versionString = gEnv->pSystem->GetProductVersion().ToString();

					CUserLobby* pLobby = TryGetLobby(CSteamID(pResult->m_ulSteamIDLobby));
					pLobby->SetData("version", versionString.c_str());

					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Steam] [Lobby] Creating lobby with game version: %s", versionString.c_str());

					for (IListener* pListener : m_listeners)
					{
						pListener->OnCreatedLobby(pLobby);
						pListener->OnJoinedLobby(pLobby);
					}
				}

				// Remove callback so that we don't call update all the time
				m_service.SetAwaitingCallback(-1);
			}

			void CMatchmaking::OnLobbyMatchList(LobbyMatchList_t* pLobbyMatchList, bool bIOFailure)
			{
				m_service.SetAwaitingCallback(-1);

				for (IListener* pListener : m_listeners)
				{
					pListener->OnLobbyQueryComplete(pLobbyMatchList->m_nLobbiesMatching);
				}
			}

			void CMatchmaking::OnJoinRequested(GameLobbyJoinRequested_t* pCallback)
			{
				JoinLobby(CreateLobbyIdentifier(pCallback->m_steamIDLobby));
			}

			void CMatchmaking::OnJoin(LobbyEnter_t* pCallback, bool bIOFailure)
			{
				m_service.SetAwaitingCallback(-1);

				if (pCallback->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess)
				{
					// Make sure CUserLobby object exists on this client
					CUserLobby* pLobby = TryGetLobby(pCallback->m_ulSteamIDLobby);

					for (IListener* pListener : m_listeners)
					{
						pListener->OnJoinedLobby(pLobby);
					}
				}
				else
				{
					for (IListener* pListener : m_listeners)
					{
						pListener->OnLobbyJoinFailed(CreateLobbyIdentifier(pCallback->m_ulSteamIDLobby));
					}
				}
			}

			void CMatchmaking::OnGameServerChangeRequested(GameServerChangeRequested_t* pCallback)
			{
				string serverStr = string(pCallback->m_rgchServer);
				auto iDivider = serverStr.find_first_of(':');
				// IP address
				if (iDivider != string::npos)
				{
					string ipStr = serverStr.substr(0, iDivider);
					string portStr = serverStr.substr(iDivider + 1, serverStr.length() - iDivider);

					gEnv->pConsole->ExecuteString(string("connect ").append(ipStr).append(" ").append(portStr).c_str());
				}
				else // Web address
				{
					gEnv->pConsole->ExecuteString(string("connect ").append(serverStr));
				}
			}

			void CMatchmaking::OnInvited(LobbyInvite_t* pInvite)
			{
				ISteamUtils* pSteamUtils = SteamUtils();
				if (!pSteamUtils || pInvite->m_ulGameID != pSteamUtils->GetAppID())
					return;

				// TODO: Display confirmation dialog
			}
		}
	}
}