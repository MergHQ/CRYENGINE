// Copyright 2015-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

namespace pfx2
{

// GPU Random number generators based on this article:
//		http://www.reedbeta.com/blog/2013/01/12/quick-and-easy-gpu-random-numbers-in-d3d11/


static const float g_MaxU32toF = 1.0f / float(0xFFFFFFFF);
static const float g_MaxI32toF = 1.0f / float(0x80000000);

// Works for both scalar and SSE
template<typename T>
ILINE T RngXorShift(T rngState)
{
	rngState ^= (rngState << Scalar(13));
	rngState ^= (rngState >> Scalar(17));
	rngState ^= (rngState << Scalar(5));
	return rngState;
}

template<typename T>
ILINE T Jumble(T seed)
{
	seed = RngXorShift(seed);
	seed = (seed ^ convert<T>(61)) ^ (seed >> Scalar(16));
	seed += seed << Scalar(3);
	seed ^= seed >> Scalar(4);
	seed *= convert<T>(0x27d4eb2d);
	seed ^= seed >> Scalar(15);
	return seed;
}

ILINE uint32 SChaosKey::Rand()
{
	return m_key = Jumble(m_key);
}

ILINE uint32 SChaosKey::Rand(uint32 range)
{
	return Rand() % range;
}

ILINE float SChaosKey::RandUNorm()
{
	return float(Rand()) * g_MaxU32toF;
}

ILINE float SChaosKey::RandSNorm()
{
	return float(int32(Rand())) * g_MaxI32toF;  // Cast rand as signed to generate signed float
}
ILINE float SChaosKey::operator()(Range range)
{
	return range(float(Rand()));
}
ILINE SChaosKey::Range::Range(float lo, float hi)
{
	scale = (hi - lo) * g_MaxU32toF;
	start = lo;
}

#ifdef CRY_PFX2_USE_SSE

ILINE SChaosKeyV::SChaosKeyV(uint32 key)
{
	m_keys = convert<uint32v>(key, key + 1, key + 0x100, key + 0x10000);
}

ILINE uint32v SChaosKeyV::Rand()
{
	return m_keys = Jumble(m_keys);
}

ILINE uint32v SChaosKeyV::Rand(uint32 range)
{
	return Rand() % convert<uint32v>(range);
}

ILINE floatv SChaosKeyV::RandUNorm()
{
	// Cast rand as signed, as only signed SIMD int-to-float conversions are available
	int32v iv = Rand();
	floatv fv = ToFloatv(iv);
	return MAdd(fv, ToFloatv(g_MaxU32toF), ToFloatv(0.5f));
}

ILINE floatv SChaosKeyV::RandSNorm()
{
	int32v iv = Rand();
	floatv fv = ToFloatv(iv);
	return fv * ToFloatv(g_MaxI32toF);
}

ILINE floatv SChaosKeyV::operator()(Range range)
{
	int32v iv = Rand();
	return range(ToFloatv(iv));
}

ILINE SChaosKeyV::Range::Range(float lo, float hi)
{
	scale = ToFloatv((hi - lo) * g_MaxU32toF);
	start = ToFloatv((hi + lo) * 0.5f);
}

#endif

}
