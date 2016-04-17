// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#if !defined(__CRYREBROADCASTER_H__)
#define __CRYREBROADCASTER_H__

class CCryLobby;
struct SRebroadcasterEntry;

class CCryRebroadcaster
{
public:
	CCryRebroadcaster(CCryLobby* pLobby);
	~CCryRebroadcaster();

	/////////////////////////////////////////////////////////////////////////////
	// Rebroadcaster
	bool IsEnabled(void) const;
	void AddConnection(INetChannel* pChannel, TNetChannelID channelID);

#if NETWORK_REBROADCASTER
	void          ShowMesh(void) const;
	void          DebugMode(bool enable) { m_debugMode = enable; }

	TNetChannelID GetLocalChannelID(void) const;

	void          AddConnection(const SRebroadcasterConnection& connection);
	void          DeleteConnection(TNetChannelID channelID, bool remoteDisconnect, bool netsync);
	void          DeleteConnection(const CNetChannel* pChannel, bool remoteDisconnect, bool netsync);

	bool          GetAddressForChannelID(TNetChannelID channelID, TNetAddress& address) const;
	TNetChannelID GetChannelIDForAddress(const TNetAddress& address) const;
	void          SetStatus(TNetChannelID fromChannelID, TNetChannelID toChannelID, ERebroadcasterConnectionStatus status, bool netsync);
	TNetChannelID FindRoute(TNetChannelID fromChannelID, TNetChannelID toChannelID);

	void          SetNetNub(CNetNub* pNetNub);
	void          Reset(void);

	bool          IsInRebroadcaster(TNetChannelID channelID);

protected:
	#if NETWORK_HOST_MIGRATION
	void InitiateHostMigration(uint32 frameDelay = 0);
	#endif

	// Rebroadcaster helpers
	TNetChannelID                  GetHostChannelID(void) const;
	uint32                         GetConnectionCount(void) const;
	bool                           IsServer(const TNetAddress& address) const;
	ERebroadcasterConnectionStatus GetStatus(TNetChannelID fromChannelID, TNetChannelID toChannelID) const;
	ERebroadcasterConnectionStatus GetStatusByIndex(uint32 fromIndex, uint32 toIndex) const;
	void                           SetStatusByIndex(uint32 fromIndex, uint32 toIndex, ERebroadcasterConnectionStatus status, bool netsync);
	void                           DeleteConnectionByIndex(uint32 index, bool netsync);
	uint32                         GetIndexForChannelID(TNetChannelID channelID) const;
	uint32                         GetInsertionIndex(TNetChannelID channelID);
	const char*                    GetNameForIndex(uint32 index) const;
	TNetChannelID                  GetChannelIDForIndex(uint32 index) const;

	/////////////////////////////////////////////////////////////////////////////

	CCryLobby*            m_pLobby;
	SRebroadcasterEntry*  m_pEntries;
	CNetNub*              m_pNetNub;
	mutable TNetChannelID m_routePath[MAXIMUM_NUMBER_OF_CONNECTIONS];
	mutable uint32        m_routePathIndex;
	mutable TNetChannelID m_hostChannelID;
	mutable TNetChannelID m_localChannelID;
	bool                  m_debugMode;
	mutable bool          m_masterEnable;
#endif
};

#endif // End [!defined(__CRYREBROADCASTER_H__)]
// [EOF]
