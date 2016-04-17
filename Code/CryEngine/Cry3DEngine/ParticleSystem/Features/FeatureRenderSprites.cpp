// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleSystem/ParticleRender.h"
#include "ParticleSystem/ParticleEmitter.h"
#include <CryMath/RadixSort.h>

CRY_PFX2_DBG

volatile bool gFeatureRenderSprites = false;

namespace pfx2
{

SERIALIZATION_DECLARE_ENUM(ESortMode,
                           None,
                           BackToFront,
                           FrontToBack,
                           OldToNew,
                           NewToOld
                           )

SERIALIZATION_DECLARE_ENUM(EFacingMode,
                           Screen,
                           Camera,
                           Velocity,
                           Free
                           )

struct SSpritesContext
{
	SSpritesContext(CParticleComponentRuntime* pComponentRuntime, const SComponentParams& params, const SCameraInfo& camInfo, const SVisEnviron& visEnviron, const SPhysEnviron& physEnviron, CREParticle* pRE, TParticleHeap& memHeap, size_t numParticles)
		: m_context(pComponentRuntime)
		, m_params(params)
		, m_camInfo(camInfo)
		, m_visEnviron(visEnviron)
		, m_physEnviron(physEnviron)
		, m_pRE(pRE)
		, m_particleIds(memHeap, numParticles)
		, m_spriteAlphas(memHeap, numParticles)
		, m_numParticles(numParticles)
		, m_numSprites(0)
		, m_renderFlags(0)
		, m_rangeId(0)
	{
	}
	const SUpdateContext              m_context;
	const SComponentParams&           m_params;
	const SCameraInfo&                m_camInfo;
	const SVisEnviron&                m_visEnviron;
	const SPhysEnviron&               m_physEnviron;
	AABB                              m_bounds;
	CREParticle*                      m_pRE;
	TParticleHeap::Array<TParticleId> m_particleIds;
	TParticleHeap::Array<float>       m_spriteAlphas;
	size_t                            m_numParticles;
	size_t                            m_numSprites;
	uint64                            m_renderFlags;
	size_t                            m_rangeId;
};

class CFeatureRenderSprites : public CParticleRenderBase
{
private:
	typedef CParticleRenderBase                     BaseClass;
	typedef SParticleAxes (CFeatureRenderSprites::* AxesFn)(const SSpritesContext& context, TParticleId particleId);

public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureRenderSprites();

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override;
	virtual void ComputeVertices(CParticleComponentRuntime* pComponentRuntime, const SCameraInfo& camInfo, CREParticle* pRE, uint64 uRenderFlags, float fMaxPixels) override;
	virtual void Serialize(Serialization::IArchive& ar) override;

protected:
	bool SupportsWaterCulling() const override { return true; }

private:
	void CullParticles(SSpritesContext* pSpritesContext);
	void SortSprites(SSpritesContext* pSpritesContext);
	void WriteToGPUMem(const SSpritesContext& spritesContext);

	void WritePositions(const SSpritesContext& spritesContext, FixedDynArray<Vec3>& gpuPositions);
	template<const bool hasAngles2D>
	void WriteAxes(const SSpritesContext& spritesContext, FixedDynArray<SParticleAxes>& gpuAxes);
	template<typename TAxesSampler, const bool hasAngles2D>
	void WriteAxes(const SSpritesContext& spritesContext, FixedDynArray<SParticleAxes>& gpuAxes, const TAxesSampler& axesSampler);
	template<bool hasColors, bool hasTiling, bool hasAnimation>
	void WriteColorSTs(const SSpritesContext& spritesContext, FixedDynArray<SParticleColorST>& gpuColorSTs);

	ESortMode   m_sortMode;
	EFacingMode m_facingMode;
	UFloat10    m_aspectRatio;
	UFloat10    m_axisScale;
	UUnitFloat  m_sphericalProjection;
	SFloat      m_sortBias;
	bool        m_flipU, m_flipV;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureRenderSprites, "Render", "Sprites", "Editor/Icons/Particles/sprites.png", renderFeatureColor);

//////////////////////////////////////////////////////////////////////////

CFeatureRenderSprites::CFeatureRenderSprites()
	: m_sortMode(ESortMode::None)
	, m_facingMode(EFacingMode::Screen)
	, m_aspectRatio(1.0f)
	, m_axisScale(0.1f)
	, m_sphericalProjection(0.0f)
	, m_sortBias(0.0f)
	, m_flipU(false)
	, m_flipV(false)
{
}

void CFeatureRenderSprites::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	BaseClass::AddToComponent(pComponent, pParams);
	pParams->m_renderObjectFlags |= FOB_POINT_SPRITE;
	pParams->m_particleObjFlags |= CREParticle::ePOF_USE_VERTEX_PULL_MODEL;
	if (m_facingMode == EFacingMode::Velocity)
		pComponent->AddParticleData(EPVF_Velocity);
	else if (m_facingMode == EFacingMode::Free)
		pComponent->AddParticleData(EPQF_Orientation);
	if (m_facingMode != EFacingMode::Free)
		pParams->m_shaderData.m_sphericalApproximation = m_sphericalProjection;
	else
		pParams->m_shaderData.m_sphericalApproximation = 0.0f;
	pParams->m_renderObjectSortBias = m_sortBias;
}

void CFeatureRenderSprites::Serialize(Serialization::IArchive& ar)
{
	BaseClass::Serialize(ar);
	ar(m_sortMode, "SortMode", "Sort Mode");
	ar(m_facingMode, "FacingMode", "Facing Mode");
	ar(m_aspectRatio, "AspectRatio", "Aspect Ratio");
	if (m_facingMode == EFacingMode::Velocity)
		ar(m_axisScale, "AxisScale", "Axis Scale");
	if (m_facingMode != EFacingMode::Free)
		ar(m_sphericalProjection, "SphericalProjection", "Spherical Projection");
	ar(m_sortBias, "SortBias", "Sort Bias");
	ar(m_flipU, "FlipU", "Flip U");
	ar(m_flipV, "FlipV", "Flip V");
}

void CFeatureRenderSprites::ComputeVertices(CParticleComponentRuntime* pComponentRuntime, const SCameraInfo& camInfo, CREParticle* pRE, uint64 uRenderFlags, float fMaxPixels)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
	PARTICLE_LIGHT_PROFILER();

	CParticleContainer& container = pComponentRuntime->GetContainer();
	const CParticleEmitter* pEmitter = pComponentRuntime->GetEmitter();

	TParticleId lastParticleId = container.GetLastParticleId();
	if (lastParticleId == 0)
		return;

	const auto bounds = pComponentRuntime->GetBounds();
	const auto& physEnv = pEmitter->GetPhysicsEnv();
	const bool isAfterWater = (uRenderFlags & FOB_AFTER_WATER) != 0;
	if ((isAfterWater && physEnv.m_tUnderWater.Value == ETrinaryNames::If_True) ||
	    (!isAfterWater && physEnv.m_tUnderWater.Value == ETrinaryNames::If_False))
		return;

	uint32 threadId = JobManager::GetWorkerThreadId();
	TParticleHeap& memHeap = GetPSystem()->GetMemHeap(threadId);

	SSpritesContext spritesContext(
	  pComponentRuntime, pComponentRuntime->GetComponentParams(),
	  camInfo, pEmitter->GetVisEnv(),
	  physEnv, pRE, memHeap,
	  lastParticleId);
	spritesContext.m_bounds = pEmitter->GetBBox();
	spritesContext.m_renderFlags = uRenderFlags;

	CullParticles(&spritesContext);

	if (spritesContext.m_numSprites != 0)
	{
		SortSprites(&spritesContext);
		WriteToGPUMem(spritesContext);
		pComponentRuntime->GetEmitter()->AddDrawCallCounts(
		  spritesContext.m_numSprites,
		  spritesContext.m_numParticles - spritesContext.m_numSprites);
		GetPSystem()->GetProfiler().AddEntry(pComponentRuntime, EPS_RendereredParticles, uint(spritesContext.m_numSprites));
	}
}

void CFeatureRenderSprites::CullParticles(SSpritesContext* pSpritesContext)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	const SUpdateContext& context = pSpritesContext->m_context;
	const SVisibilityParams& visibility = context.m_params.m_visibility;
	const CParticleEmitter& emitter = *context.m_runtime.GetEmitter();

	// size and distance culling
	const CCamera& camera = *pSpritesContext->m_camInfo.pCamera;
	const float camAng = tan_tpl(camera.GetFov() * 0.5f) * 2.0f;
	const float angularRes = camera.GetViewSurfaceZ() / camAng;
	const float nearDist = pSpritesContext->m_bounds.GetDistance(camera.GetPosition());
	const float farDist = pSpritesContext->m_bounds.GetFarDistance(camera.GetPosition());
	const float maxScreen = min(+visibility.m_maxScreenSize, GetCVars()->e_ParticlesMaxDrawScreen);

	const float minCamDist = max(+visibility.m_minCameraDistance, camera.GetNearPlane());
	const float maxCamDist = ZeroIsHuge(visibility.m_maxCameraDistance);
	const float invMaxAng = 1.0f / (maxScreen * camAng * 0.5f);
	const float invMinAng = angularRes * emitter.GetViewDistRatio() * visibility.m_viewDistanceMultiple / max(GetCVars()->e_ParticlesMinDrawPixels, 0.125f);

	const bool cullFrustum = camera.IsAABBVisible_FH(pSpritesContext->m_bounds) != CULL_INCLUSION;
	const bool cullNear = nearDist < visibility.m_minCameraDistance * 2.0f
	                      || maxScreen < 2 && nearDist < context.m_params.m_maxParticleSize * invMaxAng * 2.0f
	                      || m_facingMode != EFacingMode::Screen && nearDist < camera.GetNearPlane() * 2.0f;
	const bool cullFar = farDist > maxCamDist * 0.75f
	                     || GetCVars()->e_ParticlesMinDrawPixels > 0.0f;
	const bool culling = cullFrustum || cullNear || cullFar;

	// frustum culling
	Matrix34 invViewTM = pSpritesContext->m_camInfo.pCamera->GetViewMatrix();
	float projectH;
	float projectV;

	if (cullFrustum)
	{
		Vec3 frustum = camera.GetEdgeP();
		float invX = abs(1.0f / frustum.x);
		float invZ = abs(1.0f / frustum.z);

		non_const(invViewTM.GetRow4(0)) *= frustum.y * invX;
		non_const(invViewTM.GetRow4(2)) *= frustum.y * invZ;

		projectH = sqrt_tpl(sqr(frustum.x) + sqr(frustum.y)) * invX;
		projectV = sqrt_tpl(sqr(frustum.z) + sqr(frustum.y)) * invZ;
	}

	CParticleContainer& container = pSpritesContext->m_context.m_container;
	TIOStream<uint8> states = container.GetTIOStream<uint8>(EPDT_State);
	IFStream alphas = container.GetIFStream(EPDT_Alpha, 1.0f);
	IFStream sizes = container.GetIFStream(EPDT_Size);
	IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
	auto& particleIds = pSpritesContext->m_particleIds;
	auto& spriteAlphas = pSpritesContext->m_spriteAlphas;

	CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
	{
		const uint8 state = states.Load(particleId);
		if (!(state & ESB_Alive))
			continue;

		float cull = 1.0f;
		const float size = sizes.Load(particleId);
		if (!cullFar)
			cull = (size > 0.0f);

		if (culling)
		{
			const Vec3 position = positions.Load(particleId);
			Vec3 posCam;

			if (cullFrustum)
			{
				// compute camera-relative position
				posCam = invViewTM * position;
				if (max(abs(posCam.x) - size * projectH, abs(posCam.z) - size * projectV) >= posCam.y)
					continue;
			}
			else
			{
				// just compute depth
				posCam.y = invViewTM.m10 * position.x + invViewTM.m11 * position.y + invViewTM.m12 * position.z + invViewTM.m13;
			}

			if (cullNear + cullFar)
			{
				const float invDist = 1.0f / posCam.y;
				if (cullNear)
				{
					const float ratio = max(size * invMaxAng, minCamDist) * invDist;
					cull *= min((1.0f - ratio) * 2.0f, 1.0f);
				}
				if (cullFar)
				{
					const float ratio = min(size * invMinAng, maxCamDist) * invDist;
					cull *= min((ratio - 1.0f) * 3.0f, 1.0f);
				}
			}
		}

		const float alpha = alphas.SafeLoad(particleId) * cull;
		if (alpha > 0.0f)
		{
			particleIds[pSpritesContext->m_numSprites++] = particleId;
			spriteAlphas[particleId] = alpha;
		}
	}
	CRY_PFX2_FOR_END;

	if ((GetCVars()->e_ParticlesDebug & AlphaBit('c')) == 0)
	{
		// vis area clipping
		CRY_PFX2_ASSERT(container.HasData(EPDT_Size));
		Matrix34 viewTM = pSpritesContext->m_camInfo.pCamera->GetMatrix();
		Vec3 normal = -viewTM.GetColumn0();
		IVisArea* pVisArea = pSpritesContext->m_visEnviron.GetClipVisArea(pSpritesContext->m_camInfo.pCameraVisArea, pSpritesContext->m_bounds);

		if (pVisArea)
		{
			const uint count = pSpritesContext->m_numSprites;
			for (uint i = 0, j = 0; i < count; ++i)
			{
				TParticleId particleId = particleIds[i];

				Sphere sphere;
				const float radius = sizes.Load(particleId) * 0.5f;
				sphere.center = positions.Load(particleId);
				sphere.radius = radius;
				if (pSpritesContext->m_visEnviron.ClipVisAreas(pVisArea, sphere, normal))
					spriteAlphas[particleId] *= (sphere.radius - radius) > 0.0f;

				if (spriteAlphas[particleId] > 0.0f)
					particleIds[j++] = particleId;
				else
					--pSpritesContext->m_numSprites;
			}
		}
	}

	// water clipping
	//    PFX2_TODO : Optimize : this routine is *!REALLY!* slow when there are water volumes on the map
	const bool doCullWater = (pSpritesContext->m_physEnviron.m_tUnderWater.Value == ETrinaryNames::Both);
	if (doCullWater)
	{
		CRY_PFX2_ASSERT(container.HasData(EPDT_Size));
		Plane waterPlane;
		bool isAfterWater = (pSpritesContext->m_renderFlags & FOB_AFTER_WATER) != 0;
		float clipWaterSign = isAfterWater ? 1.0f : -1.0f;
		const uint count = pSpritesContext->m_numSprites;
		for (uint i = 0, j = 0; i < count; ++i)
		{
			TParticleId particleId = particleIds[i];

			const float radius = sizes.Load(particleId) * 0.5f;
			const Vec3 position = positions.Load(particleId);
			float waterDist = pSpritesContext->m_physEnviron.GetWaterPlane(
			  waterPlane, position, radius) * clipWaterSign;
			waterDist += isAfterWater ? radius * 2.0f : 0.0f;
			const float waterAlpha = SATURATE(waterDist * __fres(radius));
			spriteAlphas[particleId] *= waterAlpha;

			if (waterAlpha > 0.0f)
				particleIds[j++] = particleId;
			else
				--pSpritesContext->m_numSprites;
		}
	}
}

void CFeatureRenderSprites::SortSprites(SSpritesContext* pSpritesContext)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	if (m_sortMode == ESortMode::None || GetCVars()->e_ParticlesSortQuality == 0)
		return;

	uint32 threadId = JobManager::GetWorkerThreadId();
	TParticleHeap& memHeap = GetPSystem()->GetMemHeap(threadId);
	const uint numSprites = pSpritesContext->m_numSprites;

	TParticleHeap::Array<uint32, uint, CRY_PFX2_PARTICLES_ALIGNMENT> indices(memHeap, numSprites);
	TParticleHeap::Array<float, uint, CRY_PFX2_PARTICLES_ALIGNMENT> keys(memHeap, numSprites);
	auto& particleIds = pSpritesContext->m_particleIds;

	const CParticleContainer& container = pSpritesContext->m_context.m_container;
	const bool byAge = (m_sortMode == ESortMode::NewToOld) || (m_sortMode == ESortMode::OldToNew);
	const bool byDistance = (m_sortMode == ESortMode::FrontToBack) || (m_sortMode == ESortMode::BackToFront);
	const bool invertKey = (m_sortMode == ESortMode::OldToNew) || (m_sortMode == ESortMode::FrontToBack);

	if (byAge)
	{
		CRY_PFX2_ASSERT(container.HasData(EPDT_NormalAge));
		CRY_PFX2_ASSERT(container.HasData(EPDT_LifeTime));
		IFStream ages = container.GetIFStream(EPDT_NormalAge);
		IFStream lifeTimes = container.GetIFStream(EPDT_LifeTime);

		for (size_t i = 0; i < numSprites; ++i)
		{
			const TParticleId particleId = particleIds[i];
			const float age = ages.Load(particleId) * lifeTimes.Load(particleId);
			keys[i] = age;
		}
	}
	else if (byDistance)
	{
		IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);

		Vec3 camPos = pSpritesContext->m_camInfo.pCamera->GetPosition();
		for (size_t i = 0; i < numSprites; ++i)
		{
			const TParticleId particleId = particleIds[i];
			const Vec3 pPos = positions.Load(particleId);
			const float dist = __fres(camPos.GetSquaredDistance(pPos));
			keys[i] = dist;
		}
	}
	if (invertKey)
	{
		for (size_t i = 0; i < numSprites; ++i)
			keys[i] = __fres(keys[i]);
	}

	RadixSort(indices.begin(), indices.end(), keys.begin(), keys.end(), memHeap);

	for (size_t i = 0; i < numSprites; ++i)
		indices[i] = particleIds[indices[i]];
	memcpy(particleIds.data(), indices.data(), sizeof(uint32) * indices.size());
}

void CFeatureRenderSprites::WriteToGPUMem(const SSpritesContext& spritesContext)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	const CParticleContainer& container = spritesContext.m_context.m_container;
	const bool hasAngles2D = container.HasData(EPDT_Angle2D);
	const bool hasColors = container.HasData(EPDT_Color);
	const bool hasTiling = container.HasData(EPDT_Tile);
	const bool hasAnimation = spritesContext.m_params.m_textureAnimation.IsAnimating();
	const uint numSprites = spritesContext.m_numSprites;

	SRenderVertices* pRenderVertices = spritesContext.m_pRE->AllocPullVertices(numSprites);

	WritePositions(spritesContext, pRenderVertices->aPositions);
	if (hasAngles2D)
		WriteAxes<true>(spritesContext, pRenderVertices->aAxes);
	else
		WriteAxes<false>(spritesContext, pRenderVertices->aAxes);

	switch (hasColors + hasTiling * 2 + hasAnimation * 4)
	{
	case 0:
		WriteColorSTs<false, false, false>(spritesContext, pRenderVertices->aColorSTs);
		break;
	case 1:
		WriteColorSTs<true, false, false>(spritesContext, pRenderVertices->aColorSTs);
		break;
	case 2:
		WriteColorSTs<false, true, false>(spritesContext, pRenderVertices->aColorSTs);
		break;
	case 3:
		WriteColorSTs<true, true, false>(spritesContext, pRenderVertices->aColorSTs);
		break;
	case 4:
		WriteColorSTs<false, false, true>(spritesContext, pRenderVertices->aColorSTs);
		break;
	case 5:
		WriteColorSTs<true, false, true>(spritesContext, pRenderVertices->aColorSTs);
		break;
	case 6:
		WriteColorSTs<false, true, true>(spritesContext, pRenderVertices->aColorSTs);
		break;
	case 7:
		WriteColorSTs<true, true, true>(spritesContext, pRenderVertices->aColorSTs);
		break;
	}
}

void CFeatureRenderSprites::WritePositions(const SSpritesContext& spritesContext, FixedDynArray<Vec3>& gpuPositions)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	const CParticleContainer& container = spritesContext.m_context.m_container;
	IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
	const auto& particleIds = spritesContext.m_particleIds;
	CWriteCombinedBuffer<Vec3, vertexBufferSize, vertexChunckSize> wcPositions(gpuPositions);
	const uint numSprites = spritesContext.m_numSprites;

	const uint spritesPerChunk = vertexChunckSize / sizeof(Vec3);
	const uint numChunks = (numSprites / spritesPerChunk) + 1;
	for (uint chunk = 0, spriteIdx = 0; chunk < numChunks; ++chunk)
	{
		const uint chunkSprites = min(spritesPerChunk, numSprites - chunk * spritesPerChunk);
		if (!wcPositions.CheckAvailable(chunkSprites))
			break;

		for (uint sprite = 0; sprite < chunkSprites; ++sprite, ++spriteIdx)
		{
			const TParticleId particleId = particleIds[spriteIdx];
			const Vec3 position = positions.Load(particleId);
			wcPositions.Array().push_back(position);
		}
	}
}

class CSpriteFacingModeScreen
{
public:
	CSpriteFacingModeScreen(const CParticleContainer& container, const CCamera& camera)
		: m_sizes(container.GetIFStream(EPDT_Size))
	{
		Matrix34 viewTM = camera.GetMatrix();
		m_xAxis = viewTM.GetColumn0();
		m_yAxis = -viewTM.GetColumn2();
	}

	ILINE SParticleAxes Sample(TParticleId particleId) const
	{
		SParticleAxes axes;
		const float size = m_sizes.Load(particleId);
		axes.xAxis = m_xAxis * size;
		axes.yAxis = m_yAxis * size;
		return axes;
	}

private:
	const IFStream m_sizes;
	Vec3           m_xAxis;
	Vec3           m_yAxis;
};

class CSpriteFacingModeCamera
{
public:
	CSpriteFacingModeCamera(const CParticleContainer& container, const CCamera& camera)
		: m_positions(container.GetIVec3Stream(EPVF_Position))
		, m_sizes(container.GetIFStream(EPDT_Size))
		, m_cameraPosition(camera.GetPosition()) {}

	ILINE SParticleAxes Sample(TParticleId particleId) const
	{
		SParticleAxes axes;
		const Vec3 particlePosition = m_positions.Load(particleId);
		const float size = m_sizes.Load(particleId);

		const Vec3 up = Vec3(0.0f, 0.0f, 1.0f);
		const Vec3 normal = particlePosition - m_cameraPosition;
		axes.xAxis = -up.Cross(normal).GetNormalized() * size;
		axes.yAxis = -axes.xAxis.Cross(normal).GetNormalized() * size;

		return axes;
	}

private:
	const IVec3Stream m_positions;
	const IFStream    m_sizes;
	Vec3              m_cameraPosition;
};

class CSpriteFacingModeVelocity
{
public:
	CSpriteFacingModeVelocity(const CParticleContainer& container, const CCamera& camera, float axisScale)
		: m_positions(container.GetIVec3Stream(EPVF_Position))
		, m_velocities(container.GetIVec3Stream(EPVF_Velocity))
		, m_sizes(container.GetIFStream(EPDT_Size))
		, m_cameraPosition(camera.GetPosition())
		, m_cameraXAxis(camera.GetMatrix().GetColumn0())
		, m_axisScale(axisScale) {}

	ILINE SParticleAxes Sample(TParticleId particleId) const
	{
		SParticleAxes axes;
		const Vec3 particlePosition = m_positions.Load(particleId);
		const Vec3 particleVelocity = m_velocities.Load(particleId);
		const float size = m_sizes.Load(particleId);

		const Vec3 moveDirection = particleVelocity.GetNormalizedSafe(m_cameraXAxis);
		const Vec3 normal = (particlePosition - m_cameraPosition).GetNormalized();
		const float axisSize = max(size, particleVelocity.GetLength() * m_axisScale);
		axes.xAxis = moveDirection * axisSize;
		axes.yAxis = -moveDirection.Cross(normal) * size;

		return axes;
	}

private:
	const IVec3Stream m_positions;
	const IVec3Stream m_velocities;
	const IFStream    m_sizes;
	Vec3              m_cameraPosition;
	Vec3              m_cameraXAxis;
	float             m_axisScale;
};

class CSpriteFacingModeFree
{
public:
	CSpriteFacingModeFree(const CParticleContainer& container)
		: m_orientations(container.GetIQuatStream(EPQF_Orientation))
		, m_sizes(container.GetIFStream(EPDT_Size)) {}

	ILINE SParticleAxes Sample(TParticleId particleId) const
	{
		SParticleAxes axes;
		const Quat orientation = m_orientations.Load(particleId);
		const float size = m_sizes.Load(particleId);
		axes.xAxis = orientation.GetColumn0() * size;
		axes.yAxis = -orientation.GetColumn1() * size;
		return axes;
	}

private:
	const IQuatStream m_orientations;
	const IFStream    m_sizes;
};

template<const bool hasAngles2D>
void CFeatureRenderSprites::WriteAxes(const SSpritesContext& spritesContext, FixedDynArray<SParticleAxes>& gpuAxes)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	const CParticleContainer& container = spritesContext.m_context.m_container;
	const CCamera& camera = *spritesContext.m_camInfo.pCamera;

	switch (m_facingMode)
	{
	case EFacingMode::Screen:
		WriteAxes<CSpriteFacingModeScreen, hasAngles2D>(spritesContext, gpuAxes, CSpriteFacingModeScreen(container, camera));
		break;
	case EFacingMode::Camera:
		WriteAxes<CSpriteFacingModeCamera, hasAngles2D>(spritesContext, gpuAxes, CSpriteFacingModeCamera(container, camera));
		break;
	case EFacingMode::Velocity:
		WriteAxes<CSpriteFacingModeVelocity, hasAngles2D>(spritesContext, gpuAxes, CSpriteFacingModeVelocity(container, camera, m_axisScale));
		break;
	case EFacingMode::Free:
		WriteAxes<CSpriteFacingModeFree, hasAngles2D>(spritesContext, gpuAxes, CSpriteFacingModeFree(container));
		break;
	}
}

template<typename TAxesSampler, const bool hasAngles2D>
void CFeatureRenderSprites::WriteAxes(const SSpritesContext& spritesContext, FixedDynArray<SParticleAxes>& gpuAxes, const TAxesSampler& axesSampler)
{
	const CParticleContainer& container = spritesContext.m_context.m_container;
	IFStream angles = container.GetIFStream(EPDT_Angle2D, 0.0f);
	const auto& particleIds = spritesContext.m_particleIds;
	CWriteCombinedBuffer<SParticleAxes, vertexBufferSize, vertexChunckSize> wcAxes(gpuAxes);
	const uint numSprites = spritesContext.m_numSprites;
	const float flipU = m_flipU ? -1.0f : 1.0f;
	const float flipV = m_flipV ? -1.0f : 1.0f;

	const uint spritesPerChunk = vertexChunckSize / sizeof(SParticleAxes);
	const uint numChunks = (numSprites / spritesPerChunk) + 1;
	for (uint chunk = 0, spriteIdx = 0; chunk < numChunks; ++chunk)
	{
		const uint chunkSprites = min(spritesPerChunk, numSprites - chunk * spritesPerChunk);
		if (!wcAxes.CheckAvailable(chunkSprites))
			break;

		for (uint sprite = 0; sprite < chunkSprites; ++sprite, ++spriteIdx)
		{
			const TParticleId particleId = particleIds[spriteIdx];

			SParticleAxes axes = axesSampler.Sample(particleId);
			if (hasAngles2D)
			{
				const float angle = angles.Load(particleId);
				RotateAxes(&axes.xAxis, &axes.yAxis, angle);
			}
			axes.xAxis *= m_aspectRatio * flipU;
			axes.yAxis *= flipV;

			wcAxes.Array().push_back(axes);
		}
	}
}

template<bool hasColors, bool hasTiling, bool hasAnimation>
void CFeatureRenderSprites::WriteColorSTs(const SSpritesContext& spritesContext, FixedDynArray<SParticleColorST>& gpuColorSTs)
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	const SComponentParams& params = spritesContext.m_params;
	const CParticleContainer& container = spritesContext.m_context.m_container;
	const uint numSprites = spritesContext.m_numSprites;
	const auto& particleIds = spritesContext.m_particleIds;
	const auto& spriteAlphas = spritesContext.m_spriteAlphas;
	IFStream ages = container.GetIFStream(EPDT_NormalAge);
	IFStream lifetimes = container.GetIFStream(EPDT_LifeTime);
	IFStream alphas = container.GetIFStream(EPDT_Alpha, 1.0f);
	IColorStream colors = container.GetIColorStream(EPDT_Color);
	TIStream<uint8> tiles = container.GetTIStream<uint8>(EPDT_Tile);
	CWriteCombinedBuffer<SParticleColorST, vertexBufferSize, vertexChunckSize> wcColorSTs(gpuColorSTs);

	const bool hasAbsFrameRate = params.m_textureAnimation.HasAbsoluteFrameRate();

	const uint spritesPerChunk = vertexChunckSize / sizeof(SParticleColorST);
	const uint numChunks = (numSprites / spritesPerChunk) + 1;
	for (uint chunk = 0, spriteIdx = 0; chunk < numChunks; ++chunk)
	{
		const uint chunkSprites = min(spritesPerChunk, numSprites - chunk * spritesPerChunk);
		if (!wcColorSTs.CheckAvailable(chunkSprites))
			break;

		for (uint sprite = 0; sprite < chunkSprites; ++sprite, ++spriteIdx)
		{
			const TParticleId particleId = particleIds[spriteIdx];

			SParticleColorST colorST;

			colorST.st.dcolor = 0;
			if (hasTiling)
				colorST.st.z = tiles.Load(particleId);

			if (hasAnimation)
			{
				float age = ages.Load(particleId);
				float animPos = hasAbsFrameRate ?
				                params.m_textureAnimation.GetAnimPosAbsolute(age * lifetimes.Load(particleId))
				                : params.m_textureAnimation.GetAnimPosRelative(age);

				colorST.st.z += int(animPos);
				colorST.st.w = FloatToUFrac8Saturate(animPos - int(animPos));
			}

			colorST.color.dcolor = ~0;
			if (hasColors)
				colorST.color = colors.Load(particleId);
			colorST.color.a = FloatToUFrac8Saturate(spriteAlphas[particleId]);

			wcColorSTs.Array().push_back(colorST);
		}
	}
}

}
