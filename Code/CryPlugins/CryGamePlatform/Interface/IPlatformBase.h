// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PlatformTypes.h"
#include "PlatformUserIdentifier.h"

namespace Cry
{
	namespace GamePlatform
	{
		//! Interface grouping functionalities common to game platform plugin and services
		struct IBase 
		{
			virtual ~IBase() = default;

			//! Gets a descriptive name of the class
			virtual const char* GetName() const = 0;

			//! Starts a server on the platform, registering it with the network - thus allowing future discovery through the matchmaking systems
			virtual IServer* CreateServer(bool bLocal) = 0;
			//! Gets the currently running platform server started via CreateServer
			virtual IServer* GetLocalServer() const = 0;

			//! Gets the platform's leaderboard implementation, allowing querying and posting to networked leaderboards
			virtual ILeaderboards* GetLeaderboards() const = 0;
			//! Gets the platform's statistics implementation, allowing querying and posting to networked statistics
			virtual IStatistics* GetStatistics() const = 0;
			//! Gets the platform's remote storage implementation, allowing for storage of player data over the network
			virtual IRemoteStorage* GetRemoteStorage() const = 0;
			//! Gets the platform's matchmaking implementation, allowing querying of servers and lobbies
			virtual IMatchmaking* GetMatchmaking() const = 0;
			//! Gets the platform's networking implementation, allowing for sending packets to specific users
			virtual INetworking* GetNetworking() const = 0;

			// Retrieves the authentication token required to communicate with web servers
			virtual bool GetAuthToken(string& tokenOut, int& issuerId) = 0;

			//! Checks the connection to the platform services
			virtual EConnectionStatus GetConnectionStatus() const = 0;
			//! Asynchronously checks whether the user can play Multiplayer
			//! The callback can be executed either during or after CanAcessMultiplayerServices
			//! \param asynchronousCallback The callback to execute when the platform services have responded
			virtual void CanAccessMultiplayerServices(std::function<void(bool authorized)> asynchronousCallback) = 0;
		};
	}
}