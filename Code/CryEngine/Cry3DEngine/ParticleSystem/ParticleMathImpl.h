// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

namespace pfx2
{

// Vector functions

ILINE Vec3 HMin(const Vec3v& v0)
{
	return Vec3(
	  hmin(v0.x),
		hmin(v0.y),
		hmin(v0.z));
}

ILINE Vec3 HMax(const Vec3v& v0)
{
	return Vec3(
	  hmax(v0.x),
		hmax(v0.y),
		hmax(v0.z));
}

ILINE Vec3v Add(const Vec3v& a, floatv b)
{
	return Vec3v(a.x + b, a.y + b, a.z + b);
}
ILINE Vec3v Sub(const Vec3v& a, floatv b)
{
	return Vec3v(a.x - b, a.y - b, a.z - b);
}

template<typename T>
ILINE Vec3_tpl<T> MAdd(const Vec3_tpl<T>& a, const Vec3_tpl<T>& b, const Vec3_tpl<T>& c)
{
	return Vec3_tpl<T>(
	  MAdd(a.x, b.x, c.x),
	  MAdd(a.y, b.y, c.y),
	  MAdd(a.z, b.z, c.z));
}

template<typename T>
ILINE Vec3_tpl<T> MAdd(const Vec3_tpl<T>& a, T b, const Vec3_tpl<T>& c)
{
	return Vec3_tpl<T>(
	  MAdd(a.x, b, c.x),
	  MAdd(a.y, b, c.y),
	  MAdd(a.z, b, c.z));
}

template<typename T>
ILINE T DeltaTime(T frameTime, T normAge, T lifeTime)
{
	// Birth time = -age * life
	// Death time = -age * life + life
	// Delta time = min(- age * life + life, 0) - max(-age * life, -dT)
	T age = normAge * lifeTime;
	T startTime = -min(age, frameTime);
	T endTime = min(lifeTime - age, convert<T>());
	T deltaTime = max(endTime - startTime, convert<T>());
	return deltaTime;
}

template<typename T>
ILINE T StartTime(T curTime, T frameTime, T absAge)
{
	// Birth time = T - age*life
	// Start time = T - min(age*life, dT)
	return curTime - min(absAge, frameTime);
}


ILINE uint8 FloatToUFrac8Saturate(float v)
{
	return uint8(saturate(v) * 255.0f + 0.5f);
}

ILINE Vec3 ToVec3(UCol color)
{
	return Vec3(
	  ufrac8_to_float(color.r),
	  ufrac8_to_float(color.g),
	  ufrac8_to_float(color.b));
}

ILINE UCol ToUCol(const Vec3& color)
{
	UCol result;
	result.r = float_to_ufrac8(color.x);
	result.g = float_to_ufrac8(color.y);
	result.b = float_to_ufrac8(color.z);
	result.a = 0xff;
	return result;
}

ILINE void RotateAxes(Vec3* v0, Vec3* v1, float angle)
{
	CRY_PFX2_ASSERT(v0 && v1);
	Vec2 vRot;
	sincos(angle, &vRot.x, &vRot.y);
	Vec3 vXAxis = *v0 * vRot.y - *v1 * vRot.x;
	Vec3 vYAxis = *v0 * vRot.x + *v1 * vRot.y;
	*v0 = vXAxis;
	*v1 = vYAxis;
}

ILINE Vec3 PolarCoordToVec3(float azimuth, float altitude)
{
	float azc, azs;
	float alc, als;
	sincos(azimuth, &azs, &azc);
	sincos(altitude, &als, &alc);
	return Vec3(azc*alc, azs*alc, als);
}

ILINE Vec2 CirclePoint(float a)
{
	Vec2 p;
	sincos(a, &p.y, &p.x);
	return p;
}

ILINE Vec2 DiskPoint(float a, float r2)
{
	Vec2 p = CirclePoint(a);
	float r = sqrt(r2);
	return p * r;
}

inline Vec3 SpherePoint(float a, float z)
{
	Vec3 p;
	sincos(a, &p.y, &p.x);
	float h = sqrt(1.0f - sqr(z));
	p.x *= h;
	p.y *= h;
	p.z = z;
	return p;
}

inline Vec3 BallPoint(float a, float z, float r3)
{
	static CubeRootApprox cubeRootApprox(0.125f);

	Vec3 p = SpherePoint(a, z);
	float r = cubeRootApprox(r3);
	return p * r;
}


inline Quat AddAngularVelocity(Quat const& initial, Vec3 const& angularVel, float deltaTime)
{
	const float haldDt = deltaTime * 0.5f;
	const Quat rotated = Quat::exp(angularVel * haldDt) * initial;
	return rotated.GetNormalized();
}

}

#ifdef CRY_PFX2_USE_SSE
	#include "ParticleMathImplSSE.h"
#else

// PFX2_TODO : move all of this to another file (or just implement NEON for once)
namespace pfx2
{

ILINE Vec3v ToVec3v(UColv color)
{
	return Vec3v(
	  ufrac8_to_float(color.r),
	  ufrac8_to_float(color.g),
	  ufrac8_to_float(color.b));
}

ILINE UColv ToUColv(const Vec3v& color)
{
	UColv result;
	result.r = float_to_ufrac8(color.x);
	result.g = float_to_ufrac8(color.y);
	result.b = float_to_ufrac8(color.z);
	result.a = 0xff;
	return result;
}


}

#endif
