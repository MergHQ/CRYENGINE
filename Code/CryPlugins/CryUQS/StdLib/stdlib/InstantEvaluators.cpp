// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryAISystem/IAISystem.h>
#include <CryAISystem/INavigationSystem.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		void CStdLibRegistration::InstantiateInstantEvaluatorFactoriesForRegistration()
		{
			static const client::CInstantEvaluatorFactory<CInstantEvaluator_TestMinDistance> instantEvaluatorFactory_TestMinDistance("std::TestMinDistance");
			static const client::CInstantEvaluatorFactory<CInstantEvaluator_TestMaxDistance> instantEvaluatorFactory_TestMaxDistance("std::TestMaxDistance");
			static const client::CInstantEvaluatorFactory<CInstantEvaluator_TestLocationInNavMesh> instantEvaluatorFactory_TestLocationInNavMesh("std::TestLocationInNavMesh");
			static const client::CInstantEvaluatorFactory<CInstantEvaluator_ScoreDistance> instantEvaluatorFactory_ScoreDistance("std::ScoreDistance");
			static const client::CInstantEvaluatorFactory<CInstantEvaluator_ScoreDistanceInverse> instantEvaluatorFactory_ScoreDistanceInverse("std::ScoreDistanceInverse");
			static const client::CInstantEvaluatorFactory<CInstantEvaluator_ScoreRandom> instantEvaluatorFactory_ScoreRandom("std::ScoreRandom");
		}

		//===================================================================================
		//
		// CInstantEvaluator_TestMinDistance
		//
		//===================================================================================

		client::IInstantEvaluator::ERunStatus CInstantEvaluator_TestMinDistance::DoRun(const SRunContext& runContext, const SParams& params) const
		{
			const float minRequiredDistanceSqr = sqr(params.minRequiredDistance);
			const float actualDistanceSqr = (params.pos1.value - params.pos2.value).GetLengthSquared();

			if (actualDistanceSqr < minRequiredDistanceSqr)
			{
				runContext.evaluationResult.bDiscardItem = true;
			}

			return ERunStatus::Finished;
		}

		//===================================================================================
		//
		// CInstantEvaluator_TestMaxDistance
		//
		//===================================================================================

		client::IInstantEvaluator::ERunStatus CInstantEvaluator_TestMaxDistance::DoRun(const SRunContext& runContext, const SParams& params) const
		{
			const float maxAllowedDistanceSqr = sqr(params.maxAllowedDistance);
			const float actualDistanceSqr = (params.pos1.value - params.pos2.value).GetLengthSquared();

			if (actualDistanceSqr > maxAllowedDistanceSqr)
			{
				runContext.evaluationResult.bDiscardItem = true;
			}

			return ERunStatus::Finished;
		}

		//===================================================================================
		//
		// CInstantEvaluator_TestLocationInNavMesh
		//
		//===================================================================================

		CInstantEvaluator_TestLocationInNavMesh::CInstantEvaluator_TestLocationInNavMesh()
		{
			m_pNavSys = gEnv->pAISystem->GetNavigationSystem();
			assert(m_pNavSys);
		}

		client::IInstantEvaluator::ERunStatus CInstantEvaluator_TestLocationInNavMesh::DoRun(const SRunContext& runContext, const SParams& params) const
		{
			const bool bIsInNavMesh = m_pNavSys->IsLocationValidInNavigationMesh(params.navigationAgentTypeID, params.locationToTest.value);
			runContext.evaluationResult.bDiscardItem = !bIsInNavMesh;
			return ERunStatus::Finished;
		}

		//===================================================================================
		//
		// CInstantEvaluator_ScoreDistance
		//
		//===================================================================================

		client::IInstantEvaluator::ERunStatus CInstantEvaluator_ScoreDistance::DoRun(const SRunContext& runContext, const SParams& params) const
		{
			// guard against potential div-by-zero and other weird behavior
			IF_UNLIKELY(params.distanceThreshold <= 0.0f)
			{
				runContext.error.Format("param 'distanceThreshold' should be > 0.0 (but is: %f)", params.distanceThreshold);
				return ERunStatus::ExceptionOccurred;
			}

			const float distanceBetweenPoints = (params.pos1.value - params.pos2.value).GetLength();
			runContext.evaluationResult.score = std::min(distanceBetweenPoints, params.distanceThreshold) / params.distanceThreshold;
			return ERunStatus::Finished;
		}

		//===================================================================================
		//
		// CInstantEvaluator_ScoreDistanceInverse
		//
		//===================================================================================

		client::IInstantEvaluator::ERunStatus CInstantEvaluator_ScoreDistanceInverse::DoRun(const SRunContext& runContext, const SParams& params) const
		{
			// guard against potential div-by-zero and other weird behavior
			IF_UNLIKELY(params.distanceThreshold <= 0.0f)
			{
				runContext.error.Format("param 'distanceThreshold' should be > 0.0 (but is: %f)", params.distanceThreshold);
				return ERunStatus::ExceptionOccurred;
			}

			const float distanceBetweenPoints = (params.pos1.value - params.pos2.value).GetLength();
			runContext.evaluationResult.score = 1.0f - std::min(distanceBetweenPoints, params.distanceThreshold) / params.distanceThreshold;
			return ERunStatus::Finished;
		}

		//===================================================================================
		//
		// CInstantEvaluator_ScoreRandom
		//
		//===================================================================================

		client::IInstantEvaluator::ERunStatus CInstantEvaluator_ScoreRandom::DoRun(const SRunContext& runContext, const SParams& params) const
		{
			runContext.evaluationResult.score = cry_random(0.0f, 1.0f);
			return ERunStatus::Finished;
		}

	}
}
