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

template<int i>
ILINE int ExtractI32(uint32v a)
{
	return _mm_extract_epi32(a, i);
}

template<int i>
ILINE float ExtractF32(floatv a)
{
	float r;
	_MM_EXTRACT_FLOAT(r, a, i);
	return r;
}

#else

template<int i>
ILINE int ExtractI32(uint32v a)
{
	return _mm_cvtsi128_si32(_mm_shuffle_epi32(a, _MM_SHUFFLE(i, i, i, i)));
}

template<int i>
ILINE float ExtractF32(floatv a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a, a, _MM_SHUFFLE(i, i, i, i)));
}

#endif

ILINE floatv LoadIndexed4(const float* __restrict pStream, const uint32v index, float defaultVal)
{
	const i32mask4 mask = index == convert<uint32v>(gInvalidId);
	const uint32v mIndex = _mm_andnot_si128((__m128i)mask, index);
	const uint32v mFirst = _mm_shuffle_epi32(mIndex, 0);

	if (All(index == mFirst + _mm_set_epi32(3, 2, 1, 0)))
		return _mm_loadu_ps(pStream + _mm_cvtsi128_si32(mFirst));

	floatv output = _mm_load1_ps(pStream + _mm_cvtsi128_si32(mFirst));
	if (!All(mFirst == mIndex))
	{
		const float f1 = *(pStream + ExtractI32<1>(mIndex));
		const float f2 = *(pStream + ExtractI32<2>(mIndex));
		const float f3 = *(pStream + ExtractI32<3>(mIndex));
		const floatv m = _mm_castsi128_ps(_mm_set_epi32(0, 0, 0, ~0));
		const floatv v0 = _mm_set_ps(f3, f2, f1, 0.0f);
		output = _mm_or_ps(v0, _mm_and_ps(output, m));
	}

	return if_else(mask, _mm_set1_ps(defaultVal), output);
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
	static_assert(sizeof(UColv) == sizeof(floatv), "cannot use floatv to store UColv");
	const float defaultValF = _mm_cvtss_f32(_mm_castsi128_ps(_mm_set1_epi32(defaultVal.dcolor)));
	const float* streamF = reinterpret_cast<const float*>(m_pStream);
	floatv dataF = detail::LoadIndexed4(streamF, pIdv, defaultValF);
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

ILINE float HMin(floatv v0)
{
	const floatv v1 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 3, 1, 1));
	const floatv v2 = _mm_min_ps(v0, v1);
	const floatv v3 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(2, 2, 2, 2));
	const floatv res = _mm_min_ps(v3, v2);
	return _mm_cvtss_f32(res);
}

ILINE float HMax(floatv v0)
{
	const floatv v1 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(3, 3, 1, 1));
	const floatv v2 = _mm_max_ps(v0, v1);
	const floatv v3 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(2, 2, 2, 2));
	const floatv res = _mm_max_ps(v3, v2);
	return _mm_cvtss_f32(res);
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
	const floatv toFloat = _mm_set1_ps(1.0f / 255.0f);
	const uint32v redMask = _mm_set1_epi32(0x00ff0000);
	const uint32v greenMask = _mm_set1_epi32(0x0000ff00);
	const uint32v blueMask = _mm_set1_epi32(0x000000ff);
	result.r = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_and_si128(color, redMask), 16)), toFloat);
	result.g = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_and_si128(color, greenMask), 8)), toFloat);
	result.b = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(color, blueMask)), toFloat);
	return result;
}

ILINE UColv ColorFvToUColv(const ColorFv& color)
{
	UColv result;
	const floatv fromFloat = _mm_set1_ps(255.0f);
	const uint32v mask = _mm_set1_epi32(0x000000ff);
	const uint32v alphaMask = _mm_set1_epi32(0xff000000);

	const uint32v red = _mm_slli_epi32(_mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.r, fromFloat)), mask), 16);
	const uint32v green = _mm_slli_epi32(_mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.g, fromFloat)), mask), 8);
	const uint32v blue = _mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.b, fromFloat)), mask);

	result = _mm_or_si128(alphaMask, _mm_or_si128(red, _mm_or_si128(green, blue)));

	return result;
}

ILINE UColv ToUColv(UCol color)
{
	return _mm_set1_epi32(color.dcolor);
}

}
