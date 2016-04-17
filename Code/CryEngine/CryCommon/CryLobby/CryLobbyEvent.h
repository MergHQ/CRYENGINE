// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __ICRYLOBBYEVENT_H__
#define __ICRYLOBBYEVENT_H__

#pragma once

#include <CryLobby/ICryMatchMaking.h> // <> required for Interfuscator

class CCryLobbyPacket;

enum ECryLobbySystemEvent
{
	eCLSE_EthernetState,                //!< returns CryLobbyEthernetStateData	(gives status of ethernet cable e.g. unplugged)
	eCLSE_OnlineState,                  //!< returns CryLobbyOnlineStateData		(gives status of online connection e.g. signed in)
	eCLSE_InviteAccepted,               //!< returns CryLobbyInviteAcceptedData
	eCLSE_UserInviteAccepted,           //!< returns CryLobbyUserInviteAcceptedData
	eCLSE_NatType,                      //!< returns CryLobbyNatTypeData
	eCLSE_UserPacket,                   //!< returns CryLobbyUserPacketData
	eCLSE_RoomOwnerChanged,             //!< returns CryLobbyRoomOwnerChanged
	eCLSE_SessionUserJoin,              //!< returns SCryLobbySessionUserData
	eCLSE_SessionUserLeave,             //!< returns SCryLobbySessionUserData
	eCLSE_SessionUserUpdate,            //!< returns SCryLobbySessionUserData
	eCLSE_PartyMembers,                 //!< returns SCryLobbyPartyMembers. Called whenever the Live party list changes.
	eCLSE_UserProfileChanged,           //!< returns SCryLobbyUserProfileChanged. Called whenever the user changes a profile setting using the platform UI.
	eCLSE_OnlineNameRejected,           //!< returns SCryLobbyOnlineNameRejectedData.
	eCLSE_FriendOnlineState,            //!< returns SCryLobbyFriendOnlineStateData
	eCLSE_FriendRequest,                //!< returns SCryLobbyFriendIDData
	eCLSE_FriendMessage,                //!< returns SCryLobbyFriendMessageData
	eCLSE_FriendAuthorised,             //!< returns SCryLobbyFriendIDData
	eCLSE_FriendRevoked,                //!< returns SCryLobbyFriendIDData
	eCLSE_FriendsListUpdated,           //!< returns NULL
	eCLSE_ReceivedInvite,               //!< returns SCryLobbyFriendMessageData
	eCLSE_SessionClosed,                //!< returns SCryLobbySessionEventData
	eCLSE_ChatRestricted,               //!< returns SCryLobbyChatRestrictedData
	eCLSE_ForcedFromRoom,               //!< returns SCryLobbyForcedFromRoomData;
	eCLSE_LoginFailed,                  //!< returns SCryLobbyOnlineStateData; for services that don't use the eOS_SigningIn intermediate state
	eCLSE_SessionRequestInfo,           //!< returns SCryLobbySessionRequestInfo
	eCLSE_KickedFromSession,            //!< returns SCryLobbySessionEventData
	eCLSE_KickedHighPing,               //!< returns SCryLobbySessionEventData
	eCLSE_KickedReservedUser,           //!< returns SCryLobbySessionEventData
	eCLSE_KickedLocalBan,               //!< returns SCryLobbySessionEventData
	eCLSE_KickedGlobalBan,              //!< returns SCryLobbySessionEventData
	eCLSE_KickedGlobalBanStage1,        //!< returns SCryLobbySessionEventData
	eCLSE_KickedGlobalBanStage2,        //!< returns SCryLobbySessionEventData
	eCLSE_StartJoiningGameByPlaygroup,  //!< returns NULL
	eCLSE_JoiningGameByPlaygroupResult, //!< returns SCryLobbyJoinedGameByPlaygroupData
	eCLSE_LeavingGameByPlaygroup,       //!< returns SCryLobbySessionEventData
	eCLSE_PlaygroupUserJoin,            //!< returns SCryLobbyPlaygroupUserData
	eCLSE_PlaygroupUserLeave,           //!< returns SCryLobbyPlaygroupUserData
	eCLSE_PlaygroupUserUpdate,          //!< returns SCryLobbyPlaygroupUserData
	eCLSE_PlaygroupClosed,              //!< returns SCryLobbyPlaygroupEventData
	eCLSE_PlaygroupLeaderChanged,       //!< returns SCryLobbyPlaygroupUserData
	eCLSE_KickedFromPlaygroup,          //!< returns SCryLobbyPlaygroupEventData
	eCLSE_PlaygroupUpdated,             //!< returns SCryLobbyPlaygroupEventData
	eCLSE_DedicatedServerSetup,         //!< returns SCryLobbyDedicatedServerSetupData; Dedicated servers allocated via CCryDedicatedServerArbitrator will receive this event when they are setup by the host calling SessionSetupDedicatedServer.
	eCLSE_DedicatedServerRelease,       //!< returns NULL; Dedicated servers allocated via CCryDedicatedServerArbitrator will receive this event when they are released by the host calling SessionReleaseDedicatedServer or when the server losses all of its connections.
	eCLSE_SessionQueueUpdate,           //!< returns SCryLobbySessionQueueData
};

enum ECableState
{
	eCS_Unknown   = -1,
	eCS_Unplugged = 0,
	eCS_Disconnected,         //!< Indicates the network link is down (same as unplugged but means the game was using wireless).
	eCS_Connected,
};

enum EOnlineState
{
	eOS_Unknown   = -1,
	eOS_SignedOut = 0,
	eOS_SigningIn,
	eOS_SignedIn,
};

enum ENatType
{
	eNT_Unknown = -1,
	eNT_Open    = 0,
	eNT_Moderate,
	eNT_Strict
};

enum EForcedFromRoomReason
{
	eFFRR_Unknown = 0,
	eFFRR_Left,
	eFFRR_Kicked,
	eFFRR_ServerForced,
	eFFRR_ServerInternalError,
	eFFRR_ConnectionError,
	eFFRR_SignedOut
};

//! curState will indicate the new state of the ethernet interface.
struct SCryLobbyEthernetStateData
{
	ECableState m_curState;
};

//! m_curState will indicate the new state of the user identified by user (expect multiple of these).
//! m_reason gives you a reason for the state change. Only really meaningful if m_curState is eOS_SignedOut.
struct SCryLobbyOnlineStateData
{
	uint32       m_user;
	EOnlineState m_curState;
	uint32       m_reason;
	bool         m_serviceConnected;
};

// Defined in ICryLobby
struct SCryLobbyFriendIDData;
struct SCryLobbyFriendOnlineStateData;
struct SCryLobbyFriendMessageData;
struct SCryLobbyInviteAcceptedData;

struct SCryLobbyNatTypeData
{
	ENatType m_curState;            //!< Current nat type.
};

struct SCryLobbyUserPacketData
{
	CCryLobbyPacket*             pPacket;
	CrySessionHandle             session;
	SCryMatchMakingConnectionUID connection;
};

struct SCryLobbyRoomOwnerChanged
{
	CrySessionHandle m_session;
	uint32           m_ip;
	uint16           m_port;
};

struct SCryLobbyOnlineNameRejectedData
{
	uint32 m_user;                        //!< Local user index.
	size_t m_numSuggestedNames;           //!< Do not modify. May be 0.
	char** m_ppSuggestedNames;            //!< Do not modify. Will be NULL if m_numSuggestedNames is 0.
};

struct SCryLobbySessionUserData;      //!< Defined in ICryMatchMaking.
struct SCryLobbySessionEventData;     //!< Defined in ICryMatchMaking.
struct SCryLobbyPartyMembers;         //!< Defined in ICryLobby.

struct SCryLobbyUserProfileChanged
{
	uint32 m_user;
};

struct SCryLobbyChatRestrictedData
{
	uint32 m_user;
	bool   m_chatRestricted;
};

struct SCryLobbyForcedFromRoomData
{
	CrySessionHandle      m_session;
	EForcedFromRoomReason m_why;
};

struct SCryLobbySessionQueueData
{
	int m_placeInQueue;
};

struct SCryLobbySessionRequestInfo;         //!< Defined in ICryMatchMaking.
struct SCryLobbyPlaygroupUserData;
struct SCryLobbyPlaygroupEventData;
struct SCryLobbyJoinedGameByPlaygroupData;

struct SCryLobbyDedicatedServerSetupData
{
	CCryLobbyPacket* pPacket;
	CrySessionHandle session;
};

struct SCryLobbyDedicatedServerReleaseData
{
	CrySessionHandle session;
};

union UCryLobbyEventData
{
	SCryLobbyEthernetStateData*          pEthernetStateData;
	SCryLobbyOnlineStateData*            pOnlineStateData;
	SCryLobbyInviteAcceptedData*         pInviteAcceptedData;
	SCryLobbyUserInviteAcceptedData*     pUserInviteAcceptedData;
	SCryLobbyNatTypeData*                pNatTypeData;
	SCryLobbyUserPacketData*             pUserPacketData;
	SCryLobbyRoomOwnerChanged*           pRoomOwnerChanged;
	SCryLobbySessionUserData*            pSessionUserData;
	SCryLobbyPlaygroupUserData*          pPlaygroupUserData;
	SCryLobbyPartyMembers*               pPartyMembers;
	SCryLobbyUserProfileChanged*         pUserProfileChanged;
	SCryLobbyOnlineNameRejectedData*     pSuggestedNames;
	SCryLobbyFriendIDData*               pFriendIDData;
	SCryLobbyFriendOnlineStateData*      pFriendOnlineStateData;
	SCryLobbyFriendMessageData*          pFriendMesssageData;
	SCryLobbySessionEventData*           pSessionEventData;
	SCryLobbyPlaygroupEventData*         pPlaygroupEventData;
	SCryLobbyChatRestrictedData*         pChatRestrictedData;
	SCryLobbyForcedFromRoomData*         pForcedFromRoomData;
	SCryLobbySessionRequestInfo*         pSessionRequestInfo;
	SCryLobbySessionQueueData*           pSessionQueueData;
	SCryLobbyDedicatedServerSetupData*   pDedicatedServerSetupData;
	SCryLobbyDedicatedServerReleaseData* pDedicatedServerReleaseData;
	SCryLobbyJoinedGameByPlaygroupData*  pJoinedByPlaygroupData;
};

typedef void (* CryLobbyEventCallback)(UCryLobbyEventData eventData, void* userParam);

struct SCryLobbyFriendIDData
{
	CryUserID m_user;
};

struct SCryLobbyFriendOnlineStateData : public SCryLobbyFriendIDData
{
	EOnlineState m_curState;    //!< Indicates the new state of the user identified by m_user.
};

#define LOBBY_MESSAGE_SIZE (256)
struct SCryLobbyFriendMessageData : public SCryLobbyFriendIDData
{
	char m_message[LOBBY_MESSAGE_SIZE];
};

#endif // __ICRYLOBBYEVENT_H__
