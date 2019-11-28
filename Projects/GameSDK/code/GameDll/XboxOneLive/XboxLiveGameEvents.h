// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _XBOXLIVE_GAMEEVENTS_H_
#define _XBOXLIVE_GAMEEVENTS_H_

#pragma once

#if CRY_PLATFORM_DURANGO
	#define XBOXLIVE_GAME_EVENTS_ENABLED 1
#else
	#define XBOXLIVE_GAME_EVENTS_ENABLED 0
#endif

// Tracking is currently not yet included
#define XBOXLIVE_GAME_EVENT_TRACKING 0

#include <CryNetwork/ISerialize.h>

#if XBOXLIVE_GAME_EVENTS_ENABLED && XBOXLIVE_GAME_EVENT_TRACKING 
	// XBL Events + Debug log (Durango - Debug/Profile)
	#include "XboxLive/RyseEtw.h"
	#define XBL_GAME_EVENT(name, ...) \
		XboxLiveGameEvents::TrackXBLGameEvent(#name, __VA_ARGS__); \
		EventWrite##name(__VA_ARGS__); 
	#define XBL_GAME_EVENT_TODO(name, ...) \
		XboxLiveGameEvents::TrackXBLGameEvent(#name, __VA_ARGS__);

#elif XBOXLIVE_GAME_EVENTS_ENABLED
	// XBL Events (Durango - Release)
	#include "CryEngineSDK_XBLiveEvents.h"
	#define XBL_GAME_EVENT(name, ...) \
		EventWrite##name(__VA_ARGS__); 
	#define XBL_GAME_EVENT_TODO(name, ...)  (void)(0)

#elif XBOXLIVE_GAME_EVENT_TRACKING
	// Debug log (Non Durango platform - Debug/Profile) 
	#define XBL_GAME_EVENT(name, ...) \
		XboxLiveGameEvents::TrackXBLGameEvent(#name, __VA_ARGS__);
	#define XBL_GAME_EVENT_TODO(name, ...) \
		XboxLiveGameEvents::TrackXBLGameEvent(#name, __VA_ARGS__);

#else
	#define XBL_GAME_EVENT(name, ...)  (void)(0)
	#define XBL_GAME_EVENT_TODO(name, ...)  (void)(0)
#endif

struct SUserXUID;

namespace XboxLiveGameEvents
{
	struct EventParamId
	{
		EventParamId();
		EventParamId( const char* name );

		operator int() const
		{
			return m_eventParamId;
		}

		bool Valid() const;

		void Set( const char* name );

		int m_eventParamId;
	};

	void Init();
	void Shutdown();

	// GUID struct that is serializable.
	struct SNetGUID
	{
		SNetGUID()
		{
			ZeroStruct(guid);
		}

		void Serialize(TSerialize ser)
		{
			ser.BeginGroup("guid");
			ser.Value("hi", *reinterpret_cast<int64*>(&guid.Data1));
			ser.Value("lo", *reinterpret_cast<int64*>(guid.Data4));
			ser.EndGroup();
		}

		GUID guid;
	};

	extern const GUID InvalidGUID;
	extern const wchar_t* InvalideCorrelationId;

	bool CreateGUID(GUID& newGUID);
	ILINE bool CreateGUID(SNetGUID& newGUID) { return CreateGUID(newGUID.guid); }

	void GetUserXUID(SUserXUID & userXUID, unsigned int userIdx = 0);

	bool GetXDPLevelId( XboxLiveGameEvents::EventParamId& outParam );
	bool GetXDPGameModeId( XboxLiveGameEvents::EventParamId& outParam );
	void GetXDPAttackContextId( const char* szAttackContext, XboxLiveGameEvents::EventParamId& outParam );

	// general information for multiplayer
	struct SMultiplayerParamIds
	{
		const wchar_t* MPCorrelationId;
		const GUID* pPlaylistId;
		const GUID* pMatchGuid;
		const GUID* pRoundGuid;

		XboxLiveGameEvents::EventParamId godId;
		XboxLiveGameEvents::EventParamId tilesetId;

		SMultiplayerParamIds();
	};

	enum eMultiplayerParamIdType
	{
		eMPIT_ALL						= -1,

		eMPIT_MP_CORRELATION_ID = BIT(0),
		eMPIT_PLAYLIST_ID		= BIT(1),
		eMPIT_MATCH_GUID		= BIT(2),	// playlist runthrough to distinguish two different tries within same playlist in case player choose replay
		eMPIT_ROUND_GUID		= BIT(3),
		eMPIT_GOD_ID				= BIT(3),
		eMPIT_TILESET_ID		= BIT(4),

		eMPIT_MP_SESSION	= eMPIT_MP_CORRELATION_ID,
		eMPIT_PLAYLIST		= eMPIT_MP_SESSION | eMPIT_PLAYLIST_ID | eMPIT_MATCH_GUID | eMPIT_GOD_ID,
		eMPIT_ROUND				= eMPIT_PLAYLIST | eMPIT_ROUND_GUID | eMPIT_TILESET_ID,

	};
	void QueryMultiplayerParamIds(SMultiplayerParamIds& params, uint32 filter = eMPIT_ALL, EntityId entityId = 0);
}

#endif //_XBOXLIVE_GAMEEVENTS_H_
