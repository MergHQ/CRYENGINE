// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Config.h"
#include "CryLobby.h"
#include "CryRebroadcaster.h"
#include "CryMatchMaking.h"

CCryRebroadcaster::CCryRebroadcaster(CCryLobby* pLobby)
#if NETWORK_REBROADCASTER
	: m_pLobby(pLobby)
	, m_pNetNub(NULL)
	, m_routePathIndex(0)
	, m_hostChannelID(0)
	, m_localChannelID(0)
	, m_debugMode(false)
	, m_masterEnable(true)
#endif
{
#if NETWORK_REBROADCASTER
	assert(pLobby);

	// Initialise the rebroadcaster status mesh
	m_pEntries = new SRebroadcasterEntry[MAXIMUM_NUMBER_OF_CONNECTIONS];
	assert(m_pEntries);
#endif
}

CCryRebroadcaster::~CCryRebroadcaster()
{
#if NETWORK_REBROADCASTER
	SAFE_DELETE_ARRAY(m_pEntries);
#endif
}

bool CCryRebroadcaster::IsEnabled(void) const
{
#if NETWORK_REBROADCASTER
	if (!gEnv->IsEditor() && gEnv->bMultiplayer)
	{
		// Once the rebroadcaster is turned off, it cannot be turned on again until
		// the rebroadcaster has been torn down by all sessions being freed (see CCryMatchMaking::FreeSessionHandle())
		m_masterEnable = m_masterEnable & (gEnv->pConsole->GetCVar("net_enable_rebroadcaster")->GetIVal() != 0);
		return m_masterEnable;
	}
#endif

	return false;
}

#if NETWORK_REBROADCASTER
void CCryRebroadcaster::ShowMesh(void) const
{
	CryFixedStringT<128> temp;

	// Console text colouring:
	// $0 = black | $1 = white  | $2 = blue    | $3 = green  | $4 = red
	// $5 = cyan  | $6 = yellow | $7 = magenta | $8 = orange | %9 = grey

	if (!IsEnabled())
	{
		CryLog("$6Rebroadcaster is disabled - cannot be re-enabled until game disconnects");
		return;
	}

	temp.Format(" ID : Name             : %s", m_debugMode ? "Type : Address                :" : "");
	CryLog(temp);
	uint32 count = GetConnectionCount();
	for (uint32 fromIndex = 0; fromIndex < count; ++fromIndex)
	{
		uint16 fromChannelID = GetChannelIDForIndex(fromIndex);
		temp.Format(" $%i%02i : %-16s : ", (fromIndex % 2) ? 9 : 1, fromChannelID, GetNameForIndex(fromIndex));

		if (m_debugMode)
		{
			CNetChannel* pChannel = (CNetChannel*)m_pEntries[fromIndex].pChannel;
			const char* type = "----";
			TNetAddress address;

			if (pChannel)
			{
				if (pChannel->GetContextView()->IsServer())
				{
					type = "SEVR";
				}
				else if (pChannel->GetContextView()->IsClient())
				{
					type = "CLNT";
				}
				else if (pChannel->GetContextView()->IsPeer())
				{
					type = "REBR";
				}

				address = pChannel->GetIP();
			}
			else
			{
				GetAddressForChannelID(m_pEntries[fromIndex].connection.channelID, address);
			}

			char ipstring[32];
			CNetwork::DecodeAddress(address, ipstring, false);

			temp.Format("%s%s : %-22s : ", temp.c_str(), type, ipstring);
		}

		for (uint32 toIndex = 0; toIndex < count; ++toIndex)
		{
			switch (GetStatusByIndex(fromIndex, toIndex))
			{
			case eRCS_Disabled:
				temp += "$4N";
				break;
			case eRCS_Enabled:
				temp += "$3Y";
				break;
			case eRCS_Unknown: // intentional fall-through
				temp += "$6X";
			default:
				break;
			}
		}

		CryLog(temp);
	}
}

TNetChannelID CCryRebroadcaster::GetHostChannelID(void) const
{
	if (IsEnabled())
	{
		if (m_hostChannelID == 0)
		{
			CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)(m_pLobby->GetMatchMaking());
			if (pMatchMaking)
			{
				m_hostChannelID = (TNetChannelID)pMatchMaking->GetHostConnectionUID();
			}
		}
	}
	else
	{
		m_hostChannelID = 0;
	}

	return m_hostChannelID;
}

TNetChannelID CCryRebroadcaster::GetLocalChannelID(void) const
{
	if (IsEnabled())
	{
		if (m_localChannelID == 0)
		{
			CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)(m_pLobby->GetMatchMaking());

			if (pMatchMaking)
			{
				m_localChannelID = (TNetChannelID)pMatchMaking->GetLocalConnectionUID();
			}
		}
	}
	else
	{
		m_localChannelID = 0;
	}

	return m_localChannelID;
}
#endif

void CCryRebroadcaster::AddConnection(INetChannel* pChannel, TNetChannelID channelID)
{
#if NETWORK_REBROADCASTER
	if (IsEnabled() && pChannel)
	{
		uint32 address = 0;
		uint16 port = 0;
		const char* name = pChannel->GetNickname();
		uint32 newConnectionIndex = MAXIMUM_NUMBER_OF_CONNECTIONS;
		if (CNetwork::DecodeAddress(reinterpret_cast<CNetChannel*>(pChannel)->GetIP(), &address, &port))
		{
			if (channelID == 0)
			{
				channelID = GetChannelIDForAddress(reinterpret_cast<CNetChannel*>(pChannel)->GetIP());
			}

			newConnectionIndex = GetInsertionIndex(channelID);
		}

		// Can we add the connection?
		if (newConnectionIndex < MAXIMUM_NUMBER_OF_CONNECTIONS)
		{
			m_pEntries[newConnectionIndex].pChannel = pChannel;

			if (name != NULL)
			{
				cry_strcpy(m_pEntries[newConnectionIndex].connection.name, name);
			}

			char ipstring[32];
			CNetwork::DecodeAddress(address, port, ipstring, false);
			CryLog("Rebroadcaster: %s connection for channel %i [index %i] (%s)",
			       ((m_pEntries[newConnectionIndex].connection.channelID == 0) ? "Adding" : "Updating"),
			       ((m_pEntries[newConnectionIndex].connection.channelID == 0) ? channelID : m_pEntries[newConnectionIndex].connection.channelID),
			       newConnectionIndex,
			       ipstring);

			// Server is responsible for mesh population
			if (gEnv->bServer && channelID != 0)
			{
				m_pEntries[newConnectionIndex].connection.channelID = channelID;

				// Synchronise the rebroadcaster mesh
				for (uint32 connectionIndex = 0; connectionIndex < MAXIMUM_NUMBER_OF_CONNECTIONS && m_pEntries[connectionIndex].connection.channelID != 0; ++connectionIndex)
				{
					// The new connection needs fully populating with all the entries in the mesh (unless it's us!)
					if (m_pEntries[newConnectionIndex].connection.channelID != GetLocalChannelID())
					{
						CryLog("Rebroadcaster: Sending populate message for channel %i to channel %i", m_pEntries[connectionIndex].connection.channelID, m_pEntries[newConnectionIndex].connection.channelID);
						SRebroadcasterPopulateMessage populateMsg(m_pEntries[connectionIndex].connection);
						CNetChannel::SendRebroadcasterPopulateWith(populateMsg, m_pEntries[newConnectionIndex].pChannel);
					}

					// Existing connections only need to know about the new connection (unless it's us!)
					if (m_pEntries[connectionIndex].connection.channelID != GetLocalChannelID() &&
					    connectionIndex != newConnectionIndex)
					{
						CryLog("Rebroadcaster: Sending populate message for channel %i to channel %i", m_pEntries[newConnectionIndex].connection.channelID, m_pEntries[connectionIndex].connection.channelID);
						SRebroadcasterPopulateMessage populateMsg(m_pEntries[newConnectionIndex].connection);
						CNetChannel::SendRebroadcasterPopulateWith(populateMsg, m_pEntries[connectionIndex].pChannel);
					}
				}
			}
		}
		else
		{
			NET_ASSERT(!"CCryRebroadcaster::AddConnection() : mesh full");
		}
	}
#endif
}

#if NETWORK_REBROADCASTER
void CCryRebroadcaster::AddConnection(const SRebroadcasterConnection& connection)
{
	if (IsEnabled())
	{
		TNetAddress address;
		GetAddressForChannelID(connection.channelID, address);
		char ipstring[32];
		CNetwork::DecodeAddress(address, ipstring, false);

		uint32 index = GetInsertionIndex(connection.channelID);
		CryLog("Rebroadcaster: Received populate message for channel %i (%s)",
		       connection.channelID,
		       ipstring);

		if (index < MAXIMUM_NUMBER_OF_CONNECTIONS)
		{
			if (m_pEntries[index].connection.channelID == 0 &&  // If this is a new connection, and
			    connection.channelID != GetHostChannelID() &&   // not with the server, and
			    !gEnv->bServer &&                               // not *on* the server, and
			    connection.channelID > GetLocalChannelID())     // only with clients more recently joined than me
			{
				CryLog("Rebroadcaster: Creating rebroadcaster channel from channel %i to channel %i (%s)",
				       GetLocalChannelID(),
				       connection.channelID,
				       ipstring);

				// Create a rebroadcaster channel
				m_pNetNub->ConnectTo(ipstring, "RebroadcasterChannel");
			}

			memcpy(m_pEntries[index].connection.name, connection.name, REBROADCASTER_CHANNEL_NAME_SIZE);
			m_pEntries[index].connection.channelID = connection.channelID;
		}

		CryLog("Rebroadcaster: Mesh has %i entries", GetConnectionCount());
	}
}

void CCryRebroadcaster::DeleteConnection(const CNetChannel* pChannel, bool remoteDisconnect, bool netsync)
{
	if (IsEnabled())
	{
		TNetChannelID channelID = 0;
		uint32 count = GetConnectionCount();
		for (uint32 index = 0; index < count; ++index)
		{
			if (m_pEntries[index].pChannel == pChannel)
			{
				channelID = m_pEntries[index].connection.channelID;
			}
		}

		if (channelID > 0)
		{
			DeleteConnection(channelID, remoteDisconnect, netsync);
		}
	}
}

void CCryRebroadcaster::DeleteConnection(TNetChannelID channelID, bool remoteDisconnect, bool netsync)
{
	if (IsEnabled())
	{
		uint32 index = GetIndexForChannelID(channelID);

		// If we are deleting our own connection or the connection to the server, reset the entire mesh
		if (index < MAXIMUM_NUMBER_OF_CONNECTIONS && (channelID == GetHostChannelID() || channelID == GetLocalChannelID()))
		{
			CryLog("Rebroadcaster: Deleting %s channel", channelID == GetLocalChannelID() ? "my" : "server");

			if (remoteDisconnect && (channelID == GetHostChannelID()) && (!gEnv->bServer))
			{
				InitiateHostMigration();
			}
			else
			{
				Reset();
			}
		}
		else
		{
			DeleteConnectionByIndex(index, netsync);
		}
	}
}

void CCryRebroadcaster::DeleteConnectionByIndex(uint32 index, bool netsync)
{
	if (IsEnabled())
	{
		if (index < MAXIMUM_NUMBER_OF_CONNECTIONS)
		{
			uint32 count = GetConnectionCount();

			// Remove the column
			for (uint32 row = 0; row < count; ++row)
			{
				for (uint32 column = index; column < count - 1; ++column)
				{
					SetStatusByIndex(row, column, GetStatusByIndex(row, column + 1), false);
				}

				// Set last column to default status
				SetStatusByIndex(row, count - 1, eRCS_Enabled, false);
			}

			// Remove the row
			TNetAddress addr;
			GetAddressForChannelID(m_pEntries[index].connection.channelID, addr);
			char ipstring[32];
			CNetwork::DecodeAddress(addr, ipstring, false);
			CryLog("Rebroadcaster: Deleting entry for channel %i (%s)",
			       m_pEntries[index].connection.channelID,
			       ipstring);
			memmove(&m_pEntries[index], &m_pEntries[index + 1], sizeof(SRebroadcasterEntry) * (MAXIMUM_NUMBER_OF_CONNECTIONS - index - 1));
			m_pEntries[MAXIMUM_NUMBER_OF_CONNECTIONS - 1].Reset();
			--count;

			if (netsync)
			{
				SRebroadcasterDeleteMessage deleteMsg(m_pEntries[index].connection.channelID);

				for (uint32 deleteIndex = 0; deleteIndex < count; ++deleteIndex)
				{
					// Don't send to ourself
					if (m_pEntries[deleteIndex].connection.channelID != GetLocalChannelID())
					{
						CNetChannel::SendRebroadcasterDeleteWith(deleteMsg, m_pEntries[deleteIndex].pChannel);
					}
				}
			}
		}
	}
}

void CCryRebroadcaster::Reset(void)
{
	CryLog("Rebroadcaster: Deleting entire mesh");

	for (uint32 resetIndex = 0; resetIndex < MAXIMUM_NUMBER_OF_CONNECTIONS; ++resetIndex)
	{
		m_pEntries[resetIndex].Reset();
	}

	m_pNetNub = NULL;
	m_hostChannelID = 0;
	m_localChannelID = 0;
	m_masterEnable = true;
}

bool CCryRebroadcaster::IsInRebroadcaster(TNetChannelID channelID)
{
	uint32 index = GetIndexForChannelID(channelID);
	return (index < MAXIMUM_NUMBER_OF_CONNECTIONS);
}

uint32 CCryRebroadcaster::GetConnectionCount(void) const
{
	uint32 count = 0;

	if (IsEnabled())
	{
		while (m_pEntries[count].connection.channelID != 0)
		{
			++count;
		}
	}

	return count;
}

uint32 CCryRebroadcaster::GetIndexForChannelID(TNetChannelID channelID) const
{
	uint32 count = GetConnectionCount();
	uint32 index = 0;

	if (IsEnabled())
	{
		for (index = 0; index < count; ++index)
		{
			if (m_pEntries[index].connection.channelID == channelID)
			{
				break;
			}
		}
	}

	return (index < count) ? index : MAXIMUM_NUMBER_OF_CONNECTIONS;
}

bool CCryRebroadcaster::GetAddressForChannelID(TNetChannelID channelID, TNetAddress& address) const
{
	bool found = false;

	if (IsEnabled())
	{
		CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)(m_pLobby->GetMatchMaking());
		CRY_ASSERT_MESSAGE(pMatchMaking, "Matchmaking not initialised - rebroadcaster will fail");
		if (pMatchMaking)
		{
			found = pMatchMaking->GetAddressFromChannelID(channelID, address);
		}
		else
		{
			CryLog("Rebroadcaster: *WARNING* matchmaking not initialised - rebroadcaster will fail");
		}
	}

	return found;
}

TNetChannelID CCryRebroadcaster::GetChannelIDForAddress(const TNetAddress& address) const
{
	TNetChannelID channelID = 0;

	if (IsEnabled())
	{
		CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)(m_pLobby->GetMatchMaking());
		CRY_ASSERT_MESSAGE(pMatchMaking, "Matchmaking not initialised - rebroadcaster will fail");
		if (pMatchMaking)
		{
			pMatchMaking->GetChannelIDFromAddress(address, &channelID);
		}
		else
		{
			CryLog("Rebroadcaster: *WARNING* matchmaking not initialised - rebroadcaster will fail");
		}
	}

	return channelID;
}

void CCryRebroadcaster::SetStatus(TNetChannelID fromChannelID, TNetChannelID toChannelID, ERebroadcasterConnectionStatus status, bool netsync)
{
	SetStatusByIndex(GetIndexForChannelID(fromChannelID), GetIndexForChannelID(toChannelID), status, netsync);
}

TNetChannelID CCryRebroadcaster::FindRoute(TNetChannelID fromChannelID, TNetChannelID toChannelID)
{
	TNetChannelID routeChannelID = 0;
	uint32 connectionCount = GetConnectionCount();

	if (IsEnabled())
	{
		if (fromChannelID == toChannelID)
		{
			// Sending to ourselves, so don't need to do anything else
			routeChannelID = toChannelID;
		}
		else
		{
			// Work out a route
			switch (GetStatus(fromChannelID, toChannelID))
			{
			case eRCS_Disabled:
				{
					// No direct route, so need to work one out
					uint32 fromIndex = GetIndexForChannelID(fromChannelID);

					for (uint32 toIndex = 0; !routeChannelID && toIndex < connectionCount; ++toIndex)
					{
						// Check every enabled connection from here (except back to here)
						if (fromIndex != toIndex && GetStatusByIndex(fromIndex, toIndex) == eRCS_Enabled)
						{
							// Check if we've been here before
							bool beenHereBefore = false;
							for (uint32 pathIndex = 0; !beenHereBefore && pathIndex < m_routePathIndex; ++pathIndex)
							{
								if (m_routePath[pathIndex] == m_pEntries[fromIndex].connection.channelID)
								{
									beenHereBefore = true;
								}
							}

							// Store this channel in the path already walked
							if (!beenHereBefore)
							{
								m_routePath[m_routePathIndex++] = m_pEntries[fromIndex].connection.channelID;

								// Try to find a route from this connected channel to the final destination channel.
								// This will recursively find a valid route or eventually bail if there isn't one (returns 0)
								routeChannelID = FindRoute(m_pEntries[toIndex].connection.channelID, toChannelID);
								if (routeChannelID != 0)
								{
									routeChannelID = m_pEntries[toIndex].connection.channelID;
								}

								--m_routePathIndex;
							}
						}
					}
				}
				break;

			case eRCS_Enabled:
			// Direct route, no further processing required
			// Intentional fall-through

			case eRCS_Unknown:
				// The mesh isn't fully synchronised yet - assume this route is valid for now
				routeChannelID = toChannelID;
				break;

			default:
				break;
			}
		}

	#if NETWORK_HOST_MIGRATION
		if (!gEnv->bHostMigrating &&                          // If we're not already migrating the host...
		    routeChannelID == 0 && m_routePathIndex == 0 &&   // and no route could be found (and we're not currently in the middle of finding a route)...
		    toChannelID == GetHostChannelID())                // and the route was to the server...
		{
			CryLogAlways("[Host Migration]: Rebroadcaster unable to find route to server");
			InitiateHostMigration();
		}
	#endif
	}
	else
	{
		routeChannelID = toChannelID;
	}

	return routeChannelID;
}

	#if NETWORK_HOST_MIGRATION
void CCryRebroadcaster::InitiateHostMigration(uint32 frameDelay /* = 0 */)
{
	if (IsEnabled())
	{
		bool autoMigrate = CNetCVars::Get()->netAutoMigrateHost;
		if (gEnv->bHostMigrating || !autoMigrate)
		{
			return;
		}

		uint32 connectionCount = GetConnectionCount();
		uint32 newHostIndex = MAXIMUM_NUMBER_OF_CONNECTIONS;
		for (newHostIndex = 0; newHostIndex < connectionCount; ++newHostIndex)
		{
			if (m_pEntries[newHostIndex].connection.channelID != GetHostChannelID())
			{
				// Found the first non-server client in the list
				// Provided everyone else in the game has an identical list, they should choose the
				// same host - the only time this could go wrong would be if the list were being
				// modified at the time of host migration (i.e. someone joining or someone leaving
				// who's not the server); in this case, the game would 'shard' - but it's an unlikely
				// edge case
				break;
			}
		}

		if (newHostIndex < connectionCount)
		{
			if (m_pEntries[newHostIndex].connection.channelID == GetLocalChannelID())
			{
				// Ooh, pick me!  Pick me!
				CryLog("Rebroadcaster: Host migration initiated - becoming the new host");
				CNetwork::Get()->HostMigration_Start(LOCAL_CONNECTION_STRING, true, frameDelay);
			}
			else
			{
				// And the new server is...
				TNetAddress address;
				GetAddressForChannelID(m_pEntries[newHostIndex].connection.channelID, address);
				char ipstring[23];
				CNetwork::DecodeAddress(address, ipstring, true);
				CryLog("Rebroadcaster: Host migration initiated - new host is %s", ipstring);
				CNetwork::Get()->HostMigration_Start(ipstring, false, frameDelay);

				CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)m_pLobby->GetMatchMaking();

				if (pMatchMaking)
				{
					pMatchMaking->HostMigrationSetNewServerAddress(address);
				}
			}
		}
		else
		{
			// Couldn't find a new host - fail
			CryLog("Rebroadcaster: Host migration failed - unable to determine new host");
		}

		Reset();
	}
}
	#endif

bool CCryRebroadcaster::IsServer(const TNetAddress& address) const
{
	bool isServer = false;

	if (IsEnabled())
	{
		TNetChannelID hostChannelID = GetHostChannelID();
		TNetChannelID channelID = GetChannelIDForAddress(address);

		if (channelID == hostChannelID)
		{
			isServer = true;
		}
	}

	return isServer;
}

void CCryRebroadcaster::SetNetNub(CNetNub* pNetNub)
{
	if (IsEnabled())
	{
		if (m_pNetNub == NULL)
		{
			m_pNetNub = pNetNub;
		}
	}
}

ERebroadcasterConnectionStatus CCryRebroadcaster::GetStatus(TNetChannelID fromChannelID, TNetChannelID toChannelID) const
{
	return GetStatusByIndex(GetIndexForChannelID(fromChannelID), GetIndexForChannelID(toChannelID));
}

ERebroadcasterConnectionStatus CCryRebroadcaster::GetStatusByIndex(uint32 fromIndex, uint32 toIndex) const
{
	ERebroadcasterConnectionStatus status = eRCS_Unknown;

	if (IsEnabled())
	{
		if (fromIndex < MAXIMUM_NUMBER_OF_CONNECTIONS && toIndex < MAXIMUM_NUMBER_OF_CONNECTIONS)
		{
			uint8 mask = 1 << (toIndex & 0x0007);
			uint8 flag = m_pEntries[fromIndex].status[toIndex / 8] & mask;
			flag >>= (toIndex & 0x0007);
			status = static_cast<ERebroadcasterConnectionStatus>(flag);
		}
	}

	return status;
}

void CCryRebroadcaster::SetStatusByIndex(uint32 fromIndex, uint32 toIndex, ERebroadcasterConnectionStatus status, bool netsync)
{
	if (!IsEnabled() || fromIndex == toIndex)
	{
		return;
	}

	// Set the status of a given channel of communication
	NET_ASSERT(fromIndex < MAXIMUM_NUMBER_OF_CONNECTIONS && toIndex < MAXIMUM_NUMBER_OF_CONNECTIONS);
	if (fromIndex < MAXIMUM_NUMBER_OF_CONNECTIONS && toIndex < MAXIMUM_NUMBER_OF_CONNECTIONS)
	{
		uint8 mask = 1 << (toIndex % 8);
		switch (status)
		{
		case eRCS_Enabled:
			m_pEntries[fromIndex].status[toIndex / 8] |= mask;
			break;
		case eRCS_Disabled:
			m_pEntries[fromIndex].status[toIndex / 8] &= ~mask;
			break;
		default:
			break;
		}

		if (netsync)
		{
			SRebroadcasterUpdateMessage updateMsg(m_pEntries[fromIndex].connection.channelID, m_pEntries[toIndex].connection.channelID, status);
			uint32 count = GetConnectionCount();

			for (uint32 index = 0; index < count; ++index)
			{
				// Don't send to ourself
				if (m_pEntries[index].connection.channelID != GetLocalChannelID())
				{
					CNetChannel::SendRebroadcasterUpdateWith(updateMsg, m_pEntries[index].pChannel);
				}
			}
		}
	}
}

uint32 CCryRebroadcaster::GetInsertionIndex(TNetChannelID channelID)
{
	assert(channelID != 0);

	uint32 index = MAXIMUM_NUMBER_OF_CONNECTIONS;

	if (IsEnabled())
	{
		for (index = 0; index < MAXIMUM_NUMBER_OF_CONNECTIONS; ++index)
		{
			if (channelID < m_pEntries[index].connection.channelID)
			{
				// We're using the channel ID, and need to insert a new entry to preserve channel
				// ID order.  Generally speaking, entries will be added in order, but *could* be
				// added out of order because of network latencies.
				uint32 count = GetConnectionCount();
				CryLog("Rebroadcaster: Connection for channel %i received out of order - rearranging connection mesh", channelID);

				NET_ASSERT(count < MAXIMUM_NUMBER_OF_CONNECTIONS);
				if (count < MAXIMUM_NUMBER_OF_CONNECTIONS)
				{
					for (uint32 row = 0; row < count; ++row)
					{
						for (uint32 column = count; column > index; --column)
						{
							// Insert a new column
							SetStatusByIndex(row, column, GetStatusByIndex(row, column - 1), false);
						}

						// Set newly created column to default status
						SetStatusByIndex(row, count, eRCS_Enabled, false);

						if (count - row > index)
						{
							// Insert a new row
							m_pEntries[count - row] = m_pEntries[count - row - 1];
							CryLog("Rebroadcaster: Rearranging - moved entry %i to %i", count - row - 1, count - row);
						}
					}

					m_pEntries[index].Reset(); // Not strictly necessary, but useful to see what's going on in memory
					CryLog("Rebroadcaster: Rearranging - freed up entry %i in connection mesh for in-order insertion of channel %i", index, channelID);
					break;
				}
			}
			else if ((m_pEntries[index].connection.channelID == channelID) ||
			         (m_pEntries[index].connection.channelID == 0))
			{
				// Either we've found an entry that matches the channel ID of the passed INetChannel, or the next free entry
				break;
			}
		}
	}

	// If the index returned is MAXIMUM_NUMBER_OF_CONNECTIONS, then we've not found a
	// matching entry, or we've run out of space in the table.  Either way, this needs
	// to be handled by the caller.
	return index;
}

const char* CCryRebroadcaster::GetNameForIndex(uint32 index) const
{
	uint32 count = GetConnectionCount();
	char* name = NULL;

	if (IsEnabled() && index < count)
	{
		name = m_pEntries[index].connection.name;
	}

	return name;
}

TNetChannelID CCryRebroadcaster::GetChannelIDForIndex(uint32 index) const
{
	uint32 count = GetConnectionCount();
	TNetChannelID channelID = 0;

	if (IsEnabled() && index < count)
	{
		channelID = m_pEntries[index].connection.channelID;
	}

	return channelID;
}

#endif // NETWORK_REBROADCASTER
// [EOF]
