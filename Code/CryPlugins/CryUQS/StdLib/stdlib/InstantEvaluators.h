// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <CryAISystem/INavigationSystem.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		//===================================================================================
		//
		// CInstantEvaluator_TestMinDistance
		//
		// - discards the item currently being evaluated if the distance between 2 points is smaller than a certain amount
		//
		//===================================================================================

		class CInstantEvaluator_TestMinDistance : public client::CInstantEvaluatorBase<
			CInstantEvaluator_TestMinDistance,
			client::IInstantEvaluatorFactory::ECostCategory::Cheap,
			client::IInstantEvaluatorFactory::EEvaluationModality::Testing
		>
		{
		public:
			struct SParams
			{
				Vec3      pos1;
				Vec3      pos2;
				float     minRequiredDistance;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("pos1", pos1);
					UQS_EXPOSE_PARAM("pos2", pos2);
					UQS_EXPOSE_PARAM("minRequiredDistance", minRequiredDistance);
				UQS_EXPOSE_PARAMS_END
			};

			ERunStatus    DoRun(const SRunContext& runContext, const SParams& params) const;
		};

		//===================================================================================
		//
		// CInstantEvaluator_TestMaxDistance
		//
		// - discards the item currently being evaluated if the distance between 2 points is larger than a certain amount
		//
		//===================================================================================

		class CInstantEvaluator_TestMaxDistance : public client::CInstantEvaluatorBase<
			CInstantEvaluator_TestMaxDistance,
			client::IInstantEvaluatorFactory::ECostCategory::Cheap,
			client::IInstantEvaluatorFactory::EEvaluationModality::Testing
		>
		{
		public:
			struct SParams
			{
				Vec3      pos1;
				Vec3      pos2;
				float     maxAllowedDistance;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("pos1", pos1);
					UQS_EXPOSE_PARAM("pos2", pos2);
					UQS_EXPOSE_PARAM("maxAllowedDistance", maxAllowedDistance);
				UQS_EXPOSE_PARAMS_END
			};

			ERunStatus    DoRun(const SRunContext& runContext, const SParams& params) const;
		};

		//===================================================================================
		//
		// CInstantEvaluator_TestLocationInNavMesh
		//
		// - checks for whether a given Vec3 resides in the NavMesh (roughly on a walkable surface)
		// - discards the item if it's not on the NavMesh
		//
		//===================================================================================

		class CInstantEvaluator_TestLocationInNavMesh : public client::CInstantEvaluatorBase<
			CInstantEvaluator_TestLocationInNavMesh,
			client::IInstantEvaluatorFactory::ECostCategory::Expensive,
			client::IInstantEvaluatorFactory::EEvaluationModality::Testing
		>
		{
		public:
			struct SParams
			{
				Vec3                    locationToTest;
				NavigationAgentTypeID   navigationAgentTypeID;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("locationToTest", locationToTest);
					UQS_EXPOSE_PARAM("navigationAgentTypeID", navigationAgentTypeID);
				UQS_EXPOSE_PARAMS_END
			};

		public:
			explicit                    CInstantEvaluator_TestLocationInNavMesh();
			ERunStatus                  DoRun(const SRunContext& runContext, const SParams& params) const;

		private:
			INavigationSystem*          m_pNavSys;
		};

		//===================================================================================
		//
		// CInstantEvaluator_ScoreDistance
		//
		// - scores the distance between 2 points
		// - the higher the distance the better the score
		// - a threshold is used as a "reference distance" for increasing the score linearly; larger distances clamp the score to 1.0
		//
		//===================================================================================

		class CInstantEvaluator_ScoreDistance : public client::CInstantEvaluatorBase<
			CInstantEvaluator_ScoreDistance,
			client::IInstantEvaluatorFactory::ECostCategory::Cheap,
			client::IInstantEvaluatorFactory::EEvaluationModality::Scoring
		>
		{
		public:
			struct SParams
			{
				Vec3      pos1;
				Vec3      pos2;
				float     distanceThreshold;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("pos1", pos1);
					UQS_EXPOSE_PARAM("pos2", pos2);
					UQS_EXPOSE_PARAM("distanceThreshold", distanceThreshold);
				UQS_EXPOSE_PARAMS_END
			};

			ERunStatus    DoRun(const SRunContext& runContext, const SParams& params) const;
		};

		//===================================================================================
		//
		// CInstantEvaluator_ScoreDistanceInverse
		//
		// - scores the distance between 2 points
		// - the shorter the distance the better the score
		// - a threshold is used as a "reference distance" for decreasing the score linearly; larger distances clamp the score to 0.0
		//
		//===================================================================================

		class CInstantEvaluator_ScoreDistanceInverse : public client::CInstantEvaluatorBase<
			CInstantEvaluator_ScoreDistanceInverse,
			client::IInstantEvaluatorFactory::ECostCategory::Cheap,
			client::IInstantEvaluatorFactory::EEvaluationModality::Scoring
		>
		{
		public:
			struct SParams
			{
				Vec3      pos1;
				Vec3      pos2;
				float     distanceThreshold;

				UQS_EXPOSE_PARAMS_BEGIN
					UQS_EXPOSE_PARAM("pos1", pos1);
					UQS_EXPOSE_PARAM("pos2", pos2);
					UQS_EXPOSE_PARAM("distanceThreshold", distanceThreshold);
				UQS_EXPOSE_PARAMS_END
			};

			ERunStatus    DoRun(const SRunContext& runContext, const SParams& params) const;
		};

		//===================================================================================
		//
		// CInstantEvaluator_ScoreRandom
		//
		// - provides a random score between [0.0 .. 1.0]
		//
		//===================================================================================

		class CInstantEvaluator_ScoreRandom : public client::CInstantEvaluatorBase<
			CInstantEvaluator_ScoreRandom,
			client::IInstantEvaluatorFactory::ECostCategory::Cheap,
			client::IInstantEvaluatorFactory::EEvaluationModality::Scoring
		>
		{
		public:
			struct SParams
			{
				// nothing
				UQS_EXPOSE_PARAMS_BEGIN
				UQS_EXPOSE_PARAMS_END
			};

			ERunStatus    DoRun(const SRunContext& runContext, const SParams& params) const;
		};

	}
}
