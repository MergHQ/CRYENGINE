// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __ICRYLOBBY_H__
#define __ICRYLOBBY_H__

#pragma once

#define CRYLOBBY_USER_NAME_LENGTH        32
#define CRYLOBBY_USER_GUID_STRING_LENGTH 40

struct SCryUserID : public CMultiThreadRefCount
{
	virtual bool                                              operator==(const SCryUserID& other) const = 0;
	virtual bool                                              operator<(const SCryUserID& other) const = 0;

	virtual CryFixedStringT<CRYLOBBY_USER_GUID_STRING_LENGTH> GetGUIDAsString() const
	{
		return CryFixedStringT<CRYLOBBY_USER_GUID_STRING_LENGTH>("");
	}
};

struct CryUserID
{
	CryUserID() : userID()
	{
	}

	CryUserID(SCryUserID* ptr) : userID(ptr)
	{
	}

	const SCryUserID* get() const
	{
		return userID.get();
	}

	bool operator!=(const CryUserID& other) const
	{
		return !(*this == other);
	}

	bool operator==(const CryUserID& other) const
	{
		if (other.IsValid() && IsValid())
		{
			return ((*other.userID) == (*userID));
		}
		if ((!other.IsValid()) && (!IsValid()))
		{
			return true;
		}
		return false;
	}

	bool operator<(const CryUserID& other) const
	{
		// In the case where one is invalid, the invalid one is considered less than the valid one
		if (other.IsValid())
		{
			if (IsValid())
			{
				return (*userID) < (*other.userID);
			}
			else
			{
				return true;
			}
		}
		return false;
	}

	bool IsValid() const
	{
		return (userID.get() != NULL);
	}

	_smart_ptr<SCryUserID> userID;
};

const CryUserID CryUserInvalidID = NULL;

struct ICryMatchMaking;
struct ICryVoice;
struct ICryTCPService;
struct ICryStats;
struct ICryLobbyUI;
struct ICryFriends;
struct ICryFriendsManagement;
struct ICryReward;
struct ICryOnlineStorage;
struct IHostMigrationEventListener;
struct ICrySignIn;

#include <CryLobby/ICryTCPService.h> // <> required for Interfuscator
#include <CrySystem/ISystem.h>

#if USE_STEAM
	#define USE_LOBBYIDADDR 1
#else
	#define USE_LOBBYIDADDR 0
#endif

#define XBOX_RELEASE_USE_SECURE_SOCKETS 1

#if CRY_PLATFORM_ORBIS
	#define USE_PSN 1
#else
	#define USE_PSN 0
#endif

#if CRY_PLATFORM_WINDOWS
	#define USE_PC_PROFANITY_FILTER 1
#else
	#define USE_PC_PROFANITY_FILTER 0
#endif

#define CRYLOBBY_USER_PACKET_START 128
#define CRYLOBBY_USER_PACKET_MAX   255

typedef uint32 CryLobbyTaskID;
const CryLobbyTaskID CryLobbyInvalidTaskID = 0xffffffff;

typedef uint16 CryPing;
#define CRYLOBBY_INVALID_PING  (CryPing(~0))

#define CRYSESSIONID_STRINGLEN 48
struct SCrySessionID : public CMultiThreadRefCount
{
	// <interfuscator:shuffle>
	virtual bool operator==(const SCrySessionID& other) = 0;
	virtual bool operator<(const SCrySessionID& other) = 0;
	virtual bool IsFromInvite() const = 0;
	virtual void AsCStr(char* pOutString, int inBufferSize) const = 0;
	// </interfuscator:shuffle>
};

typedef _smart_ptr<SCrySessionID> CrySessionID;
const CrySessionID CrySessionInvalidID = NULL;

enum ECryLobbyError
{
	eCLE_Success                    = 0,  //!< Task is successfully started if returned from function and successfully finished if callback is called with this error.
	eCLE_SuccessContinue            = 1,  //!< For tasks that return multiple results the callback will be called with this error if there is a valid result and the callback will be called again.
	eCLE_ServiceNotSupported        = 2,  //!< The service is not supported on this platform.
	eCLE_AlreadyInitialised         = 3,  //!< Service has already been initialised.
	eCLE_NotInitialised             = 4,  //!< Service has not been initialised.
	eCLE_TooManyTasks               = 5,  //!< The task could not be started because too many tasks are already running.
	eCLE_OutOfMemory                = 6,  //!< Not enough memory to complete task.
	eCLE_OutOfSessionUserData       = 7,  //!< Trying to register too much session user data.
	eCLE_UserDataNotRegistered      = 8,  //!< Using a session user data id that has not been registered.
	eCLE_UserDataTypeMissMatch      = 9,  //!< Live - The data type of the session user data is not compatible with the data type defined in the xlast program.
	eCLE_TooManySessions            = 10, //!< The session could not be created because there are too many session already created.
	eCLE_InvalidSession             = 11, //!< The specified session handle does not exist.
	eCLE_InvalidRequest             = 12, //!< The task being performed is invalid.
	eCLE_SPAFileOutOfDate           = 13, //!< Live - The SPA file used doesn't match the one on the live servers.
	eCLE_ConnectionFailed           = 14, //!< Connection to session host failed.
	eCLE_SessionFull                = 15, //!< Can't join session because it is full.
	eCLE_SessionWrongState          = 16, //!< The session is in the wrong state for the requested operation to be performed.
	eCLE_UserNotSignedIn            = 17, //!< The user specified is not signed in.
	eCLE_InvalidParam               = 18, //!< An invalid parameter was passed to function.
	eCLE_TimeOut                    = 19, //!< The current task has timed out waiting for a response0.
	eCLE_InsufficientPrivileges     = 20, //!< User had insufficient privileges to perform the requested task. In Live a silver account is being used when a gold account is required.
	eCLE_AlreadyInSession           = 21, //!< Trying to join a session that has already been joined.
	eCLE_LeaderBoardNotRegistered   = 22, //!< Using a leaderboard id that has not been registered.
	eCLE_UserNotInSession           = 23, //!< Trying to write to a leaderboard for a user who is not in the session.
	eCLE_OutOfUserData              = 24, //!< Trying to register too much user data.
	eCLE_NoUserDataRegistered       = 25, //!< Trying to read/write user data when no data has been registered.
	eCLE_ReadDataNotWritten         = 26, //!< Trying to read user data that has never been written.
	eCLE_UserDataMissMatch          = 27, //!< Trying to write user data that is different to registered data.
	eCLE_InvalidUser                = 28, //!< Trying to use an invalid user id.
	eCLE_PSNContextError            = 29, //!< Somethings wrong with one of the various PSN contexts.
	eCLE_PSNWrongSupportState       = 30, //!< Invalid state in PSN support state machine.
	eCLE_SuccessUnreachable         = 31, //!< For session search the callback will be called with this error if a session was found but is unreachable and the callback will be called again with other results.
	eCLE_ServerNotDefined           = 32, //!< Server not set.
	eCLE_WorldNotDefined            = 33, //!< World not set.
	eCLE_SystemIsBusy               = 34, //!< System is busy. XMB is doing something else and cannot work on the new task. trying to bring up more than 1 UI dialogs at a time, or look at friends list while it is being downloaded in the background, etc.
	eCLE_TooManyParameters          = 35, //!< Too many parameters were passed to the task. e.g an array of SCryLobbyUserData has more items than the task expected.
	eCLE_NotEnoughParameters        = 36, //!< Not enough parameters were passed to the task. e.g an array of SCryLobbyUserData has less items than the task expected.
	eCLE_DuplicateParameters        = 37, //!< Duplicate parameters were passed to the task. e.g an array of SCryLobbyUserData has the same item more than once.
	eCLE_ExceededReadWriteLimits    = 38, //!< This error can be returned if the underlying SDK has limits on how often the task can be run.
	eCLE_InvalidTitleID             = 39, //!< Title is using an invalid title id.
	eCLE_IllegalSessionJoin         = 40, //!< An illegal session join has been performed. e.g in Live a session has been created or joined that has invites enabled when the user is already in a session with invites enabled.
	eCLE_InternetDisabled           = 41, //!< PSN: No access to Internet.
	eCLE_NoOnlineAccount            = 42, //!< PSN: No online account.
	eCLE_NotConnected               = 43, //!< PSN: Not connected.
	eCLE_CyclingForInvite           = 44, //!< PSN: Currently handling a invite.
	eCLE_CableNotConnected          = 45, //!< Ethernet cable is not connected.
	eCLE_SessionNotMigratable       = 46, //!< An attempt to migrate a non-migratable session was detected.
	eCLE_SuccessInvalidSession      = 47, //!< If SessionEnd or SessionDelete is called for an invalid session, return this as a successful failure.
	eCLE_RoomDoesNotExist           = 48, //!< A PSN request was triggered for a room that doesn't exist on the PSN service.
	eCLE_PSNUnavailable             = 49, //!< PSN: service is currently unavailable (might be under maintenance, etc. Not the same as unreachable!).
	eCLE_TooManyOrders              = 50, //!< Too many DLC store orders.
	eCLE_InvalidOrder               = 51, //!< Order does not exist.
	eCLE_OrderInUse                 = 52, //!< Order already has an active task.
	eCLE_OnlineAccountBlocked       = 53, //!< PSN: Account is suspended or banned.
	eCLE_AgeRestricted              = 54, //!< PSN: Online access restricted because of age (child accounts only).
	eCLE_ReadDataCorrupt            = 55, //!< UserData is corrupted/wrong size/invalid. (Not the same as eCLE_ReadDataNotWritten, which means not present).
	eCLE_PasswordIncorrect          = 56, //!< client passed the wrong password to the server in the session join request.
	eCLE_InvalidInviteFriendData    = 57, //!< PSN: Invite friend data is invalid/unavailable.
	eCLE_InvalidJoinFriendData      = 58, //!< PSN: Join friend data is invalid/unavailable.
	eCLE_InvalidPing                = 60, //!< No valid ping value found for user in session.
	eCLE_CDKeyMalformed             = 61, //!< Malformed CD key.
	eCLE_CDKeyUnknown               = 62, //!< CD key not in database for this game.
	eCLE_CDKeyAuthFailed            = 63, //!< CD key authentication failed.
	eCLE_CDKeyDisabled              = 64, //!< CD key has been disabled.
	eCLE_CDKeyInUse                 = 65, //!< CD key is in use.
	eCLE_MultipleSignIn             = 66, //!< Disconnected because another sign in to the same account occurred.
	eCLE_Banned                     = 67, //!< Banned user attempting to join session.
	eCLE_CDKeyTimeOut               = 68, //!< CD key validation time out.
	eCLE_IncompleteLoginCredentials = 69, //!< Incomplete login credentials.
	eCLE_WrongVersion               = 70, //!< Trying to join a game or squad that is using a different version.
	eCLE_NoServerAvailable          = 71, //!< Unable to retrieve a dedicated server from the dedicated server arbitrator.
	eCLE_ArbitratorTimeOut          = 72, //!< Timed out trying to access the dedicated server arbitrator.
	eCLE_RequiresInvite             = 73, //!< Session requires an invite to be able to join.
	eCLE_SteamInitFailed            = 74, //!< Steam: unable to init main Steam library.
	eCLE_SteamBlocked               = 75, //!< Steam: user is blocked from performing this action (e.g. blocked from joining a session).
	eCLE_NothingToEnumerate         = 76, //!< Failed to create an xbox enumerator as there is nothing to enumerate.
	eCLE_ServiceNotConnected        = 77,//!< Platform online service is not connected.
	eCLE_GlobalBan                  = 78, //!< Banned from playing online.
	eCLE_Kicked                     = 79, //!< Kicked from server.

	eCLE_Cancelled                  = 79,
	eCLE_UnhandledNickError         = 80,

	eCLE_InternalError,

	eCLE_NumErrors
};

enum ECryLobbyService
{
	eCLS_LAN,
	eCLS_Online,
	eCLS_NumServices
};

#if USE_STEAM
class CLobbyString : public string
{
public:
	CLobbyString() : string() {}
	CLobbyString(uint32 n) : string() { char buff[32]; cry_sprintf(buff, "%u", n); assign(buff); }
	CLobbyString(string s) : string(s) {}
	CLobbyString(uint16 n) : string() { char buff[32]; cry_sprintf(buff, "%u", n); assign(buff); }
	CLobbyString(int n) : string() { char buff[32]; cry_sprintf(buff, "%i", n); assign(buff); }
	bool operator==(const CLobbyString& other) { return compare(other) == 0; }
	bool operator==(CLobbyString& other)       { return compare(other) == 0; }
	CLobbyString(const CLobbyString& s) : string(s) {}
private:
	CLobbyString(const char* p){}
};
typedef CLobbyString CryLobbyUserDataID;
#else
typedef uint32       CryLobbyUserDataID;
#endif

enum ECryLobbyUserDataType
{
	eCLUDT_Int64,
	eCLUDT_Int32,
	eCLUDT_Int16,
	eCLUDT_Int8,
	eCLUDT_Float64,
	eCLUDT_Float32,
	eCLUDT_Int64NoEndianSwap
};

struct SCryLobbyUserData
{
	CryLobbyUserDataID    m_id;
	ECryLobbyUserDataType m_type;

	SCryLobbyUserData()
	{
		m_id = CryLobbyUserDataID();
		m_type = eCLUDT_Int64;
		m_int64 = 0;
	}

	union
	{
		int64 m_int64;
		f64   m_f64;
		int32 m_int32;
		f32   m_f32;
		int16 m_int16;
		int8  m_int8;
	};

	const SCryLobbyUserData& operator=(const SCryLobbyUserData& src)
	{
		m_id = src.m_id;
		m_type = src.m_type;

		switch (m_type)
		{
		case eCLUDT_Int64:
			m_int64 = src.m_int64;
			break;
		case eCLUDT_Int32:
			m_int32 = src.m_int32;
			break;
		case eCLUDT_Int16:
			m_int16 = src.m_int16;
			break;
		case eCLUDT_Int8:
			m_int8 = src.m_int8;
			break;
		case eCLUDT_Float64:
			m_f64 = src.m_f64;
			break;
		case eCLUDT_Float32:
			m_f32 = src.m_f32;
			break;
		case eCLUDT_Int64NoEndianSwap:
			m_int64 = src.m_int64;
			break;
		default:
			CryLog("Unhandled ECryLobbyUserDataType %d", m_type);
			break;
		}

		return *this;
	};

	bool operator==(const SCryLobbyUserData& other)
	{
		if ((m_id == other.m_id) && (m_type == other.m_type))
		{
			switch (m_type)
			{
			case eCLUDT_Int64:
				return m_int64 == other.m_int64;
			case eCLUDT_Int32:
				return m_int32 == other.m_int32;
			case eCLUDT_Int16:
				return m_int16 == other.m_int16;
			case eCLUDT_Int8:
				return m_int8 == other.m_int8;
			case eCLUDT_Float64:
				return m_f64 == other.m_f64;
			case eCLUDT_Float32:
				return m_f32 == other.m_f32;
			case eCLUDT_Int64NoEndianSwap:
				return m_int64 == other.m_int64;
			default:
				CryLog("Unhandled ECryLobbyUserDataType %d", m_type);
				return false;
			}
		}

		return false;
	}

	bool operator!=(const SCryLobbyUserData& other)
	{
		if ((m_id == other.m_id) && (m_type == other.m_type))
		{
			switch (m_type)
			{
			case eCLUDT_Int64:
				return m_int64 != other.m_int64;
			case eCLUDT_Int32:
				return m_int32 != other.m_int32;
			case eCLUDT_Int16:
				return m_int16 != other.m_int16;
			case eCLUDT_Int8:
				return m_int8 != other.m_int8;
			case eCLUDT_Float64:
				return m_f64 != other.m_f64;
			case eCLUDT_Float32:
				return m_f32 != other.m_f32;
			case eCLUDT_Int64NoEndianSwap:
				return m_int64 != other.m_int64;
			default:
				CryLog("Unhandled ECryLobbyUserDataType %d", m_type);
				return true;
			}
		}

		return true;
	}
};

enum ELobbyFriendStatus
{
	eLFS_Offline,
	eLFS_Online,
	eLFS_OnlineSameTitle,
	eLFS_Pending
};

typedef uint32 CryLobbyUserIndex;
const CryLobbyUserIndex CryLobbyInvalidUserIndex = 0xffffffff;

struct SCryLobbyPartyMember
{
	CryUserID          m_userID;
	char               m_name[CRYLOBBY_USER_NAME_LENGTH];
	ELobbyFriendStatus m_status;
	CryLobbyUserIndex  m_userIndex;
};

struct SCryLobbyPartyMembers
{
	uint32                m_numMembers;
	SCryLobbyPartyMember* m_pMembers;
};

// The callback below is fired whenever the lobby system needs access to data that is unavailable through the standard API.
//Its usually used to work around problems with specific platform lobbies requiring data not exposed through standard functionality.

//Basically whenever this callback is fired, you are required to fill in the requestedParams that are asked for.
//At present specifications are that the callback can fire multiple times.
//
// 'XSvc'		uint32	Service Id for XLSP servers
// 'XPor'		uint16	Port for XLSP server to communicate with Telemetry
// 'XSNm'		void*		ptr to a static const string containging the name of the required XLSP service. is used to filter the returned server list from Live. return NULL for no filtering
// 'LUnm'		void*		ptr to user name for local user - used by LAN (due to lack of guid) (is copied internally - DO NOT PLACE ON STACK)
//
// PS3 ONLY
// 'PCom'		void*		ptr to static SceNpCommunitcationId					(not copied - DO NOT PLACE ON STACK!)
// 'PPas'		void*		ptr to static SceNpCommunicationPassphrase	(not copied - DO NOT PLACE ON STACK!)
// 'PSig'		void*		ptr to static SceNpCommunicationSignature		(not copied - DO NOT PLACE ON STACK!)
// 'PInM'		char*		ptr to string used for the custom XMB button for sending invites to friends.
// 'PInS'		char*		ptr to string used for the XMB game invite message subject text.
// 'PInB'		char*		ptr to string used for the XMB game invite message body text.
// 'PFrS'		char*		ptr to string used for the XMB friend request message subject text.
// 'PFrB'		char*		ptr to string used for the XMB friend request message body text.
// 'PAge'		int32		Age limit of game (set to 0 for now, probably wants to come from some kind of configuration file)
// 'PSto'		char*		Store ID. AKA NP Commerce ServiceID and also top level category Id.
// 'PDlc'		int8		Input is DLC pack index to test is installed, output is boolean result.
//
// PS4 ONLY
// PTit and PSec are the PS4 replacements for the PS3 PCom, PPas and PSig.
// 'PTit'		void*		ptr to static SceNpTitleId									(not copied - DO NOT PLACE ON STACK!)
// 'PSec'		void*		ptr to static SceNpTitleSecret							(not copied - DO NOT PLACE ON STACK!)
// 'PSAd'		SCryLobbySessionAdvertisement*	in/out structure to request extra session advertising info from the game
// 'PAge'		SCryLobbyAgeRestrictionSetup*		Age limit settings per region
// 'PPlu'		int32		Age limit of game (set to 0 for now, probably wants to come from some kind of configuration file)
//
// 'CSgs'		uint32	ECryLobbyLoginGUIState constant to indicate whether other data requested is available
// 'CPre'		SCryLobbyPresenceConverter* in/out structure for converting a list of SCryLobbyUserData into a single string for presense
// 'Mspl'		Matchmaking session password length (not including null terminator)

#define CLCC_LAN_USER_NAME                           'LUnm'
#define CLCC_LIVE_TITLE_ID                           'XTtl'
#define CLCC_XLSP_SERVICE_ID                         'XSvc'
#define CLCC_LIVE_SERVICE_CONFIG_ID                  'XSCf'
#define CLCC_XLSP_SERVICE_PORT                       'XPor'
#define CLCC_XLSP_SERVICE_NAME                       'XSNm'
#define CLCC_PSN_COMMUNICATION_ID                    'PCom'
#define CLCC_PSN_COMMUNICATION_PASSPHRASE            'PPas'
#define CLCC_PSN_COMMUNICATION_SIGNATURE             'PSig'
#define CLCC_PSN_CUSTOM_MENU_GAME_INVITE_STRING      'PInM'
#define CLCC_PSN_CUSTOM_MENU_GAME_JOIN_STRING        'PJoM'
#define CLCC_PSN_INVITE_SUBJECT_STRING               'PInS'
#define CLCC_PSN_INVITE_BODY_STRING                  'PInB'
#define CLCC_PSN_FRIEND_REQUEST_SUBJECT_STRING       'PFrS'
#define CLCC_PSN_FRIEND_REQUEST_BODY_STRING          'PFrB'
#define CLCC_PSN_AGE_LIMIT                           'PAge'
#define CLCC_PSN_PLUS_TEST_REQUIRED                  'PPlu'
#define CLCC_PSN_STORE_ID                            'PSto'
#define CLCC_PSN_TICKETING_ID                        'PTik'
#define CLCC_PSN_IS_DLC_INSTALLED                    'PDlc'
#define CLCC_PSN_TITLE_ID                            'PTit'
#define CLCC_PSN_TITLE_SECRET                        'PSec'
#define CLCC_PSN_TICKETING_ID                        'PTik'
#define CLCC_PSN_CREATE_SESSION_ADVERTISEMENT        'PCSA'
#define CLCC_PSN_UPDATE_SESSION_ADVERTISEMENT        'PUSA'
#define CLCC_CRYLOBBY_EXPORTGAMESTATE                'CEgs'
#define CLCC_CRYLOBBY_IMPORTGAMESTATE                'CIgs'
#define CLCC_CRYLOBBY_LOBBYDATANAME                  'CLdn'
#define CLCC_CRYLOBBY_LOBBYDATAUSAGE                 'CLdu'
#define CLCC_CRYLOBBY_PRESENCE_CONVERTER             'CPre' // Used by PSN for converting presence info into a string form
#define CLCC_CRYLOBBY_QUERYTRANSLATEHASH             'CQth'
#define CLCC_CRYLOBBY_TRANSLATEHASH                  'CTrh'

#define CLCC_CRYSTATS_ENCRYPTION_KEY                 'CEnc' // Used for all platforms to encrypt UserData buffers

#define CLCC_MATCHMAKING_SESSION_PASSWORD_MAX_LENGTH 'Mspl' // Used to determine the maximum length of the session password (not including NULL character)

#define CLCC_STEAM_APPID                             'SaID' // Request for the Steam application id (only required during development)

enum ECryLobbyLeaderboardType
{
	eCLLT_P2P,
	eCLLT_Dedicated,
	eCLLT_Num
};

//! A lobby service may query the game for the usage of each datum registered with SessionRegisterUserData.
enum ECryLobbyDataUsage
{
	eCLDU_ServerGameVersion,
	eCLDU_ServerMapNameHash,     //!< Configuration callback may be invoked to query whether conversion to string is supported.
	eCLDU_ServerGameTypeHash,    //!< Configuration callback may be invoked to query whether conversion to string is supported.
	eCLDU_ServerGameVariantHash, //!< Configuration callback may be invoked to query whether conversion to string is supported.
	eCLDU_ServerNumTeams,
	eCLDU_ServerGameState,       //!< Configuration callback may be invoked to translate between game's value (whatever the game uses) and engine's value (composed of EGameModeBit bits).
	eCLDU_ServerTeamPlay,
	eCLDU_ServerFragLimit,
	eCLDU_ServerTeamFragLimit,
	eCLDU_ServerTimeElapsed,
	eCLDU_ServerTimeLimit,
	eCLDU_ServerRoundTimeLimit,
	eCLDU_ServerRoundTimeElapsed,
	eCLDU_ServerCountry,
	eCLDU_ServerRegion,
	eCLDU_ServerHostUserID,
	eCLDU_ServerOther,

	eCLDU_FirstPlatformSpecific // Must be last
};

enum ECryGameStateFlag
{
	eCGSF_CanJoin       = BIT(0),
	eCGSF_InGame        = BIT(1),
	eCGSF_ServerExiting = BIT(2)
};

struct SCryLobbyUserDataTranslateHashParam
{
	SCryLobbyUserData data;
	const char*       pStrVal;
};

struct SCryLobbyUserDataUsageParam
{
	CryLobbyUserDataID id;
	ECryLobbyDataUsage usage;
};

struct SCryLobbyUserDataNameParam
{
	CryLobbyUserDataID id;
	const char*        pName;
};

struct SCryLobbyUserDataQueryTranslateHashParam
{
	CryLobbyUserDataID id;
	bool               canTranslate;
};

struct SCryLobbyUserDataGameStateParam
{
	SCryLobbyUserData data;
	uint32            gameStateBits;
};

struct SConfigurationParams
{
	uint32 m_fourCCID;
	union
	{
		uint64 m_64;
		void*  m_pData;
		uint32 m_32;
		uint16 m_16;
		uint8  m_8;
	};
};

struct SAgeData
{
	int  languageCode;
	char countryCode[2];
};

typedef void (* CryLobbyConfigurationCallback)(ECryLobbyService service, SConfigurationParams* requestedParams, uint32 paramCount);

typedef void (* CryLobbyCallback)(ECryLobbyService service, ECryLobbyError error, void* arg);

//! \param taskID    Task ID allocated when the function was executed.
//! \param error     Error code - eCLE_Success if the function succeeded or an error that occurred while processing the function.
//! \param privilege A bitfield of CLPF_* specifying the users privileges. Bits get set to indicate the user should be blocked from an action if their privilege level isn't high enough.
//! \param pArg      Pointer to application-specified data.
typedef void (* CryLobbyPrivilegeCallback)(CryLobbyTaskID taskID, ECryLobbyError error, uint32 privilege, void* pArg);

//! \param taskID    Task ID allocated when the function was executed.
//! \param error     Error code - eCLE_Success if the function succeeded or an error that occurred while processing the function.
//! \param pArg      Pointer to application-specified data.
typedef void (* CryLobbyOnlineStateCallback)(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg);

//! \param taskID      Task ID allocated when the function was executed.
//! \param error       Error code - eCLE_Success if the function succeeded or an error that occurred while processing the function.
//! \param pString
//! \param isProfanity true if the string contained a profanity, otherwise false.
//! \param pArg        Pointer to application-specified data.
typedef void (* CryLobbyCheckProfanityCallback)(CryLobbyTaskID taskID, ECryLobbyError error, const char* pString, bool isProfanity, void* pArg);

//! Flags for use with ICryLobby::GetLobbyServiceFlag.
enum ECryLobbyServiceFlag
{
	eCLSF_AllowMultipleSessions = BIT(0),       //!< Does the lobby service allow multiple sessions to be joined? (Default True).
	eCLSF_SupportHostMigration  = BIT(1)        //!< Does the lobby service support host migration? (Default: True).
};

//! If the underlying SDK has a frontend invites can be accepted from then this callback will be called if an invite is accepted this way.
//! When this call back is called the game must switch to the given service and join the session.
//! If the user is in a session already then the old session must be deleted before the join is started.
//! Structure returned when a registered callback is triggered for invite acceptance.
struct SCryLobbyInviteAcceptedData
{
	ECryLobbyService m_service;       //!< Which of the CryLobby services this is for.
	uint32           m_user;          //!< Id of local user this pertains to.
	CrySessionID     m_id;            //!< Session identifier of which session to join.
	ECryLobbyError   m_error;
};

enum ECryLobbyInviteType
{
	eCLIT_InviteToSquad,
	eCLIT_JoinSessionInProgress,
	eCLIT_InviteToSession,
};

//! Separate data type for platforms where invites revolve around users, not sessions.
struct SCryLobbyUserInviteAcceptedData
{
	ECryLobbyService    m_service;
	uint32              m_user;
	CryUserID           m_inviterId;
	ECryLobbyError      m_error;
	ECryLobbyInviteType m_type;
};

//! When a call to set Rich Presence is made, the SCryLobbyUserData is just passed back to the game code, so.
//! It's the game code's responsibility to correctly turn all the bits and bobs of SCryLobbyUserData into a single UTF-8 string.
//! [in] = maximum size of the buffer pre-allocated [SCE_NP_BASIC_PRESENCE_EXTENDED_STATUS_SIZE_MAX].
//! [out] = modify the value to contain the actual length of the string in bytes.
struct SCryLobbyPresenceConverter
{
	SCryLobbyUserData* m_pData;                //!< Pointer of the SCryLobbyUserData passed into CryLobbyUI::SetRichPresence. [in].
	uint32             m_numData;              //!< Number of SCryLobbyUserData in array pointed to by m_pData. [in].
	uint8*             m_pStringBuffer;        //!< Buffer to write the result string in to. This buffer is pre-allocated internally, no need to create your own. [in/out].
	uint32             m_sizeOfStringBuffer;   //!< Size of the string buffer in bytes, not characters. [in/out].
	CrySessionID       m_sessionId;            //!< Joinable session ID, to allow friends to join your game if desired. [out].
};

//! Used for creating a session and updating the session details.
//! \note In SDK 1.500 the linkdata (m_pData) could not be updated once created. Feature is due to be added in 1.700.
//! \note Updating localised session name/status will overwrite/replace all previous name/status values. Values not updated are discarded.
//! \note Is to NOT possible to update the number of slots, private or editable flags after creation.
struct SCryLobbySessionAdvertisement
{
	void*  m_pJPGImage;
	uint32 m_sizeofJPGImage;
	uint8* m_pData;
	uint32 m_sizeofData;
	char** m_pLocalisedLanguages;
	char** m_pLocalisedSessionNames;
	char** m_pLocalisedSessionStatus;
	uint32 m_numLanguages;
	uint32 m_numSlots;
	bool   m_bIsPrivate;
	bool   m_bIsEditableByAll;
};

//! A request to fill in an XMB string requires a buffer allocated by the calling function, and a value defining the size of the buffer.
//! It's the game's responsibility to fill the buffer with a UTF-8 string, and not to overflow the buffer.
struct SCryLobbyXMBString
{
	uint8* m_pStringBuffer;               //! Pointer to a UTF-8 compatible string buffer.
	uint32 m_sizeOfStringBuffer;          //!< Maximum sizeof the buffer.
};

//! A request to fill supply age restriction settings for different regions.
struct SCryLobbyAgeRestrictionSetup
{
	struct SCountry
	{
		char id[3];
		int8 age;
	};
	int32           m_numCountries;     //!< Number of specified countries.
	const SCountry* m_pCountries;       //!< Array of m_numCountries SCryLobbyAgeRestrictionSetup::SCountry entries.
};

// Privilege bits returned from ICryLobbyService::GetUserPrivileges in the CryLobbyPrivilegeCallback.
const uint32 CLPF_BlockMultiplayerSessons = BIT(0);     //!< If set the user is not allowed to participate in multiplayer game sessions.
const uint32 CLPF_NotOnline = BIT(1);                   //!< If set the user is not connected to PSN.

struct SCrySystemTime
{
	uint16 m_Year;
	uint16 m_Milliseconds;
	uint8  m_Month;
	uint8  m_DayOfWeek;
	uint8  m_Day;
	uint8  m_Hour;
	uint8  m_Minute;
	uint8  m_Second;
};

struct ICryLobbyService
{
public:
	// <interfuscator:shuffle>
	virtual ~ICryLobbyService(){}
	virtual ICryMatchMaking*       GetMatchMaking() = 0;
	virtual ICryVoice*             GetVoice() = 0;
	virtual ICryStats*             GetStats() = 0;
	virtual ICryLobbyUI*           GetLobbyUI() = 0;
	virtual ICryFriends*           GetFriends() = 0;
	virtual ICryFriendsManagement* GetFriendsManagement() = 0;
	virtual ICryReward*            GetReward() = 0;
	virtual ICryOnlineStorage*     GetOnlineStorage() = 0;
	virtual ICrySignIn*            GetSignIn() = 0;

	//! Cancel the given task. The task will still be running in the background but the callback will not be called when it finishes.
	//! \param TaskID			- The task to cancel.
	virtual void CancelTask(CryLobbyTaskID lTaskID) = 0;

	//! Get an ICryTCPService.
	//! \param pService Service.
	//! \return Pointer to ICryTCPService for given server & port.
	virtual ICryTCPServicePtr GetTCPService(const char* pService) = 0;

	//! Get an ICryTCPService.
	//! \param pServer server
	//! \param port port
	//! \param pUrlPrefix URL prefix
	//! \return Pointer to ICryTCPService for given server & port.
	virtual ICryTCPServicePtr GetTCPService(const char* pServer, uint16 port, const char* pUrlPrefix) = 0;

	//! Get the user id of a user signed in to this service locally.
	//! \param user	The pad number of the local user.
	//! \return CryUserID of the user.
	virtual CryUserID GetUserID(uint32 user) = 0;

	//! Get the given users privileges.
	//! \param user      The pad number of the local user.
	//! \param pTaskID   Pointer to buffer to store the task ID to identify this call in the callback.
	//! \param pCB       Callback function that is called when function completes.
	//! \param pCbArg    Pointer to application-specified data that is passed to the callback.
	//! \return eCLE_Success if function successfully started or an error code if function failed to start.
	virtual ECryLobbyError GetUserPrivileges(uint32 user, CryLobbyTaskID* pTaskID, CryLobbyPrivilegeCallback pCB, void* pCBArg) = 0;

	//! Check a string for profanity.
	//! \param pString   String to check.
	//! \param pTaskID   Pointer to buffer to store the task ID to identify this call in the callback.
	//! \param pCb       Callback function that is called when function completes.
	//! \param pCbArg    Pointer to application-specified data that is passed to the callback.
	//! \return eCLE_Success if function successfully started or an error code if function failed to start.
	virtual ECryLobbyError CheckProfanity(const char* const pString, CryLobbyTaskID* pTaskID, CryLobbyCheckProfanityCallback pCb, void* pCbArg) = 0;

	//! Returns the current time from the online time server (must be signed in).
	//! \param pSystemTime Pointer to the CrySystemTime structure to contain the time. Only year, month, day, hours, minutes and seconds will be populated.
	//! \return eCLE_Success if the function was successful, or an error code if not.
	virtual ECryLobbyError GetSystemTime(uint32 user, SCrySystemTime* pSystemTime) = 0;

	// </interfuscator:shuffle>
};

#include "CryLobbyEvent.h"    // Separated out for readability

//! Allows users of the lobby to not initialise features that they don't require.
enum ECryLobbyServiceFeatures
{
	eCLSO_Base          = BIT(0),
	eCLSO_Matchmaking   = BIT(1),
	eCLSO_Voice         = BIT(2),
	eCLSO_Stats         = BIT(3),
	eCLSO_LobbyUI       = BIT(4),
	eCLSO_Friends       = BIT(5),
	eCLSO_Reward        = BIT(6),
	eCLSO_TCPService    = BIT(7),
	eCLSO_OnlineStorage = BIT(8),

	eCLSO_All           = eCLSO_Base | eCLSO_Matchmaking | eCLSO_Voice | eCLSO_Stats | eCLSO_LobbyUI | eCLSO_Friends | eCLSO_Reward | eCLSO_TCPService | eCLSO_OnlineStorage
};

//! Intended to make future extension easier. At present I only need the port exposing.
struct SCryLobbyParameters
{
	uint16 m_listenPort;         //!< Listen port the lobby service will use for connections.
	uint16 m_connectPort;        //!< Connect port the lobby service will use for connections.
};

struct ICryLobby
{
public:
	// <interfuscator:shuffle>
	virtual ~ICryLobby(){}

	//! Initialise a lobby service.
	//! \param service   The service to be initialised.
	//! \param features  Which features of the service to initialise (e.g.  ECryLobbyServiceFeatures(eCLSO_All & (~eCLSO_Reward))).
	//! \param cfgCb     Callback function that is called whenever the lobby system needs access to data that is unavailable through the standard API.
	//! \param cb        Callback function that is called when function completes.
	//! \param cbArg     Pointer to application-specified data that is passed to the callback.
	//! \return eCLE_Success if function successfully started or an error code if function failed to start.
	virtual ECryLobbyError Initialise(ECryLobbyService service, ECryLobbyServiceFeatures features, CryLobbyConfigurationCallback cfgCb, CryLobbyCallback cb, void* cbArg) = 0;

	//! Shut down a lobby service.
	//! \param Service		The service to be shut down.
	//! \param Features		Which features of the service to terminate (e.g.  ECryLobbyServiceFeatures(eCLSO_All & (~eCLSO_Reward))).
	//! \param Cb					Callback function that is called when function completes.
	//! \param CbArg			Pointer to application-specified data that is passed to the callback.
	//! \return eCLE_Success if function successfully started or an error code if function failed to start.
	virtual ECryLobbyError Terminate(ECryLobbyService service, ECryLobbyServiceFeatures features, CryLobbyCallback cb, void* cbArg) = 0;

	//! Should be called at least once a frame, causes the lobby->game message queue to be acted on.
	//! \return eCLE_Success if function successfully completed, or an error code if function failed for some reason.
	virtual ECryLobbyError ProcessEvents() = 0;

	//! Set the current lobby service.
	//! \param Service The service to use.
	//! \return The service that was in use before the call.
	virtual ECryLobbyService SetLobbyService(ECryLobbyService service) = 0;

	//! Get the current lobby service type.
	//! \return The ECryLobbyService of the current lobby service.
	virtual ECryLobbyService GetLobbyServiceType() = 0;

	//! Get the current lobby service.
	//! \return Pointer to an ICryLobbyService for the current lobby service.
	virtual ICryLobbyService* GetLobbyService() = 0;

	//! Get the current matchmaking service.
	//! \return Pointer to an ICryMatchMaking associated with the current lobby service.
	virtual ICryMatchMaking* GetMatchMaking() = 0;

	//! Get the current voice service.
	//! \return Pointer to an ICryVoice associated with the current lobby service.
	virtual ICryVoice* GetVoice() = 0;

	//! Get the current stats service.
	//! \return Pointer to an ICryStats associated with the current lobby service.
	virtual ICryStats* GetStats() = 0;

	//! Get the current lobby ui service.
	//! \return Pointer to an ICryLobbyUI associated with the current lobby service.
	virtual ICryLobbyUI* GetLobbyUI() = 0;

	//! Get the current friends service.
	//! \return Pointer to an ICryFriends associated with the current lobby service.
	virtual ICryFriends* GetFriends() = 0;

	//! Get the current friends management service.
	//! \return Pointer to an ICryFriendsManagement associated with the current lobby service.
	virtual ICryFriendsManagement* GetFriendsManagement() = 0;

	//! Get the current reward service (achievements/trophies).
	//! \return Pointer to an ICryReward associated with the current lobby service.
	virtual ICryReward* GetReward() = 0;

	//! Get the current online storage service (title/user storage on PSN and LIVE).
	//! \return Pointer to an ICryOnlineStorage associated with the current lobby service.
	virtual ICryOnlineStorage* GetOnlineStorage() = 0;

	//! Get the requested lobby service.
	//! \param Service		The service to wanted.
	//! \return	Pointer to an ICryLobbyService for the lobby service requested.
	virtual ICryLobbyService* GetLobbyService(ECryLobbyService service) = 0;

	//! Register interest in particular lobby events (such as invite accepted), the passed in callback will be fired when the event happens.
	//! \param Cb					Callback function that will be called.
	//! \param Arg				Pointer to application-specified data.
	virtual void RegisterEventInterest(ECryLobbySystemEvent eventOfInterest, CryLobbyEventCallback cb, void* pUserArg) = 0;

	//! Unregister interest in particular lobby events (such as invite accepted), the passed in callback will no longer be fired when the event happens.
	//! \param Cb					Callback function that will no longer be called.
	//! \param Arg				Pointer to application-specified data that was passed in when the callback was registered.
	virtual void UnregisterEventInterest(ECryLobbySystemEvent eventOfInterest, CryLobbyEventCallback cb, void* pUserArg) = 0;

	//! Retrieve various information about the current lobby service.
	//! \return SCryLobbyParameters - reference to a structure that contains some essential information about the lobby.
	virtual const SCryLobbyParameters& GetLobbyParameters() const = 0;

	//! Is the given flag true for the lobby service of the given type?
	//! \param service Lobby service to be queried
	//! \param flag Flag to be queried
	//! \return true if the flag is true.
	virtual bool GetLobbyServiceFlag(ECryLobbyService service, ECryLobbyServiceFlag flag) = 0;

	//! Set the user packet end point.
	//! \param End	- The user packet end point. Packet types end and higher get translated to disconnect packets.
	virtual void SetUserPacketEnd(uint32 end) = 0;

	// The following three functions are used to allow clients to start loading assets earlier in the flow when using the lobby service.
	// At present we don't support more than one nub session, so these are in the lobby for easier access.

	//! An optimisation for client load times, allows the engine to load the assets ahead of being informed by the server.
	virtual void SetCryEngineLoadHint(const char* levelName, const char* gameRules) = 0;

	//! \return Level name previously set by SetCryEngineLoadHint or "" if never set.
	virtual const char* GetCryEngineLevelNameHint() = 0;

	//! \return Game rules name previously set by SetCryEngineLoadHint or "" if never set.
	virtual const char* GetCryEngineRulesNameHint() = 0;

	/////////////////////////////////////////////////////////////////////////////
	// Host Migration
	virtual void AddHostMigrationEventListener(IHostMigrationEventListener* pListener, const char* pWho, uint32 priority) = 0;
	virtual void RemoveHostMigrationEventListener(IHostMigrationEventListener* pListener) = 0;
	/////////////////////////////////////////////////////////////////////////////
	// </interfuscator:shuffle>
};

#endif // __ICRYLOBBY_H__
