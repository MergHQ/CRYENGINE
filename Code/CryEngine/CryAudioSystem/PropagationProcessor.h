// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(CRY_AUDIO_USE_OCCLUSION)
	#include "RayInfo.h"
	#include <CryAudio/IAudioInterfacesCommonData.h>
	#include <CryPhysics/IPhysics.h>

namespace CryAudio
{
constexpr uint8 g_numberLow = 7;
constexpr uint8 g_numberMedium = 9;
constexpr uint8 g_numberHigh = 11;
constexpr uint8 g_numRaySamplePositionsLow = g_numberLow * g_numberLow;
constexpr uint8 g_numRaySamplePositionsMedium = g_numberMedium * g_numberMedium;
constexpr uint8 g_numRaySamplePositionsHigh = g_numberHigh * g_numberHigh;
constexpr uint8 g_numConcurrentRaysLow = 1;
constexpr uint8 g_numConcurrentRaysMedium = 2;
constexpr uint8 g_numConcurrentRaysHigh = 4;
constexpr uint8 g_numInitialSamplePositions = 5;

class CPropagationProcessor final
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
	static void UpdateOcclusionPlanes();

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
	bool  CanRunOcclusion();
	float CastInitialRay(Vec3 const& origin, Vec3 const& target, bool const accumulate);

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

	struct SRayDebugInfo final
	{
		SRayDebugInfo() = default;

		Vec3  begin = ZERO;
		Vec3  end = ZERO;
		float occlusionValue = 0.0f;
		float distanceToNearestObstacle = FLT_MAX;
		uint8 numHits = 0;
		uint8 samplePosIndex = 0;
	};

	SRayDebugInfo m_rayDebugInfos[g_numConcurrentRaysHigh];
	ColorB        m_listenerOcclusionPlaneColor;
	Vec3          m_collisionSpherePositions[g_numRaySamplePositionsHigh] = { ZERO };
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};
}      // namespace CryAudio
#endif // CRY_AUDIO_USE_OCCLUSION