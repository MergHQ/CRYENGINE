// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryPhysics/IPhysics.h>

namespace CryAudio
{
class CObject;

constexpr size_t g_maxRayHits = 10;

class CRayInfo
{
public:

	CRayInfo() = default;

	void Reset();

	CObject* pObject = nullptr;
	size_t   samplePosIndex = 0;
	size_t   numHits = 0;
	float    totalSoundOcclusion = 0.0f;
	ray_hit  hits[g_maxRayHits];

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	Vec3  startPosition = ZERO;
	Vec3  direction = ZERO;
	float distanceToFirstObstacle = FLT_MAX;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
