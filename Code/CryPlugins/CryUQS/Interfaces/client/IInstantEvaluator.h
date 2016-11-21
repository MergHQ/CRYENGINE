// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
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
				explicit                        SRunContext(core::SItemEvaluationResult& _evaluationResult, const core::SQueryBlackboard& _blackboard, shared::IUqsString& _error);
				core::SItemEvaluationResult&    evaluationResult;
				const core::SQueryBlackboard&   blackboard;
				shared::IUqsString&             error;
			};

			virtual                             ~IInstantEvaluator() {}
			virtual ERunStatus                  Run(const SRunContext& runContext, const void* pParams) const = 0;
		};

		inline IInstantEvaluator::SRunContext::SRunContext(core::SItemEvaluationResult& _evaluationResult, const core::SQueryBlackboard& _blackboard, shared::IUqsString& _error)
			: evaluationResult(_evaluationResult)
			, blackboard(_blackboard)
			, error(_error)
		{}

	}
}
