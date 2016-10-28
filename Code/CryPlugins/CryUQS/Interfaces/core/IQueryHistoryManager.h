// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// SDebugCameraView
		//
		// - camera view for interactive item debugging in the 3D world
		// - gets passed in from game code to the IHub on each update call
		//
		//===================================================================================

		struct SDebugCameraView
		{
			Vec3   pos;
			Vec3   dir;
		};

		//===================================================================================
		//
		// IQueryHistoryManager
		//
		// - gives access to the histories and allows to control them
		// - this goes hand in hand with the IQueryHistoryConsumer
		// - there are always 2 query histories around: the "live" one which grows whenever a single query instance finishes and a "deserialized" one which was loaded from disk
		//
		//===================================================================================

		struct IQueryHistoryManager
		{
			enum EHistoryOrigin
			{
				Live         = 0,   // designates the "live" query history to which more and more historic queries will be added whenever a single query instance has just finished running
				Deserialized = 1    // designates the "deserialized" query history which got loaded from disk; loading a new one will discard the previously loaded one (but never affect the "live" one, of course)
			};

			virtual                 ~IQueryHistoryManager() {}

			//
			// listener management
			//

			virtual void            RegisterQueryHistoryListener(IQueryHistoryListener* pListener) = 0;
			virtual void            UnregisterQueryHistoryListener(IQueryHistoryListener* pListener) = 0;

			//
			// - 3D debug rendering of the items and general debug primitives
			// - figures out which item is currently focused by the camera and fires according event(s)
			//

			virtual void            UpdateDebugRendering3D(const SDebugCameraView& view) = 0;

			//
			// Saving of the "live" query history and loading a previously saved one back into memory. When loading one back, the "live" one will *not* be affected and just keep
			// growing during gameplay.
			//

			virtual bool            SerializeLiveQueryHistory(const char* xmlFilePath, shared::IUqsString& error) = 0;
			virtual bool            DeserializeQueryHistory(const char* xmlFilePath, shared::IUqsString& error) = 0;

			//
			// selects the query history from which we will be able to select one of its historic queries for rendering their debug primitives in the 3D world
			//

			virtual void            MakeQueryHistoryCurrent(EHistoryOrigin whichHistory) = 0;

			//
			// returns which query history is currently the one from which a historic query can be picked for debug rendering
			//

			virtual EHistoryOrigin  GetCurrentQueryHistory() const = 0;

			//
			// Clears all historic queries from the given query history in memory. This can be used to free some memory or clear entries in a UI control.
			//

			virtual void            ClearQueryHistory(EHistoryOrigin whichHistory) = 0;

			//
			// outputs a list of all historic queries of given query history into the receiver
			//

			virtual void            EnumerateHistoricQueries(EHistoryOrigin whichHistory, IQueryHistoryConsumer& receiver) const = 0;

			//
			// From the current query history, this method picks the one historic query whose debug primitives shall be rendered in the 3D world.
			// Also, the potential item that the camera focuses is one of the items of this historic query.
			//

			virtual void            MakeHistoricQueryCurrentForInWorldRendering(EHistoryOrigin whichHistory, const CQueryID& queryIDToMakeCurrent) = 0;

			//
			// counterpart of MakeHistoricQueryCurrentForInWorldRendering()
			//

			virtual CQueryID        GetCurrentHistoricQueryForInWorldRendering(EHistoryOrigin fromWhichHistory) const = 0;

			//
			// outputs details of the given historic query of given history to the receiver
			//

			virtual void            GetDetailsOfHistoricQuery(EHistoryOrigin whichHistory, const CQueryID& queryIDToGetDetailsFor, IQueryHistoryConsumer& receiver) const = 0;

			//
			// Outputs details of the potentially currently focused item to the receiver. There will only ever be at most one item
			// focused by the camera, so no extra information about the query history and historic query is needed here.
			//

			virtual void            GetDetailsOfFocusedItem(IQueryHistoryConsumer& receiver) const = 0;

			//
			// rough amount of memory (in bytes) used by each of the 2 query histories so far
			//

			virtual size_t          GetRoughMemoryUsageOfQueryHistory(EHistoryOrigin whichHistory) const = 0;

			//
			// number of historic queries stored in given history
			//

			virtual size_t          GetHistoricQueriesCount(EHistoryOrigin whichHistory) const = 0;
		};

	}
}
