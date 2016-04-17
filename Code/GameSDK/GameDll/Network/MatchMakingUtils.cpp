// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "GameCVars.h"

#include "MatchMakingUtils.h"
#include "Lobby/GameLobbyData.h"
#include <CryString/UnicodeFunctions.h>
#include <CryString/StringUtils.h>

#define ROUND_TO_ROUND_HOPPER_NAME "RoundToRound"
#define ROUND_TO_ROUND_PLAYLIST_ID L"00000000-0000-0000-0000-00000000001F"

#pragma warning ( disable:4717 ) // stupid warning in vccorlib.h

#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
#include <ESraLibCore.h>
#include <ESraLibCore/ComicService.h>

#include <Windows.Xbox.System.h>
#include <Windows.Xbox.UI.h>
#include <Windows.Xbox.System.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Microsoft::Xbox::Services::Matchmaking;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Xbox::UI;
using namespace ABI::Windows::Xbox::System;

#include <ESraLibCore/AbiUtil.h>
#include <ESraLibCore/json_util.h>

static const GUID SERVICECONFIG_GUID = 
{ 0xe1039253, 0x2550, 0x49c7, { 0xb7, 0x85, 0x49, 0x34, 0xf0, 0x78, 0xc6, 0x85 }};
#endif

namespace MatchmakingUtils
{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	static std::vector< DeferredUIAction > s_DeferredActionList;
	static CryCriticalSection s_CryCritsec;
	static bool s_ProcessDeferredActions = false;
#endif

	void OnPostUpdate( float )
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		if(!s_ProcessDeferredActions)
			return;

		CryAutoLock<CryCriticalSection> lock(s_CryCritsec);
		for( size_t i = 0; i < s_DeferredActionList.size(); ++i )
		{
			s_DeferredActionList[i]();
		}

		s_DeferredActionList.clear();
#endif
	}
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	void PushDeferredAction( DeferredUIAction action )
	{

		if (action == nullptr)
		{
			return;
		}

		CryAutoLock<CryCriticalSection> lock(s_CryCritsec);
		s_DeferredActionList.push_back( action );

	}
#endif
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	std::shared_ptr< Live::State::User > GetCurrentUser()
	{
		IPlatformOS* pPlatformOS = GetISystem()->GetPlatformOS();
		int userIndex = pPlatformOS->GetFirstSignedInUser();
		if (userIndex == IPlatformOS::Unknown_User)
		{
			return nullptr;
		}

		return Live::State::Manager::Instance().GetUserById( pPlatformOS->UserGetId(userIndex) );
	}
#endif

	void ShowAchievementsUI()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user != nullptr )
		{
			ComPtr<IUser> pUser;
			ComPtr<IAsyncAction> pAction;

			user->GetPlatformUser( &pUser );
			Interface::Statics<SystemUI>()->LaunchAchievementsAsync( pUser.Get(), 0x65D9B841, &pAction );

			auto task = ABI::Concurrency::task_from_async(pAction.Get());

			task.then([](HRESULT)
			{
				PushDeferredAction([]()
				{
					CryLogAlways("Achievement UI dismissed");
				});
			});
		}
#endif
	}

#define MAX_PARTY 3
	EPartyValidationResult IsPartyValid()
	{
		EPartyValidationResult errorCode = ePVR_NoError;
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user != nullptr )
		{
			auto members = Live::State::Party::Instance().GetPartyMembers();
			size_t memberCount = members->size();

			if ( memberCount > MAX_PARTY )
			{
				errorCode = ePVR_PartyTooLarge;
			}

			bool currentUserIsInParty = false;
			size_t localCount = 0;
			for ( auto itr = members->cbegin(); itr != members->cend(); ++itr )
			{
				if ( (*itr).Locality_ == Live::State::Party::Member::Locality::Enum::Local )
				{
					++localCount;
					if( itr->Xuid_ == user->Xuid() )
						currentUserIsInParty = true;
				}
				
			}

			if ( localCount > 1 || ( localCount == 1 && !currentUserIsInParty ) )
			{
				errorCode = ePVR_TooManyLocalUsers;
			}
		}
#endif
		return errorCode;
	}

	void ShowPartyUI()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user != nullptr )
		{
			ComPtr<IUser> pUser;
			ComPtr<IAsyncAction> pAction;

			user->GetPlatformUser( &pUser );
			Interface::Statics<SystemUI>()->ShowSendInvitesAsync( pUser.Get(), &pAction );

			auto task = ABI::Concurrency::task_from_async(pAction.Get());

			task.then([](HRESULT)
			{
				PushDeferredAction([]()
				{
					CryLogAlways("Party UI dismissed");
				});
			});
		}
#endif
	}
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	void SendMatchmakingStatus( Live::MPSessionState state )
	{

		PushDeferredAction([state]()
		{
			CryLogAlways( "MatchmakingUtils: Setting Matchmaking Status: %d", state );
		});

	}
#endif
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	static GUID s_matchStatusChangedGuid = NULL_GUID;

	concurrency::task< HRESULT > MonitorMatchmaking( std::shared_ptr< Live::State::User > user )
	{
		UNREFERENCED_PARAMETER(user);
		SendMatchmakingStatus( Live::MPSessionState::MatchSearching );

		if( IsEqualGUID( s_matchStatusChangedGuid, NULL_GUID ) )
		{
			HRESULT hr = Live::State::Party::Instance().RegisterMatchStatusChangedCallback(
				[]( Live::MultiplayerSession::Ref const & sessionRef, TicketStatus status ) -> void
			{
				// Updating status due to status callback since Party doesn't know which user to update.
				auto user = MatchmakingUtils::GetCurrentUser();
				user->UpdateMatchTicketStatus( sessionRef, status );

				// The ordering in the TicketStatus enum seems to like to change.
				// This will prevent a change to the ordering in the TicketStatus enum
				// from breaking UI.
				Live::MPSessionState uiState = Live::MPSessionState::Unknown;
				switch(status)
				{
				case TicketStatus_Unknown:
					uiState = Live::MPSessionState::Unknown;
					break;
				case TicketStatus_Expired:
					uiState = Live::MPSessionState::MatchTicketExpired;
					break;
				case TicketStatus_Searching:
					uiState = Live::MPSessionState::MatchSearching;
					break;
				case TicketStatus_Found:
					uiState = Live::MPSessionState::MatchFound;
					break;
				case TicketStatus_Canceled:
					uiState = Live::MPSessionState::Canceled;
					break;
				default:
					break;
				}

				SendMatchmakingStatus( uiState );
				CryLogAlways( "MatchmakingUtils: Matchmaking status updated to: %d, %d", status, uiState );

			}, &s_matchStatusChangedGuid);

			assert( hr == S_OK );
			CryLogAlways( "MatchmakingUtils: MonitorMatchmaking returned 0x%08x", hr );
		}

		return concurrency::create_task( [](){ return S_OK; });
	}

	PREFAST_SUPPRESS_WARNING(6102);PREFAST_SUPPRESS_WARNING(6103);
	HRESULT PrepareConstants( string const & gameModeId, string const & playlistId, int mpRank, HSTRING * constants )
	{
		using namespace ABI::Windows::Data::Json;

		if(!constants)
			return E_INVALIDARG;

		wstring hopperName;
		Unicode::Convert(hopperName, gameModeId);
		
		ComPtr<IJsonObject> attr;
		Activate::Instance(&attr);

		// MatchpartitionId
		{
			uint32 val = GameLobbyData::GetVersion();
			
			CryLogAlways( "[MatchmakingUtils] GameLobbyData::GetVersion() (MatchPartitionId) = '%d'", val );

			RETURN_FAILED( util::SetValueJson(attr.Get(), L"MatchPartitionId", val) );
		}

		// Hopper name
		RETURN_FAILED( util::SetValueJson( attr.Get(), L"HopperName", hopperName.c_str() ) );

		// Round to Round players are put in a different hopper, specified by game mode name
		if ( gameModeId == ROUND_TO_ROUND_HOPPER_NAME )
		{
			// all round to round players get the same playlist, which is the single round to round playlist id
			RETURN_FAILED( util::SetValueJson( attr.Get(), L"PlaylistId", ROUND_TO_ROUND_PLAYLIST_ID) );
		}
		else
		{
			// if we're not playing round to round, take custom match on playlist id into account, and 
			// stamp the selected playlist id into the match ticket
			if ( playlistId != "" )
			{
				std::wstring playlistWide( playlistId.begin(), playlistId.end() );

				RETURN_FAILED( util::SetValueJson( attr.Get(), L"PlaylistId", playlistWide.c_str() ) );
			}
		}
		int localRank = mpRank;

#if ENABLE_MATCH_DEBUG_CODE
		localRank = 4;
#endif
		// Rank
		RETURN_FAILED( util::SetValueJson( attr.Get(), L"Rank", localRank ) );

		if(!attr.Get())
		{
			return E_FAIL;
		}
		ComPtr<IJsonValue> val;
		RETURN_FAILED( attr.As(&val) );
		return val->Stringify(constants);
	}

	// The third of the important functions, demonstrates the three parts of 
	// synchronously waiting for a match.  After kicking off the match via MatchmakeForSession 
	// (the one created with CreateSessionAsync) you need to listen for callbacks against
	// RegisterMatchCallback, periodically poll the ticket for status, and also have a drop dead
	// to avoid orphaning a task chain.
	concurrency::task< HRESULT > DoMatchmaking( std::shared_ptr< Live::State::User > user, const string& gameModeId, const string& playlistId, int mpRank )
	{
		using namespace ABI::Windows::Data::Json;

		wstring hopperName;
		Unicode::Convert(hopperName, gameModeId);

		Wrappers::HString ticketAttributes;
		PrepareConstants( gameModeId, playlistId, mpRank, ticketAttributes.GetAddressOf() );

		// The main call for this function.
		return user->MatchmakeForSessionAsync( 
			hopperName.c_str(),
			ticketAttributes.GetRawBuffer(nullptr), 
			Live::State::User::RegisterForParty::True).then(
			[]( HRESULT res )
		{
			if( FAILED( res ) )
			{
				SendMatchmakingStatus( Live::MPSessionState::Error );
			}
			return res;
		}).then(
			[user]( HRESULT res )
		{
			RETURN_FAILED_TASK( res );
			return MonitorMatchmaking( user );
		});
	}

	concurrency::task< HRESULT > PartySession( std::shared_ptr< Live::State::User > user, const string& gameModeId, const string& playlistId )
	{
		// Before matching, you must have a session to contain information 
		// for the matchmaking service to use.  Note the use of RegisterForParty::False.
		// We're not planning on inviting party members to the match.

		HString constants;
		// we pass 0 as the rank, since we aren't matching in the party case, so the rank that gets put in the session doesn't matter
		PrepareConstants( gameModeId, playlistId, 0, constants.GetAddressOf() );
	
		return user->CreateSessionAsync(SERVICECONFIG_GUID, L"RyseProdMatch", constants.GetRawBuffer(nullptr), nullptr, Live::State::User::RegisterForParty::True).then(
			[user, gameModeId, playlistId](HRESULT hr)
		{
			if( FAILED( hr ) )
			{
				SendMatchmakingStatus( Live::MPSessionState::Error );
			}
			return hr;
		});
	}

	// Primary mechanism for testing synchronous and asynchronous match.  The 
	// second of the important functions in this sample.  Shows the basic flow of a 
	// matchmade session:  CreateSessionAsync->MatchmakeForSessionAsync->
	// if(Found) AcceptMatchTicketAsync->AddPropertiesAsync->tell the rest of the title it should
	// be playing a multiplayer game.
	concurrency::task< HRESULT > MatchTest( std::shared_ptr< Live::State::User > user, const string& gameModeId, const string& playlistId, int mpRank )
	{
		// Before matching, you must have a session to contain information 
		// for the matchmaking service to use.  Note the use of RegisterForParty::False.
		// We're not planning on inviting party members to the match.
		using namespace Live;
		using namespace Live::State;
		using namespace ABI::Microsoft::Xbox::Services::Matchmaking;

		auto const matchType = user->GetMatchType();
		auto const matchTicketStatus = user->GetMatchTicketStatus();
		if( matchType == MatchType::PartyMatchSession && matchTicketStatus != TicketStatus_Canceled && matchTicketStatus != TicketStatus_Expired )
		{
			CryLogAlways( "[MatchmakingUtils] MatchTest: Matchmaking in progress, resuming from state: %d", matchTicketStatus );
			return concurrency::create_task([](){ return S_OK; });
		}

		HString constants;
		PrepareConstants( gameModeId, playlistId, mpRank, constants.GetAddressOf() );

		return user->CreateSessionAsync(SERVICECONFIG_GUID, L"MatchTicketSession", constants.GetRawBuffer(nullptr), nullptr, State::User::RegisterForParty::False).then(
			[user, gameModeId, playlistId, mpRank](HRESULT hr)
		{
			if( FAILED( hr ) )
			{
				SendMatchmakingStatus( MPSessionState::Error );
			}
			RETURN_FAILED_TASK(hr);

			return DoMatchmaking( user, gameModeId, playlistId, mpRank );
		});
	}

	// This function exists to exercise the Live::State::User and MultiplayerSession
	// IPSec related code.  We call it as part of the testbed even though we never open a socket to
	// catch bugs before they become a real problem.  See notes on OnAccept for where you'd want to
	// integrate this into a title.
	concurrency::task< HRESULT > EnsureSDAAvailable( std::shared_ptr< Live::State::User > user )
	{
		using namespace ABI::Windows::Xbox::Networking;

		static std::mutex m;
		static std::condition_variable cv;
		static std::atomic<bool> complete;

		complete = false;

		auto timeout = std::make_shared< std::thread >( 
			[]()
		{
			std::unique_lock<std::mutex> lk(m);
			if (!cv.wait_for(lk, std::chrono::seconds(60), []() -> bool { return complete; }))
				complete = true;

			return;
		});

		concurrency::task_completion_event< HRESULT > findAddressResult;

		concurrency::extras::create_iterative_task(
			[user, findAddressResult]()
		{
			// handle cancellation
			{
				std::unique_lock<std::mutex> lk(m);
				if( !!complete )
				{
					return concurrency::create_task(
						[]()
					{
						return std::make_tuple( concurrency::extras::iterative_task_status::complete, E_FAIL );
					});
				}
			}

			unsigned int addresses = 0;
			ComPtr< ISecureDeviceAddress > secureDeviceAddress;
			HRESULT hr = user->GetRemoteSecureDeviceAddresses(1, &secureDeviceAddress, &addresses);
			if(addresses)
			{
				findAddressResult.set(S_OK);
				return concurrency::create_task(
					[hr]()
				{ 
					return std::make_tuple( concurrency::extras::iterative_task_status::complete, hr ); 
				});
			}
			// if not address, do more work then loop
			return concurrency::extras::create_delayed_task( 
				std::chrono::milliseconds(10000), 
				[user]()
			{
				return user->GetSessionAsync().then(
					[](HRESULT hr)
				{
					return std::make_tuple( concurrency::extras::iterative_task_status::incomplete, hr );
				});
			});
		});

		// Actually fires in response to a Found state on the callback.  The sentinel thread
		// indirectly causes it to fire when it deletes the match ticket.
		return concurrency::create_task(findAddressResult).then(
			[user, timeout, findAddressResult]( HRESULT hr )
		{
			{
				std::unique_lock< std::mutex > lk(m);
				if(!complete)
				{
					complete = true;
					cv.notify_one();
				}
			}

			timeout->join();
			return hr;
		});
	}

	void loggingCall( LPCSTR str )
	{
		CryLogAlways( "ESraLibCore: %s", str );
	}

	HRESULT OnActivated( int const activationType )
	{
		CryLogAlways( "MatchmakingUtils: OnActivated called with activationType %d.", activationType );

		{
			Live::State::s_loggingCall = &loggingCall; 
			HRESULT hr = Live::State::Party::Instance().ReregisterCallbacks();
			CryLogAlways( "MatchmakingUtils: OnActivated Party ReregisterCallbacks returned 0x%08x", hr );
		}
		
		// These registrations (and the underlying registrations in Party) need to be hardened to retry at
		// some future point if registration fails.
		static GUID partySessionChangedGuid = NULL_GUID; 
		if( IsEqualGUID( partySessionChangedGuid, NULL_GUID ) )
		{
			HRESULT hr = Live::State::Party::Instance().RegisterPartySessionCallback(
				[]()
			{
				CryLogAlways( "MatchmakingUtils: GameSessionReady callback fired" );
				PartySessionReady();
				gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_MATCHMAKING_GAME_SESSION_READY, 0, 0 );
			}, &partySessionChangedGuid);

			assert( hr == S_OK );
			CryLogAlways( "MatchmakingUtils: OnActivated RegisterPartySession returned 0x%08x", hr );
		}

		static GUID partyMembersChangedGuid = NULL_GUID; 
		if( IsEqualGUID( partyMembersChangedGuid, NULL_GUID ) )
		{
			HRESULT hr = Live::State::Party::Instance().RegisterPartyMembersChangedCallback( 
				[](Live::MultiplayerSession::Ref sessionRef)
			{
				CryLogAlways( "MatchmakingUtils: PartyMembersChanged callback fired" );
				enum 
				{
					Xuid = 0,
					UserPtr
				};

				auto user = MatchmakingUtils::GetCurrentUser();
				[user, sessionRef]()
				{
					if( user->SessionRef() == sessionRef )
					{
						return user->GetSessionAsync( ).then(
							[user]( HRESULT res )
						{
							UNREFERENCED_PARAMETER(res);
							return user->AddAllPartyMembersToSession();
						});
					}

					return concurrency::create_task( [](){ return S_OK; } );
				}().then(
					[]( HRESULT res )
				{
					UNREFERENCED_PARAMETER(res);
					PushDeferredAction(
						[]()
					{
						CryLogAlways("UI: SetPartysize");
					});
				});

				return;
			}, &partyMembersChangedGuid);

			assert( hr == S_OK );
			CryLogAlways( "MatchmakingUtils: OnActivated RegisterPartyMembers returned 0x%08x", hr );
		}

		static bool resumeRegistered = false;
		if(!resumeRegistered)
		{
			using namespace ABI::Windows::Xbox::Multiplayer;
			EventRegistrationToken resumeToken;

			HRESULT hr = Interface::Statics<ABI::Windows::ApplicationModel::Core::ICoreApplication>()->add_Resuming(Callback< IEventHandler< IInspectable *> >(
				[]( IInspectable * source, IInspectable * args ) -> HRESULT
			{
				HRESULT hr = Live::State::Party::Instance().ReregisterCallbacks();
				CryLogAlways( "MatchmakingUtils: OnActivated::Resume Party ReregisterCallbacks returned 0x%08x", hr );
				return S_OK;
			}).Get(), &resumeToken);

			if(SUCCEEDED(hr))
			{
				resumeRegistered = true;
			}
			CryLogAlways( "MatchmakingUtils: OnActivated Resume registration returned 0x%08x", hr );
		}
		
		static bool networkStatusChangedRegistered = false;
		if(!networkStatusChangedRegistered)
		{
			using namespace ABI::Windows::Networking::Connectivity;
			EventRegistrationToken networkStatusChangedToken;

			HRESULT hr = Interface::Statics<INetworkInformation>()->add_NetworkStatusChanged(Callback< INetworkStatusChangedEventHandler >(
				[]( IInspectable * source ) -> HRESULT
			{
				CryLogAlways( "MatchmakingUtils: Network status changed" );
				ComPtr<IConnectionProfile> profile;
				HRESULT res = Interface::Statics<INetworkInformation>()->GetInternetConnectionProfile( &profile );
				if( FAILED(res) || profile.Get() == nullptr )
					return S_OK;

				NetworkConnectivityLevel level = NetworkConnectivityLevel_None;
				profile->GetNetworkConnectivityLevel(&level);
				
				PREFAST_SUPPRESS_WARNING(6102);
				CryLogAlways( "MatchmakingUtils: Network status changed to %d", level );

				if( level != NetworkConnectivityLevel_XboxLiveAccess )
				{
					CryLogAlways( "MatchmakingUtils: Reporting Live connectivity lost to UI" );
					SendMatchmakingStatus( Live::MPSessionState::Error );
				}

				return S_OK;
			}).Get(), &networkStatusChangedToken);

			if(SUCCEEDED(hr))
			{
				networkStatusChangedRegistered = true;
			}
			CryLogAlways( "MatchmakingUtils: OnActivated Network status registration returned 0x%08x", hr );
		}

		static MatchmakingUtils::INetworkingUser_impl s_networkingUser;

		ICryLobby *pLobby = gEnv->pNetwork->GetLobby();
		if (pLobby)
		{
			ICryMatchMaking *pMatchmaking = pLobby->GetMatchMaking();
			if (pMatchmaking)
			{
				pMatchmaking->SetINetworkingUser(&s_networkingUser);
			}
		}	

		ShouldShowMatchFoundPopup();

		return S_OK;
	}
#endif
	
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	// Protocol activation handler.  
	HRESULT OnSraMadeMatchActivation( LPCWSTR scidStr, LPCWSTR templateName, LPCWSTR sessionIdStr )
	{
		using namespace Live::State;

		Live::MultiplayerSession::Ref session;
		{
			RETURN_FAILED( util::GuidFromString( scidStr, &session.ServiceConfigId_ ) );
			session.TemplateName_ = templateName;
			RETURN_FAILED( util::GuidFromString( sessionIdStr, &session.SessionName_ ) );
		}

		return Party::Instance().SetPrearrangedSession( session );
	}

	concurrency::task<bool> IsPartySessionValid( std::shared_ptr<Live::State::User> user )
	{
		using namespace ABI::Microsoft::Xbox::Services::Matchmaking;
		using namespace ABI::Microsoft::Xbox::Services::Multiplayer;
		
		return Live::State::Party::Instance().GetPartyViewAsync().then(
			[user](HRESULT res)
		{
			UNREFERENCED_PARAMETER(res);
			if( Live::State::Party::Instance().GetMatchType() == Live::State::MatchType::PartySession )
			{
				// Get into your group session now.
				// Probably need to check to see if session is stale.
				CryLog( "MatchmakingUtils: IsPartySessionValid: PartySession Found" );

				auto session = std::make_shared< Live::MultiplayerSession >();
				session->Ref_ = Live::State::Party::Instance().PartySessionRef();
				session->Me_.Xuid_ = user->Xuid();

				return user->GetSessionAsync( session ).then(
					[user, session]( HRESULT res )
				{
					bool const userIsInSession = [&]()
					{
						if(FAILED(res))
							return false;

						for( auto const member : session->Members_ )
						{
							if( member.Xuid_ == user->Xuid() )
								return true;
						}
						return false;
					}();

					if( SUCCEEDED(res) && session->Members_.size() >= 2 && userIsInSession )
					{
						MultiplayerSessionMemberStatus const multiplayerSessionStatus = user->StatusInSession();
						CryLog( "MatchmakingUtils: IsPartySessionValid: Status in Session: %d", multiplayerSessionStatus );

						if( multiplayerSessionStatus == MultiplayerSessionMemberStatus_Ready 
							|| multiplayerSessionStatus == MultiplayerSessionMemberStatus_Active 
							|| multiplayerSessionStatus == MultiplayerSessionMemberStatus_Inactive )
						{
							return true;
						}
					}

					CryLog( "MatchmakingUtils: IsPartySessionValid: PartySession is stale, clearing." );
					user->ChangeSessionAsync( session->Ref_.ServiceConfigId_, session->Ref_.TemplateName_.c_str(), session->Ref_.SessionName_ ).then(
						[user]( HRESULT res )
					{
						user->DeleteSessionAsync( Live::State::User::RegisterForParty::True );
					});
					return false;
				});
			}

			return concurrency::create_task([](){ return false; });
		});
	}


	concurrency::task<HRESULT> TryPartySessionReady( std::shared_ptr<Live::State::User> user )
	{
		using namespace ABI::Microsoft::Xbox::Services::Matchmaking;

		auto const pendingMatchType = user->GetPendingMatchType();
		auto const matchType = user->GetMatchType();

		if( matchType != Live::State::MatchType::PartySession && pendingMatchType != Live::State::MatchType::PartySession )
			return concurrency::create_task([](){ return E_NOT_VALID_STATE; });

		return IsPartySessionValid( user ).then(
			[user]( bool isValid )
		{
			if(!isValid)
				return concurrency::create_task([](){ return E_FAIL; });
			
			if(!IsEqualGUID(s_matchStatusChangedGuid, NULL_GUID))
			{
				Live::State::Party::Instance().UnregisterMatchStatusChangedCallback(s_matchStatusChangedGuid);
				s_matchStatusChangedGuid = NULL_GUID;
			}

			SendMatchmakingStatus(Live::MPSessionState::MatchFound);

			return user->DeleteMatchTicketAsync().then( 
				[user]( HRESULT res )
			{
				RETURN_FAILED_TASK( res );
				return user->ChangeToPartySessionAsync().then(
					[user]( HRESULT res )
				{				
					RETURN_FAILED(res);

					PushDeferredAction([user]()
					{
						string xuidString;
						// todo michiel
						//XuidToString(user->Xuid(), xuidString);
						//SendReadyState( xuidString, false );
					});
					return res;
				}).then(
					[user]( HRESULT res )
				{				
					RETURN_FAILED_TASK(res);
					return user->SetReadyState( false );
				});
			});
		});
	}

	concurrency::task<HRESULT> OnTryPartySessionReadySucceeded( std::shared_ptr<Live::State::User> user )
	{
		return EnsureSDAAvailable( user ).then(
			[user]( HRESULT res )
		{
			if(FAILED(res))
			{
				SendMatchmakingStatus( Live::MPSessionState::Error );
				return res;
			}

			Live::State::User::StartPollingSession( 
				[user]()
			{
				return user->GetSessionAsync().then( 
					[user]( HRESULT res ) 
				{
					//todo michiel
					if(FAILED(res)/* || !gEnv->pSystem->GetPlatformOS()->HasNetworkConnectivity()*/)
					{
						CryLogAlways("Error from Polling session");
						user->ClearOtherSessionMembers();
					}
					return res;
				});
			}, 
				[]()
			{
				MatchmakingUtils::PushDeferredAction([]()
				{
					CryLogAlways("UI Update party list");
				});
			});

			SendMatchmakingStatus( Live::MPSessionState::SessionReady );

			return S_OK;
		}).then(
			[user](HRESULT hr)
		{
			// the continuation is here to capture the user object, so it doesn't get deleted
			// while the task is running.
			return hr;
		});
	}
#endif

#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	// This flow is an extracted version of what you want to do to handle a protocol activation
	// specifically for a match success.  It's not a generally useful example.  This code can
	// generally be ignored.
	HRESULT PartySessionReady( )
	{
		CryLog( "MatchmakingUtils: PartySessionReady fired." );
		using namespace ABI::Microsoft::Xbox::Services::Matchmaking;

		auto user = MatchmakingUtils::GetCurrentUser();
		if(!user)
			return E_NOT_VALID_STATE;

		TryPartySessionReady( user ).then(
			[user]( HRESULT res )
		{
			RETURN_FAILED_TASK( res );
			return OnTryPartySessionReadySucceeded( user );
		});
		return S_OK;
	}
#endif
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	HRESULT SetReadyState( const bool isReady )
	{

		auto user = MatchmakingUtils::GetCurrentUser();
		if (user == nullptr)
		{
			CryLogAlways( "[MatchmakingUtils]" __FILE__ "(" STRINGIFY(__LINE__) "): user fetch failed" );
			return E_NOT_VALID_STATE;
		}
				
		user->SetReadyState( isReady ).then(
			[user, isReady]( HRESULT hr )
		{
			PushDeferredAction([user, isReady]()
			{
				string xuidString;
				Live::Xuid xuid = user->Xuid();

				string xuidStr;
				xuidStr.Format( "%lld", xuid );

				wstring x = CryStringUtils::UTF8ToWStr( xuidStr.c_str() );
				CryLogAlways("Send ready state to UI with XUID: %s", x);
			});
		});

		return S_OK;
	}
#endif
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	HRESULT InitiateSession( const string& gameModeId, const string& playlistId, int mpRank )
	{

		{
			HRESULT hr = Live::State::Party::Instance().ReregisterCallbacks();
			CryLogAlways( "MatchmakingUtils: InitiateSession Party ReregisterCallbacks returned 0x%08x", hr );
		}

		std::shared_ptr< Live::State::User > user = MatchmakingUtils::GetCurrentUser();
		if(!user.get() || IsPartyValid() != EPartyValidationResult::ePVR_NoError)
		{
			CryLogAlways( "[MatchmakingUtils]" __FILE__ "(" STRINGIFY(__LINE__) "): user.get failed or party invalid" );
			CryLogAlways("UI: Set party is valid");
			return E_NOT_VALID_STATE;
		}

		if( Live::State::Party::Instance().PartySessionRef().IsValid() )
		{
			CryLogAlways( "MatchmakingUtils: InitiateSession: Pre-existing Party Session Found" );
			TryPartySessionReady( user ).then(
				[user, gameModeId, playlistId, mpRank]( HRESULT res )
			{
				if( FAILED(res) )
				{
					CryLogAlways( "MatchmakingUtils: InitiateSession: Pre-existing Party Session was stale, deleting and matching." );
					return user->DeleteSessionAsync( Live::State::User::RegisterForParty::True ).then(
						[gameModeId, playlistId, mpRank]( HRESULT res )
					{
						UNREFERENCED_PARAMETER(res);
						return InitiateSession(gameModeId, playlistId, mpRank);
					});
				}

				return OnTryPartySessionReadySucceeded( user );
			});
			return S_OK;
		}

		//  Busted logic, cannot handle system with two signed in users in a party with no remote users.
		if(Live::State::Party::Instance().GetPartyMembers()->size() <= 1)
		{
			CryLogAlways( "MatchmakingUtils: InitiateSession: Matchmaking" );
			MatchmakingUtils::MatchTest( user, gameModeId, playlistId, mpRank ).then(
				[user](HRESULT hr)
			{
				using namespace Live::State;

				if( FAILED( hr ) )
				{
					CryLogAlways( "[MatchmakingUtils]" __FILE__ "(" STRINGIFY(__LINE__) "): MatchTest call failed: 0x%08X", hr );
					SendMatchmakingStatus( Live::MPSessionState::Error );
				}
				return hr;
			});

			return S_OK;
		}

		CryLogAlways( "MatchmakingUtils: InitiateSession: Creating Session for Party" );
		MatchmakingUtils::PartySession( user, gameModeId, playlistId ).then(
			[user](HRESULT hr)
		{
			using namespace Live::State;

			if(FAILED( hr))
			{
				CryLogAlways( "[MatchmakingUtils]" __FILE__ "(" STRINGIFY(__LINE__) "): PartySession call failed: 0x%08X", hr );
				SendMatchmakingStatus( Live::MPSessionState::Error );
				return hr;
			}

			// the following code checks to see if we've successfully created a session and can start the game
			auto const pendingMatchType = user->GetPendingMatchType();
			auto const matchType = user->GetMatchType();

			if( matchType == Live::State::MatchType::PartySession || pendingMatchType == Live::State::MatchType::PartySession )
			{
				// if we get here, we know session creation worked and we should start a game
				SendMatchmakingStatus( Live::MPSessionState::MatchFound );
			}
			else
			{
				// if we get here, session creation failed/expired/broke.  Tell the user, change UI to the main MP screen
				SendMatchmakingStatus( Live::MPSessionState::Error );
			}

			return hr;
		});

		return S_OK;
	}
#endif

	void QuitMatchmaking(bool nonBlocking /*= true*/)
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user != nullptr )
		{
			if(!IsEqualGUID(s_matchStatusChangedGuid, NULL_GUID))
			{
				Live::State::Party::Instance().UnregisterMatchStatusChangedCallback(s_matchStatusChangedGuid);
				s_matchStatusChangedGuid = NULL_GUID;
			}

			// user is captured here, to keep it alive until the async task returns
			auto task = user->SetReadyState( false ).then(
				[user](HRESULT)
			{
				return user->DeleteSessionAsync().then(
					[user](HRESULT)
				{
				});
			});

			if (!nonBlocking)
			{
				// primarily for the case where Title gets suspended (power off, being terminated, etc ... )
				// tasks have to be done before the suspended handler returns -- 14/11/2013 yeonwoon
				task.wait();
			}
		}
#endif
	}

	void OnMatchFoundPopupDeclined()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user != nullptr )
		{
			// user is captured here, to keep it alive until the async task returns
			user->ChangeToPartySessionAsync().then(
				[user](HRESULT)
			{
				return user->DeleteSessionAsync();
			}).then(
				[user](HRESULT)
			{
			});
		}
#endif
	}

	void ShouldShowMatchFoundPopup()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		CryLog( "MatchmakingUtils: ShouldShowMatchFoundPopup called" );
		Live::State::Party::Instance().GetPartyViewAsync().then(
			[]( HRESULT )
		{
			using namespace ABI::Microsoft::Xbox::Services::Matchmaking;
			using namespace ABI::Microsoft::Xbox::Services::Multiplayer;

			CryLog( "MatchmakingUtils: ShouldShowMatchFoundPopup PartyView retrieved." );

			auto user = MatchmakingUtils::GetCurrentUser();
			if(!user)
			{
				CryLog( "MatchmakingUtils: ShouldShowMatchFoundPopup no user found." );
				return;
			}

			auto const pendingMatchType = user->GetPendingMatchType();

			CryLog( "MatchmakingUtils: ShouldShowMatchFoundPopup pending match type: %d.", pendingMatchType );

			if( pendingMatchType == Live::State::MatchType::PartyMatchSession )
			{
				CryLog( "MatchmakingUtils: PartyMatchSession Found" );

				user->ChangeToPartyMatchSessionAsync().then(
					[user]( HRESULT res )
				{
					CryLog( "MatchmakingUtils: Change to PartyMatchSession result: 0x%08x", res );

					if(res == S_FALSE)
					{
						RETURN_FAILED_TASK(E_FAIL);
					}

					RETURN_FAILED_TASK(res);
					TicketStatus const currentStatus = user->GetMatchTicketStatus();
					CryLog( "MatchmakingUtils: TicketStatus: %d", currentStatus );

					if(  currentStatus == TicketStatus_Canceled || currentStatus == TicketStatus_Expired )
					{
						return user->DeleteSessionAsync( );
					}

					SendMatchmakingStatus( Live::MPSessionState::MatchSearching );
					return MonitorMatchmaking( user );	
				});
				return;
			}
			else if( pendingMatchType == Live::State::MatchType::PartySession )
			{
				IsPartySessionValid( user ).then(
					[]( bool isValid )
				{
					if(!isValid)
						return;

					PushDeferredAction([]()
					{
						CryLog( "MatchmakingUtils: GAME_SESSION_READY");
						gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_MATCHMAKING_GAME_SESSION_READY, 0, 0 );
					});
					return;
				});
			}
		});
#endif
	}

	void EverythingIsLoadedNow()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		s_ProcessDeferredActions = true;
#endif
	}

	int GetAverageMatchTime()
	{
		int wait = -1;
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user->GetMatchTicketStatus() == TicketStatus_Searching )
		{
			wait = user->GetAverageMatchWaitTime();
		}
#endif
		return wait;
	}

	void AssignLocalUserToGroup( const int groupNumber )
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		std::shared_ptr< Live::State::User > user = MatchmakingUtils::GetCurrentUser();
		if(!user.get())
		{
			return;
		}

		user->SetGroupNumber(groupNumber).then(
			[]( HRESULT res )
		{ 
			PushDeferredAction([res]()
			{
				if ( res == S_OK )
				{
					CryLogAlways("UI: UPDATE PARTY LIST");
				}
				else
				{
					CryLogAlways("UI: Group settings failed");
				}
			});
		}); 
#endif
	}

	wstring GetGamertag( const unsigned long long xuid )
	{
		// Standard default gamertag.  If nothing else, at least it lets you make sure that real ones will fit.
		wstring gt = L"WWWWWWWWWWWWW";

#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if (user != nullptr)
		{
			if (xuid == user->Xuid())
			{
				int userIndex = gEnv->pSystem->GetPlatformOS()->GetFirstSignedInUser();
				int userId = gEnv->pSystem->GetPlatformOS()->UserGetId( userIndex );

				ComPtr<IUser> pUser;
				Interface::Statics<IUser>()->GetUserById( userId, pUser.GetAddressOf() );

				ComPtr<IUserDisplayInfo> dispInfo;
				pUser->get_DisplayInfo( dispInfo.GetAddressOf() );

				HString hstr;
				dispInfo->get_GameDisplayName( hstr.GetAddressOf() );

				gt = hstr.GetRawBuffer( nullptr );
			}
			else
			{
				gt = user->GetGameDisplayName( xuid ).c_str();
			}
		}
#endif

		return gt;
	}

	unsigned long long GetLocalXuid( )
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user != nullptr )
		{
			return user->Xuid();
		}
#endif
		return 0;
	}

	bool IsInvitePending()
	{
		bool invitePending = false;

#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		if (Live::State::Party::Instance().GetReservedMembers())
		{
			if (Live::State::Party::Instance().GetReservedMembers()->size())
			{
				invitePending = true;
			}
		}
#endif

		return invitePending;
	}

	bool IsMemberReady( unsigned long long xuid )
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if ( user != nullptr )
		{
			return user->IsMemberReady( xuid );
		}
#endif
		return false;
	}

	void ShowGamercard( unsigned long long xuid )
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		ComPtr< IUser > user;
		HRESULT hr = E_FAIL;
		int userIndex = gEnv->pSystem->GetPlatformOS()->GetFirstSignedInUser();

		if (userIndex != IPlatformOS::Unknown_User)
		{
			unsigned int requestor = gEnv->pSystem->GetPlatformOS()->UserGetId(userIndex);
			hr = Interface::Statics<IUser>()->GetUserById(requestor, &user);

			if (SUCCEEDED(hr))
			{
				string xuidStr;
				xuidStr.Format( "%lld", xuid );
				
				wstring x = CryStringUtils::UTF8ToWStr( xuidStr.c_str() );

				HStringReference ref(x.c_str());

				ComPtr<ABI::Windows::Foundation::IAsyncAction> action;
				Interface::Statics<ISystemUI>()->ShowProfileCardAsync( user.Get(), ref.Get(), action.GetAddressOf());

				ABI::Concurrency::task_from_async( action ).then([ref, user, x](HRESULT)
				{
					//empty task here to capture the ref-counted objects we need to survive until after the async task completes
				});
			}
		}
#endif
	}

	string GetPlaylistId()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if( user.get() == nullptr )
			return "";

		std::wstring playlistId = user->GetRequestedPlaylistId();
		return std::string( playlistId.begin(), playlistId.end()).c_str();
#else
		return "";
#endif
	}

	wstring GetHopperName()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
	auto user = MatchmakingUtils::GetCurrentUser();
		if( user.get() == nullptr )
			return L"";

		return user->GetHopperName().c_str();
#else
		return L"";
#endif
	}

	bool IsHostForFoundMatch()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if( user.get() == nullptr )
			return false;

		user->StopPollingSession();
		return user->IsHost();
#else
		return false;
#endif
	}

	void SetLaunching()
	{
#if CRY_PLATFORM_DURANGO && defined(SUPPORT_DURANGO_LEGACY_MULTIPLAYER)
		auto user = MatchmakingUtils::GetCurrentUser();
		if( user.get() == nullptr )
			return;

		user->SetLaunching().then(
			[user](HRESULT)
		{
			return Concurrency::extras::create_delayed_task(std::chrono::milliseconds(11000), 
				[user]()
			{
				return user->SetReadyState(false);
			});
		}).then(
			[user](HRESULT)
		{
		});
#endif
	}

}
