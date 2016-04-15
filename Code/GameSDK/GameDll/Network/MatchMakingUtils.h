// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   MatchMakingUtils.h
//  Version:     v1.00
//  Created:     17/06/2013 by Cullen Waters.
//  Description: Encapsulates Matchmaking functionality for main menu.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MATCHMAKINGTILS_H__
#define __MATCHMAKINGTILS_H__

#define ENABLE_MATCH_DEBUG_CODE 0

#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
#include <ESraLibCore.h>
#include <ESraLibCore\ABIUtil_extra.h>
#include <CryNetwork/INetworkingState.h>
#endif

#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
#include <ESraLibCore.h>
#endif


namespace Live
{
	namespace State
	{
		class User;
		class Party;
	}
}

namespace Concurrency
{
	template< typename T >
	class task;
}

namespace MatchmakingUtils
{
	

	void OnPostUpdate( float deltaTime );

	// Networking helper functions
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	typedef std::function< void() > DeferredUIAction;
	void PushDeferredAction( DeferredUIAction action );


	void SendMatchmakingStatus( Live::MPSessionState state );

	std::shared_ptr< Live::State::User > GetCurrentUser();

	class INetworkingUser_impl : public ::Live::State::INetworkingUser
	{
	public:
		INetworkingUser_impl()
		{}

		virtual ~INetworkingUser_impl() 
		{}

		virtual Live::Xuid Xuid() const override
		{
			auto user = GetCurrentUser();
			if(!user)
				return 0;

			return user->Xuid();
		}

		virtual bool IsHost() const override
		{
			auto user = GetCurrentUser();
			if(!user)
				return false;

			return user->IsHost();
		}

		virtual bool HasValidSession() const override
		{
			auto user = GetCurrentUser();
			if(!user)
				return false;

			return user->HasValidSession();
		}

		virtual GUID GetSessionName() override
		{
			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return NULL_GUID;

			return user->SessionRef().SessionName_;	
		}

		// May not need this one
		virtual HRESULT ChangeSessionAsync( GUID const & scid, LPCWSTR const templateName, GUID const & sessionId, ABI::Windows::Foundation::IAsyncAction ** action ) override
		{
			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return E_NOT_VALID_STATE;

			*action = ABI::Concurrency::async_from_task(user->ChangeSessionAsync( scid, templateName, sessionId ) ).Detach();
			return S_OK;
		}

		// May not need this one
		virtual HRESULT DeleteSessionAsync( ABI::Windows::Foundation::IAsyncAction ** action ) override
		{
			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return E_NOT_VALID_STATE;

			*action = ABI::Concurrency::async_from_task( user->SetReadyState( false ).then(
				[user]( HRESULT res )
			{				
				UNREFERENCED_PARAMETER(res);
				return user->DeleteSessionAsync();
			})).Detach();
			return S_OK;
		}

		// May not need this one.
		virtual HRESULT GetSessionAsync( ABI::Windows::Foundation::IAsyncAction ** action ) override
		{
			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return E_NOT_VALID_STATE;

			*action = ABI::Concurrency::async_from_task( user->GetSessionAsync() ).Detach();
			return S_OK;
		}

		virtual HRESULT GetHostSecureDeviceAddress( ABI::Windows::Xbox::Networking::ISecureDeviceAddress ** secureDeviceaddresses, unsigned int * addresses ) const override
		{
			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return E_NOT_VALID_STATE;

			unsigned int sdaCt = 0;
			std::vector< Microsoft::WRL::ComPtr< ABI::Windows::Xbox::Networking::ISecureDeviceAddress > > sdas(1);
			HRESULT const res = user->GetHostSecureDeviceAddress(sdas.data(), &sdaCt);

			if(addresses)
				*addresses = sdaCt;

			RETURN_FAILED(res);
			sdas.resize(sdaCt);

			int ind = 0;
			for( auto & sdaddr : sdas )
			{
				secureDeviceaddresses[ind] = sdaddr.Detach();
				++ind;
			}

			return S_OK;
		}
		
		virtual HRESULT GetRemoteSecureDeviceAddresses( unsigned int const countAddresses, ABI::Windows::Xbox::Networking::ISecureDeviceAddress ** secureDeviceaddresses, unsigned int * addresses ) const override
		{
			if(countAddresses == 0)
				return E_INVALIDARG;

			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return E_NOT_VALID_STATE;

			unsigned int sdaCt = 0;
			std::vector< Microsoft::WRL::ComPtr< ABI::Windows::Xbox::Networking::ISecureDeviceAddress > > sdas(countAddresses);
			HRESULT const res = user->GetRemoteSecureDeviceAddresses(countAddresses, sdas.data(), &sdaCt);
			if(addresses)
				*addresses = sdaCt;

			RETURN_FAILED(res);
			sdas.resize(sdaCt);

			int ind = 0;
			for( auto & sdaddr : sdas )
			{
				secureDeviceaddresses[ind] = sdaddr.Detach();
				++ind;
			}
			return S_OK;
		}

		virtual HRESULT GetRemoteXuids( unsigned int const countXuids, Live::Xuid * xuids, unsigned int * returnedXuids ) const override
		{
			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return E_NOT_VALID_STATE;

			return user->GetRemoteXuids(countXuids, xuids, returnedXuids);
		}

		virtual HRESULT GetHostXuid( Live::Xuid& hostXuid ) const override
		{
			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
				return E_NOT_VALID_STATE;

			return user->GetHostXuid(hostXuid);
		}
	};

	Concurrency::task< HRESULT > DoMatchmaking( std::shared_ptr< Live::State::User > user, const string& gameModeId, const string& playlistId, int mpRank );
	Concurrency::task< HRESULT > PartySession( std::shared_ptr< Live::State::User > user, const string& gameModeId, const string& playlistId );
	Concurrency::task< HRESULT > MatchTest( std::shared_ptr< Live::State::User > user, const string& gameModeId, const string& playlistId, int mpRank );
	Concurrency::task< HRESULT > EnsureSDAAvailable( std::shared_ptr< Live::State::User > user );
	HRESULT OnActivated( int const activationType );
	HRESULT OnSraMadeMatchActivation( LPCWSTR scidStr, LPCWSTR templateName, LPCWSTR sessionIdStr );
#endif

	enum EPartyValidationResult
	{
		ePVR_NoError,
		ePVR_PartyTooLarge,
		ePVR_TooManyLocalUsers,
	};

	EPartyValidationResult IsPartyValid();
	void ShowAchievementsUI();
	void ShowPartyUI( );

	HRESULT InitiateSession( const string& gameModeId, const string& playlistId, int mpRank );

	void AssignLocalUserToGroup( const int groupNumber );

	HRESULT PartySessionReady( );
	void QuitMatchmaking(bool nonBlocking = true);
	void OnMatchFoundPopupDeclined();
	void ShouldShowMatchFoundPopup();
	void AwardComicGold();
	void EverythingIsLoadedNow();
	int GetAverageMatchTime();
	wstring GetGamertag( const unsigned long long xuid );
	unsigned long long GetLocalXuid( );
	bool IsInvitePending();
	bool IsMemberReady( unsigned long long xuid );
	void ShowGamercard( unsigned long long xuid );
	string GetPlaylistId();
	wstring GetHopperName();
	bool IsHostForFoundMatch();
	void SetLaunching();
	HRESULT SetReadyState( const bool isReady );
}

#endif