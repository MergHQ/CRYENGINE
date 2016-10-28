// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		struct IItemMonitor;  // below

		//===================================================================================
		//
		// ItemMonitorUniquePtr
		//
		//===================================================================================

		typedef std::unique_ptr<IItemMonitor> ItemMonitorUniquePtr;

		//===================================================================================
		//
		// IItemMonitor
		//
		// - can be optionally installed by a generator to monitor changes in the generated items that might yield to a corruption of the reasoning space
		// - item monitors persist throughout the lifetime of a query
		// - depending on whether it's a hierarchical query, the item monitor may get carried over to the next child query until the parent query decides that monitoring is no longer needed
		// - multiple monitors can be installed for a single query
		// - this would be the case if the items are really complex and cover different domains whereas each of them might become corrupt in its own way
		//
		//===================================================================================

		struct IItemMonitor
		{
			enum EHealthState
			{
				StillHealthy,
				CorruptionOccurred,
			};

			virtual               ~IItemMonitor() {}
			virtual EHealthState  UpdateAndCheckForCorruption(shared::IUqsString& outExplanationInCaseOfCorruption) = 0;
		};

	}
}
