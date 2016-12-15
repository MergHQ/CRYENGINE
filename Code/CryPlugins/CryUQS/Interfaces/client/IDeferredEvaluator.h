// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		//===================================================================================
		//
		// IDeferredEvaluator
		//
		//===================================================================================

		struct IDeferredEvaluator
		{
			enum class EUpdateStatus
			{
				BusyButBlockedDueToResourceShortage,      // the caller should not spawn more of these evaluators while this gets returned by at least one of them; it means for example that a raycast queue has exceeded its quota
				BusyWaitingForExternalSchedulerFeedback,  // there's no actual work being done by the deferred-evaluator; basically he's waiting for feedback from an external system that will not arrive before the next frame; no need to waste processing time on him
				BusyDoingTimeSlicedWork,                  // the deferred-evaluator is actually using some of the processor time of the calling thread to do some work; parts of the time-budget will be granted to him
				Finished,
				ExceptionOccurred,
			};

			struct SUpdateContext
			{
				explicit                        SUpdateContext(core::SItemEvaluationResult& _evaluationResult, const core::SQueryBlackboard& _blackboard, shared::IUqsString& _error);
				core::SItemEvaluationResult&    evaluationResult;
				const core::SQueryBlackboard&   blackboard;
				shared::IUqsString&             error;
			};

			virtual                             ~IDeferredEvaluator() {}
			virtual EUpdateStatus               Update(const SUpdateContext& updateContext) = 0;
		};

		inline IDeferredEvaluator::SUpdateContext::SUpdateContext(core::SItemEvaluationResult& _evaluationResult, const core::SQueryBlackboard& _blackboard, shared::IUqsString& _error)
			: evaluationResult(_evaluationResult)
			, blackboard(_blackboard)
			, error(_error)
		{}

	}
}
