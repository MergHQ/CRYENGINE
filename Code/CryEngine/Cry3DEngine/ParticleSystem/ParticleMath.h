// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     05/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ParticleCommon.h"

namespace pfx2
{

using namespace crymath;

#ifdef CRY_PFX2_USE_SSE
	#define CRY_PFX2_PARTICLESGROUP_STRIDE 4 // can be 8 for AVX or 64 forGPU
#else
	#define CRY_PFX2_PARTICLESGROUP_STRIDE 1
#endif
#define CRY_PFX2_PARTICLESGROUP_LOWER(id) ((id) & ~((CRY_PFX2_PARTICLESGROUP_STRIDE - 1)))
#define CRY_PFX2_PARTICLESGROUP_UPPER(id) ((id) | ((CRY_PFX2_PARTICLESGROUP_STRIDE - 1)))

// 4x vector types of standard types
#ifdef CRY_PFX2_USE_SSE
typedef f32v4  floatv;
typedef i32v4  uint32v;
typedef i32v4  int32v;
typedef uint32 uint8v;
typedef i32v4  UColv;
#else
typedef float  floatv;
typedef uint32 uint32v;
typedef int32  int32v;
typedef uint8  uint8v;
typedef UCol   UColv;
#endif

// 4x vector types of custom types
typedef uint32v           TParticleIdv;
typedef uint32v           TEmitterIdv;
typedef Vec3_tpl<floatv>  Vec3v;
typedef Vec4_tpl<floatv>  Vec4v;
typedef Plane_tpl<floatv> Planev;

#ifdef CRY_PFX2_USE_SSE
struct ColorFv
{
	ColorFv() {}
	ColorFv(floatv _r, floatv _g, floatv _b)
		: r(_r), g(_g), b(_b) {}
	floatv r, g, b;
};
#else
typedef ColorF ColorFv;
#endif

struct IFStream
{
public:
	explicit IFStream(const float* pStream = nullptr, float defaultVal = 0.0f);
	bool   IsValid() const { return m_safeMask != 0; }
	float  Load(TParticleId pId) const;
	float  Load(TParticleId pId, float defaultVal) const;
	float  SafeLoad(TParticleId pId) const;
#ifdef CRY_PFX2_USE_SSE
	floatv Load(TParticleGroupId pgId) const;
	floatv Load(TParticleIdv pIdv, float defaultVal = 0.0f) const;
	floatv SafeLoad(TParticleGroupId pgId) const;
#endif
private:
	const float* __restrict m_pStream;
	const floatv            m_safeSink;
	const uint32            m_safeMask;
};

struct IOFStream
{
public:
	explicit IOFStream(float* pStream = nullptr);
	bool   IsValid() const { return m_pStream != 0; }
	float  Load(TParticleId pId) const;
	void   Store(TParticleId pId, float value);
#ifdef CRY_PFX2_USE_SSE
	floatv Load(TParticleGroupId pgId) const;
	void   Store(TParticleGroupId pgId, floatv value);
#endif
private:
	float* __restrict m_pStream;
};

struct IVec3Stream
{
public:
	IVec3Stream(const float* pX, const float* pY, const float* pZ, Vec3 defaultVal = Vec3());
	Vec3  Load(TParticleId pId) const;
	Vec3  SafeLoad(TParticleId pId) const;
#ifdef CRY_PFX2_USE_SSE
	Vec3v Load(TParticleGroupId pgId) const;
	Vec3v Load(TParticleIdv pIdv, Vec3 defaultVal = Vec3(ZERO)) const;
	Vec3v SafeLoad(TParticleGroupId pgId) const;
#endif
private:
	const Vec3v             m_safeSink;
	const float* __restrict m_pXStream;
	const float* __restrict m_pYStream;
	const float* __restrict m_pZStream;
	const uint32            m_safeMask;
};

struct IOVec3Stream
{
public:
	IOVec3Stream(float* pX, float* pY, float* pZ);
	Vec3  Load(TParticleId pId) const;
	void  Store(TParticleId pId, Vec3 value);
#ifdef CRY_PFX2_USE_SSE
	Vec3v Load(TParticleGroupId pgId) const;
	void  Store(TParticleGroupId pgId, const Vec3v& value) const;
#endif
private:
	float* __restrict m_pXStream;
	float* __restrict m_pYStream;
	float* __restrict m_pZStream;
};

struct IQuatStream
{
public:
	IQuatStream(const float* pX, const float* pY, const float* pZ, const float* pW, Quat defaultVal = Quat());
	Quat Load(TParticleId pId) const;
	Quat SafeLoad(TParticleId pId) const;

private:
	const Quat              m_safeSink;
	const float* __restrict m_pXStream;
	const float* __restrict m_pYStream;
	const float* __restrict m_pZStream;
	const float* __restrict m_pWStream;
	const uint32            m_safeMask;
};

struct IOQuatStream
{
public:
	IOQuatStream(float* pX, float* pY, float* pZ, float* pW);
	Quat Load(TParticleId pId) const;
	void Store(TParticleId pId, Quat value);

private:
	float* __restrict m_pXStream;
	float* __restrict m_pYStream;
	float* __restrict m_pZStream;
	float* __restrict m_pWStream;
};

struct IOColorStream
{
public:
	explicit IOColorStream(UCol* pStream);
	bool  IsValid() const { return m_pStream != 0; }
	UCol  Load(TParticleId pId) const;
	void  Store(TParticleId pId, UCol value);
#ifdef CRY_PFX2_USE_SSE
	UColv Load(TParticleGroupId pgId) const;
	void  Store(TParticleGroupId pgId, UColv value);
#endif
private:
	UCol* __restrict m_pStream;
};

template<typename T, typename Tv = T>
struct TIStream
{
public:
	explicit TIStream(const T* pStream = 0, const T& defaultVal = T());
	TIStream(const TIStream<T, Tv>& src);
	TIStream<T, Tv>& operator=(const TIStream<T, Tv>& src);
	bool IsValid() const { return m_safeMask != 0; }
	T    Load(TParticleId pId) const;
	T    Load(TParticleId pId, T defaultVal) const;
	T    SafeLoad(TParticleId pId) const;
#ifdef CRY_PFX2_USE_SSE
	Tv   Load(TParticleGroupId pgId) const;
	Tv   Load(TParticleIdv pIdv, T defaultVal) const;
#endif
private:
	const T* __restrict m_pStream;
	T                   m_safeSink;
	uint32              m_safeMask;
};

template<typename T>
struct TIOStream
{
public:
	explicit TIOStream(T* pStream);
	bool IsValid() const { return m_pStream != 0; }
	T    Load(TParticleId pId) const;
	void Store(TParticleId pId, T value);

private:
	T* __restrict m_pStream;
};

///////////////////////////////////////////////////////////////////////////
// Vector/scalar conversions

floatv  ToFloatv(float v);
uint32v ToUint32v(uint32 v);
floatv  ToFloatv(uint32v v);
uint32v ToUint32v(floatv v);
Vec3v   ToVec3v(Vec3 v);
Vec4v   ToVec4v(Vec4 v);
Planev  ToPlanev(Plane v);

uint8  FloatToUFrac8Saturate(float v);

///////////////////////////////////////////////////////////////////////////
// General operator and function equivalents
template<typename T, typename U> T ILINE Add(const T& a, const U& b)              { return a + b; }
template<typename T, typename U> T ILINE Sub(const T& a, const U& b)              { return a - b; }
template<typename T, typename U> T ILINE Mul(const T& a, const U& b)              { return a * b; }

template<typename T, typename U> T ILINE MAdd(const T& a, const U& b, const T& c) { return a * b + c; }

///////////////////////////////////////////////////////////////////////////
// Vectorized functions

float  HMin(floatv v0);
float  HMax(floatv v0);

// Vector functions

Vec3v Add(const Vec3v& a, const floatv b);
Vec3v Sub(const Vec3v& a, const floatv b);

Vec3  HMin(const Vec3v& v0);
Vec3  HMax(const Vec3v& v0);

// Efficient vector MAdd

template<typename T> Vec3_tpl<T> MAdd(const Vec3_tpl<T>& a, const Vec3_tpl<T>& b, const Vec3_tpl<T>& c);
template<typename T> Vec3_tpl<T> MAdd(const Vec3_tpl<T>& a, T b, const Vec3_tpl<T>& c);

// Color

ColorF  ToColorF(UCol color);
ColorFv ToColorFv(UColv color);
ColorFv ToColorFv(ColorF color);
UCol    ColorFToUCol(const ColorF& color);
UColv   ColorFvToUColv(const ColorFv& color);
UColv   ToUColv(UCol color);

#ifdef CRY_PFX2_USE_SSE
ColorFv operator+(const ColorFv& a, const ColorFv& b);
ColorFv operator*(const ColorFv& a, const ColorFv& b);
ColorFv operator*(const ColorFv& a, floatv b);
#endif

// Special
floatv DeltaTime(floatv normAge, floatv frameTime);

// non vectorized

void RotateAxes(Vec3* v0, Vec3* v1, const float angle);

}

#include "ParticleMathImpl.h"
