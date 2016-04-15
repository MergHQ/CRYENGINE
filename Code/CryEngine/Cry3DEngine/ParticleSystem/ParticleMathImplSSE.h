// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     28/07/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

namespace pfx2
{

namespace detail
{

#ifdef CRY_PFX2_USE_SEE4

ILINE __m128 Blendv(__m128 a, __m128 b, __m128 c)
{
	return _mm_blendv_ps(a, b, c);
}

template<int i>
ILINE int ExtractI32(__m128i a)
{
	return _mm_extract_epi32(a, i);
}

template<int i>
ILINE float ExtractF32(__m128 a)
{
	float r;
	_MM_EXTRACT_FLOAT(r, a, i);
	return r;
}

#else

ILINE __m128 Blendv(__m128 a, __m128 b, __m128 c)
{
	return _mm_or_ps(_mm_and_ps(c, b), _mm_andnot_ps(c, a));
}

template<int i>
ILINE int ExtractI32(__m128i a)
{
	return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, _MM_SHUFFLE(i, i, i, i)));
}

template<int i>
ILINE float ExtractF32(__m128 a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a, a, _MM_SHUFFLE(i, i, i, i)));
}

#endif

ILINE bool AllEqual(__m128 a, __m128 b)
{
	return _mm_movemask_ps(_mm_cmpeq_ps(a, b)) == 0xf;
}

ILINE bool AllEqual(__m128i a, __m128i b)
{
	return _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(a, b))) == 0xf;
}

ILINE __m128 LoadIndexed4(const float* __restrict pStream, const __m128i index, float defaultVal)
{
	const __m128i mask = _mm_cmpeq_epi32(index, _mm_set1_epi32(gInvalidId));
	const __m128i mIndex = _mm_andnot_si128(mask, index);
	const __m128i mFirst = _mm_shuffle_epi32(mIndex, 0);

	if (AllEqual(index, _mm_add_epi32(mFirst, _mm_set_epi32(3, 2, 1, 0))))
		return _mm_loadu_ps(pStream + _mm_cvtsi128_si32(mFirst));

	__m128 output = _mm_load1_ps(pStream + _mm_cvtsi128_si32(mFirst));
	if (!AllEqual(mFirst, mIndex))
	{
		const float f1 = *(pStream + ExtractI32<1>(mIndex));
		const float f2 = *(pStream + ExtractI32<2>(mIndex));
		const float f3 = *(pStream + ExtractI32<3>(mIndex));
		const __m128 m = _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, ~0));
		const __m128 v0 = _mm_set_ps(f3, f2, f1, 0.0f);
		output = _mm_or_ps(v0, _mm_and_ps(output, m));
	}

	return CMov(_mm_castsi128_ps(mask), _mm_set1_ps(defaultVal), output);
}

}

//////////////////////////////////////////////////////////////////////////
// Streams

ILINE floatv IFStream::Load(TParticleGroupId pgId) const
{
	return _mm_load_ps(m_pStream + pgId);
}

ILINE floatv IFStream::Load(TParticleIdv pIdv, float defaultVal) const
{
	return detail::LoadIndexed4(m_pStream, pIdv, defaultVal);
}

ILINE floatv IFStream::SafeLoad(TParticleGroupId pgId) const
{
	return _mm_load_ps(m_pStream + (pgId & m_safeMask));
}

ILINE floatv IOFStream::Load(TParticleGroupId pgId) const
{
	return _mm_load_ps(m_pStream + pgId);
}

ILINE void IOFStream::Store(TParticleGroupId pgId, floatv value)
{
	_mm_store_ps(m_pStream + pgId, value);
}

ILINE Vec3v IVec3Stream::Load(TParticleGroupId pgId) const
{
	return Vec3v(
	  _mm_load_ps(m_pXStream + pgId),
	  _mm_load_ps(m_pYStream + pgId),
	  _mm_load_ps(m_pZStream + pgId));
}

ILINE Vec3v IVec3Stream::Load(TParticleIdv pIdv, Vec3 defaultVal) const
{
	return Vec3v(
	  detail::LoadIndexed4(m_pXStream, pIdv, defaultVal.x),
	  detail::LoadIndexed4(m_pYStream, pIdv, defaultVal.y),
	  detail::LoadIndexed4(m_pZStream, pIdv, defaultVal.z));
}

ILINE Vec3v IVec3Stream::SafeLoad(TParticleGroupId pgId) const
{
	return Vec3v(
	  _mm_load_ps(m_pXStream + (pgId & m_safeMask)),
	  _mm_load_ps(m_pYStream + (pgId & m_safeMask)),
	  _mm_load_ps(m_pZStream + (pgId & m_safeMask)));
}

ILINE Vec3v IOVec3Stream::Load(TParticleGroupId pgId) const
{
	return Vec3v(
	  _mm_load_ps(m_pXStream + pgId),
	  _mm_load_ps(m_pYStream + pgId),
	  _mm_load_ps(m_pZStream + pgId));
}

ILINE void IOVec3Stream::Store(TParticleGroupId pgId, const Vec3v& value) const
{
	_mm_store_ps(m_pXStream + pgId, value.x);
	_mm_store_ps(m_pYStream + pgId, value.y);
	_mm_store_ps(m_pZStream + pgId, value.z);
}

ILINE UColv IOColorStream::Load(TParticleGroupId pgId) const
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(m_pStream + pgId));
}

ILINE void IOColorStream::Store(TParticleGroupId pgId, UColv value)
{
	_mm_store_si128(reinterpret_cast<__m128i*>(m_pStream + pgId), value);
}

template<typename T, typename Tv>
Tv TIStream<T, Tv >::Load(TParticleGroupId pgId) const
{
	static_assert(sizeof(T) == 4, "No Vector load for this type");
	return _mm_load_si128(reinterpret_cast<const __m128i*>(m_pStream + pgId));
}

template<>
ILINE UColv TIStream<UCol, UColv >::Load(TParticleIdv pIdv, UCol defaultVal) const
{
	static_assert(sizeof(UColv) == sizeof(__m128), "cannot use __m128 to store UColv");
	const float defaultValF = _mm_cvtss_f32(_mm_castsi128_ps(_mm_set1_epi32(defaultVal.dcolor)));
	const float* streamF = reinterpret_cast<const float*>(m_pStream);
	__m128 dataF = detail::LoadIndexed4(streamF, pIdv, defaultValF);
	return _mm_castps_si128(dataF);
}

//////////////////////////////////////////////////////////////////////////
// Functions

ILINE floatv ToFloatv(float v)
{
	return _mm_set1_ps(v);
}

ILINE uint32v ToUint32v(uint32 v)
{
	return _mm_set1_epi32(v);
}

ILINE floatv ToFloatv(int32v v)
{
	return _mm_cvtepi32_ps(v);
}

ILINE uint32v ToUint32v(floatv v)
{
	return _mm_cvtps_epi32(v);
}

ILINE Vec3v ToVec3v(Vec3 v)
{
	return Vec3v(
	  _mm_set1_ps(v.x),
	  _mm_set1_ps(v.y),
	  _mm_set1_ps(v.z));
}

ILINE Vec4v ToVec4v(Vec4 v)
{
	return Vec4v(
	  _mm_set1_ps(v.x),
	  _mm_set1_ps(v.y),
	  _mm_set1_ps(v.z),
	  _mm_set1_ps(v.w));
}

ILINE Planev ToPlanev(Plane v)
{
	return Planev(
	  ToVec3v(v.n),
	  ToFloatv(v.d));
}

ILINE float __fsel(const float _a, const float _b, const float _c)
{
	const __m128 zero = _mm_set_ss(0.0f);
	const __m128 av = _mm_set_ss(_a);
	const __m128 bv = _mm_set_ss(_b);
	const __m128 cv = _mm_set_ss(_c);
	const __m128 rv = detail::Blendv(bv, cv, _mm_cmplt_ps(av, zero));
	return _mm_cvtss_f32(rv);
}

ILINE float __fres(const float _a)
{
	const __m128 av = _mm_set_ss(_a);
	const float res = _mm_cvtss_f32(_mm_rcp_ss(av));
	return res;
}

#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
ILINE bool operator==(floatv a, floatv b)
{
	return _mm_movemask_ps(_mm_cmpeq_ps(a, b)) == 0xf;
}

ILINE bool operator==(const TParticleIdv& a, const TParticleIdv& b)
{
	return _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(a, b))) == 0xf;
}

ILINE bool operator==(const TParticleIdv& a, const TParticleId& b)
{
	return (a == TParticleIdv(_mm_set1_epi32(b)));
}

ILINE floatv operator+(const floatv a, const floatv b)
{
	return _mm_add_ps(a, b);
}

ILINE floatv operator-(const floatv a, const floatv b)
{
	return _mm_sub_ps(a, b);
}

ILINE floatv operator-(const floatv a)
{
	return _mm_sub_ps(_mm_set1_ps(0.0f), a);
}

ILINE floatv operator*(const floatv a, const floatv b)
{
	return _mm_mul_ps(a, b);
}

ILINE floatv operator&(const floatv a, const floatv b)
{
	return _mm_and_ps(a, b);
}

ILINE uint32v operator*(uint32v a, uint32v b)
{
	const uint32v m = _mm_set_epi32(~0, 0, ~0, 0);
	const uint32v v0 = _mm_mul_epu32(a, b);
	const uint32v v1 = _mm_shuffle_epi32(_mm_mul_epu32(_mm_shuffle_epi32(a, 0xf5), _mm_shuffle_epi32(b, 0xf5)), 0xa0);
	return _mm_or_si128(_mm_and_si128(m, v1), _mm_andnot_si128(m, v0));
}

ILINE uint32v operator/(uint32v a, uint32 b)
{
	uint32v v = a;
	v.m128i_i32[0] /= b;
	v.m128i_i32[1] /= b;
	v.m128i_i32[2] /= b;
	v.m128i_i32[3] /= b;
	return v;
}

ILINE uint32v operator%(uint32v a, uint32 b)
{
	uint32v v = a;
	v.m128i_i32[0] %= b;
	v.m128i_i32[1] %= b;
	v.m128i_i32[2] %= b;
	v.m128i_i32[3] %= b;
	return v;
}

#endif

ILINE floatv Add(const floatv v0, const floatv v1)
{
	return _mm_add_ps(v0, v1);
}

ILINE floatv Sub(const floatv v0, const floatv v1)
{
	return _mm_sub_ps(v0, v1);
}

ILINE floatv Mul(const floatv v0, const floatv v1)
{
	return _mm_mul_ps(v0, v1);
}

ILINE floatv MAdd(const floatv a, const floatv b, const floatv c)
{
#ifdef CRY_PFX2_USE_FMA
	return _mm_fmadd_ps(a, b, c);
#else
	return _mm_add_ps(_mm_mul_ps(a, b), c);
#endif
}

ILINE floatv Sqrt(const floatv v0)
{
	return _mm_sqrt_ps(v0);
}

ILINE floatv CMov(floatv condMask, floatv trueVal, floatv falseVal)
{
	return detail::Blendv(falseVal, trueVal, condMask);
}

ILINE Vec3v Min(const Vec3v& a, const Vec3v& b)
{
	return Vec3v(
	  Min(a.x, b.x),
	  Min(a.y, b.y),
	  Min(a.z, b.z));
}

ILINE Vec3v Max(const Vec3v& a, const Vec3v& b)
{
	return Vec3v(
	  Max(a.x, b.x),
	  Max(a.y, b.y),
	  Max(a.z, b.z));
}

ILINE floatv Min(floatv a, floatv b)
{
	return _mm_min_ps(a, b);
}

ILINE floatv Max(floatv a, floatv b)
{
	return _mm_max_ps(a, b);
}

ILINE float HMin(floatv v0)
{
	const __m128 v1 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 3, 1, 1));
	const __m128 v2 = _mm_min_ps(v0, v1);
	const __m128 v3 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 res = _mm_min_ps(v3, v2);
	return _mm_cvtss_f32(res);
}

ILINE float HMax(floatv v0)
{
	const __m128 v1 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 3, 1, 1));
	const __m128 v2 = _mm_max_ps(v0, v1);
	const __m128 v3 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 res = _mm_max_ps(v3, v2);
	return _mm_cvtss_f32(res);
}

// a < 0 ? c : b
ILINE floatv FSel(floatv a, floatv b, floatv c)
{
	const __m128 mask = _mm_cmplt_ps(a, _mm_set1_ps(0.0f));
	return _mm_or_ps(_mm_and_ps(mask, c), _mm_andnot_ps(mask, b));
}

ILINE floatv Rcp(floatv a)
{
	return _mm_rcp_ps(a);
}

ILINE float SafeRcp(const float f)
{
	const __m128 a = _mm_max_ss(_mm_set_ss(f), _mm_set_ss(FLT_EPSILON));
	return _mm_cvtss_f32(_mm_rcp_ss(a));
}

ILINE floatv Lerp(floatv low, floatv high, floatv v)
{
	const __m128 one = _mm_set1_ps(1.0f);
	const __m128 oneMinusV = _mm_sub_ps(one, v);
	const __m128 res = MAdd(high, v, Mul(low, oneMinusV));
	return res;
}

ILINE floatv Clamp(floatv x, floatv minv, floatv maxv)
{
	return _mm_max_ps(_mm_min_ps(x, maxv), minv);
}

ILINE floatv Saturate(floatv x)
{
	return _mm_max_ps(_mm_min_ps(x, _mm_set1_ps(1.0f)), _mm_set1_ps(0.0f));
}

// Vec3v operators

ILINE Vec3v Add(const Vec3v& a, const Vec3v& b)
{
	return Vec3v(
	  _mm_add_ps(a.x, b.x),
	  _mm_add_ps(a.y, b.y),
	  _mm_add_ps(a.z, b.z));
}

ILINE Vec3v Add(const Vec3v& a, const floatv b)
{
	return Vec3v(
	  _mm_add_ps(a.x, b),
	  _mm_add_ps(a.y, b),
	  _mm_add_ps(a.z, b));
}

ILINE Vec3v Sub(const Vec3v& a, const Vec3v& b)
{
	return Vec3v(
	  _mm_sub_ps(a.x, b.x),
	  _mm_sub_ps(a.y, b.y),
	  _mm_sub_ps(a.z, b.z));
}

ILINE Vec3v Sub(const Vec3v& a, const floatv b)
{
	return Vec3v(
	  _mm_sub_ps(a.x, b),
	  _mm_sub_ps(a.y, b),
	  _mm_sub_ps(a.z, b));
}

ILINE Vec3v Mul(const Vec3v& a, const floatv b)
{
	return Vec3v(
	  _mm_mul_ps(a.x, b),
	  _mm_mul_ps(a.y, b),
	  _mm_mul_ps(a.z, b));
}

ILINE Vec4v Add(const Vec4v& a, const Vec4v& b)
{
	return Vec4v(
	  _mm_add_ps(a.x, b.x),
	  _mm_add_ps(a.y, b.y),
	  _mm_add_ps(a.z, b.z),
	  _mm_add_ps(a.w, b.w));
}

ILINE Vec3v MAdd(const Vec3v& a, const Vec3v& b, const Vec3v& c)
{
	return Vec3v(
	  _mm_add_ps(_mm_mul_ps(a.x, b.x), c.x),
	  _mm_add_ps(_mm_mul_ps(a.y, b.y), c.y),
	  _mm_add_ps(_mm_mul_ps(a.z, b.z), c.z));
}

ILINE Vec3v MAdd(const Vec3v& a, const floatv b, const Vec3v& c)
{
	return Vec3v(
	  _mm_add_ps(_mm_mul_ps(a.x, b), c.x),
	  _mm_add_ps(_mm_mul_ps(a.y, b), c.y),
	  _mm_add_ps(_mm_mul_ps(a.z, b), c.z));
}

ILINE const floatv DeltaTime(const floatv normAge, const floatv frameTime)
{
	return FSel(normAge, frameTime, -Mul(normAge, frameTime));
}

ILINE floatv DistFromPlane(Planev plane, Vec3v point)
{
	return Add(Dot(plane.n, point), plane.d);
}

ILINE ColorFv operator+(const ColorFv& a, const ColorFv& b)
{
	return ColorFv(
	  Add(a.r, b.r),
	  Add(a.g, b.g),
	  Add(a.b, b.b));
}

ILINE ColorFv operator*(const ColorFv& a, const ColorFv& b)
{
	return ColorFv(
	  Mul(a.r, b.r),
	  Mul(a.g, b.g),
	  Mul(a.b, b.b));
}

ILINE ColorFv operator*(const ColorFv& a, floatv b)
{
	return ColorFv(
	  Mul(a.r, b),
	  Mul(a.g, b),
	  Mul(a.b, b));
}

ILINE ColorFv ToColorFv(UColv color)
{
	ColorFv result;
	const __m128 toFloat = _mm_set1_ps(1.0f / 255.0f);
	const __m128i redMask = _mm_set1_epi32(0x00ff0000);
	const __m128i greenMask = _mm_set1_epi32(0x0000ff00);
	const __m128i blueMask = _mm_set1_epi32(0x000000ff);
	result.r = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_and_si128(color, redMask), 16)), toFloat);
	result.g = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_and_si128(color, greenMask), 8)), toFloat);
	result.b = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(color, blueMask)), toFloat);
	return result;
}

ILINE UColv ColorFvToUColv(const ColorFv& color)
{
	UColv result;
	const __m128 fromFloat = _mm_set1_ps(255.0f);
	const __m128i mask = _mm_set1_epi32(0x000000ff);
	const __m128i alphaMask = _mm_set1_epi32(0xff000000);

	const __m128i red = _mm_slli_epi32(_mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.r, fromFloat)), mask), 16);
	const __m128i green = _mm_slli_epi32(_mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.g, fromFloat)), mask), 8);
	const __m128i blue = _mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.b, fromFloat)), mask);

	result = _mm_or_si128(alphaMask, _mm_or_si128(red, _mm_or_si128(green, blue)));

	return result;
}

ILINE UColv ToUColv(UCol color)
{
	return _mm_set1_epi32(color.dcolor);
}

}
