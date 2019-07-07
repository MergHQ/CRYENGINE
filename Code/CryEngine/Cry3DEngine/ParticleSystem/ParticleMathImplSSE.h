// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

namespace pfx2
{

namespace detail
{

}

ILINE Vec3v ToVec3v(UColv color)
{
	Vec3v result;
	const floatv toFloat = _mm_set1_ps(1.0f / 255.0f);
	const uint32v redMask = _mm_set1_epi32(0x00ff0000);
	const uint32v greenMask = _mm_set1_epi32(0x0000ff00);
	const uint32v blueMask = _mm_set1_epi32(0x000000ff);
	result.x = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_and_si128(color, redMask), 16)), toFloat);
	result.y = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_and_si128(color, greenMask), 8)), toFloat);
	result.z = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(color, blueMask)), toFloat);
	return result;
}

ILINE UColv ToUColv(const Vec3v& color)
{
	UColv result;
	const floatv fromFloat = _mm_set1_ps(255.0f);
	const uint32v mask = _mm_set1_epi32(0x000000ff);
	const uint32v alphaMask = _mm_set1_epi32(0xff000000);

	const uint32v red = _mm_slli_epi32(_mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.x, fromFloat)), mask), 16);
	const uint32v green = _mm_slli_epi32(_mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.y, fromFloat)), mask), 8);
	const uint32v blue = _mm_and_si128(_mm_cvtps_epi32(_mm_mul_ps(color.z, fromFloat)), mask);

	result = _mm_or_si128(alphaMask, _mm_or_si128(red, _mm_or_si128(green, blue)));

	return result;
}

//////////////////////////////////////////////////////////////////////////
// Quaternions

ILINE floatv sin_fast(floatv x)
{
	const floatv one = ToFloatv(1.0f);
	const floatv negOne = ToFloatv(-1.0f);
	const floatv half = ToFloatv(0.5f);
	const floatv negHalfPi = ToFloatv(-gf_PI * 0.5f);
	const floatv pi = ToFloatv(gf_PI);
	const floatv ipi = ToFloatv(1.0f / gf_PI);
	const floatv ipi2 = ToFloatv(1.0f / gf_PI2);

	const floatv x1 = MAdd(frac(x * ipi), pi, negHalfPi);
	const floatv m = __fsel(Sub(frac(x * ipi2), half), one, negOne);

	const floatv p0 = ToFloatv(-0.4964738f);
	const floatv p1 = ToFloatv(0.036957536f);
	const floatv x2 = x1 * x1;
	const floatv x4 = x2 * x2;
	const floatv result = MAdd(x4, p1, MAdd(x2, p0, one)) * m;

	return result;
}

ILINE floatv cos_fast(floatv x)
{
	const floatv halfPi = ToFloatv(gf_PI * 0.5f);
	return sin_fast(x - halfPi);
}

ILINE Quatv quat_exp_fast(Vec3v v)
{
	const floatv lenSqr = v.len2();
	const floatv len = sqrt_fast(lenSqr);
	const floatv invLen = rcp_fast(max(len, convert<floatv>(FLT_MIN)));
	const floatv s = sin_fast(len) * invLen;
	const floatv c = cos_fast(len);

	return Quatv(c, v.x * s, v.y * s, v.z * s);
}

inline Quatv AddAngularVelocity(Quatv const& initial, Vec3v const& angularVel, floatv deltaTime)
{
	const floatv haldDt = deltaTime * ToFloatv(0.5f);
	const Quatv rotated = quat_exp_fast(angularVel * haldDt) * initial;
	return rotated.GetNormalized();
}

}
