// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RayInfo.h"
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CryPhysics/IPhysics.h>

namespace CryAudio
{
class CObject;

class CPropagationProcessor
{
public:

	CPropagationProcessor() = delete;
	CPropagationProcessor(CPropagationProcessor const&) = delete;
	CPropagationProcessor(CPropagationProcessor&&) = delete;
	CPropagationProcessor& operator=(CPropagationProcessor const&) = delete;
	CPropagationProcessor& operator=(CPropagationProcessor&&) = delete;

	static bool s_bCanIssueRWIs;

	using RayInfoVec = std::vector<CRayInfo>;
	using RayOcclusionVec = std::vector<float>;

	CPropagationProcessor(CObject& object);
	~CPropagationProcessor();

	void Init();

	// PhysicsSystem callback
	static int  OnObstructionTest(EventPhys const* pEvent);
	static void UpdateOcclusionRayFlags();

	void        Update();
	void        SetOcclusionType(EOcclusionType const occlusionType);
	float       GetOcclusion() const { return m_occlusion; }
	void        ProcessPhysicsRay(CRayInfo* const pRayInfo);
	void        ReleasePendingRays();
	bool        HasPendingRays() const { return m_remainingRays > 0; }
	bool        HasNewOcclusionValues();
	void        UpdateOcclusion();
	void        SetOcclusionRayOffset(float const offset) { m_occlusionRayOffset = std::max(0.0f, offset); }

private:

	void ProcessObstructionOcclusion();
	void CastObstructionRay(
		Vec3 const& origin,
		size_t const rayIndex,
		size_t const samplePosIndex,
		bool const bSynch);
	void   RunObstructionQuery();
	void   ProcessLow(Vec3 const& up, Vec3 const& side, bool const bSynch);
	void   ProcessMedium(Vec3 const& up, Vec3 const& side, bool const bSynch);
	void   ProcessHigh(Vec3 const& up, Vec3 const& side, bool const bSynch);
	size_t GetNumConcurrentRays() const;
	size_t GetNumSamplePositions() const;
	void   UpdateOcclusionPlanes();
	bool   CanRunOcclusion();

	float           m_lastQuerriedOcclusion;
	float           m_occlusion;
	float           m_currentListenerDistance;
	float           m_occlusionRayOffset;
	RayOcclusionVec m_raysOcclusion;

	size_t          m_remainingRays;
	size_t          m_rayIndex;

	CObject&        m_object;

	RayInfoVec      m_raysInfo;
	EOcclusionType  m_occlusionType;
	EOcclusionType  m_originalOcclusionType;
	EOcclusionType  m_occlusionTypeWhenAdaptive;

	static int      s_occlusionRayFlags;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	static size_t s_totalSyncPhysRays;
	static size_t s_totalAsyncPhysRays;

	void           DrawDebugInfo(IRenderAuxGeom& auxGeom);
	EOcclusionType GetOcclusionType() const             { return m_occlusionType; }
	EOcclusionType GetOcclusionTypeWhenAdaptive() const { return m_occlusionTypeWhenAdaptive; }
	float          GetOcclusionRayOffset() const        { return m_occlusionRayOffset; }
	void           ResetRayData();

private:

	void DrawRay(IRenderAuxGeom& auxGeom, size_t const rayIndex) const;

	struct SRayDebugInfo
	{
		SRayDebugInfo() = default;

		Vec3  begin = ZERO;
		Vec3  end = ZERO;
		Vec3  stableEnd = ZERO;
		float occlusionValue = 0.0f;
		float distanceToNearestObstacle = FLT_MAX;
		int   numHits = 0;
	};

	using RayDebugInfoVec = std::vector<SRayDebugInfo>;

	RayDebugInfoVec m_rayDebugInfos;
	ColorB          m_listenerOcclusionPlaneColor;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
