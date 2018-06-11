#include "StdAfx.h"

#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>

#include <steam/isteamuser.h>

#include "SteamPlatform.h"

#include "SteamStatistics.h"
#include "SteamLeaderboards.h"
#include "SteamRemoteStorage.h"
#include "SteamMatchmaking.h"

#include "SteamUser.h"
#include "SteamServer.h"

#include "SteamNetworking.h"
#include "SteamUserLobby.h"

#include <CrySystem/ICmdLine.h>
#include <CryInput/IInput.h>

// Included only once per DLL module.
#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/ISystem.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{

			// Bug Description: (Steamworks 1.44)
			//
			// Note: While the Steam Overlay is open when the application is in "fullscreen" mode, 
			// the Win32 ::ShowCursor() count will be increased by 1 each time the game windows gets focus again after loosing it
			//
			// Example scenario:
			// User opens Steam Overlay (Alt+Shift) when in fullscreen mode
			// User then loses focus of the game window e.g. Alt+Tab
			// User gains back focus of the game window
			// User closes the Steam Overlay
			// The Win32 ::ShowCursor() count will have increased by 1

			class BugFix_SteamOverlay_CursorCount : public IPlugin::IListener, ISystemEventListener
			{
			public:
				BugFix_SteamOverlay_CursorCount(CPlugin* steamPlugin)
					: m_overlayIsActive(false)
					, m_fixCursorBugCount(0)
				{
					if (gEnv->pSystem)
					{
						gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this, "SteamPlatform");
					}

					steamPlugin->AddListener(*this);
				}

				~BugFix_SteamOverlay_CursorCount()
				{
					if (gEnv->pSystem)
					{
						gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
					}
				}

				bool IsFullscreen()
				{
					CRY_ASSERT(gEnv);
					CRY_ASSERT(gEnv->pRenderer || gEnv->IsDedicated());

					bool bFullScreen = false;
					if (gEnv->pRenderer)
					{
						gEnv->pRenderer->EF_Query(EFQ_Fullscreen, bFullScreen);
					}
					return bFullScreen;
				}

				virtual void OnOverlayActivated(bool active)
				{
					m_overlayIsActive = active;

					if (!active && m_fixCursorBugCount)
					{
						// Force decrement the mouse cursor depending how often we lost focus while the steam overlay was active in fullscreen
						for (int i = 0; i < m_fixCursorBugCount; ++i)
						{
							gEnv->pInput->ShowCursor(false);
							CryLogAlways("GamePlatform: Applied Steam cursor bug fix");
						}
						m_fixCursorBugCount = 0;
					}
				}

				virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
				{
					if (event == ESYSTEM_EVENT_CHANGE_FOCUS || event == ESYSTEM_EVENT_GAMEWINDOW_ACTIVATE)
					{
						if (m_overlayIsActive)
						{
							const bool isInFocus = (wparam != 0);
							const bool isFullscreen = IsFullscreen();
							if (isFullscreen && !isInFocus)
							{
								// Increment counter each time we lose focus in fullscreen mode while the overlay is open
								++m_fixCursorBugCount;
							}
						}
					}
				}

			private:
				bool m_overlayIsActive;
				int  m_fixCursorBugCount;
			};

			CPlugin* CPlugin::s_pInstance = nullptr;

			//-----------------------------------------------------------------------------
			// Purpose: callback hook for debug text emitted from the Steam API
			//-----------------------------------------------------------------------------
			extern "C" void __cdecl SteamAPIDebugTextHook(int nSeverity, const char* pchDebugText)
			{
				// if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
				// if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Steam] %s", pchDebugText);
			}

			CPlugin::CPlugin()
				: m_callbackGameOverlayActivated(this, &CPlugin::OnGameOverlayActivated)
				, m_pServer(nullptr)
				, m_awaitingCallbacks(0)
				, m_authTicketHandle(k_HAuthTicketInvalid)
			{
				s_pInstance = this;

				// Map Steam API language codes to engine
				// Full list of Steam API language codes can be found here: https://partner.steamgames.com/doc/store/localization
				m_translationMappings["arabic"] = ILocalizationManager::ePILID_Arabic;
				m_translationMappings["schinese"] = ILocalizationManager::ePILID_ChineseS;
				m_translationMappings["tchinese"] = ILocalizationManager::ePILID_ChineseT;
				m_translationMappings["czech"] = ILocalizationManager::ePILID_Czech;
				m_translationMappings["danish"] = ILocalizationManager::ePILID_Danish;
				m_translationMappings["dutch"] = ILocalizationManager::ePILID_Dutch;
				m_translationMappings["english"] = ILocalizationManager::ePILID_English;
				m_translationMappings["finnish"] = ILocalizationManager::ePILID_Finnish;
				m_translationMappings["french"] = ILocalizationManager::ePILID_French;
				m_translationMappings["german"] = ILocalizationManager::ePILID_German;
				m_translationMappings["italian"] = ILocalizationManager::ePILID_Italian;
				m_translationMappings["japanese"] = ILocalizationManager::ePILID_Japanese;
				m_translationMappings["koreana"] = ILocalizationManager::ePILID_Korean;
				m_translationMappings["norwegian"] = ILocalizationManager::ePILID_Norwegian;
				m_translationMappings["polish"] = ILocalizationManager::ePILID_Polish;
				m_translationMappings["portuguese"] = ILocalizationManager::ePILID_Portuguese;
				m_translationMappings["russian"] = ILocalizationManager::ePILID_Russian;
				m_translationMappings["spanish"] = ILocalizationManager::ePILID_Spanish;
				m_translationMappings["swedish"] = ILocalizationManager::ePILID_Swedish;
				m_translationMappings["turkish"] = ILocalizationManager::ePILID_Turkish;
			}

			bool CPlugin::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
			{
				if (env.IsEditor())
				{
					// Don't use Steam integration in Sandbox
					return true;
				}

				int steam_appId = 0;
				REGISTER_CVAR(steam_appId, steam_appId, VF_REQUIRE_APP_RESTART, "Override Steam application id, only possible during development! Requires application restart.");

#if !defined(RELEASE)
				char buff[MAX_PATH];
				CryGetExecutableFolder(CRY_ARRAY_COUNT(buff), buff);
				const string appIdPath = PathUtil::Make(buff, "steam_appid", "txt");

				FILE* const pSteamAppID = fopen(appIdPath.c_str(), "wt");
				fprintf(pSteamAppID, "%d", steam_appId);
				fclose(pSteamAppID);
#endif // !defined(RELEASE)

				m_isInitialized = SteamAPI_Init();

#if defined(RELEASE)
				// Validate that the application was started through Steam,
				// If this fails we should quit the application
				if (SteamAPI_RestartAppIfNecessary(steam_appId))
				{
					gEnv->pSystem->Quit();
					return true;
				}
#else
				// Remove the file, no longer needed
				remove(appIdPath.c_str());
#endif

				gEnv->pConsole->UnregisterVariable("steam_appId");

				if (!m_isInitialized)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[Steam] SteamApi_Init failed");

#ifdef RELEASE
					return false;
#else
					// Allow silent failure (barring the error message above) in development mode
					// Otherwise we wouldn't be able to start the engine without Steam running
					return true;
#endif
				}

				EnableUpdate(IPlugin::EUpdateStep::MainUpdate, true);

				m_pStatistics = std::make_shared<CStatistics>();
				m_pSteamLeaderboards = std::make_shared<CLeaderboards>();

				m_pMatchmaking = std::make_shared<CMatchmaking>();
				m_pNetworking = std::make_shared<CNetworking>();

				m_pRemoteStorage = std::make_shared<CRemoteStorage>();

				m_steamOverlayFix = std::make_shared<BugFix_SteamOverlay_CursorCount>(this);

				ICmdLine* pCmdLine = gEnv->pSystem->GetICmdLine();

				ISteamApps* pSteamApps = SteamApps();
				if (!pSteamApps)
				{
					return false;
				}

				char betaName[256];
				if (pSteamApps->GetCurrentBetaName(betaName, 256))
				{
					CryLogAlways("[Steam] Running Steam game beta %s", betaName);
				}

				CryLogAlways("[Steam] Successfully initialized Steam API, running build id %i", pSteamApps->GetAppBuildId());

				// Check if user requested to join a lobby right away
				if (const ICmdLineArg* pCmdArg = pCmdLine->FindArg(eCLAT_Post, "connect_lobby"))
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Steam] Received pre-launch join request, loading lobby");
					CSteamID lobbyId = CSteamID((uint64)atoll(pCmdArg->GetValue()));
					if (lobbyId.IsValid())
					{
						m_pMatchmaking->JoinLobby(lobbyId.ConvertToUint64());
					}
				}

				// Set localization according to the settings made in Steam
				const char* szDesiredLanguage = pSteamApps->GetCurrentGameLanguage();
				ILocalizationManager::EPlatformIndependentLanguageID languageId = ILocalizationManager::ePILID_English; // Default to english if we the language is not supported
				const auto remapIt = m_translationMappings.find(szDesiredLanguage);
				if (remapIt != m_translationMappings.end())
				{
					languageId = remapIt->second;
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "[GamePlatform] Language mapping missing for %s", szDesiredLanguage);
				}

				CRY_ASSERT_MESSAGE(languageId != ILocalizationManager::ePILID_MAX_OR_INVALID, "Invalid language ID");
				if (languageId != ILocalizationManager::ePILID_MAX_OR_INVALID)
				{
					const char* szLanguage = gEnv->pSystem->GetLocalizationManager()->LangNameFromPILID(languageId);
					CRY_ASSERT(szLanguage != nullptr && szLanguage[0] != '\0');
					CryLogAlways("Attempting to automatically set user preferred language %s", szLanguage);
					ICVar* pVar = gEnv->pConsole->GetCVar("g_language");
					if (pVar != nullptr)
					{
						// Don't override the language if it was loaded from a config file.
						// We assume that it was set via an ingame-menu in that case.
						if ((pVar->GetFlags() & VF_WASINCONFIG) == 0)
						{
							pVar->Set(szLanguage);
						}
					}

					pVar = gEnv->pConsole->GetCVar("g_languageAudio");
					if (pVar != nullptr)
					{
						// Don't override the language if it was loaded from a config file.
						// We assume that it was set via an ingame-menu in that case.
						if ((pVar->GetFlags() & VF_WASINCONFIG) == 0)
						{
							pVar->Set(szLanguage);
						}
					}
				}
				return true;
			}

			void CPlugin::MainUpdate(float frameTime)
			{
				CRY_PROFILE_FUNCTION(PROFILE_SYSTEM);

				if (m_awaitingCallbacks > 0)
				{
					SteamAPI_RunCallbacks();
				}
			}

			int CPlugin::GetBuildIdentifier() const
			{
				if (ISteamApps* pSteamApps = SteamApps())
				{
					return pSteamApps->GetAppBuildId();
				}
				return 0;
			}

			IUser* CPlugin::GetLocalClient()
			{
				if (ISteamUser* pSteamUser = SteamUser())
				{
					return TryGetUser(pSteamUser->GetSteamID());
				}

				return nullptr;
			}

			IUser* CPlugin::GetUserById(IUser::Identifier id)
			{
				return TryGetUser(id);
			}

			void CPlugin::OnGameOverlayActivated(GameOverlayActivated_t* pData)
			{
				// Upload statistics added during gameplay
				m_pStatistics->Upload();

				for (IListener* pListener : m_listeners)
				{
					pListener->OnOverlayActivated(pData->m_bActive != 0);
				}

				m_awaitingCallbacks -= 1;
			}

			bool CPlugin::OwnsApplication(ApplicationIdentifier id) const
			{
				if (ISteamApps* pSteamApps = SteamApps())
				{
					return pSteamApps->BIsSubscribedApp(id);
				}

				return false;
			}

			ApplicationIdentifier CPlugin::GetApplicationIdentifier() const
			{
				if (ISteamUtils* pSteamUtils = SteamUtils())
				{
					return pSteamUtils->GetAppID();
				}

				return 0;
			}

			bool CPlugin::OpenDialog(EDialog dialog) const
			{
				switch (dialog)
				{
				case EDialog::Friends:
					return OpenDialog("friends");
				case EDialog::Community:
					return OpenDialog("community");
				case EDialog::Players:
					return OpenDialog("players");
				case EDialog::Settings:
					return OpenDialog("settings");
				case EDialog::Group:
					return OpenDialog("officialgamegroup");
				case EDialog::Achievements:
					return OpenDialog("achievements");
				}

				return false;
			}

			bool CPlugin::OpenDialogWithTargetUser(EUserTargetedDialog dialog, IUser::Identifier targetUserId) const
			{
				switch (dialog)
				{
				case EUserTargetedDialog::UserInfo:
					return OpenDialogWithTargetUser("steamid", targetUserId);
				case EUserTargetedDialog::FriendAdd:
					return OpenDialogWithTargetUser("friendadd", targetUserId);
				case EUserTargetedDialog::FriendRemove:
					return OpenDialogWithTargetUser("friendremove", targetUserId);
				case EUserTargetedDialog::FriendRequestAccept:
					return OpenDialogWithTargetUser("friendrequestaccept", targetUserId);
				case EUserTargetedDialog::FriendRequestIgnore:
					return OpenDialogWithTargetUser("friendrequestignore", targetUserId);
				case EUserTargetedDialog::Chat:
					return OpenDialogWithTargetUser("chat", targetUserId);
				case EUserTargetedDialog::JoinTrade:
					return OpenDialogWithTargetUser("jointrade", targetUserId);
				case EUserTargetedDialog::Stats:
					return OpenDialogWithTargetUser("stats", targetUserId);
				case EUserTargetedDialog::Achievements:
					return OpenDialogWithTargetUser("achievements", targetUserId);
				}

				return false;
			}

			bool CPlugin::OpenDialog(const char* szPage) const
			{
				if (ISteamFriends* pSteamFriends = SteamFriends())
				{
					pSteamFriends->ActivateGameOverlay(szPage);
					return true;
				}

				return false;
			}

			bool CPlugin::OpenDialogWithTargetUser(const char* szPage, IUser::Identifier targetUserId) const
			{
				if (ISteamFriends* pSteamFriends = SteamFriends())
				{
					pSteamFriends->ActivateGameOverlayToUser(szPage, targetUserId);
					return true;
				}

				return false;
			}

			bool CPlugin::OpenBrowser(const char* szURL) const
			{
				if (ISteamFriends* pSteamFriends = SteamFriends())
				{
					pSteamFriends->ActivateGameOverlayToWebPage(szURL);
					return true;
				}

				return false;
			}


			bool CPlugin::CanOpenPurchaseOverlay() const
			{
				if (ISteamUtils* pSteamUtils = SteamUtils())
				{
					return pSteamUtils->IsOverlayEnabled();
				}

				return false;
			}

			const DynArray<IUser*>& CPlugin::GetFriends()
			{
				if (ISteamFriends* const pSteamFriends = SteamFriends())
				{
					m_friends.clear();

					constexpr int friendFlags = k_EFriendFlagAll;
					const int friendCount = pSteamFriends->GetFriendCount(friendFlags);
					for (int i = 0; i < friendCount; ++i)
					{
						CSteamID friendId = pSteamFriends->GetFriendByIndex(i, friendFlags);
						m_friends.push_back(TryGetUser(friendId));
					}
				}

				return m_friends;
			}

			bool CPlugin::IsFriendWith(IUser::Identifier otherUserId) const
			{
				if (auto* pSteamFriends = SteamFriends())
				{
					return pSteamFriends->HasFriend(otherUserId, k_EFriendFlagImmediate);
				}

				return false;
			}

			EFriendRelationship CPlugin::GetFriendRelationship(IUser::Identifier otherUserId) const
			{
				if (auto pSteamFriends = SteamFriends())
				{
					switch (pSteamFriends->GetFriendRelationship(otherUserId))
					{
					case k_EFriendRelationshipNone:
						return EFriendRelationship::None;

					case k_EFriendRelationshipRequestRecipient:
						return EFriendRelationship::RequestReceived;

					case k_EFriendRelationshipFriend:
						return EFriendRelationship::Friend;

					case k_EFriendRelationshipRequestInitiator:
						return EFriendRelationship::RequestSent;

					case k_EFriendRelationshipIgnored:
						return EFriendRelationship::Blocked;

					default:
						break;
					}
				}

				return EFriendRelationship::None;
			}

			ApplicationIdentifier CPlugin::GetAppId() const
			{
				if (ISteamUtils* pSteamUtils = SteamUtils())
					return pSteamUtils->GetAppID();
				return k_uAppIdInvalid;
			}

			IServer* CPlugin::CreateServer(bool bLocal)
			{
				if (m_pServer == nullptr)
				{
					m_pServer = std::make_shared<CServer>(bLocal);
				}

				return m_pServer.get();
			}

			IUser* CPlugin::TryGetUser(CSteamID id)
			{
				uint64 cleanId = id.ConvertToUint64();

				for (const std::unique_ptr<IUser>& pUser : m_users)
				{
					if (pUser->GetIdentifier() == cleanId)
					{
						return pUser.get();
					}
				}

				m_users.emplace_back(stl::make_unique<CUser>(id));
				return m_users.back().get();
			}

			bool CPlugin::GetAuthToken(string &tokenOut, int &issuerId)
			{
				char rgchToken[2048];
				uint32 unTokenLen = 0;
				ISteamUser* pSteamUser = SteamUser();
				if (!pSteamUser)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Steam user service not available");
					return false;
				}
				m_authTicketHandle = (uint32)pSteamUser->GetAuthSessionTicket(rgchToken, sizeof(rgchToken), &unTokenLen);

				const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

				for (int i = 0; i < unTokenLen; ++i)
				{
					char const byte = rgchToken[i];

					tokenOut += hex_chars[(byte & 0xF0) >> 4];
					tokenOut += hex_chars[(byte & 0x0F) >> 0];
				}

				return true;
			}

			CRYREGISTER_SINGLETON_CLASS(CPlugin)
		}
	}
}

#include <CryCore/CrtDebugStats.h>