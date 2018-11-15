// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryParticleSystem/IParticlesPfx2.h>
#include <CryRenderer/RenderElements/CREParticle.h>

// compile options
#ifdef _DEBUG
	#define CRY_PFX2_DEBUG    // enable debugging on all particle source code
#endif
// #define CRY_PFX1_BAIL_UNSUPPORTED	// disable pfx1 features that not yet supported by pfx2 for precision profiling
// #define CRY_PFX2_LOAD_PRIORITY		// when trying to load a pfx1 effect, try to load pfx2 effect with the same name first
// #define CRY_PFX2_PROFILE_DETAILS     // more in detail profile of pfx2. Individual features and sub update parts will appear here.
// ~compile options

#if defined(CRY_PFX2_DEBUG) && CRY_PLATFORM_WINDOWS
	#pragma optimize("", off)
	#pragma inline_depth(0)
	#undef ILINE
	#define ILINE inline
#endif
#define CRY_PFX2_DBG	// obsolete

#define CRY_PFX2_ASSERT(cond) CRY_ASSERT(cond)

#ifdef CRY_PFX2_DEBUG
#	define CRY_PFX2_DEBUG_ASSERT(cond) CRY_PFX2_ASSERT(cond);
#else
#	define CRY_PFX2_DEBUG_ASSERT(cond)
#endif

#ifdef CRY_PFX2_PROFILE_DETAILS
	#define CRY_PFX2_PROFILE_DETAIL CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);
#else
	#define CRY_PFX2_PROFILE_DETAIL
#endif

#define CRY_PFX2_PARTICLES_ALIGNMENT 16

#ifdef CRY_PLATFORM_SSE2
	#define CRY_PFX2_USE_SSE
	#if (CRY_PLATFORM_DURANGO || CRY_PLATFORM_ORBIS)
		#define CRY_PFX2_USE_SEE4
	#endif
#endif

namespace pfx2
{

const uint gMinimumVersion = 1;
const uint gCurrentVersion = 13;

const TParticleId gInvalidId           = -1;
const float gInfinity                  = std::numeric_limits<float>::infinity();

using TParticleHeap                    = stl::HeapAllocator<stl::PSyncNone>;
template<typename T> using THeapArray  = TParticleHeap::Array<T, uint, CRY_PFX2_PARTICLES_ALIGNMENT>;
using TParticleIdArray                 = THeapArray<TParticleId>;
using TFloatArray                      = THeapArray<float>;

template<typename T> using TDynArray   = FastDynArray<T, uint, NAlloc::ModuleAlloc>;
template<typename T> using TSmartArray = TDynArray<_smart_ptr<T>>;

///////////////////////////////////////////////////////////////////////////////
#ifdef CRY_PFX2_USE_SSE

#define CRY_PFX2_PARTICLESGROUP_STRIDE 4 // can be 8 for AVX or 64 forGPU

struct TParticleGroupId
{
	TParticleGroupId() {}
	explicit TParticleGroupId(uint32 i) { id = i; CRY_PFX2_DEBUG_ASSERT(IsAligned(i, CRY_PFX2_PARTICLESGROUP_STRIDE)); }
	friend bool                       operator<(const TParticleGroupId a, const TParticleGroupId b)  { return a.id < b.id; }
	friend bool                       operator>(const TParticleGroupId a, const TParticleGroupId b)  { return a.id > b.id; }
	friend bool                       operator<=(const TParticleGroupId a, const TParticleGroupId b) { return a.id <= b.id; }
	friend bool                       operator>=(const TParticleGroupId a, const TParticleGroupId b) { return a.id >= b.id; }
	friend bool                       operator==(const TParticleGroupId a, const TParticleGroupId b) { return a.id == b.id; }
	friend bool                       operator!=(const TParticleGroupId a, const TParticleGroupId b) { return a.id != b.id; }
	TParticleGroupId                  operator++(int)                                                { id++; return *this; }
	TParticleGroupId&                 operator+=(uint stride)                                        { id += stride; return *this; }
	template<class type> friend type* operator+(type* ptr, TParticleGroupId id)                      { return ptr + id.id; }
	friend TParticleGroupId           operator&(TParticleGroupId id, uint32 mask)                    { return TParticleGroupId(id.id & mask); }
	uint32                            operator+() const                                              { return id; }
private:
	uint32 id;
};

#else

#define CRY_PFX2_PARTICLESGROUP_STRIDE 1

typedef TParticleId TParticleGroupId;

#endif

#define CRY_PFX2_PARTICLESGROUP_ALIGN(id) Align(id, CRY_PFX2_PARTICLESGROUP_STRIDE)


///////////////////////////////////////////////////////////////////////////////
struct SRenderContext
{
	SRenderContext(const SRendParams& rParams, const SRenderingPassInfo& passInfo)
		: m_renderParams(rParams)
		, m_passInfo(passInfo)
		, m_distance(0.0f)
		, m_lightVolumeId(0)
		, m_fogVolumeId(0) {}
	const SRendParams&        m_renderParams;
	const SRenderingPassInfo& m_passInfo;
	float                     m_distance;
	uint16                    m_lightVolumeId;
	uint16                    m_fogVolumeId;
};

///////////////////////////////////////////////////////////////////////////////
template<typename TIndex, int nStride = 1>
struct TIndexRange
{
	using TInt = decltype(+TIndex());

	struct iterator
	{
		TIndex i;
		iterator(TIndex i) : i(i) {}
		bool operator != (iterator it) const { return i != it.i; }
		void operator ++ () { i += nStride; }
		TIndex operator*() const { return i; }
	};

	TIndex m_begin, m_end;

	iterator begin() const { return iterator(m_begin); }
	iterator end() const   { return iterator(m_end); }
	TInt size() const      { return +m_end - +m_begin; }

	explicit TIndexRange(TIndex b = 0, TIndex e = 0)
		: m_begin(+b)
		, m_end(Align(+e, nStride))
	{
		CRY_PFX2_DEBUG_ASSERT(IsAligned(+b, nStride));
	}

	template<typename TIndex2, int nStride2>
	TIndexRange(TIndexRange<TIndex2, nStride2> range)
		: TIndexRange(TIndex(+range.m_begin), TIndex(Align(+range.m_end, nStride))) {}
};

typedef TIndexRange<TParticleId> SUpdateRange;
typedef TIndexRange<TParticleGroupId, CRY_PFX2_PARTICLESGROUP_STRIDE> SGroupRange;

///////////////////////////////////////////////////////////////////////////////
template<typename C>
struct SkipEmptySerialize
{
	C& container;

	SkipEmptySerialize(C& c) : container(c) {}
};

template<typename C>
SkipEmptySerialize<C> SkipEmpty(C& cont)
{
	return SkipEmptySerialize<C>(cont);
};

template<typename C>
bool Serialize(Serialization::IArchive& ar, SkipEmptySerialize<C>& cont, cstr name, cstr label)
{
	if (ar.isOutput() && !ar.isEdit() && cont.container.empty())
		return true;
	return Serialize(ar, cont.container, name, label);
}

///////////////////////////////////////////////////////////////////////////////
template<typename T>
class TSaveRestore
{
public:
	template<class... Vals>
	TSaveRestore(T& save, Vals&&... vals)
		: m_saved(save)
		, m_object(std::forward<Vals>(vals)...)
	{
		swap();
	}
	~TSaveRestore()
	{
		swap();
	}

private:
	T& m_saved;
	T  m_object;

	void swap()
	{
		char temp[sizeof(T)];
		memcpy(temp, &m_object, sizeof(T));
		memcpy(&m_object, &m_saved, sizeof(T));
		memcpy(&m_saved, temp, sizeof(T));
	}
};

///////////////////////////////////////////////////////////////////////////////
inline int ThreadMode() { return Cry3DEngineBase::GetCVars()->e_ParticlesThread; }

}



