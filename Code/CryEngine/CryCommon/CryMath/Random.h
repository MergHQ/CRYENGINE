// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/BaseTypes.h>
#include "LCGRandom.h"
#include "MTPseudoRandom.h"

namespace CryRandom_Internal
{
//! Private access point to the global random number generator.
extern CRndGen g_random_generator;
}

inline CRndGen cry_random_next()
{
	return CryRandom_Internal::g_random_generator.Next();
}

//! Seed the global random number generator.
inline void cry_random_seed(const uint32 nSeed)
{
	CryRandom_Internal::g_random_generator.Seed(nSeed);
}

inline uint32 cry_random_uint32()
{
	return CryRandom_Internal::g_random_generator.GenerateUint32();
}

//! Scalar ranged random function.
//! Any orderings work correctly: minValue <= maxValue and minValue >= minValue.
//! \return Random value between minValue and maxValue (inclusive).
template<class T>
inline T cry_random(const T minValue, const T maxValue)
{
	return CryRandom_Internal::g_random_generator.GetRandom(minValue, maxValue);
}

//! Vector (Vec2, Vec3, Vec4) ranged random function.
//! All orderings work correctly: minValue.component <= maxValue.component and
//! minValue.component >= maxValue.component.
//! \return A vector, every component of which is between minValue.component and maxValue.component (inclusive).
template<class T>
inline T cry_random_componentwise(const T& minValue, const T& maxValue)
{
	return CryRandom_Internal::g_random_generator.GetRandomComponentwise(minValue, maxValue);
}

//! \return A random unit vector (Vec2, Vec3, Vec4).
template<class T>
inline T cry_random_unit_vector()
{
	return CryRandom_Internal::g_random_generator.GetRandomUnitVector<T>();
}
