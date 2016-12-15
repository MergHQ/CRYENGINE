// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		//===================================================================================
		//
		// IGenerator
		//
		//===================================================================================

		struct IGenerator
		{
			enum class EUpdateStatus
			{
				StillGeneratingItems,
				FinishedGeneratingItems,
				ExceptionOccurred
			};

			struct SUpdateContext
			{
				explicit                       SUpdateContext(const core::CQueryID& _queryID, const core::SQueryBlackboard& _blackboard, shared::IUqsString& _error);
				core::CQueryID                 queryID;
				const core::SQueryBlackboard&  blackboard;
				shared::IUqsString&            error;
			};

			virtual                            ~IGenerator() {}
			virtual IItemFactory&              GetItemFactory() const = 0;
			virtual EUpdateStatus              Update(const SUpdateContext& updateContext, core::IItemList& itemListToPopulate) = 0;
		};

		inline IGenerator::SUpdateContext::SUpdateContext(const core::CQueryID& _queryID, const core::SQueryBlackboard& _blackboard, shared::IUqsString& _error)
			: queryID(_queryID)
			, blackboard(_blackboard)
			, error(_error)
		{}

	}
}
