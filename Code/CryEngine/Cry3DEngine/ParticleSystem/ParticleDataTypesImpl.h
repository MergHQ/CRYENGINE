// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     30/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLEDATATYPESIMPL_H
#define PARTICLEDATATYPESIMPL_H

#pragma once

namespace pfx2
{

// GPU Random number generators based on this article:
//		http://www.reedbeta.com/blog/2013/01/12/quick-and-easy-gpu-random-numbers-in-d3d11/

template<typename T> ILINE T ToType(uint32 i)          { return i; }
template<> ILINE uint32v     ToType<uint32v>(uint32 i) { return ToUint32v(i); }

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
	seed = (seed ^ ToType<T>(61)) ^ (seed >> Scalar(16));
	seed += seed << Scalar(3);
	seed ^= seed >> Scalar(4);
	seed *= ToType<T>(0x27d4eb2d);
	seed ^= seed >> Scalar(15);
	return seed;
}

ILINE SChaosKey::SChaosKey(SChaosKey key1, SChaosKey key2)
	: m_key(Jumble(key1.m_key + key2.m_key))
{
}

ILINE SChaosKey::SChaosKey(SChaosKey key1, SChaosKey key2, SChaosKey key3)
	: m_key(Jumble(key1.m_key + key2.m_key + key3.m_key))
{
}

ILINE uint32 SChaosKey::Rand()
{
	++m_key;
	uint result = Jumble(m_key);
	return result;
}

ILINE uint32 SChaosKey::Rand(uint32 range)
{
	return Rand() % range;
}

ILINE float SChaosKey::RandUNorm()
{
	return float(Rand()) * (1.0f / 4294967296.0f);
}

ILINE float SChaosKey::RandSNorm()
{
	return RandUNorm() * 2.0f - 1.0f;
}
ILINE float SChaosKey::Rand(Range range)
{
	return float(Rand()) * range.scale + range.bias;
}
ILINE SChaosKey::Range::Range(float lo, float hi)
{
	scale = (hi - lo) * (1.0f / 4294967296.0f);
	bias = lo;
}

ILINE Vec2 SChaosKey::RandCircle()
{
	float c, s;
	const float theta = RandUNorm() * 2.0f * gf_PI;
	sincos_tpl(theta, &s, &c);
	return Vec2(c, s);
}

ILINE Vec2 SChaosKey::RandDisc()
{
	const float dist = sqrtf(RandUNorm());
	return RandCircle() * dist;
}

ILINE Vec3 SChaosKey::RandSphere()
{
	float c, s;
	const float z = RandSNorm();
	const float theta = RandSNorm() * gf_PI;
	const float z2 = sqrtf(1.0f - z * z);
	sincos_tpl(theta, &s, &c);
	return Vec3(c * z2, s * z2, z);
}

#ifdef CRY_PFX2_USE_SSE

ILINE SChaosKeyV::SChaosKeyV()
{
	m_keys = _mm_set_epi32(cry_random_uint32(), cry_random_uint32(), cry_random_uint32(), cry_random_uint32());
}

ILINE uint32v SChaosKeyV::Rand()
{
	m_keys = m_keys + ToUint32v(1);
	return Jumble(m_keys);

}
ILINE uint32v SChaosKeyV::Rand(uint32 range)
{
	return Rand() % ToUint32v(range);
}
ILINE floatv SChaosKeyV::RandUNorm()
{
	uint32v iv = Rand();
	floatv fv = ToFloatv(iv);
	return MAdd(fv, ToFloatv(1.0f / 4294967296.0f), ToFloatv(0.5f));
}
ILINE floatv SChaosKeyV::RandSNorm()
{
	int32v iv = Rand();
	floatv fv = ToFloatv(iv);
	return Mul(fv, ToFloatv(1.0f / 2147483648.0f));
}
ILINE floatv SChaosKeyV::Rand(Range range)
{
	uint32v iv = Rand();
	floatv fv = ToFloatv(iv);
	return MAdd(fv, range.scale, range.bias);
}
ILINE SChaosKeyV::Range::Range(float lo, float hi)
{
	scale = ToFloatv((hi - lo) * (1.0f / 4294967296.0f));
	bias = ToFloatv((hi + lo) * 0.5f);
}

#endif

}

#endif // PARTICLEDATATYPESIMPL_H
