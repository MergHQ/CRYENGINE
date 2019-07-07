// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <UnitTest.h>
#include <CryAnimation/ICryAnimation.h>
#include <CryAnimation/CryCharAnimationParams.h>
#include <GlobalAnimationHeaderLMG.h>
#include <ParametricSampler.h>

uint32 MergeParametricExamples(const SParametricSamplerInternal& parametricSampler, f32 mergedExampleWeightsOut[MAX_LMG_ANIMS], int mergedExampleIndicesOut[MAX_LMG_ANIMS]);

uint32 MergeParametricExamples(const uint32 numExamples, const f32* const exampleBlendWeights, const int16* const exampleLocalAnimationIds, f32* mergedExampleWeightsOut, int* mergedExampleIndicesOut);

TEST(MergeParametricExamplesTest, ContainsZeroWeights)
{
	const f32 exampleBlendWeights[] = { 0.f, 1.f, -1.f, 0.f, 1.f };
	const int16 exampleLocalAnimationIds[] = { 22, 44, 44, 22, 44 };
	const uint32 examplesCount = 5;

	int mergedExampleIndices[5];
	f32 mergedExampleWeights[5];
	const uint32 mergedExampleCount = MergeParametricExamples(
		examplesCount, exampleBlendWeights, exampleLocalAnimationIds, mergedExampleWeights, mergedExampleIndices);
	REQUIRE(mergedExampleCount == 2);
	REQUIRE(mergedExampleIndices[0] == 0);
	REQUIRE(mergedExampleIndices[1] == 1);
	REQUIRE(IsEquivalent(mergedExampleWeights[0], 0.f, 0.01f));
	REQUIRE(IsEquivalent(mergedExampleWeights[0], 0.f, 0.01f));
	REQUIRE(IsEquivalent(mergedExampleWeights[1], 1.f, 0.01f));
}

TEST(MergeParametricExamplesTest, NoZeroWeights)
{
	const f32 exampleBlendWeights[] = { -0.1f, 0.9f, -0.1f, 0.f, 0.3f };
	const int16 exampleLocalAnimationIds[] = { 22, 44, 66, 22, 44 };
	const uint32 examplesCount = 5;

	int mergedExampleIndices[5];
	f32 mergedExampleWeights[5];
	const uint32 mergedExampleCount = MergeParametricExamples(examplesCount, exampleBlendWeights, exampleLocalAnimationIds, mergedExampleWeights, mergedExampleIndices);
	REQUIRE(mergedExampleCount == 3);
	REQUIRE(mergedExampleIndices[0] == 0);
	REQUIRE(mergedExampleIndices[1] == 1);
	REQUIRE(mergedExampleIndices[2] == 2);
	REQUIRE(IsEquivalent(mergedExampleWeights[0], -0.1f, 0.01f));
	REQUIRE(IsEquivalent(mergedExampleWeights[1], 1.2f, 0.01f));
	REQUIRE(IsEquivalent(mergedExampleWeights[2], -0.1f, 0.01f));
}

TEST(MergeParametricExamplesTest, WeightsAddToZero)
{
	const f32 exampleBlendWeights[] = { -1.f, 1.f, -1.f, 1.f, 1.f };
	const int16 exampleLocalAnimationIds[] = { 22, 44, 44, 22, 44 };
	const uint32 examplesCount = 5;

	int mergedExampleIndices[5];
	f32 mergedExampleWeights[5];
	const uint32 mergedExampleCount = MergeParametricExamples(examplesCount, exampleBlendWeights, exampleLocalAnimationIds, mergedExampleWeights, mergedExampleIndices);
	REQUIRE(mergedExampleCount == 2);
	REQUIRE(mergedExampleIndices[0] == 0);
	REQUIRE(mergedExampleIndices[1] == 1);
	REQUIRE(IsEquivalent(mergedExampleWeights[0], 0.f, 0.01f));
	REQUIRE(IsEquivalent(mergedExampleWeights[1], 1.f, 0.01f));
}
