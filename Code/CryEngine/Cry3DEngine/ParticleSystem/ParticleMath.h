// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParticleCommon.h"
#include <CryMath/Range.h>

namespace pfx2
{

using namespace crymath;

// 4x vector types of standard types
#ifdef CRY_PFX2_USE_SSE
typedef f32v4    floatv;
typedef u32v4    uint32v;
typedef i32v4    int32v;
typedef uint32   uint8v;
typedef mask32v4 maskv;
typedef u32v4    UColv;
#else
typedef float  floatv;
typedef uint32 uint32v;
typedef int32  int32v;
typedef uint8  uint8v;
typedef uint32 maskv;
typedef UCol   UColv;
#endif

// 4x vector types of custom types
typedef uint32v           TParticleIdv;
typedef uint32v           TEmitterIdv;
typedef Vec2_tpl<floatv>  Vec2v;
typedef Vec3_tpl<floatv>  Vec3v;
typedef Vec4_tpl<floatv>  Vec4v;
typedef Quat_tpl<floatv>  Quatv;
typedef Plane_tpl<floatv> Planev;

///////////////////////////////////////////////////////////////////////////
// Vector/scalar conversions
template<typename T> ILINE floatv ToFloatv(T t)  { return convert<floatv>(+t); }

ILINE Vec3v  ToVec3v(Vec3 const& v)   { return Vec3v(v); }
ILINE Vec4v  ToVec4v(Vec4 const& v)   { return Vec4v((Vec4f const&)v); }
ILINE Planev ToPlanev(Plane const& v) { return Planev(ToVec3v(v.n), ToFloatv(v.d)); }

uint8  FloatToUFrac8Saturate(float v);

///////////////////////////////////////////////////////////////////////////
// General operator and function equivalents
template<typename T, typename U> T ILINE Add(const T& a, const U& b)              { return a + b; }
template<typename T, typename U> T ILINE Sub(const T& a, const U& b)              { return a - b; }
template<typename T, typename U> T ILINE Mul(const T& a, const U& b)              { return a * b; }

template<typename T, typename U> T ILINE MAdd(const T& a, const U& b, const T& c) { return a * b + c; }

template<typename T> T& SetMax(T& var, T const& value)                            { return var = std::max(var, value); };
template<typename T> T& SetMin(T& var, T const& value)                            { return var = std::min(var, value); };

template<typename T> T FiniteOr(T val, T alt)                                     { return std::isfinite(val) ? val : alt; }

///////////////////////////////////////////////////////////////////////////
// Vector functions

Vec3v Add(const Vec3v& a, const floatv b);
Vec3v Sub(const Vec3v& a, const floatv b);

Vec3  HMin(const Vec3v& v0);
Vec3  HMax(const Vec3v& v0);

// Efficient vector MAdd

template<typename T> Vec3_tpl<T> MAdd(const Vec3_tpl<T>& a, const Vec3_tpl<T>& b, const Vec3_tpl<T>& c);
template<typename T> Vec3_tpl<T> MAdd(const Vec3_tpl<T>& a, T b, const Vec3_tpl<T>& c);

// Color

Vec3  ToVec3(UCol color);
Vec3v ToVec3v(UColv color);
UCol  ToUCol(const Vec3& color);
UColv ToUColv(const Vec3v& color);

// Return the correct update time for a particle
template<typename T>
T DeltaTime(T frameTime, T normAge, T lifeTime);

// Return the start-time of a particle in the previous frame
template<typename T>
T StartTime(T curTime, T frameTime, T absAge);


///////////////////////////////////////////////////////////////////////////
// Fast approx cube root, implemented by blending between square root and fourth root
struct CubeRootApprox
{
	// Specify point at which cube root is accurate (apart from 0 and 1)
	CubeRootApprox(float anchor)
	{
		float correct = pow(anchor, 0.333333f);
		float anchorSqrt = sqrt_fast(anchor);
		m_blend = (correct - anchorSqrt) / (sqrt_fast(anchorSqrt) - anchorSqrt);
	}

	template<typename T>
	T operator()(T f) const
	{
		T fsqrt = sqrt_fast(f);
		return fsqrt + (sqrt_fast(fsqrt) - fsqrt) * convert<T>(m_blend);
	}
private:
	float m_blend;
};

///////////////////////////////////////////////////////////////////////////
// Polar math

void RotateAxes(Vec3* v0, Vec3* v1, const float angle);
Vec3 PolarCoordToVec3(float azimuth, float altitude);

// Generate points on geometry from parameters

Vec2 CirclePoint(float a);
Vec2 DiskPoint(float a, float r2);
Vec3 SpherePoint(float a, float z);
Vec3 BallPoint(float a, float z, float r3);

}

// Cry_Math_SSE extension
template<> struct SIMD_traits<UCol>
{
	using scalar_t  = UCol;
	using vector4_t = pfx2::UColv;
};


#include "ParticleMathImpl.h"
