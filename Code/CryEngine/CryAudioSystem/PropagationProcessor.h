// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <CryPhysics/IPhysics.h>
#include <CryAudio/IAudioSystem.h>

static const size_t s_maxRayHits = 5;

class CAudioRayInfo
{
public:

	CAudioRayInfo(AudioObjectId const _audioObjectId)
		: audioObjectId(_audioObjectId)
		, samplePosIndex(0)
		, numHits(0)
		, totalSoundOcclusion(0.0f)
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		, startPosition(ZERO)
		, direction(ZERO)
		, distanceToFirstObstacle(FLT_MAX)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	{}

	~CAudioRayInfo() {}

	void Reset();

	AudioObjectId const audioObjectId;
	size_t              samplePosIndex;
	size_t              numHits;
	float               totalSoundOcclusion;
	ray_hit             hits[s_maxRayHits];

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	Vec3  startPosition;
	Vec3  direction;
	float distanceToFirstObstacle;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};

class CPropagationProcessor
{
public:

	static bool s_bCanIssueRWIs;

	typedef std::vector<CAudioRayInfo, STLSoundAllocator<CAudioRayInfo>> RayInfoVec;
	typedef std::vector<float, STLSoundAllocator<float>>                 RayOcclusionVec;

	CPropagationProcessor(AudioObjectId const audioObjectId, CAudioObjectTransformation const& transformation);
	~CPropagationProcessor();

	// PhysicsSystem callback
	static int OnObstructionTest(EventPhys const* pEvent);

	void       Update(float const deltaTime, float const distance, Vec3 const& audioListenerPosition);
	void       SetOcclusionType(EAudioOcclusionType const occlusionType, Vec3 const& audioListenerPosition);
	bool       CanRunObstructionOcclusion() const { return s_bCanIssueRWIs && m_occlusionType != eAudioOcclusionType_None && m_occlusionType != eAudioOcclusionType_Ignore; }
	void       GetPropagationData(SATLSoundPropagationData& propagationData) const;
	void       ProcessPhysicsRay(CAudioRayInfo* const pAudioRayInfo);
	void       ReleasePendingRays();
	bool       HasPendingRays() const { return m_remainingRays > 0; }
	bool       HasNewOcclusionValues();

private:

	void ProcessObstructionOcclusion();
	void CastObstructionRay(
	  Vec3 const& origin,
	  size_t const rayIndex,
	  size_t const samplePosIndex,
	  bool const bSynch);
	void RunObstructionQuery(Vec3 const& audioListenerPosition);
	void ProcessLow(
	  Vec3 const& audioListenerPosition,
	  Vec3 const& up,
	  Vec3 const& side,
	  bool const bSynch);
	void ProcessMedium(
	  Vec3 const& audioListenerPosition,
	  Vec3 const& up,
	  Vec3 const& side,
	  bool const bSynch);
	void ProcessHigh(
	  Vec3 const& audioListenerPosition,
	  Vec3 const& up,
	  Vec3 const& side,
	  bool const bSynch);
	size_t GetNumConcurrentRays() const;
	size_t GetNumSamplePositions() const;

	float                             m_obstruction;
	float                             m_lastQuerriedObstruction;
	float                             m_lastQuerriedOcclusion;
	float                             m_occlusion;
	float                             m_currentListenerDistance;
	RayOcclusionVec                   m_raysOcclusion;

	size_t                            m_remainingRays;
	size_t                            m_rayIndex;

	CAudioObjectTransformation const& m_transformation;

	RayInfoVec                        m_raysInfo;
	EAudioOcclusionType               m_occlusionType;
	EAudioOcclusionType               m_originalOcclusionType;
	EAudioOcclusionType               m_occlusionTypeWhenAdaptive;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	static size_t s_totalSyncPhysRays;
	static size_t s_totalAsyncPhysRays;

	void                DrawObstructionRays(IRenderAuxGeom& auxGeom) const;
	void                DrawRay(IRenderAuxGeom& auxGeom, size_t const rayIndex) const;
	EAudioOcclusionType GetOcclusionType() const             { return m_occlusionType; }
	EAudioOcclusionType GetOcclusionTypeWhenAdaptive() const { return m_occlusionTypeWhenAdaptive; }
	void                ResetRayData();

private:

	struct SRayDebugInfo
	{
		SRayDebugInfo()
			: begin(ZERO)
			, end(ZERO)
			, stableEnd(ZERO)
			, occlusionValue(0.0f)
			, distanceToNearestObstacle(FLT_MAX)
			, numHits(0)
		{}

		~SRayDebugInfo() {}

		Vec3  begin;
		Vec3  end;
		Vec3  stableEnd;
		float occlusionValue;
		float distanceToNearestObstacle;
		int   numHits;
	};

	typedef std::vector<SRayDebugInfo, STLSoundAllocator<SRayDebugInfo>> RayDebugInfoVec;

	RayDebugInfoVec m_rayDebugInfos;
	mutable float   m_timeSinceLastUpdateMS;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
