// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#if defined(CRY_AUDIO_USE_OCCLUSION)
	#include "PropagationProcessor.h"
	#include "Common.h"
	#include "Managers.h"
	#include "ListenerManager.h"
	#include "CVars.h"
	#include "Object.h"
	#include "System.h"
	#include "ObjectRequestData.h"
	#include <Cry3DEngine/I3DEngine.h>
	#include <Cry3DEngine/ISurfaceType.h>

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		#include "Debug.h"
		#include <CryRenderer/IRenderAuxGeom.h>
		#include <CryMath/Random.h>
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
constexpr uint32 g_numIndices = 6;
constexpr vtx_idx g_auxIndices[g_numIndices] = { 2, 1, 0, 2, 3, 1 };
constexpr uint32 g_numPoints = 4;
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

float g_listenerHeadSize = 0.0f;
float g_listenerHeadSizeHalf = 0.0f;

enum class EOcclusionCollisionType : EnumFlagsType
{
	None    = 0,
	Static  = BIT(6), // a,
	Rigid   = BIT(7), // b,
	Water   = BIT(8), // c,
	Terrain = BIT(9), // d,
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EOcclusionCollisionType);

struct SRayOffset final
{
	SRayOffset() = default;
	SRayOffset(SRayOffset const&) = delete;
	SRayOffset(SRayOffset&&) = delete;
	SRayOffset& operator=(SRayOffset const&) = delete;
	SRayOffset& operator=(SRayOffset&&) = delete;

	float x = 0.0f;
	float z = 0.0f;
};

SRayOffset g_raySamplePositionsLow[g_numRaySamplePositionsLow];
SRayOffset g_raySamplePositionsMedium[g_numRaySamplePositionsMedium];
SRayOffset g_raySamplePositionsHigh[g_numRaySamplePositionsHigh];

SRayOffset g_initialRaySamplePositionsLow[g_numInitialSamplePositions];
SRayOffset g_initialRaySamplePositionsMedium[g_numInitialSamplePositions];
SRayOffset g_initialRaySamplePositionsHigh[g_numInitialSamplePositions];

uint8 g_initialRaySamplePositionTopLeft = 0;
uint8 g_initialRaySamplePositionTopRight = 0;
uint8 g_initialRaySamplePositionCenter = 0;
uint8 g_initialRaySamplePositionBottomLeft = 0;
uint8 g_initialRaySamplePositionBottomRight = 0;

int CPropagationProcessor::s_occlusionRayFlags = 0;

///////////////////////////////////////////////////////////////////////////
void CRayInfo::Reset()
{
	totalSoundOcclusion = 0.0f;
	numHits = 0;
	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	startPosition.zero();
	direction.zero();
	distanceToFirstObstacle = FLT_MAX;
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

bool CPropagationProcessor::s_bCanIssueRWIs = false;

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
uint16 CPropagationProcessor::s_totalSyncPhysRays = 0;
uint16 CPropagationProcessor::s_totalAsyncPhysRays = 0;
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

///////////////////////////////////////////////////////////////////////////
int CPropagationProcessor::OnObstructionTest(EventPhys const* pEvent)
{
	auto const pRWIResult = static_cast<EventPhysRWIResult const*>(pEvent);

	if (pRWIResult->iForeignData == PHYS_FOREIGN_ID_SOUND_OBSTRUCTION)
	{
		auto& rayInfo = *(static_cast<CRayInfo*>(pRWIResult->pForeignData));
		rayInfo.numHits = static_cast<uint8>(pRWIResult->nHits);

		// The very first entry in "pHits" is reserved for a solid hit.
		// As we're using "rwi_max_piercing" when issuing a ray cast we're guaranteed to never have a solid hit.
		// This means we're safe to always ignore the very first entry in returned in "ray_hit".
		CRY_ASSERT_MESSAGE(pRWIResult->pHits[0].dist < 0.0f, "<Audio> encountered a solid hit in %s", __FUNCTION__);

		// Skip the "solid hit entry".
		ray_hit const* const pHits = &(pRWIResult->pHits[1]);

		for (uint8 i = 0; i < rayInfo.numHits; ++i)
		{
			rayInfo.hits[i].distance = pHits[i].dist;
			rayInfo.hits[i].surfaceIndex = pHits[i].surface_idx;
		}

		SObjectRequestData<EObjectRequestType::ProcessPhysicsRay> const requestData(rayInfo.pObject, rayInfo);
		CRequest const request(&requestData);
		g_system.PushRequest(request);
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "<Audio> iForeignData must be PHYS_FOREIGN_ID_SOUND_OBSTRUCTION during %s", __FUNCTION__);
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////////
CPropagationProcessor::CPropagationProcessor(CObject& object)
	: m_lastQuerriedOcclusion(0.0f)
	, m_occlusion(0.0f)
	, m_remainingRays(0)
	, m_rayIndex(0)
	, m_object(object)
	, m_currentListenerDistance(0.0f)
	, m_occlusionRayOffset(0.1f)
	, m_occlusionType(EOcclusionType::None)
	, m_originalOcclusionType(EOcclusionType::None)
	, m_occlusionTypeWhenAdaptive(EOcclusionType::Low) //will be updated in the first Update
{
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::Init()
{
	for (auto& rayInfo : m_raysInfo)
	{
		rayInfo.pObject = &m_object;
	}

	m_currentListenerDistance = g_listenerManager.GetActiveListenerTransformation().GetPosition().GetDistance(m_object.GetTransformation().GetPosition());

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	m_listenerOcclusionPlaneColor.set(cry_random<uint8>(0, 255), cry_random<uint8>(0, 255), cry_random<uint8>(0, 255), uint8(64));
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::UpdateOcclusionRayFlags()
{
	if ((g_cvars.m_occlusionCollisionTypes & EOcclusionCollisionType::Static) != 0)
	{
		s_occlusionRayFlags |= ent_static;
	}
	else
	{
		s_occlusionRayFlags &= ~ent_static;
	}

	if ((g_cvars.m_occlusionCollisionTypes & EOcclusionCollisionType::Rigid) != 0)
	{
		s_occlusionRayFlags |= (ent_sleeping_rigid | ent_rigid);
	}
	else
	{
		s_occlusionRayFlags &= ~(ent_sleeping_rigid | ent_rigid);
	}

	if ((g_cvars.m_occlusionCollisionTypes & EOcclusionCollisionType::Water) != 0)
	{
		s_occlusionRayFlags |= ent_water;
	}
	else
	{
		s_occlusionRayFlags &= ~ent_water;
	}

	if ((g_cvars.m_occlusionCollisionTypes & EOcclusionCollisionType::Terrain) != 0)
	{
		s_occlusionRayFlags |= ent_terrain;
	}
	else
	{
		s_occlusionRayFlags &= ~ent_terrain;
	}
}

///////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::Update()
{
	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	if (g_cvars.m_occlusionGlobalType > 0)
	{
		m_occlusionType = clamp_tpl<EOcclusionType>(static_cast<EOcclusionType>(g_cvars.m_occlusionGlobalType), EOcclusionType::Ignore, EOcclusionType::High);
	}
	else
	{
		m_occlusionType = m_originalOcclusionType;
	}
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	if (CanRunOcclusion())
	{
		if (m_currentListenerDistance < g_cvars.m_occlusionHighDistance)
		{
			m_occlusionTypeWhenAdaptive = EOcclusionType::High;
		}
		else if (m_currentListenerDistance < g_cvars.m_occlusionMediumDistance)
		{
			m_occlusionTypeWhenAdaptive = EOcclusionType::Medium;
		}
		else
		{
			m_occlusionTypeWhenAdaptive = EOcclusionType::Low;
		}

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		UpdateOcclusionPlanes();
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

		RunObstructionQuery();
	}
	else
	{
		m_occlusion = 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::SetOcclusionType(EOcclusionType const occlusionType)
{
	m_occlusionType = occlusionType;
	m_originalOcclusionType = occlusionType;
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::UpdateOcclusion()
{
	if (CanRunOcclusion())
	{
		if (g_cvars.m_occlusionInitialRayCastMode == 1)
		{
			Vec3 const& listenerPosition = g_listenerManager.GetActiveListenerTransformation().GetPosition();
			Vec3 const& objectPosition = m_object.GetTransformation().GetPosition();

			m_occlusion = CastInitialRay(listenerPosition, objectPosition, (g_cvars.m_occlusionAccumulate > 0));

			for (auto& rayOcclusion : m_raysOcclusion)
			{
				rayOcclusion = m_occlusion;
			}
		}
		else if (g_cvars.m_occlusionInitialRayCastMode > 1)
		{
			float occlusionValues[g_numInitialSamplePositions];
			Vec3 const& listenerPosition = g_listenerManager.GetActiveListenerTransformation().GetPosition();
			Vec3 const& objectPosition = m_object.GetTransformation().GetPosition();

			// TODO: this breaks if listener and object x and y coordinates are exactly the same.
			Vec3 const side((listenerPosition - objectPosition).Cross(Vec3Constants<float>::fVec3_OneZ).normalize());
			Vec3 const up((listenerPosition - objectPosition).Cross(side).normalize());
			bool const accumulate = g_cvars.m_occlusionAccumulate > 0;

			for (uint8 i = 0; i < g_numInitialSamplePositions; ++i)
			{
				switch (m_occlusionType)
				{
				case EOcclusionType::Adaptive:
					{
						switch (m_occlusionTypeWhenAdaptive)
						{
						case EOcclusionType::Low:
							{
								SRayOffset const& rayOffset = g_initialRaySamplePositionsLow[i];
								Vec3 const origin(listenerPosition + up* rayOffset.z + side* rayOffset.x);
								occlusionValues[i] = CastInitialRay(origin, objectPosition, accumulate);

								break;
							}
						case EOcclusionType::Medium:
							{
								SRayOffset const& rayOffset = g_initialRaySamplePositionsMedium[i];
								Vec3 const origin(listenerPosition + up* rayOffset.z + side* rayOffset.x);
								occlusionValues[i] = CastInitialRay(origin, objectPosition, accumulate);

								break;
							}
						case EOcclusionType::High:
							{
								SRayOffset const& rayOffset = g_initialRaySamplePositionsHigh[i];
								Vec3 const origin(listenerPosition + up* rayOffset.z + side* rayOffset.x);
								occlusionValues[i] = CastInitialRay(origin, objectPosition, accumulate);

								break;
							}
						default:
							{
								break;
							}
						}

						break;
					}
				case EOcclusionType::Low:
					{
						SRayOffset const& rayOffset = g_initialRaySamplePositionsLow[i];
						Vec3 const origin(listenerPosition + up* rayOffset.z + side* rayOffset.x);
						occlusionValues[i] = CastInitialRay(origin, objectPosition, accumulate);

						break;
					}
				case EOcclusionType::Medium:
					{
						SRayOffset const& rayOffset = g_initialRaySamplePositionsMedium[i];
						Vec3 const origin(listenerPosition + up* rayOffset.z + side* rayOffset.x);
						occlusionValues[i] = CastInitialRay(origin, objectPosition, accumulate);

						break;
					}
				case EOcclusionType::High:
					{
						SRayOffset const& rayOffset = g_initialRaySamplePositionsHigh[i];
						Vec3 const origin(listenerPosition + up* rayOffset.z + side* rayOffset.x);
						occlusionValues[i] = CastInitialRay(origin, objectPosition, accumulate);

						break;
					}
				default:
					{
						break;
					}
				}
			}

			m_occlusion = 0.0f;

			for (auto const value : occlusionValues)
			{
				m_occlusion += value;
			}

			m_occlusion /= static_cast<float>(g_numInitialSamplePositions);

			for (auto& rayOcclusion : m_raysOcclusion)
			{
				rayOcclusion = m_occlusion;
			}
		}
		else
		{
			m_occlusion = 0.0f;
			m_lastQuerriedOcclusion = 0.0f;
		}
	}
	else
	{
		m_occlusion = 0.0f;
		m_lastQuerriedOcclusion = 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////
float CPropagationProcessor::CastInitialRay(Vec3 const& origin, Vec3 const& target, bool const accumulate)
{
	Vec3 const direction(target - origin);
	Vec3 directionNormalized(direction);
	directionNormalized.Normalize();
	Vec3 const finalDirection(direction - (directionNormalized* m_occlusionRayOffset));
	ray_hit hits[g_maxRayHits];

	// We use "rwi_max_piercing" to allow audio rays to always pierce surfaces regardless of the "pierceability" attribute.
	// Note: The very first entry of rayInfo.hits (solid slot) is always empty.
	int const numHits = gEnv->pPhysicalWorld->RayWorldIntersection(
		origin,
		finalDirection,
		s_occlusionRayFlags,
		rwi_max_piercing,
		hits,
		static_cast<int>(g_maxRayHits),
		nullptr,
		nullptr,
		nullptr,
		PHYS_FOREIGN_ID_SOUND_OBSTRUCTION);

	CRayInfo rayInfo;
	rayInfo.numHits = static_cast<uint8>(numHits);

	// The very first entry in "hits" is reserved for a solid hit.
	// As we're using "rwi_max_piercing" when issuing a ray cast we're guaranteed to never have a solid hit.
	// This means we're safe to always ignore the very first entry in returned in "ray_hit".
	CRY_ASSERT_MESSAGE(hits[0].dist < 0.0f, "<Audio> encountered a solid hit in %s", __FUNCTION__);

	// Skip the "solid hit entry".
	ray_hit const* const pHits = &(hits[1]);

	for (uint8 i = 0; i < rayInfo.numHits; ++i)
	{
		rayInfo.hits[i].distance = pHits[i].dist;
		rayInfo.hits[i].surfaceIndex = pHits[i].surface_idx;
	}

	float finalOcclusion = 0.0f;

	if ((g_cvars.m_occlusionSetFullOnMaxHits > 0) && (rayInfo.numHits == g_maxRayHits))
	{
		finalOcclusion = 1.0f;
	}
	else if (rayInfo.numHits > 0)
	{
		ISurfaceTypeManager* const pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();

		for (uint8 i = 0; i < rayInfo.numHits; ++i)
		{
			float const distance = rayInfo.hits[i].distance;

			if (distance > 0.0f)
			{
				ISurfaceType* const pMat = pSurfaceTypeManager->GetSurfaceType(static_cast<int>(rayInfo.hits[i].surfaceIndex));

				if (pMat != nullptr)
				{
					ISurfaceType::SPhysicalParams const& physParams = pMat->GetPhyscalParams();

					if (accumulate)
					{
						finalOcclusion += physParams.sound_obstruction; // Not clamping b/w 0 and 1 for performance reasons.
					}
					else
					{
						finalOcclusion = std::max(finalOcclusion, physParams.sound_obstruction);
					}

					if (finalOcclusion >= 1.0f)
					{
						break;
					}
				}
			}
		}
	}

	return clamp_tpl(finalOcclusion, 0.0f, 1.0f);
}

//////////////////////////////////////////////////////////////////////////
bool CPropagationProcessor::CanRunOcclusion()
{
	bool canRun = false;

	if ((m_occlusionType != EOcclusionType::None) &&
	    (m_occlusionType != EOcclusionType::Ignore) &&
	    ((m_object.GetFlags() & EObjectFlags::Virtual) == 0) &&
	    s_bCanIssueRWIs)
	{
		m_currentListenerDistance = m_object.GetTransformation().GetPosition().GetDistance(g_listenerManager.GetActiveListenerTransformation().GetPosition());

		if ((m_currentListenerDistance > g_cvars.m_occlusionMinDistance) && (m_currentListenerDistance < g_cvars.m_occlusionMaxDistance))
		{
			canRun = true;
		}
	}

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	canRun ? m_object.SetFlag(EObjectFlags::CanRunOcclusion) : m_object.RemoveFlag(EObjectFlags::CanRunOcclusion);
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	return canRun;
}

///////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::ProcessPhysicsRay(CRayInfo& rayInfo)
{
	CRY_ASSERT((rayInfo.samplePosIndex >= 0) && (rayInfo.samplePosIndex < g_numRaySamplePositionsHigh));

	float finalOcclusion = 0.0f;
	uint8 numRealHits = 0;

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	float minDistance = FLT_MAX;
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	if ((g_cvars.m_occlusionSetFullOnMaxHits > 0) && (rayInfo.numHits == g_maxRayHits))
	{
		finalOcclusion = 1.0f;
		numRealHits = g_maxRayHits;
	}
	else if (rayInfo.numHits > 0)
	{
		ISurfaceTypeManager* const pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
		bool const accumulate = g_cvars.m_occlusionAccumulate > 0;

		for (uint8 i = 0; i < rayInfo.numHits; ++i)
		{
			float const distance = rayInfo.hits[i].distance;

			if (distance > 0.0f)
			{
				ISurfaceType* const pMat = pSurfaceTypeManager->GetSurfaceType(static_cast<int>(rayInfo.hits[i].surfaceIndex));

				if (pMat != nullptr)
				{
					ISurfaceType::SPhysicalParams const& physParams = pMat->GetPhyscalParams();

					if (accumulate)
					{
						finalOcclusion += physParams.sound_obstruction; // Not clamping b/w 0 and 1 for performance reasons.
					}
					else
					{
						finalOcclusion = std::max(finalOcclusion, physParams.sound_obstruction);
					}

					++numRealHits;

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
					minDistance = std::min(minDistance, distance);
	#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

					if (finalOcclusion >= 1.0f)
					{
						break;
					}
				}
			}
		}
	}

	rayInfo.numHits = numRealHits;
	rayInfo.totalSoundOcclusion = clamp_tpl(finalOcclusion, 0.0f, 1.0f);

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	rayInfo.distanceToFirstObstacle = minDistance;

	if (m_remainingRays == 0)
	{
		CryFatalError("Negative ref or ray count on audio object");
	}
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	if (--m_remainingRays == 0)
	{
		ProcessObstructionOcclusion();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::ReleasePendingRays()
{
	m_remainingRays = 0;
}

//////////////////////////////////////////////////////////////////////////
bool CPropagationProcessor::HasNewOcclusionValues()
{
	bool hasNewValues = false;

	if (fabs_tpl(m_lastQuerriedOcclusion - m_occlusion) > FloatEpsilon)
	{
		m_lastQuerriedOcclusion = m_occlusion;
		hasNewValues = true;
	}

	return hasNewValues;
}

///////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::ProcessObstructionOcclusion()
{
	m_occlusion = 0.0f;
	CRY_ASSERT_MESSAGE(m_currentListenerDistance > 0.0f, "Distance to Listener is 0 during %s", __FUNCTION__);

	uint8 const numSamplePositions = GetNumSamplePositions();
	uint8 const numConcurrentRays = GetNumConcurrentRays();

	if (numSamplePositions > 0 && numConcurrentRays > 0)
	{
		for (uint8 i = 0; i < numConcurrentRays; ++i)
		{
			CRayInfo const& rayInfo = m_raysInfo[i];
			m_raysOcclusion[rayInfo.samplePosIndex] = rayInfo.totalSoundOcclusion;
		}

		// Calculate the new occlusion average.
		for (uint8 i = 0; i < numSamplePositions; ++i)
		{
			m_occlusion += m_raysOcclusion[i];
		}

		m_occlusion = (m_occlusion / numSamplePositions);
	}

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	if ((m_object.GetFlags() & EObjectFlags::CanRunOcclusion) != 0) // only re-sample the rays about 10 times per second for a smoother debug drawing
	{
		for (uint8 i = 0; i < g_numConcurrentRaysHigh; ++i)
		{
			CRayInfo const& rayInfo = m_raysInfo[i];
			SRayDebugInfo& rayDebugInfo = m_rayDebugInfos[i];

			rayDebugInfo.begin = rayInfo.startPosition;
			rayDebugInfo.end = rayInfo.startPosition + rayInfo.direction;

			if (rayDebugInfo.stableEnd.IsZeroFast())
			{
				// to be moved to the PropagationProcessor Reset method
				rayDebugInfo.stableEnd = rayDebugInfo.end;
			}
			else
			{
				rayDebugInfo.stableEnd += (rayDebugInfo.end - rayDebugInfo.stableEnd) * 0.1f;
			}

			rayDebugInfo.distanceToNearestObstacle = rayInfo.distanceToFirstObstacle;
			rayDebugInfo.numHits = rayInfo.numHits;
			rayDebugInfo.occlusionValue = rayInfo.totalSoundOcclusion;
		}
	}
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::CastObstructionRay(
	Vec3 const& origin,
	uint8 const rayIndex,
	uint8 const samplePosIndex,
	bool const bSynch)
{
	Vec3 const direction(m_object.GetTransformation().GetPosition() - origin);
	Vec3 directionNormalized(direction);
	directionNormalized.Normalize();
	Vec3 const finalDirection(direction - (directionNormalized* m_occlusionRayOffset));

	// We use "rwi_max_piercing" to allow audio rays to always pierce surfaces regardless of the "pierceability" attribute.
	// Note: The very first entry of rayInfo.hits (solid slot) is always empty.
	CRayInfo& rayInfo = m_raysInfo[rayIndex];
	rayInfo.samplePosIndex = samplePosIndex;
	++m_remainingRays;

	if (bSynch)
	{
		ray_hit hits[g_maxRayHits];

		int const numHits = gEnv->pPhysicalWorld->RayWorldIntersection(
			origin,
			finalDirection,
			s_occlusionRayFlags,
			rwi_max_piercing,
			hits,
			static_cast<int>(g_maxRayHits),
			nullptr,
			nullptr,
			nullptr,
			PHYS_FOREIGN_ID_SOUND_OBSTRUCTION);

		rayInfo.numHits = static_cast<uint8>(numHits);

		// The very first entry in "hits" is reserved for a solid hit.
		// As we're using "rwi_max_piercing" when issuing a ray cast we're guaranteed to never have a solid hit.
		// This means we're safe to always ignore the very first entry in returned in "ray_hit".
		CRY_ASSERT_MESSAGE(hits[0].dist < 0.0f, "<Audio> encountered a solid hit in %s", __FUNCTION__);

		// Skip the "solid hit entry".
		ray_hit const* const pHits = &(hits[1]);

		for (uint8 i = 0; i < rayInfo.numHits; ++i)
		{
			rayInfo.hits[i].distance = pHits[i].dist;
			rayInfo.hits[i].surfaceIndex = pHits[i].surface_idx;
		}

		ProcessPhysicsRay(rayInfo);
	}
	else
	{
		gEnv->pPhysicalWorld->RayWorldIntersection(
			origin,
			finalDirection,
			s_occlusionRayFlags,
			rwi_max_piercing | rwi_queue,
			nullptr,
			static_cast<int>(g_maxRayHits),
			nullptr,
			nullptr,
			&rayInfo,
			PHYS_FOREIGN_ID_SOUND_OBSTRUCTION);
	}

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	rayInfo.startPosition = origin;
	rayInfo.direction = finalDirection;

	if (bSynch)
	{
		++s_totalSyncPhysRays;
	}
	else
	{
		++s_totalAsyncPhysRays;
	}
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::RunObstructionQuery()
{
	if (m_remainingRays == 0)
	{
		// Make the physics ray cast call synchronous or asynchronous depending on the distance to the listener.
		bool const bSynch = (m_currentListenerDistance <= g_cvars.m_occlusionMaxSyncDistance);

		Vec3 const& listenerPosition = g_listenerManager.GetActiveListenerTransformation().GetPosition();

		// TODO: this breaks if listener and object x and y coordinates are exactly the same.
		Vec3 const side((listenerPosition - m_object.GetTransformation().GetPosition()).Cross(Vec3Constants<float>::fVec3_OneZ).normalize());
		Vec3 const up((listenerPosition - m_object.GetTransformation().GetPosition()).Cross(side).normalize());

		switch (m_occlusionType)
		{
		case EOcclusionType::Adaptive:
			{
				switch (m_occlusionTypeWhenAdaptive)
				{
				case EOcclusionType::Low:
					ProcessLow(up, side, bSynch);
					break;
				case EOcclusionType::Medium:
					ProcessMedium(up, side, bSynch);
					break;
				case EOcclusionType::High:
					ProcessHigh(up, side, bSynch);
					break;
				default:
					CRY_ASSERT_MESSAGE(false, "Calculated Adaptive Occlusion Type invalid during %s", __FUNCTION__);
					break;
				}
			}
			break;
		case EOcclusionType::Low:
			ProcessLow(up, side, bSynch);
			break;
		case EOcclusionType::Medium:
			ProcessMedium(up, side, bSynch);
			break;
		case EOcclusionType::High:
			ProcessHigh(up, side, bSynch);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::ProcessLow(
	Vec3 const& up,
	Vec3 const& side,
	bool const bSynch)
{
	for (uint8 i = 0; i < g_numConcurrentRaysLow; ++i)
	{
		if (m_rayIndex >= g_numRaySamplePositionsLow)
		{
			m_rayIndex = 0;
		}

		Vec3 const origin(g_listenerManager.GetActiveListenerTransformation().GetPosition() + up* g_raySamplePositionsLow[m_rayIndex].z + side* g_raySamplePositionsLow[m_rayIndex].x);
		CastObstructionRay(origin, i, m_rayIndex, bSynch);
		++m_rayIndex;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::ProcessMedium(
	Vec3 const& up,
	Vec3 const& side,
	bool const bSynch)
{
	for (uint8 i = 0; i < g_numConcurrentRaysMedium; ++i)
	{
		if (m_rayIndex >= g_numRaySamplePositionsMedium)
		{
			m_rayIndex = 0;
		}

		Vec3 const origin(g_listenerManager.GetActiveListenerTransformation().GetPosition() + up* g_raySamplePositionsMedium[m_rayIndex].z + side* g_raySamplePositionsMedium[m_rayIndex].x);
		CastObstructionRay(origin, i, m_rayIndex, bSynch);
		++m_rayIndex;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::ProcessHigh(
	Vec3 const& up,
	Vec3 const& side,
	bool const bSynch)
{
	for (uint8 i = 0; i < g_numConcurrentRaysHigh; ++i)
	{
		if (m_rayIndex >= g_numRaySamplePositionsHigh)
		{
			m_rayIndex = 0;
		}

		Vec3 const origin(g_listenerManager.GetActiveListenerTransformation().GetPosition() + up* g_raySamplePositionsHigh[m_rayIndex].z + side* g_raySamplePositionsHigh[m_rayIndex].x);
		CastObstructionRay(origin, i, m_rayIndex, bSynch);
		++m_rayIndex;
	}
}

//////////////////////////////////////////////////////////////////////////
uint8 CPropagationProcessor::GetNumConcurrentRays() const
{
	uint8 numConcurrentRays = 0;

	switch (m_occlusionType)
	{
	case EOcclusionType::Adaptive:
		{
			switch (m_occlusionTypeWhenAdaptive)
			{
			case EOcclusionType::Low:
				numConcurrentRays = g_numConcurrentRaysLow;
				break;
			case EOcclusionType::Medium:
				numConcurrentRays = g_numConcurrentRaysMedium;
				break;
			case EOcclusionType::High:
				numConcurrentRays = g_numConcurrentRaysHigh;
				break;
			default:
				CRY_ASSERT_MESSAGE(false, "Calculated Adaptive Occlusion Type invalid during %s", __FUNCTION__);
				break;
			}
		}
		break;
	case EOcclusionType::Low:
		numConcurrentRays = g_numConcurrentRaysLow;
		break;
	case EOcclusionType::Medium:
		numConcurrentRays = g_numConcurrentRaysMedium;
		break;
	case EOcclusionType::High:
		numConcurrentRays = g_numConcurrentRaysHigh;
		break;
	}

	return numConcurrentRays;
}

//////////////////////////////////////////////////////////////////////////
uint8 CPropagationProcessor::GetNumSamplePositions() const
{
	uint8 numSamplePositions = 0;

	switch (m_occlusionType)
	{
	case EOcclusionType::Adaptive:
		{
			switch (m_occlusionTypeWhenAdaptive)
			{
			case EOcclusionType::Low:
				numSamplePositions = g_numRaySamplePositionsLow;
				break;
			case EOcclusionType::Medium:
				numSamplePositions = g_numRaySamplePositionsMedium;
				break;
			case EOcclusionType::High:
				numSamplePositions = g_numRaySamplePositionsHigh;
				break;
			default:
				CRY_ASSERT_MESSAGE(false, "Calculated Adaptive Occlusion Type invalid during %s", __FUNCTION__);
				break;
			}
		}
		break;
	case EOcclusionType::Low:
		numSamplePositions = g_numRaySamplePositionsLow;
		break;
	case EOcclusionType::Medium:
		numSamplePositions = g_numRaySamplePositionsMedium;
		break;
	case EOcclusionType::High:
		numSamplePositions = g_numRaySamplePositionsHigh;
		break;
	}

	return numSamplePositions;
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::UpdateOcclusionPlanes()
{
	g_listenerHeadSize = g_cvars.m_occlusionListenerPlaneSize;
	g_listenerHeadSizeHalf = g_listenerHeadSize * 0.5f;
	uint8 index = 0;

	// Low
	float step = g_listenerHeadSize / (g_numberLow - 1);

	for (uint8 i = 0; i < g_numberLow; ++i)
	{
		float const z = g_listenerHeadSizeHalf - i * step;

		for (uint8 j = 0; j < g_numberLow; ++j)
		{
			float const x = -g_listenerHeadSizeHalf + j * step;
			g_raySamplePositionsLow[index].x = x;
			g_raySamplePositionsLow[index].z = z;

			// Initial sample positions:
			// . . . . . . .
			// . 8 . . . 12.
			// . . . . . . .
			// . . . 24. . .
			// . . . . . . .
			// . 36. . . 40.
			// . . . . . . .

			switch (index)
			{
			case 8:
				{
					g_initialRaySamplePositionTopLeft = index;
					g_initialRaySamplePositionsLow[0].x = x;
					g_initialRaySamplePositionsLow[0].z = z;

					break;
				}
			case 12:
				{
					g_initialRaySamplePositionTopRight = index;
					g_initialRaySamplePositionsLow[1].x = x;
					g_initialRaySamplePositionsLow[1].z = z;

					break;
				}
			case 24:
				{
					g_initialRaySamplePositionCenter = index;
					g_initialRaySamplePositionsLow[2].x = x;
					g_initialRaySamplePositionsLow[2].z = z;

					break;
				}
			case 36:
				{
					g_initialRaySamplePositionBottomLeft = index;
					g_initialRaySamplePositionsLow[3].x = x;
					g_initialRaySamplePositionsLow[3].z = z;

					break;
				}
			case 40:
				{
					g_initialRaySamplePositionBottomRight = index;
					g_initialRaySamplePositionsLow[4].x = x;
					g_initialRaySamplePositionsLow[4].z = z;

					break;
				}
			default:
				{
					break;
				}
			}

			++index;
		}
	}

	index = 0;

	// Medium
	step = g_listenerHeadSize / (g_numberMedium - 1);

	for (uint8 i = 0; i < g_numberMedium; ++i)
	{
		float const z = g_listenerHeadSizeHalf - i * step;

		for (uint8 j = 0; j < g_numberMedium; ++j)
		{
			float const x = -g_listenerHeadSizeHalf + j * step;
			g_raySamplePositionsMedium[index].x = x;
			g_raySamplePositionsMedium[index].z = z;

			// Initial sample positions:
			// . . . . . . . . .
			// . 10. . . . . 16.
			// . . . . . . . . .
			// . . . . . . . . .
			// . . . . 40. . . .
			// . . . . . . . . .
			// . . . . . . . . .
			// . 64. . . . . 70.
			// . . . . . . . . .

			switch (index)
			{
			case 10:
				{
					g_initialRaySamplePositionTopLeft = index;
					g_initialRaySamplePositionsMedium[0].x = x;
					g_initialRaySamplePositionsMedium[0].z = z;

					break;
				}
			case 16:
				{
					g_initialRaySamplePositionTopRight = index;
					g_initialRaySamplePositionsMedium[1].x = x;
					g_initialRaySamplePositionsMedium[1].z = z;
					break;
				}
			case 40:
				{
					g_initialRaySamplePositionCenter = index;
					g_initialRaySamplePositionsMedium[2].x = x;
					g_initialRaySamplePositionsMedium[2].z = z;

					break;
				}
			case 64:
				{
					g_initialRaySamplePositionBottomLeft = index;
					g_initialRaySamplePositionsMedium[3].x = x;
					g_initialRaySamplePositionsMedium[3].z = z;

					break;
				}
			case 70:
				{
					g_initialRaySamplePositionBottomRight = index;
					g_initialRaySamplePositionsMedium[4].x = x;
					g_initialRaySamplePositionsMedium[4].z = z;

					break;
				}
			default:
				{
					break;
				}
			}

			++index;
		}
	}

	index = 0;

	// High
	step = g_listenerHeadSize / (g_numberHigh - 1);

	for (uint8 i = 0; i < g_numberHigh; ++i)
	{
		float const z = g_listenerHeadSizeHalf - i * step;

		for (uint8 j = 0; j < g_numberHigh; ++j)
		{
			float const x = -g_listenerHeadSizeHalf + j * step;
			g_raySamplePositionsHigh[index].x = x;
			g_raySamplePositionsHigh[index].z = z;

			// Initial sample positions:
			// . . . . . . . . . . .
			// . 12. . . . . . . 20.
			// . . . . . . . . . . .
			// . . . . . . . . . . .
			// . . . . . . . . . . .
			// . . . . . 60. . . . .
			// . . . . . . . . . . .
			// . . . . . . . . . . .
			// . . . . . . . . . . .
			// .100. . . . . . .108.
			// . . . . . . . . . . .

			switch (index)
			{
			case 12:
				{
					g_initialRaySamplePositionTopLeft = index;
					g_initialRaySamplePositionsHigh[0].x = x;
					g_initialRaySamplePositionsHigh[0].z = z;

					break;
				}
			case 20:
				{
					g_initialRaySamplePositionTopRight = index;
					g_initialRaySamplePositionsHigh[1].x = x;
					g_initialRaySamplePositionsHigh[1].z = z;

					break;
				}
			case 60:
				{
					g_initialRaySamplePositionCenter = index;
					g_initialRaySamplePositionsHigh[2].x = x;
					g_initialRaySamplePositionsHigh[2].z = z;

					break;
				}
			case 100:
				{
					g_initialRaySamplePositionBottomLeft = index;
					g_initialRaySamplePositionsHigh[3].x = x;
					g_initialRaySamplePositionsHigh[3].z = z;

					break;
				}
			case 108:
				{
					g_initialRaySamplePositionBottomRight = index;
					g_initialRaySamplePositionsHigh[4].x = x;
					g_initialRaySamplePositionsHigh[4].z = z;

					break;
				}
			default:
				{
					break;
				}
			}

			++index;
		}
	}
}

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::DrawDebugInfo(IRenderAuxGeom& auxGeom)
{
	if ((m_object.GetFlags() & EObjectFlags::CanRunOcclusion) != 0)
	{
		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::OcclusionRays) != 0)
		{
			uint8 const numConcurrentRays = GetNumConcurrentRays();

			for (uint8 i = 0; i < numConcurrentRays; ++i)
			{
				DrawRay(auxGeom, i);
			}
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ListenerOcclusionPlane) != 0)
		{
			SAuxGeomRenderFlags const previousRenderFlags = auxGeom.GetRenderFlags();
			SAuxGeomRenderFlags newRenderFlags;
			newRenderFlags.SetDepthTestFlag(e_DepthTestOff);
			newRenderFlags.SetAlphaBlendMode(e_AlphaBlended);
			newRenderFlags.SetCullMode(e_CullModeNone);
			auxGeom.SetRenderFlags(newRenderFlags);

			Vec3 const& listenerPosition = g_listenerManager.GetActiveListenerTransformation().GetPosition();

			// TODO: this breaks if listener and object x and y coordinates are exactly the same.
			Vec3 const side((listenerPosition - m_object.GetTransformation().GetPosition()).Cross(Vec3Constants<float>::fVec3_OneZ).normalize());
			Vec3 const up((listenerPosition - m_object.GetTransformation().GetPosition()).Cross(side).normalize());

			Vec3 const quadVertices[g_numPoints] =
			{
				Vec3(listenerPosition + up * g_listenerHeadSizeHalf + side * g_listenerHeadSizeHalf),
				Vec3(listenerPosition + up * -g_listenerHeadSizeHalf + side * g_listenerHeadSizeHalf),
				Vec3(listenerPosition + up * g_listenerHeadSizeHalf + side * -g_listenerHeadSizeHalf),
				Vec3(listenerPosition + up * -g_listenerHeadSizeHalf + side * -g_listenerHeadSizeHalf) };

			auxGeom.DrawTriangles(quadVertices, g_numPoints, g_auxIndices, g_numIndices, m_listenerOcclusionPlaneColor);
			auxGeom.SetRenderFlags(previousRenderFlags);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::DrawRay(IRenderAuxGeom& auxGeom, uint8 const rayIndex) const
{
	SAuxGeomRenderFlags const previousRenderFlags = auxGeom.GetRenderFlags();
	SAuxGeomRenderFlags newRenderFlags(e_Def3DPublicRenderflags);
	newRenderFlags.SetCullMode(e_CullModeNone);

	bool const isRayObstructed = (m_rayDebugInfos[rayIndex].numHits > 0);
	Vec3 const rayEnd = isRayObstructed ?
	                    m_rayDebugInfos[rayIndex].begin + (m_rayDebugInfos[rayIndex].end - m_rayDebugInfos[rayIndex].begin).GetNormalized() * m_rayDebugInfos[rayIndex].distanceToNearestObstacle :
	                    m_rayDebugInfos[rayIndex].end; // Only draw the ray to the first collision point.

	ColorF const& rayColor = isRayObstructed ? Debug::s_rayColorObstructed : Debug::s_rayColorFree;

	auxGeom.SetRenderFlags(newRenderFlags);

	if (isRayObstructed)
	{
		// Mark the nearest collision with a small sphere.
		auxGeom.DrawSphere(rayEnd, Debug::g_rayRadiusCollisionSphere, Debug::s_rayColorCollisionSphere);
	}

	auxGeom.DrawLine(m_rayDebugInfos[rayIndex].begin, rayColor, rayEnd, rayColor, 1.0f);
	auxGeom.SetRenderFlags(previousRenderFlags);
}

///////////////////////////////////////////////////////////////////////////
void CPropagationProcessor::ResetRayData()
{
	if (m_occlusionType != EOcclusionType::None && m_occlusionType != EOcclusionType::Ignore)
	{
		for (uint8 i = 0; i < g_numConcurrentRaysHigh; ++i)
		{
			m_raysInfo[i].Reset();
			m_rayDebugInfos[i] = SRayDebugInfo();
		}
	}
}
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}        // namespace CryAudio
#endif   // CRY_AUDIO_USE_OCCLUSION