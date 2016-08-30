// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// SSE shift-immediate operators for all compilers
// Integer type alias to allow operator overloads.
// Example: i32v4 a; i32v4 b = a << Scalar(4);
struct Scalar
{
	explicit Scalar(int _v) : v(_v) {}
	operator int() const { return v; }
	const int v;
};

#if CRY_PLATFORM_SSE2

// Support for SSE 32x4 types

	#if CRY_COMPILER_CLANG || CRY_COMPILER_GCC
// __m128i is defined as __v2di instead of __v4si, so we define the correct vector types here to avoid compile errors.
typedef __v4sf  f32v4;
typedef __v4si  i32v4;
	#else
typedef __m128  f32v4;
typedef __m128i i32v4;
	#endif

//! Define results of comparison operators
	#if CRY_COMPILER_MSVC
typedef __m128  f32mask4;
typedef __m128i i32mask4;
	#else
//! Define results of built-in comparison operators.
//! These must be cast to __m128 or __m128i when calling intrinsic functions.
typedef decltype (f32v4() == f32v4()) f32mask4;
typedef decltype (i32v4() == i32v4()) i32mask4;
	#endif

//! Override convert<> for vector intrinsic types
template<> ILINE f32v4 convert<f32v4>()         { return _mm_setzero_ps(); }
template<> ILINE f32v4 convert<f32v4>(float v)  { return _mm_set1_ps(v); }
template<> ILINE f32v4 convert<f32v4>(double v) { return _mm_set1_ps(float(v)); }
template<> ILINE f32v4 convert<f32v4>(int v)    { return _mm_set1_ps(float(v)); }
template<> ILINE f32v4 convert<f32v4>(i32v4 v)  { return _mm_cvtepi32_ps(v); }

template<> ILINE float convert<float>(f32v4 v)  { return _mm_cvtss_f32(v); }

template<> ILINE i32v4 convert<i32v4>()         { return _mm_setzero_si128(); }
template<> ILINE i32v4 convert<i32v4>(int v)    { return _mm_set1_epi32(v); }
template<> ILINE i32v4 convert<i32v4>(uint v)   { return _mm_set1_epi32(v); }
template<> ILINE i32v4 convert<i32v4>(f32v4 v)  { return _mm_cvttps_epi32(v); }

template<> ILINE int   convert<int>(i32v4 v)    { return _mm_cvtsi128_si32(v); }

//! Multiple scalar/vector conversion
template<typename V, typename S> V       convert(S s0, S s1, S s2, S s3);

template<> ILINE f32v4                   convert<f32v4>(float s0, float s1, float s2, float s3) { return _mm_setr_ps(s0, s1, s2, s3); }
template<> ILINE f32v4                   convert<f32v4>(int s0, int s1, int s2, int s3)         { return _mm_setr_ps(float(s0), float(s1), float(s2), float(s3)); }
template<> ILINE i32v4                   convert<i32v4>(int s0, int s1, int s2, int s3)         { return _mm_setr_epi32(s0, s1, s2, s3); }

template<typename D, typename S> ILINE D vcast(S val)                                           { return D(val); }
template<> ILINE f32v4                   vcast<f32v4>(i32v4 v)                                  { return _mm_castsi128_ps(v); }
template<> ILINE i32v4                   vcast<i32v4>(f32v4 v)                                  { return _mm_castps_si128(v); }

// Helpers vor defining regular and compound operators together
	#define COMMPOUND_OPERATOR(A, op, B) \
	  ILINE A & operator op ## = (A & a, B b) { return a = a op b; }

	#define COMMPOUND_OPERATORS(A, op, B) \
	  A operator op(A a, B b);            \
	  COMMPOUND_OPERATOR(A, op, B)        \
	  ILINE A operator op(A a, B b)       \


///////////////////////////////////////////////////////////////////////////
	#if CRY_COMPILER_MSVC

// Define operators for intrinsics, for compilers which don't already have them

///////////////////////////////////////////////////////////////////////////
// Integer

		#define INT32V_COMPARE_OPS(op, opneg, func)                                                                         \
		  ILINE i32mask4 operator op(i32v4 a, i32v4 b) { return func(a, b); }                                               \
		  ILINE i32mask4 operator opneg(i32v4 a, i32v4 b) { return _mm_xor_si128(func(a, b), _mm_set1_epi32(0xFFFFFFFF)); } \

INT32V_COMPARE_OPS(==, !=, _mm_cmpeq_epi32)
INT32V_COMPARE_OPS(>, <=, _mm_cmpgt_epi32)
INT32V_COMPARE_OPS(<, >=, _mm_cmplt_epi32)

		#define INT32V_OPERATORS(op) \
		  COMMPOUND_OPERATORS(i32v4, op, i32v4)

INT32V_OPERATORS(+)
{
	return _mm_add_epi32(a, b);
}
INT32V_OPERATORS(-)
{
	return _mm_sub_epi32(a, b);
}
INT32V_OPERATORS(&)
{
	return _mm_and_si128(a, b);
}
INT32V_OPERATORS(|)
{
	return _mm_or_si128(a, b);
}
INT32V_OPERATORS(^)
{
	return _mm_xor_si128(a, b);
}
INT32V_OPERATORS(<<)
{
	return _mm_sll_epi32(a, b);
}
INT32V_OPERATORS(>>)
{
	return _mm_srl_epi32(a, b);
}

INT32V_OPERATORS(*)
{
	const i32v4 m = _mm_set_epi32(~0, 0, ~0, 0);
	const i32v4 v0 = _mm_mul_epu32(a, b);
	const i32v4 v1 = _mm_shuffle_epi32(_mm_mul_epu32(_mm_shuffle_epi32(a, 0xf5), _mm_shuffle_epi32(b, 0xf5)), 0xa0);
	return _mm_or_si128(_mm_and_si128(m, v1), _mm_andnot_si128(m, v0));
}

// Slow implementations
INT32V_OPERATORS(/)
{
	i32v4 v = a;
	v.m128i_i32[0] /= b.m128i_i32[0];
	v.m128i_i32[1] /= b.m128i_i32[1];
	v.m128i_i32[2] /= b.m128i_i32[2];
	v.m128i_i32[3] /= b.m128i_i32[3];
	return v;
}

INT32V_OPERATORS(%)
{
	i32v4 v = a;
	v.m128i_i32[0] %= b.m128i_i32[0];
	v.m128i_i32[1] %= b.m128i_i32[1];
	v.m128i_i32[2] %= b.m128i_i32[2];
	v.m128i_i32[3] %= b.m128i_i32[3];
	return v;
}

ILINE i32v4 operator-(i32v4 a)
{
	return _mm_setzero_si128() - a;
}

ILINE i32v4 operator~(i32v4 a)
{
	return a ^ _mm_set1_epi32(0xFFFFFFFF);
}

///////////////////////////////////////////////////////////////////////////
// Float

		#define FLOAT32V_COMPARE_OP(op, func)                                 \
		  ILINE f32mask4 operator op(f32v4 a, f32v4 b) { return func(a, b); } \

FLOAT32V_COMPARE_OP(==, _mm_cmpeq_ps)
FLOAT32V_COMPARE_OP(!=, _mm_cmpneq_ps)
FLOAT32V_COMPARE_OP(<, _mm_cmplt_ps)
FLOAT32V_COMPARE_OP(<=, _mm_cmple_ps)
FLOAT32V_COMPARE_OP(>, _mm_cmpgt_ps)
FLOAT32V_COMPARE_OP(>=, _mm_cmpge_ps)

		#define FLOAT32V_OPERATORS(op) \
		  COMMPOUND_OPERATORS(f32v4, op, f32v4)

FLOAT32V_OPERATORS(+)
{
	return _mm_add_ps(a, b);
}

FLOAT32V_OPERATORS(-)
{
	return _mm_sub_ps(a, b);
}

FLOAT32V_OPERATORS(*)
{
	return _mm_mul_ps(a, b);
}

FLOAT32V_OPERATORS(/)
{
	return _mm_div_ps(a, b);
}

ILINE f32v4 operator-(f32v4 a)
{
	return _mm_setzero_ps() - a;
}

	#endif // CRY_COMPILER_MSVC

///////////////////////////////////////////////////////////////////////////
// Additional operators and functions for SSE intrinsics

//! Convert comparison results to boolean value
ILINE bool All(f32mask4 v)
{
	return _mm_movemask_ps(vcast<f32v4>(v)) == 0xF;
}
ILINE bool Any(f32mask4 v)
{
	return _mm_movemask_ps(vcast<f32v4>(v)) != 0;
}

template<> ILINE i32v4 if_else(i32mask4 mask, i32v4 a, i32v4 b)
{
	#ifdef CRY_PFX2_USE_SEE4
	return vcast<i32v4>(_mm_blendv_ps(vcast<f32v4>(b), vcast<f32v4>(a), vcast<f32v4>(mask)));
	#else
	return (i32v4)_mm_or_si128(_mm_and_si128((__m128i)mask, a), _mm_andnot_si128((__m128i)mask, b));
	#endif
}

template<> ILINE i32v4 if_else_zero(i32mask4 mask, i32v4 a)
{
	return (i32v4)_mm_and_si128(vcast<i32v4>(mask), a);
}

template<> ILINE f32v4 if_else(f32mask4 mask, f32v4 a, f32v4 b)
{
	#ifdef CRY_PFX2_USE_SEE4
	return (f32v4)_mm_blendv_ps(b, a, (__m128)mask);
	#else
	return (f32v4)_mm_or_ps(_mm_and_ps((__m128)mask, a), _mm_andnot_ps((__m128)mask, b));
	#endif
}

template<> ILINE f32v4 if_else_zero(f32mask4 mask, f32v4 a)
{
	return _mm_and_ps((__m128)mask, a);
}

	#if CRY_COMPILER_MSVC
// On clang and gcc, f32mask4 == i32mask4. For other compilers, define overloads.
ILINE bool             All(i32mask4 v)                          { return _mm_movemask_ps(vcast<f32v4>(v)) == 0xF; }
ILINE bool             Any(i32mask4 v)                          { return _mm_movemask_ps(vcast<f32v4>(v)) != 0; }
template<> ILINE f32v4 if_else_zero(i32mask4 mask, f32v4 a)     { return if_else_zero(vcast<f32mask4>(mask), a); }
template<> ILINE f32v4 if_else(i32mask4 mask, f32v4 a, f32v4 b) { return if_else(vcast<f32mask4>(mask), a, b); }
template<> ILINE i32v4 if_else_zero(f32mask4 mask, i32v4 a)     { return if_else_zero(vcast<i32mask4>(mask), a); }
template<> ILINE i32v4 if_else(f32mask4 mask, i32v4 a, i32v4 b) { return if_else(vcast<i32mask4>(mask), a, b); }
	#endif

// SSE shift-immediate operators for all compilers
COMMPOUND_OPERATORS(i32v4, <<, Scalar)
{
	return _mm_slli_epi32(a, b);
}
COMMPOUND_OPERATORS(i32v4, >>, Scalar)
{
	return _mm_srli_epi32(a, b);
}

///////////////////////////////////////////////////////////////////////////
// Specialize global functions
template<> ILINE f32v4 min(f32v4 v0, f32v4 v1)
{
	return _mm_min_ps(v0, v1);
}
template<> ILINE f32v4 max(f32v4 v0, f32v4 v1)
{
	return _mm_max_ps(v0, v1);
}

namespace crymath
{

ILINE f32v4 abs(f32v4 v)  { return _mm_andnot_ps(convert<f32v4>(-0.0f), v); }

ILINE f32v4 sign(f32v4 v) { return _mm_or_ps(_mm_and_ps(v, convert<f32v4>(-0.0f)), convert<f32v4>(1.0f)); }

// Note: trunc, floor, and ceil currently only work for float values in the int32 range
ILINE f32v4 trunc(f32v4 v)
{
	return convert<f32v4>(convert<i32v4>(v));
}

ILINE f32v4 floor(f32v4 v)
{
	f32v4 trunct = trunc(v);
	return trunct + if_else_zero(trunct > v, convert<f32v4>(-1.0f));
}

ILINE f32v4 ceil(f32v4 v)
{
	f32v4 trunct = trunc(v);
	return trunct + if_else_zero(trunct < v, convert<f32v4>(1.0f));
}

ILINE f32v4 rcp_fast(f32v4 op)   { return _mm_rcp_ps(op); }

ILINE f32v4 rcp(f32v4 op)        { f32v4 r = _mm_rcp_ps(op); return r + r - op * r * r; }

ILINE f32v4 rsqrt_fast(f32v4 op) { return _mm_rsqrt_ps(op); }

ILINE f32v4 rsqrt(f32v4 op)      { f32v4 r = _mm_rsqrt_ps(op); return r * (convert<f32v4>(1.5f) - op * r * r * convert<f32v4>(0.5f)); }

ILINE f32v4 sqrt_fast(f32v4 op)  { return _mm_rsqrt_ps(_mm_max_ps(op, convert<f32v4>(FLT_MIN))) * op; }

ILINE f32v4 sqrt(f32v4 op)       { return _mm_sqrt_ps(op); }

}

#endif // CRY_PLATFORM_SSE2

ILINE bool All(bool b) { return b; }
ILINE bool Any(bool b) { return b; }
