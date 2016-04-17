// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

//
//	File:Cry_Math.h
//	Description: Common math class
//
//	History:
//	-Feb 27,2003: Created by Ivo Herzeg
//
//////////////////////////////////////////////////////////////////////

#ifndef CRYMATH_H
#define CRYMATH_H

#if _MSC_VER > 1000
	#pragma once
#endif

//========================================================================================

#include <CryCore/Platform/platform.h>
#include "Cry_ValidNumber.h"
#include <CryCore/CryEndian.h>  // eLittleEndian
#include <CryMath/CryHalf.inl>
#include <CryCore/MetaUtils.h>
#include <float.h>

//! Only enable math asserts in debug builds
#if defined(_DEBUG)
	#define CRY_MATH_ASSERT(x) CRY_ASSERT_MESSAGE(x, "CRY_MATH_ASSERT")
#else
	#define CRY_MATH_ASSERT(x)
#endif

// Forward declarations
template<typename F> struct Vec2_tpl;
template<typename F> struct Vec3_tpl;
template<typename F> struct Vec4_tpl;

template<typename F> struct Ang3_tpl;
template<typename F> struct Plane_tpl;
template<typename F> struct AngleAxis_tpl;
template<typename F> struct Quat_tpl;
template<typename F> struct QuatT_tpl;
template<typename F> struct DualQuat_tpl;
template<typename F> struct QuatTS_tpl;
template<typename F> struct QuatTNS_tpl;

template<typename F> struct Diag33_tpl;
template<typename F> struct Matrix33_tpl;
template<typename F> struct Matrix34_tpl;
template<typename F> struct Matrix44_tpl;

// Definitions
const f32 gf_PI = f32(3.14159265358979323846264338327950288419716939937510);
const f64 g_PI = 3.14159265358979323846264338327950288419716939937510;       //!< pi

const f32 gf_PI2 = f32(3.14159265358979323846264338327950288419716939937510 * 2.0);
const f64 g_PI2 = 3.14159265358979323846264338327950288419716939937510 * 2.0; //!< 2*pi

const f64 sqrt2 = 1.4142135623730950488016887242097;
const f64 sqrt3 = 1.7320508075688772935274463415059;

#ifndef MAX
	#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
	#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define VEC_EPSILON (0.05f)
#define RAD_EPSILON (0.01f)
#define DEG2RAD(a)   ((a) * (gf_PI / 180.0f))
#define RAD2DEG(a)   ((a) * (180.0f / gf_PI))
#define DEG2COS(a)   (cos_tpl((a) * (gf_PI / 180.0f)))
#define COS2DEG(a)   (acos_tpl(a) * (180.0f / gf_PI))
#define RAD2HCOS(a)  (cos_tpl((a * 0.5f)))
#define HCOS2RAD(a)  (acos_tpl(a) * 2.0f)
#define DEG2HCOS(a)  (cos_tpl((a * 0.5f) * (gf_PI / 180.0f)))
#define DEG2HSIN(a)  (sin_tpl((a * 0.5f) * (gf_PI / 180.0f)))
#define HCOS2DEG(a)  (acos_tpl(a) * 2.0f * (180.0f / gf_PI))
#define SIGN_MASK(x) ((intptr_t)(x) >> ((sizeof(size_t) * 8) - 1))
#define TANGENT30   0.57735026918962576450914878050196f     // tan(30)
#define TANGENT30_2 0.57735026918962576450914878050196f * 2 // 2*tan(30)
#define LN2         0.69314718055994530941723212145818f     // ln(2)

ILINE f32                 __fsel(const f32 _a, const f32 _b, const f32 _c)  { return (_a < 0.0f) ? _c : _b; }
ILINE f64                 __fsel(const f64 _a, const f64 _b, const f64 _c)  { return (_a < 0.0f) ? _c : _b; }
ILINE f32                 __fself(const f32 _a, const f32 _b, const f32 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f32                 __fsels(const f32 _a, const f32 _b, const f32 _c) { return (_a < 0.0f) ? _c : _b; }
ILINE f32                 __fres(const f32 _a)                              { return 1.f / _a; }
template<class T> ILINE T isel(int c, T a, T b)                             { return (c < 0) ? b : a; }
template<class T> ILINE T isel(int64 c, T a, T b)                           { return (c < 0) ? b : a; }
template<class T> ILINE T iselnz(int c, T a, T b)                           { return c ? a : b; }
template<class T> ILINE T iselnz(uint32 c, T a, T b)                        { return c ? a : b; }
template<class T> ILINE T iselnz(int64 c, T a, T b)                         { return c ? a : b; }
template<class T> ILINE T iselnz(uint64 c, T a, T b)                        { return c ? a : b; }

//! Provides fast way of checking against 0 (saves fcmp).
ILINE bool fzero(const float& val) { return val == 0.0f; }

//! Provides fast way of checking against 0 (saves fcmp).
ILINE bool fzero(float* pVal) { return *pVal == 0.0f; }

// Define min/max
#ifdef min
	#undef min
#endif //min

#ifdef max
	#undef max
#endif //max

// Bring min and max from std namespace to global scope.
template<class T> ILINE T                        min(const T& a, const T& b)                { return b < a ? b : a; }
template<class T> ILINE T                        max(const T& a, const T& b)                { return a < b ? b : a; }
template<class T, class _Compare> ILINE const T& min(const T& a, const T& b, _Compare comp) { return comp(b, a) ? b : a; }
template<class T, class _Compare> ILINE const T& max(const T& a, const T& b, _Compare comp) { return comp(a, b) ? b : a; }
ILINE int                                        min_branchless(int a, int b)               { int diff = a - b; int mask = diff >> 31; return (b & (~mask)) | (a & mask); }

template<class T> ILINE T                        clamp_tpl(T X, T Min, T Max)               { return X < Min ? Min : X < Max ? X : Max; }
template<class T> ILINE void                     Limit(T& val, const T& min, const T& max)
{
	if (val < min)
		val = min;
	else if (val > max)
		val = max;
}
template<class T> ILINE T Lerp(const T& a, const T& b, float s) { return T(a + (b - a) * s); }

// the portability functions for CPU_X86

#if CRY_PLATFORM_SSE2
	#include <xmmintrin.h>
#endif

ILINE f32   fabs_tpl(f32 op)                            { return fabsf(op); }
ILINE f64   fabs_tpl(f64 op)                            { return fabs(op); }
ILINE int32 fabs_tpl(int32 op)                          { int32 mask = op >> 31; return op + mask ^ mask; }

ILINE f32   floor_tpl(f32 op)                           { return floorf(op); }
ILINE f64   floor_tpl(f64 op)                           { return floor(op); }
ILINE int32 floor_tpl(int32 op)                         { return op; }

ILINE f32   ceil_tpl(f32 op)                            { return ceilf(op); }
ILINE f64   ceil_tpl(f64 op)                            { return ceil(op); }
ILINE int32 ceil_tpl(int32 op)                          { return op; }

ILINE f32   fmod_tpl(f32 x, f32 y)                      { return (f32)fmodf(x, y); }
ILINE f64   fmod_tpl(f64 x, f64 y)                      { return (f32)fmod(x, y); }

ILINE void  sincos_tpl(f32 angle, f32* pSin, f32* pCos) { *pSin = f32(sin(angle));  *pCos = f32(cos(angle));  }
ILINE void  sincos_tpl(f64 angle, f64* pSin, f64* pCos) { *pSin = f64(sin(angle));  *pCos = f64(cos(angle));  }

ILINE f32   cos_tpl(f32 op)                             { return cosf(op); }
ILINE f64   cos_tpl(f64 op)                             { return cos(op); }

ILINE f32   sin_tpl(f32 op)                             { return sinf(op); }
ILINE f64   sin_tpl(f64 op)                             { return sin(op); }

ILINE f32   acos_tpl(f32 op)                            { return acosf(clamp_tpl(op, -1.0f, +1.0f)); }
ILINE f64   acos_tpl(f64 op)                            { return acos(clamp_tpl(op, -1.0, +1.0)); }

ILINE f32   asin_tpl(f32 op)                            { return asinf(clamp_tpl(op, -1.0f, +1.0f)); }
ILINE f64   asin_tpl(f64 op)                            { return asin(clamp_tpl(op, -1.0, +1.0)); }

ILINE f32   atan_tpl(f32 op)                            { return atanf(op); }
ILINE f64   atan_tpl(f64 op)                            { return atan(op); }

ILINE f32   atan2_tpl(f32 op1, f32 op2)                 { return atan2f(op1, op2); }
ILINE f64   atan2_tpl(f64 op1, f64 op2)                 { return atan2(op1, op2); }

ILINE f32   tan_tpl(f32 op)                             { return tanf(op); }
ILINE f64   tan_tpl(f64 op)                             { return tan(op); }

ILINE f32   exp_tpl(f32 op)                             { return expf(op); }
ILINE f64   exp_tpl(f64 op)                             { return exp(op); }

ILINE f32   log_tpl(f32 op)                             { return logf(op); }
ILINE f64   log_tpl(f64 op)                             { return log(op); }

ILINE f32   pow_tpl(f32 x, f32 y)                       { return (f32) pow((f64)x, (f64)y); }
ILINE f64   pow_tpl(f64 x, f64 y)                       { return pow(x, y); }

#if CRY_PLATFORM_SSE2
ILINE f32 sqrt_tpl(f32 op)        { __m128 s = _mm_sqrt_ss(_mm_set_ss(op));  float r; _mm_store_ss(&r, s);   return r; }
ILINE f64 sqrt_tpl(f64 op)        { return sqrt(op); }

ILINE f32 sqrt_fast_tpl(f32 op)   { __m128 s = _mm_sqrt_ss(_mm_set_ss(op)); float r; _mm_store_ss(&r, s);   return r; }
ILINE f64 sqrt_fast_tpl(f64 op)   { return sqrt(op); }

ILINE f32 isqrt_tpl(f32 op)       { __m128 s = _mm_rsqrt_ss(_mm_set_ss(op));  float r; _mm_store_ss(&r, s);   return r * (1.5f - op * r * r * 0.5f); }
ILINE f64 isqrt_tpl(f64 op)       { return 1.0 / sqrt(op); }

ILINE f32 isqrt_fast_tpl(f32 op)  { __m128 s = _mm_rsqrt_ss(_mm_set_ss(op)); float r; _mm_store_ss(&r, s);   return r * (1.5f - op * r * r * 0.5f); }
ILINE f64 isqrt_fast_tpl(f64 op)  { return 1.0 / sqrt(op); }

ILINE f32 isqrt_safe_tpl(f32 op2) { f32 op = op2 + +FLT_MIN;  __m128 s = _mm_rsqrt_ss(_mm_set_ss(op));  float r; _mm_store_ss(&r, s);   return r * (1.5f - op * r * r * 0.5f); }
ILINE f64 isqrt_safe_tpl(f64 op)  { return 1.0 / sqrt(op + DBL_MIN); }
#elif CRY_PLATFORM_NEON
	#include "arm_neon.h"

template<int n>
float isqrt_helper(float f)
{
	float32x2_t v = vdup_n_f32(f);
	float32x2_t r = vrsqrte_f32(v);
	// n+1 newton iterations because initial approximation is crude
	for (int i = 0; i <= n; ++i)
		r = vrsqrts_f32(v * r, r) * r;
	return vget_lane_f32(r, 0);
}

ILINE f32 sqrt_tpl(f32 op)       { return op != 0.0f ? op* isqrt_helper<1>(op) : op; }
ILINE f64 sqrt_tpl(f64 op)       { return sqrt(op); }

ILINE f32 sqrt_fast_tpl(f32 op)  { return op != 0.0f ? op* isqrt_helper<0>(op) : op; }
ILINE f64 sqrt_fast_tpl(f64 op)  { return sqrt(op); }

ILINE f32 isqrt_tpl(f32 op)      { return isqrt_helper<1>(op); }
ILINE f64 isqrt_tpl(f64 op)      { return 1.0 / sqrt(op); }

ILINE f32 isqrt_fast_tpl(f32 op) { return isqrt_helper<0>(op); }
ILINE f64 isqrt_fast_tpl(f64 op) { return 1.0 / sqrt(op); }

ILINE f32 isqrt_safe_tpl(f32 op) { return isqrt_helper<1>(op + FLT_MIN); }
ILINE f64 isqrt_safe_tpl(f64 op) { return 1.0 / sqrt(op + DBL_MIN); }
#else
	#error unsupported CPU
#endif

ILINE int32               int_round(f32 f)                   { return f < 0.f ? int32(f - 0.5f) : int32(f + 0.5f); }
ILINE int32               pos_round(f32 f)                   { return int32(f + 0.5f); }
ILINE int64               int_round(f64 f)                   { return f < 0.0 ? int64(f - 0.5) : int64(f + 0.5); }
ILINE int64               pos_round(f64 f)                   { return int64(f + 0.5); }

ILINE int32               int_ceil(f32 f)                    { int32 i = int32(f); return (f > f32(i)) ? i + 1 : i; }
ILINE int64               int_ceil(f64 f)                    { int64 i = int64(f); return (f > f64(i)) ? i + 1 : i; }

ILINE float               ufrac8_to_float(float u)           { return u * (1.f / 255.f); }
ILINE float               ifrac8_to_float(float i)           { return i * (1.f / 127.f); }
ILINE uint8               float_to_ufrac8(float f)           { int i = pos_round(f * 255.f);  CRY_MATH_ASSERT(i >= 0 && i < 256);   return uint8(i); }
ILINE int8                float_to_ifrac8(float f)           { int i = int_round(f * 127.f);  CRY_MATH_ASSERT(abs(i) <= 127);   return int8(i); }

template<class F> ILINE F sqr(const F& op)                   { return op * op; }
template<class F> ILINE F sqr(const Vec2_tpl<F>& op)         { return op | op; }
template<class F> ILINE F sqr(const Vec3_tpl<F>& op)         { return op | op; }
template<class F> ILINE F sqr_signed(const F& op)            { return op * fabs_tpl(op); }
template<class F> ILINE F cube(const F& op)                  { return op * op * op; }

template<class F> ILINE F square(F fOp)                      { return(fOp * fOp); }
ILINE float               div_min(float n, float d, float m) { return n * d < m * d * d ? n / d : m; }

ILINE int32               sgnnz(f64 x)
{
	union
	{
		f32   f;
		int32 i;
	} u;
	u.f = (f32)x;
	return ((u.i >> 31) << 1) + 1;
}
ILINE int32 sgnnz(f32 x)
{
	union
	{
		f32   f;
		int32 i;
	} u;
	u.f = x;
	return ((u.i >> 31) << 1) + 1;
}
ILINE int32 sgnnz(int32 x) { return ((x >> 31) << 1) + 1; }
ILINE f32   fsgnnz(f32 x)
{
	union
	{
		f32   f;
		int32 i;
	} u;
	u.f = x;
	u.i = (u.i & 0x80000000) | 0x3f800000;
	return u.f;
}

ILINE int32 isneg(f32 x)
{
	union
	{
		f32    f;
		uint32 i;
	} u;
	u.f = x;
	return (int32)(u.i >> 31);
}
ILINE int32 isneg(f64 x)
{
	union
	{
		f32    f;
		uint32 i;
	} u;
	u.f = (f32)x;
	return (int32)(u.i >> 31);
}
ILINE int32 isneg(int32 x) { return (int32)((uint32)x >> 31);  }

ILINE int32 sgn(f64 x)
{
	union
	{
		f32   f;
		int32 i;
	} u;
	u.f = (f32)x;
	return (u.i >> 31) + ((u.i - 1) >> 31) + 1;
}
ILINE int32 sgn(f32 x)
{
	union
	{
		f32   f;
		int32 i;
	} u;
	u.f = x;
	return (u.i >> 31) + ((u.i - 1) >> 31) + 1;
}
ILINE int32 sgn(int32 x) { return (x >> 31) + ((x - 1) >> 31) + 1; }
ILINE f32   fsgnf(f32 x) { return f32(sgn(x)); }

ILINE int32 isnonneg(f32 x)
{
	union
	{
		f32    f;
		uint32 i;
	} u;
	u.f = x;
	return (int32)(u.i >> 31 ^ 1);
}
ILINE int32 isnonneg(f64 x)
{
	union
	{
		f32    f;
		uint32 i;
	} u;
	u.f = (f32)x;
	return (int32)(u.i >> 31 ^ 1);
}
ILINE int32 isnonneg(int32 x)          { return (int32)((uint32)x >> 31 ^ 1); }

ILINE int32 getexp(f32 x)              { return (int32)(*(uint32*)&x >> 23 & 0x0FF) - 127;  }
ILINE int32 getexp(f64 x)              { return (int32)(*((uint32*)&x + 1) >> 20 & 0x7FF) - 1023; }
ILINE f32&  setexp(f32& x, int32 iexp) { (*(uint32*)& x &= ~(0x0FF << 23)) |= (iexp + 127) << 23;   return x;  }
ILINE f64&  setexp(f64& x, int32 iexp) { (*((uint32*)&x + 1) &= ~(0x7FF << 20)) |= (iexp + 1023) << 20;  return x; }

ILINE int32 iszero(f32 x)
{
	union
	{
		f32   f;
		int32 i;
	} u;
	u.f = x;
	u.i &= 0x7FFFFFFF;
	return -(u.i >> 31 ^ (u.i - 1) >> 31);
}
ILINE int32 iszero(f64 x)
{
	union
	{
		f32   f;
		int32 i;
	} u;
	u.f = (f32)x;
	u.i &= 0x7FFFFFFF;
	return -((u.i >> 31) ^ (u.i - 1) >> 31);
}
ILINE int32 iszero(int32 x)   { return -(x >> 31 ^ (x - 1) >> 31); }
#if CRY_PLATFORM_64BIT && !defined(__clang__)
ILINE int64 iszero(__int64 x) { return -(x >> 63 ^ (x - 1) >> 63); }
#endif
#if CRY_PLATFORM_64BIT && defined(__clang__) && !CRY_PLATFORM_LINUX && !CRY_PLATFORM_ANDROID
ILINE int64 iszero(int64_t x) { return -(x >> 63 ^ (x - 1) >> 63); }
#endif
#if CRY_PLATFORM_64BIT && (CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE)
ILINE int64 iszero(long int x) { return -(x >> 63 ^ (x - 1) >> 63); }
#endif

ILINE float                   if_neg_else(float test, float val_neg, float val_nonneg) { return (float)__fsel(test, val_nonneg, val_neg); }
ILINE float                   if_pos_else(float test, float val_pos, float val_nonpos) { return (float)__fsel(-test, val_nonpos, val_pos); }
template<class F> ILINE int32 inrange(F x, F end1, F end2)                             { return isneg(fabs_tpl(end1 + end2 - x * (F)2) - fabs_tpl(end1 - end2)); }
template<class F> ILINE F     cond_select(int32 bFirst, F op1, F op2)                  { F arg[2] = { op1, op2 };   return arg[bFirst ^ 1]; }

template<class F> ILINE int32 idxmax3(const F* pdata)
{
	int32 imax = isneg(pdata[0] - pdata[1]);
	imax |= isneg(pdata[imax] - pdata[2]) << 1;
	return imax & (2 | (imax >> 1 ^ 1));
}
template<class F> ILINE int32 idxmax3(const Vec3_tpl<F>& vec)
{
	int32 imax = isneg(vec.x - vec.y);
	imax |= isneg(vec[imax] - vec.z) << 1;
	return imax & (2 | (imax >> 1 ^ 1));
}
template<class F> ILINE int32 idxmin3(const F* pdata)
{
	int32 imin = isneg(pdata[1] - pdata[0]);
	imin |= isneg(pdata[2] - pdata[imin]) << 1;
	return imin & (2 | (imin >> 1 ^ 1));
}
template<class F> ILINE int32 idxmin3(const Vec3_tpl<F>& vec)
{
	int32 imin = isneg(vec.y - vec.x);
	imin |= isneg(vec.z - vec[imin]) << 1;
	return imin & (2 | (imin >> 1 ^ 1));
}
//! Approximation of exp(-x).
ILINE float approxExp(float x) { return __fres(1.f + x); }

//! Approximation of 1.f - exp(-x).
ILINE float approxOneExp(float x) { return x * __fres(1.f + x); }

//! \return i if x==1<<i (i=0..63)
ILINE int ilog2(uint64 x)
{
#if CRY_PLATFORM_X64 && CRY_COMPILER_MSVC
	unsigned long i;
	_BitScanReverse64(&i, x);
	return i;
#elif CRY_COMPILER_CLANG || CRY_COMPILER_GCC
	return __builtin_clzll(x);
#else
	union
	{
		float f;
		uint  i;
	} u;
	u.f = (float)x;
	return (u.i >> 23) - 127;
#endif
}

template<class T>
int solve_quadratic(T a, T b, T c, T x[2])
{
	if (a == T(0))
	{
		if (b != T(0))
		{
			x[0] = -c / b;
			return 1;
		}
		if (c != T(0))
			return 0;
		return -1;
	}
	else
	{
		b /= T(2);
		T d = b * b - a * c;

		if (d > T(0))
		{
			T s = sqrt_tpl(d);
			T q = -(b + (b > T(0) ? T(1) : T(-1)) * s);

			x[0] = q / a;
			x[1] = c / q;
			return 2;
		}
		if (d == T(0))
		{
			x[0] = -b / a;
			return 1;
		}
		return 0;
	}
}

static int32 inc_mod3[] = { 1, 2, 0 }, dec_mod3[] = { 2, 0, 1 };
#ifdef PHYSICS_EXPORTS
	#define incm3(i) inc_mod3[i]
	#define decm3(i) dec_mod3[i]
#else
ILINE int32 incm3(int32 i) { return i + 1 & (i - 2) >> 31; }
ILINE int32 decm3(int32 i) { return i - 1 + ((i - 1) >> 31 & 3); }
#endif

//////////////////////////////////////////////////////////////////////////
enum type_zero { ZERO };
enum type_min { VMIN };
enum type_max { VMAX };
enum type_identity { IDENTITY };

#include "Cry_Vector2.h"
#include "Cry_Vector3.h"
#include "Cry_Vector4.h"
#include "Cry_MatrixDiag.h"
#include "Cry_Matrix33.h"
#include "Cry_Matrix34.h"
#include "Cry_Matrix44.h"
#include "Cry_Quat.h"
#include "Cry_HWVector3.h"
#include "Cry_HWMatrix.h"
#include "Cry_XOptimise.h"

//////////////////////////////////////////////////////////////////////////

//! This function relaxes a value (val) towards a desired value (to) whilst maintaining continuity
//! of val and its rate of change (valRate). timeDelta is the time between this call and the previous one.
//! The caller would normally keep val and valRate as working variables, and smoothTime is normally
//! a fixed parameter. The to/timeDelta values can change.
//!
//! Implementation details:
//!
//! This is a critically damped spring system. A linear spring is attached between "val" and "to" that
//! drags "val" to "to". At the same time a damper between the two removes oscillations; it's tweaked
//! so it doesn't dampen more than necessary. In combination this gives smooth ease-in and ease-out behavior.
//!
//! smoothTime can be interpreted in a couple of ways:
//! - it's the "expected time to reach the target when at maximum velocity" (the target will, however, not be reached
//!   in that time because the speed will decrease the closer it gets to the target)
//! - it's the 'lag time', how many seconds "val" lags behind "to". If your
//!   target has a certain speed, the lag distance is simply the smoothTime times that speed.
//! - it's 2/omega, where omega is the spring's natural frequency (or less formally a measure of the spring stiffness)
//!
//! The implementation is stable for varying timeDelta, but for performance reasons it uses a polynomial approximation
//! to the exponential function. The approximation works well (within 0.1% of accuracy) when smoothTime > 2*deltaTime,
//! which is usually the case. (but it might be troublesome when you want a stiff spring or have frame hikes!)
//! The implementation handles cases where smoothTime==0 separately and reliably. In that case the target will be
//! reached immediately, and valRate is updated appropriately.
//!
//! Based on "Critically Damped Ease-In/Ease-Out Smoothing", Thomas Lowe, Game Programming Gems IV

// disable divide by zero warning for the divides by smoothTime
// & timeDelta (warning appears even though these cases are guarded)
#pragma warning(push)
#pragma warning(disable:4723)

//! \param[in,out] &val     value to be smoothed
//! \param[in,out]&valRate  rate of change of the value
//! \param[in] timeDelta    time interval
//! \param[in] to           the target value
//! \param[in] smoothTime   timescale for smoothing
template<typename T> ILINE void SmoothCD(
  T& val,
  T& valRate,
  const float timeDelta,
  const T& to,
  const float smoothTime)
{
	if (smoothTime > 0.0f)
	{
		const float omega = 2.0f / smoothTime;
		const float x = omega * timeDelta;
		const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
		const T change = (val - to);
		const T temp = (T)((valRate + change * omega) * timeDelta);
		valRate = (T)((valRate - temp * omega) * exp);
		val = (T)(to + (change + temp) * exp);
	}
	else if (timeDelta > 0.0f)
	{
		valRate = (T)((to - val) / timeDelta);
		val = to;
	}
	else
	{
		val = to;
		valRate -= valRate; // zero it...
	}
}

//! \param[in,out]	val,       //!< value to be smoothed
//! \param[in,out]	valRate,   //!< rate of change of the value
//! \param[in]	timeDelta,     //!< time interval
//! \param[in]	to,            //!< the target value
//! \param[in]	smoothTime,    //!< timescale for smoothing
//! \param[in]	maxValRate)    //!< maximum allowed rate of change
template<typename T> ILINE void SmoothCDWithMaxRate(
  T& val,
  T& valRate,
  const float timeDelta,
  const T& to,
  const float smoothTime,
  const T& maxValRate)
{
	if (smoothTime > 0.0f)
	{
		const float omega = 2.0f / smoothTime;
		const float x = omega * timeDelta;
		const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
		const T unclampedChange = val - to;
		const T maxChange = maxValRate * smoothTime;
		const T clampedChange = clamp_tpl<T>(unclampedChange, -maxChange, maxChange);
		const T clampedTo = val - clampedChange;
		const T temp = (T)((valRate + clampedChange * omega) * timeDelta);
		valRate = (T)((valRate - temp * omega) * exp);
		val = (T)(clampedTo + (clampedChange + temp) * exp);
	}
	else if (timeDelta > 0.0f)
	{
		const T unclampedRate = (T)((to - val) / timeDelta);
		valRate = clamp_tpl<T>(unclampedRate, -maxValRate, maxValRate);
		val += valRate * timeDelta;
	}
	else
	{
		val = to;
		valRate -= valRate; // zero it...
	}
}
#pragma warning(pop)

//! Smoothes linear blending into cubic (b-spline) with 0-derivatives near 0 and 1.
inline f32 SmoothBlendValue(const f32 fBlend)
{
	const f32 fBlendAdj = fBlend - 0.5f;
	return (f32)__fsel(-fBlend, 0.f, __fsel(fBlend - 1.f, 1.f, 0.5f - 2.f * (fBlendAdj * fBlendAdj * fBlendAdj) + 1.5f * fBlendAdj));
}

//! Function for safe comparsion of floating point  values.
ILINE bool fcmp(f32 fA, f32 fB, f32 fEpsilon = FLT_EPSILON)
{
	return fabs(fA - fB) <= fEpsilon;
}
#endif //math
