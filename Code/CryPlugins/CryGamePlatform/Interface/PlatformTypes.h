// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformIdentifier.h"
#include <CrySerialization/CryStringsImpl.h>

class ITexture;

namespace Cry
{
	namespace GamePlatform
	{
		struct IAccount;
		struct IAchievement;
		struct ILeaderboards;
		struct IUserLobby;
		struct IMatchmaking;
		struct INetworking;
		struct IRemoteStorage;
		struct IServer;
		struct IService;
		struct IStatistics;
		struct IUser;
		struct IUserGeneratedContent;
		class  UserIdentifier;
		
		struct SRichPresence
		{
			enum class ETimer
			{
				None = -1,
				Elapsed,
				Remaining
			};

			CryFixedStringT<128> status;
			CryFixedStringT<128> details;
			ETimer countdownTimer = ETimer::None;
			int64 seconds = 0;
			uint32 partySize = 0;
			uint32 partyMax = 0;

			bool operator==(const SRichPresence& other) const
			{
				return countdownTimer == other.countdownTimer
					&& partySize == other.partySize
					&& partyMax == other.partyMax
					&& seconds == other.seconds
					&& details == other.status
					&& status == other.status;
			}

			bool operator!=(const SRichPresence& other) const
			{
				const bool areEqual = this->operator==(other);
				return !areEqual;
			}
		};

		//! Type of in-game overlay dialog, used together with IPlugin::OpenDialog
		enum class EDialog
		{
			Friends,
			Community,
			Players,
			Settings,
			Group,
			Achievements
		};

		//! Represents the current connection to the platform's services
		enum class EConnectionStatus
		{
			Disconnected,
			Connecting,
			ObtainingIP,
			Connected
		};

		enum class ESignInStatus
		{
			Unknown,
			Offline,
			Online
		};

		enum class EFriendRelationship
		{
			None,
			Friend,
			RequestSent,
			RequestReceived,
			Blocked,
		};

		//! Determines the size of a user's avatar, used when retrieving the avatar as a texture (see GetAvatar)
		enum class EAvatarSize
		{
			Large,
			Medium,
			Small
		};

		//! Type of in-game overlay dialog with a target user, used together with IPlugin::OpenDialogWithTargetUser
		enum class EUserTargetedDialog
		{
			UserInfo,
			FriendAdd,
			FriendRemove,
			FriendRequestAccept,
			FriendRequestIgnore,
			Chat,
			JoinTrade,
			Stats,
			Achievements
		};

		//! Flags to be passed to platform service for querying/downloading 
		//! information about users.
		enum EUserInformationFlags : uint8
		{
			eUIF_Name =   BIT8(0), //!< Personal name
			eUIF_Avatar = BIT8(1)  //!< Avatar image
		};

		//! Only purpose of this typedef is to make sure we always use the correct type
		//! to store combinations of EUserInformationFlags bits.
		using UserInformationMask = std::underlying_type<EUserInformationFlags>::type;

		//! Platform-specific identifier of an application or DLC
		using ApplicationIdentifier = unsigned int;

		//! Unique identifier for a service
		using ServiceIdentifier = CryGUID;

		constexpr ServiceIdentifier NullServiceID = CryGUID::Null();
		constexpr ServiceIdentifier SteamServiceID = CryClassID("{4DFCDCE3-9985-42E5-A702-5A64D849DC8F}"_cry_guid);
		constexpr ServiceIdentifier PSNServiceID = CryClassID("{F7729E26-464F-4BE9-A58E-06C72C03EAE1}"_cry_guid);
		constexpr ServiceIdentifier DiscordServiceID = CryClassID("{D68238FE-AA88-4C0C-9E9C-56A848AE0F37}"_cry_guid);

		constexpr const char* GetServiceDebugName(const ServiceIdentifier& svcId)
		{
			return (svcId == SteamServiceID) ? "Steam" : 
					((svcId == PSNServiceID) ? "PSN" : 
					((svcId == DiscordServiceID) ? "Discord" :
					((svcId == NullServiceID) ? "Null" : "Unknown")));
		}

		namespace Detail
		{
			using NumericIdentifierValue = uint64; // Steam, PSN
			using StringIdentifierValue = CryFixedStringT<32>; // Discord

			struct SAccountTraits
			{
				using ServiceType = ServiceIdentifier;
				// Note: When adding types here make sure you update the code using stl::holds_alternative
				// and stl::get
				using ValueType = CryVariant<StringIdentifierValue, NumericIdentifierValue>;
			
			private:
				template<size_t I>
				using RawType = typename stl::variant_alternative<I, ValueType>::type;
			
			public:
				static const char* ToDebugString(const ServiceIdentifier& svcId, const ValueType& value)
				{
					static stack_string debugStr;
					
					if (stl::holds_alternative<StringIdentifierValue>(value))
					{
						debugStr.Format("%sAccount:%s", GetServiceDebugName(svcId), stl::get<StringIdentifierValue>(value).c_str());
					}
					else if (stl::holds_alternative<NumericIdentifierValue>(value))
					{
						debugStr.Format("%sAccount:%" PRIu64, GetServiceDebugName(svcId), stl::get<NumericIdentifierValue>(value));
					}
					
					return debugStr.c_str();
				}

				static bool AsUint64(const ValueType& value, uint64& out)
				{
					if (stl::holds_alternative<StringIdentifierValue>(value))
					{
						const StringIdentifierValue& str = stl::get<StringIdentifierValue>(value);
						const int ok = sscanf_s(str.c_str(), "%" PRIx64, out);
						return ok == 1;
					}
					else if (stl::holds_alternative<NumericIdentifierValue>(value))
					{
						out = stl::get<NumericIdentifierValue>(value);
						return true;
					}

					return false;
				}

				template<class StrType>
				static bool AsString(const ValueType& value, StrType& out)
				{
					if (stl::holds_alternative<StringIdentifierValue>(value))
					{
						out = stl::get<StringIdentifierValue>(value).c_str();
						return true;
					}
					else if (stl::holds_alternative<NumericIdentifierValue>(value))
					{
						out.Format("%" PRIx64, stl::get<NumericIdentifierValue>(value));
						return true;
					}

					return false;
				}
			};

			struct SLobbyTraits
			{
				using ServiceType = ServiceIdentifier;
				using ValueType = NumericIdentifierValue;

				static const char* ToDebugString(const ServiceIdentifier& svcId, const ValueType& value)
				{
					static stack_string debugStr;
					debugStr.Format("%sLobby:%" PRIu64, GetServiceDebugName(svcId), value);
					return debugStr.c_str();
				}
				
				static bool AsUint64(const ValueType& value, uint64& out)
				{
					out = value;
					return true;
				}
				
				template<class StrType>
				static bool AsString(const ValueType& value, StrType& out)
				{
					out.Format("%" PRIu64, value);
					return true;
				}
			};
		}

		//! Identifies a game platform user on a specific service.
		using AccountIdentifier = Identifier<Detail::SAccountTraits>;

		//! Identifies a game platform lobby.
		using LobbyIdentifier = Identifier<Detail::SLobbyTraits>;
	}
}

