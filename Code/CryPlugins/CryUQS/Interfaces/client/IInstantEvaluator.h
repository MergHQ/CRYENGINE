// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Client
	{

		//===================================================================================
		//
		// IInstantEvaluator
		//
		//===================================================================================

		struct IInstantEvaluator
		{
			enum class ERunStatus
			{
				Finished,
				ExceptionOccurred
			};

			struct SRunContext
			{
				explicit                        SRunContext(Core::SItemEvaluationResult& _evaluationResult, const Core::SQueryContext& _queryContext, Shared::IUqsString& _error);
				Core::SItemEvaluationResult&    evaluationResult;
				const Core::SQueryContext&      queryContext;
				Shared::IUqsString&             error;
			};

			virtual                             ~IInstantEvaluator() {}
			virtual ERunStatus                  Run(const SRunContext& runContext, const void* pParams) const = 0;
		};

		inline IInstantEvaluator::SRunContext::SRunContext(Core::SItemEvaluationResult& _evaluationResult, const Core::SQueryContext& _queryContext, Shared::IUqsString& _error)
			: evaluationResult(_evaluationResult)
			, queryContext(_queryContext)
			, error(_error)
		{}

	}
}
