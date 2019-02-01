// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RayInfo.h"
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CryPhysics/IPhysics.h>

namespace CryAudio
{
class CObject;

constexpr uint8 g_numberLow = 7;
constexpr uint8 g_numberMedium = 9;
constexpr uint8 g_numberHigh = 11;
constexpr uint8 g_numRaySamplePositionsLow = g_numberLow * g_numberLow;
constexpr uint8 g_numRaySamplePositionsMedium = g_numberMedium * g_numberMedium;
constexpr uint8 g_numRaySamplePositionsHigh = g_numberHigh * g_numberHigh;
constexpr uint8 g_numConcurrentRaysLow = 1;
constexpr uint8 g_numConcurrentRaysMedium = 2;
constexpr uint8 g_numConcurrentRaysHigh = 4;

class CPropagationProcessor
{
public:

	CPropagationProcessor() = delete;
	CPropagationProcessor(CPropagationProcessor const&) = delete;
	CPropagationProcessor(CPropagationProcessor&&) = delete;
	CPropagationProcessor& operator=(CPropagationProcessor const&) = delete;
	CPropagationProcessor& operator=(CPropagationProcessor&&) = delete;

	static bool s_bCanIssueRWIs;

	CPropagationProcessor(CObject& object);
	~CPropagationProcessor() = default;

	void Init();

	// PhysicsSystem callback
	static int  OnObstructionTest(EventPhys const* pEvent);
	static void UpdateOcclusionRayFlags();

	void        Update();
	void        SetOcclusionType(EOcclusionType const occlusionType);
	float       GetOcclusion() const { return m_occlusion; }
	void        ProcessPhysicsRay(CRayInfo& rayInfo);
	void        ReleasePendingRays();
	bool        HasPendingRays() const { return m_remainingRays > 0; }
	bool        HasNewOcclusionValues();
	void        UpdateOcclusion();
	void        SetOcclusionRayOffset(float const offset) { m_occlusionRayOffset = std::max(0.0f, offset); }

private:

	void ProcessObstructionOcclusion();
	void CastObstructionRay(
		Vec3 const& origin,
		uint8 const rayIndex,
		uint8 const samplePosIndex,
		bool const bSynch);
	void  RunObstructionQuery();
	void  ProcessLow(Vec3 const& up, Vec3 const& side, bool const bSynch);
	void  ProcessMedium(Vec3 const& up, Vec3 const& side, bool const bSynch);
	void  ProcessHigh(Vec3 const& up, Vec3 const& side, bool const bSynch);
	uint8 GetNumConcurrentRays() const;
	uint8 GetNumSamplePositions() const;
	void  UpdateOcclusionPlanes();
	bool  CanRunOcclusion();

	float          m_lastQuerriedOcclusion;
	float          m_occlusion;
	float          m_currentListenerDistance;
	float          m_occlusionRayOffset;
	float          m_raysOcclusion[g_numRaySamplePositionsHigh] = { 0.0f };

	uint8          m_remainingRays;
	uint8          m_rayIndex;

	CObject&       m_object;

	CRayInfo       m_raysInfo[g_numConcurrentRaysHigh];
	EOcclusionType m_occlusionType;
	EOcclusionType m_originalOcclusionType;
	EOcclusionType m_occlusionTypeWhenAdaptive;

	static int     s_occlusionRayFlags;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
public:

	static uint16 s_totalSyncPhysRays;
	static uint16 s_totalAsyncPhysRays;

	void           DrawDebugInfo(IRenderAuxGeom& auxGeom);
	EOcclusionType GetOcclusionType() const             { return m_occlusionType; }
	EOcclusionType GetOcclusionTypeWhenAdaptive() const { return m_occlusionTypeWhenAdaptive; }
	float          GetOcclusionRayOffset() const        { return m_occlusionRayOffset; }
	void           ResetRayData();

private:

	void DrawRay(IRenderAuxGeom& auxGeom, uint8 const rayIndex) const;

	struct SRayDebugInfo
	{
		SRayDebugInfo() = default;

		Vec3  begin = ZERO;
		Vec3  end = ZERO;
		Vec3  stableEnd = ZERO;
		float occlusionValue = 0.0f;
		float distanceToNearestObstacle = FLT_MAX;
		uint8 numHits = 0;
	};

	SRayDebugInfo m_rayDebugInfos[g_numConcurrentRaysHigh];
	ColorB        m_listenerOcclusionPlaneColor;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};
} // namespace CryAudio
