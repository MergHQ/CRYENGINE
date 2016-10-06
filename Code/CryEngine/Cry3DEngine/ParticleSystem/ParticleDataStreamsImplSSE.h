// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

namespace pfx2
{

namespace detail
{

ILINE floatv LoadIndexed4(const float* __restrict pStream, const uint32v index, float defaultVal)
{
	const mask32v4 mask = index == convert<uint32v>(gInvalidId);
	const uint32v mIndex = _mm_andnot_si128(mask, index);
	const uint32v mFirst = _mm_shuffle_epi32(mIndex, 0);

	if (All(index == mFirst + convert<uint32v>(0, 1, 2, 3)))
		return _mm_loadu_ps(pStream + get_element<0>(mFirst));

	floatv output = _mm_load1_ps(pStream + get_element<0>((mFirst)));
	if (!All(mFirst == mIndex))
	{
		const float f1 = *(pStream + get_element<1>(mIndex));
		const float f2 = *(pStream + get_element<2>(mIndex));
		const float f3 = *(pStream + get_element<3>(mIndex));
		const floatv m = vcast<floatv>(convert<uint32v>(~0, 0, 0, 0));
		const floatv v0 = convert<floatv>(0.0f, f1, f2, f3);
		output = _mm_or_ps(v0, _mm_and_ps(output, m));
	}

	return if_else(mask, convert<floatv>(defaultVal), output);
}

}

//////////////////////////////////////////////////////////////////////////
// floatv

template<>
ILINE floatv TIStream<float, floatv >::Load(TParticleGroupId pgId) const
{
	return _mm_load_ps(m_pStream + pgId);
}

template<>
ILINE floatv TIStream<float, floatv >::Load(TParticleIdv pIdv, float defaultVal) const
{
	return detail::LoadIndexed4(m_pStream, pIdv, defaultVal);
}

template<>
ILINE floatv TIStream<float, floatv >::SafeLoad(TParticleGroupId pgId) const
{
	return _mm_load_ps(m_pStream + (pgId & m_safeMask));
}

template<>
ILINE void TIOStream<float, floatv >::Store(TParticleGroupId pgId, floatv value)
{
	_mm_store_ps(const_cast<float*>(m_pStream) + pgId, value);
}

//////////////////////////////////////////////////////////////////////////
// uint32

template<>
ILINE uint32v TIStream<uint32, uint32v >::Load(TParticleGroupId pgId) const
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(m_pStream + pgId));
}

template<>
ILINE uint32v TIStream<uint32, uint32v >::Load(TParticleIdv pIdv, uint32 defaultVal) const
{
	const float defaultValF = _mm_cvtss_f32(_mm_castsi128_ps(_mm_set1_epi32(defaultVal)));
	const float* streamF = reinterpret_cast<const float*>(m_pStream);
	__m128 dataF = detail::LoadIndexed4(streamF, pIdv, defaultValF);
	return _mm_castps_si128(dataF);
}

template<>
ILINE uint32v TIStream<uint32, uint32v >::SafeLoad(TParticleGroupId pgId) const
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(m_pStream + (pgId & m_safeMask)));
}

template<>
ILINE void TIOStream<uint32, uint32v >::Store(TParticleGroupId pgId, uint32v value)
{
	_mm_store_si128(reinterpret_cast<__m128i*>(const_cast<uint32*>(m_pStream) + pgId), value);
}

//////////////////////////////////////////////////////////////////////////
// UColv

template<>
ILINE UColv TIStream<UCol, UColv >::Load(TParticleGroupId pgId) const
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(m_pStream + pgId));
}

template<>
ILINE UColv TIStream<UCol, UColv >::Load(TParticleIdv pIdv, UCol defaultVal) const
{
	const float defaultValF = _mm_cvtss_f32(_mm_castsi128_ps(_mm_set1_epi32(defaultVal.dcolor)));
	const float* streamF = reinterpret_cast<const float*>(m_pStream);
	__m128 dataF = detail::LoadIndexed4(streamF, pIdv, defaultValF);
	return _mm_castps_si128(dataF);
}

template<>
ILINE UColv TIStream<UCol, UColv >::SafeLoad(TParticleGroupId pgId) const
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(m_pStream + (pgId & m_safeMask)));
}

template<>
ILINE void TIOStream<UCol, UColv >::Store(TParticleGroupId pgId, UColv value)
{
	_mm_store_si128(reinterpret_cast<__m128i*>(const_cast<UCol*>(m_pStream) + pgId), value);
}

//////////////////////////////////////////////////////////////////////////

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

ILINE void IOVec3Stream::Store(TParticleGroupId pgId, const Vec3v& value) const
{
	_mm_store_ps(const_cast<float*>(m_pXStream) + pgId, value.x);
	_mm_store_ps(const_cast<float*>(m_pYStream) + pgId, value.y);
	_mm_store_ps(const_cast<float*>(m_pZStream) + pgId, value.z);
}

ILINE Quatv IQuatStream::Load(TParticleGroupId pgId) const
{
	return Quatv(
		_mm_load_ps(m_pWStream + pgId),
		_mm_load_ps(m_pXStream + pgId),
		_mm_load_ps(m_pYStream + pgId),
		_mm_load_ps(m_pZStream + pgId));
}

ILINE Quatv IQuatStream::Load(TParticleIdv pIdv, Quat defaultVal) const
{
	return Quatv(
		detail::LoadIndexed4(m_pWStream, pIdv, defaultVal.w),
		detail::LoadIndexed4(m_pXStream, pIdv, defaultVal.v.x),
		detail::LoadIndexed4(m_pYStream, pIdv, defaultVal.v.y),
		detail::LoadIndexed4(m_pZStream, pIdv, defaultVal.v.z));
}

ILINE Quatv IQuatStream::SafeLoad(TParticleGroupId pgId) const
{
	return Quatv(
		_mm_load_ps(m_pWStream + (pgId & m_safeMask)),
		_mm_load_ps(m_pXStream + (pgId & m_safeMask)),
		_mm_load_ps(m_pYStream + (pgId & m_safeMask)),
		_mm_load_ps(m_pZStream + (pgId & m_safeMask)));
}

ILINE void IOQuatStream::Store(TParticleGroupId pgId, const Quatv& value) const
{
	_mm_store_ps(const_cast<float*>(m_pWStream) + pgId, value.w);
	_mm_store_ps(const_cast<float*>(m_pXStream) + pgId, value.v.x);
	_mm_store_ps(const_cast<float*>(m_pYStream) + pgId, value.v.y);
	_mm_store_ps(const_cast<float*>(m_pZStream) + pgId, value.v.z);
}

}
