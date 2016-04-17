// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryLobby/ICryMatchMaking.h>

#if NETWORK_HOST_MIGRATION
	#include "Context/INetContextState.h"

	#if ENABLE_HOST_MIGRATION_STATE_CHECK
		#define HMSCD_MAX_ENTITY_NAME_LENGTH 16

enum EHostMigrationStateCheckDataType
{
	eHMSCDT_NID
};

struct SHostMigrationStateCheckDataNID
{
	uint32 aspectsEnabled;
	uint16 id;
	uint16 salt;
	char   name[HMSCD_MAX_ENTITY_NAME_LENGTH];
	bool   allocated;
	bool   controlled;
	bool   owned;
	uint8  spawnType;
};

struct SHostMigrationStateCheckData
{
	EHostMigrationStateCheckDataType type;

	union
	{
		SHostMigrationStateCheckDataNID nid;
	};
};
	#endif
#endif

struct ICryMatchMakingPrivate : public ICryMatchMaking
{
public:

	virtual TNetAddress       GetHostAddressFromSessionHandle(CrySessionHandle h) = 0;
	virtual ECryLobbyService  GetLobbyServiceTypeForNubSession() = 0;
	virtual void              SessionDisconnectRemoteConnectionFromNub(CrySessionHandle gh, EDisconnectionCause cause) = 0;
	virtual uint16            GetChannelIDFromGameSessionHandle(CrySessionHandle gh) = 0;
#if NETWORK_HOST_MIGRATION
	virtual bool              IsNubSessionMigrating() const = 0;
	virtual void              HostMigrationStoreNetContextState(INetContextState* pNetContextState) = 0;
	virtual INetContextState* HostMigrationGetNetContextState() = 0;
	virtual void              HostMigrationResetNetContextState() = 0;
	#if ENABLE_HOST_MIGRATION_STATE_CHECK
	virtual void              HostMigrationStateCheckAddDataItem(SHostMigrationStateCheckData* pData) = 0;
	#endif
#endif

	virtual void         LobbyAddrIDTick()                                                                                                {}                // Need to implement if matchmaking service uses LobbyIdAddr's rather than SIPV4Addr
	virtual bool         LobbyAddrIDHasPendingData()                                                                                      { return false; } // Need to implement if matchmaking service uses LobbyIdAddr's rather than SIPV4Addr

	virtual ESocketError LobbyAddrIDSend(const uint8* buffer, uint32 size, const TNetAddress& addr)                                       { return eSE_SocketClosed; } // Need to implement if matchmaking service uses LobbyIdAddr's rather than SIPV4Addr
	virtual void         LobbyAddrIDRecv(void (* cb)(void*, uint8* buffer, uint32 size, CRYSOCKET sock, TNetAddress& addr), void* cbData) {}                           // Need to implement if matchmaking service uses LobbyIdAddr's rather than SIPV4Addr

	virtual bool         IsNetAddressInLobbyConnection(const TNetAddress& addr)                                                           { return true; }
};
