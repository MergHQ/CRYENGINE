// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Platform/CryLibrary.h>
#include <CrySystem/ILocalizationManager.h>
#include <rail/sdk/rail_function_helper.h>

namespace Cry
{
	namespace GamePlatform
	{
		namespace Rail
		{
			//! Collection of boilerplate code often used by the plugin to work with the SDK.
			class Helper
			{
			public:
				//! Loads DLL and retrieves required entry points.
				//! Must be called first, and only once.
				static bool Setup(rail::RailGameID gameId);
				//! Initializes the SDK.
				//! \return Returns true on success.
				static bool Initialize();
				//! Shuts down the SDK and unloads the DLL.
				static void Finalize();

				static void RegisterEvent(rail::RAIL_EVENT_ID eventId, rail::IRailEvent& listener);
				static void UnregisterEvent(rail::RAIL_EVENT_ID eventId, rail::IRailEvent& listener);
				static void FireEvents();

				static rail::IRailApps* Apps();
				static rail::IRailBrowserHelper* BrowserHelper();
				static rail::IRailFloatingWindow* FloatingWindow();
				static rail::IRailFriends* Friends();
				static rail::IRailGame* Game();
				static rail::IRailGameServerHelper* GameServerHelper();
				static rail::IRailNetwork* Network();
				static rail::IRailPlayer* Player();
				static rail::IRailRoomHelper* RoomHelper();
				static rail::IRailSystemHelper* SystemHelper();
				static rail::IRailUsersHelper* UsersHelper();
				static rail::IRailUtils* Utils();
				static rail::IRailZoneServerHelper* ZoneHelper();

				static rail::IRailFactory* Factory();

				static rail::RailID PlayerID();
				static rail::RailGameID GameID();

				//! Logging and troubleshooting.
				//! ##@{
				static const char* ErrorString(rail::RailResult code);
				static const char* EnumString(rail::EnumRoomType value);
				static const char* EnumString(rail::EnumRailUsersInviteType value);
				static const char* EnumString(rail::EnumRailNotifyWindowType value);
				static const char* EnumString(rail::EnumRailWindowType value);
				static const char* EnumString(rail::EnumRailUsersInviteResponseType value);
				static const char* EnumString(rail::EnumRoomMemberActionStatus value);
				//! ##@}.

				//! \return true on success. languageId is not modified if unsuccessful.
				static bool MapLanguageCode(const rail::RailString& code, ILocalizationManager::EPlatformIndependentLanguageID& languageId);

			private:
				template<typename TFPtr>
				static bool GetFptr(const char* szName, TFPtr& pFun);

				static const char* const szLibName;
				static const std::unordered_map<string, ILocalizationManager::EPlatformIndependentLanguageID, stl::hash_stricmp<string>, stl::hash_stricmp<string>> s_translationMappings;
				static std::unordered_map<rail::RailResult, rail::RailString> s_errorCache;

				static HMODULE s_libHandle;

				static rail::helper::RailFuncPtr_RailNeedRestartAppForCheckingEnvironment s_pNeedRestart;
				static rail::helper::RailFuncPtr_RailInitialize s_pInitialize;
				static rail::helper::RailFuncPtr_RailFinalize s_pFinalize;
				static rail::helper::RailFuncPtr_RailRegisterEvent s_pRegisterEvent;
				static rail::helper::RailFuncPtr_RailUnregisterEvent s_pUnregisterEvent;
				static rail::helper::RailFuncPtr_RailFireEvents s_pFireEvents;
				static rail::helper::RailFuncPtr_RailFactory s_pFactory;
				static rail::helper::RailFuncPtr_RailGetSdkVersion s_pGetSdkVersion;
			};

			inline bool Helper::Initialize()
			{
				if (s_pInitialize)
				{
					return s_pInitialize();
				}

				return false;
			}

			inline void Helper::Finalize()
			{
				if (s_pFinalize)
				{
					s_pFinalize();
				}

				if (s_libHandle)
				{
					CryFreeLibrary(s_libHandle);
				}

				s_libHandle = nullptr;
				s_pNeedRestart = nullptr;
				s_pInitialize = nullptr;
				s_pFinalize = nullptr;
				s_pRegisterEvent = nullptr;
				s_pUnregisterEvent = nullptr;
				s_pFireEvents = nullptr;
				s_pFactory = nullptr;
				s_pGetSdkVersion = nullptr;
			}

			template<typename TFPtr>
			inline bool Helper::GetFptr(const char* szName, TFPtr& pFun)
			{
				void* pFunVoid = CryGetProcAddress(s_libHandle, szName);
				CRY_ASSERT(pFunVoid, "[Rail][Helper] Function '%s' not found in '%s'!", szName, szLibName);
				pFun = (TFPtr)pFunVoid;

				return pFun == nullptr;
			}

			inline void Helper::RegisterEvent(rail::RAIL_EVENT_ID eventId, rail::IRailEvent& listener)
			{
				if (s_pRegisterEvent)
				{
					s_pRegisterEvent(eventId, &listener);
				}
			}

			inline void Helper::UnregisterEvent(rail::RAIL_EVENT_ID eventId, rail::IRailEvent& listener)
			{
				if (s_pUnregisterEvent)
				{
					s_pUnregisterEvent(eventId, &listener);
				}
			}

			inline void Helper::FireEvents()
			{
				if (s_pFireEvents)
				{
					s_pFireEvents();
				}
			}

			inline rail::IRailApps* Helper::Apps()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailApps() : nullptr;
			}

			inline rail::IRailBrowserHelper* Helper::BrowserHelper()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailBrowserHelper() : nullptr;
			}

			inline rail::IRailFloatingWindow* Helper::FloatingWindow()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailFloatingWindow() : nullptr;
			}

			inline rail::IRailFriends* Helper::Friends()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailFriends() : nullptr;
			}

			inline rail::IRailGame* Helper::Game()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailGame() : nullptr;
			}

			inline rail::IRailGameServerHelper* Helper::GameServerHelper()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailGameServerHelper() : nullptr;
			}

			inline rail::IRailNetwork* Helper::Network()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailNetworkHelper() : nullptr;
			}

			inline rail::IRailPlayer* Helper::Player()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailPlayer() : nullptr;
			}

			inline rail::IRailRoomHelper* Helper::RoomHelper()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailRoomHelper() : nullptr;
			}

			inline rail::IRailSystemHelper* Helper::SystemHelper()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailSystemHelper() : nullptr;
			}

			inline rail::IRailUsersHelper* Helper::UsersHelper()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailUsersHelper() : nullptr;
			}

			inline rail::IRailUtils* Helper::Utils()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailUtils() : nullptr;
			}

			inline rail::IRailZoneServerHelper* Helper::ZoneHelper()
			{
				rail::IRailFactory* const pFactory = Factory();
				return pFactory ? pFactory->RailZoneServerHelper() : nullptr;
			}

			inline rail::IRailFactory* Helper::Factory()
			{
				return s_pFactory ? s_pFactory() : nullptr;
			}

			inline rail::RailID Helper::PlayerID()
			{
				rail::IRailPlayer* const pPlayer = Player();
				return pPlayer ? pPlayer->GetRailID() : rail::kInvalidRailId;
			}

			inline rail::RailGameID Helper::GameID()
			{
				rail::IRailGame* const pPlayer = Game();
				return pPlayer ? pPlayer->GetGameID() : rail::kInvalidGameId;
			}

			inline const char* Helper::EnumString(rail::EnumRoomType value)
			{
				switch (value)
				{
				case rail::kRailRoomTypePrivate:     return "Private";
				case rail::kRailRoomTypeWithFriends: return "WithFriends";
				case rail::kRailRoomTypePublic:      return "Public";
				case rail::kRailRoomTypeHidden:      return "Hidden";
				}

				return "Unknown Room Type";
			}

			inline const char* Helper::EnumString(rail::EnumRailUsersInviteType value)
			{
				switch (value)
				{
				case rail::kRailUsersInviteTypeGame: return "Game";
				case rail::kRailUsersInviteTypeRoom: return "Room";
				}

				return "Unknown Invite Type";
			}

			inline const char* Helper::EnumString(rail::EnumRailNotifyWindowType value)
			{
				switch (value)
				{
				case rail::kRailNotifyWindowUnknown: return "Unknown";
				case rail::kRailNotifyWindowOverlayPanel: return "Overlay Panel";
				case rail::kRailNotifyWindowUnlockAchievement: return "Achievement Unlocked";
				case rail::kRailNotifyWindowFriendInvite: return "Friend Invite";
				case rail::kRailNotifyWindowAddFriend: return "Add Friend";
				case rail::kRailNotifyWindowAntiAddiction: return "Anti-Addiction";
				}

				return "Unknown Notify";
			}

			inline const char* Helper::EnumString(rail::EnumRailWindowType value)
			{
				switch (value)
				{
				case rail::kRailWindowUnknown:         return "Unknown";
				case rail::kRailWindowFriendList:      return "FriendList";
				case rail::kRailWindowPurchaseProduct: return "PurchaseProduct";
				case rail::kRailWindowAchievement:     return "Achievement";
				case rail::kRailWindowUserSpaceStore:  return "UserSpaceStore";
				case rail::kRailWindowLeaderboard:     return "Leaderboard";
				case rail::kRailWindowHomepage:        return "Homepage";
				case rail::kRailWindowLiveCommenting:  return "LiveCommenting";
				}

				return "Unknown Window Type";
			}

			inline const char* Helper::EnumString(rail::EnumRailUsersInviteResponseType value)
			{
				switch (value)
				{
				case rail::kRailInviteResponseTypeUnknown:  return "Unknown";
				case rail::kRailInviteResponseTypeAccepted: return "Accepted";
				case rail::kRailInviteResponseTypeRejected: return "Rejected";
				case rail::kRailInviteResponseTypeIgnore:   return "Ignore";
				case rail::kRailInviteResponseTypeTimeout:  return "Timeout";
				}

				return "Unknown Invite Response";
			}

			inline const char* Helper::EnumString(rail::EnumRoomMemberActionStatus value)
			{
				switch (value)
				{
				case rail::kMemberEnteredRoom:      return "Entered";
				case rail::kMemberLeftRoom:         return "Left";
				case rail::kMemberDisconnectServer: return "Disconnect";
				}

				return "Unknown Member Action";
			}
		}
	}
}
