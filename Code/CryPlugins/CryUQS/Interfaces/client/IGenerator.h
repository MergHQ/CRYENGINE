// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Client
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
				explicit                       SUpdateContext(const Core::SQueryContext& _queryContext, Shared::IUqsString& _error);
				const Core::SQueryContext&     queryContext;
				Shared::IUqsString&            error;
			};

			virtual                            ~IGenerator() {}
			virtual IItemFactory&              GetItemFactory() const = 0;
			virtual EUpdateStatus              Update(const SUpdateContext& updateContext, Core::IItemList& itemListToPopulate) = 0;
		};

		inline IGenerator::SUpdateContext::SUpdateContext(const Core::SQueryContext& _queryContext, Shared::IUqsString& _error)
			: queryContext(_queryContext)
			, error(_error)
		{}

	}
}
