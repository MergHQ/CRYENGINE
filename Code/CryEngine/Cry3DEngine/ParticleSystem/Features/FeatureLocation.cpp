// Copyright 2015-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FeatureCommon.h"
#include <CrySystem/Testing/CryTest.h>
#include <CryMath/SNoise.h>
#include "Target.h"

namespace pfx2
{

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationOffset

class CFeatureLocationOffset : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureLocationOffset()
		: m_offset(ZERO)
		, m_scale(1.0f)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->GetSpatialExtents.add(this);
		pComponent->GetEmitOffsets.add(this);
		pComponent->InitParticles.add(this);
		pComponent->UpdateGPUParams.add(this);
		m_scale.AddToComponent(pComponent, this);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_offset, "Offset", "Offset");
		ar(m_scale, "Scale", "Scale");
	}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) override
	{
		if (!m_scale.HasModifiers())
			return;

		SInstanceUpdateBuffer<float> sizes(runtime, m_scale);
		for (uint i = 0; i < runtime.GetNumInstances(); ++i)
		{
			float e = abs(sizes[i]) * sizes.Range().Length();
			e = e * scales[i] + 1.0f;
			extents[i] += e;
		}
	}

	virtual void GetEmitOffsets(const CParticleComponentRuntime& runtime, TVarArray<Vec3> offsets, uint firstInstance) override
	{
		SInstanceUpdateBuffer<float> sizes(runtime, m_scale);
		for (uint i = 0; i < offsets.size(); ++i)
		{
			uint idx = firstInstance + i;
			const float scale = sizes[idx] * (sizes.Range().start + sizes.Range().end) * 0.5f;
			offsets[i] += m_offset * scale;
		}
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		CParticleContainer& parentContainer = runtime.GetParentContainer();
		CParticleContainer& container = runtime.GetContainer();
		const Quat defaultQuat = runtime.GetEmitter()->GetLocation().q;
		IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		STempInitBuffer<float> scales(runtime, m_scale);

		Vec3 oOffset = m_offset;
		for (auto particleId : runtime.SpawnedRange())
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const float scale = scales.SafeLoad(particleId);
			const Vec3 wPosition0 = positions.Load(particleId);
			const Quat wQuat = parentQuats.SafeLoad(parentId);
			const Vec3 wOffset = wQuat * oOffset;
			const Vec3 wPosition1 = wPosition0 + wOffset * scale;
			positions.Store(particleId, wPosition1);
		}
	}

	virtual void UpdateGPUParams(CParticleComponentRuntime& runtime, gpu_pfx2::SUpdateParams& params) override
	{
		params.offset = m_offset;
		params.scale.x = m_scale.GetValueRange(runtime)(0.5f);
		params.initFlags |= gpu_pfx2::eFeatureInitializationFlags_LocationOffset;
	}

private:
	Vec3                                 m_offset;
	CParamMod<EDD_PerParticle, UFloat10> m_scale;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationOffset, "Location", "Offset", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationBox

class CFeatureLocationBox : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->InitParticles.add(this);
		pComponent->GetSpatialExtents.add(this);
		pComponent->UpdateGPUParams.add(this);
		m_scale.AddToComponent(pComponent, this);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_box, "Dimension", "Dimension");
		ar(m_scale, "Scale", "Scale");
	}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) override
	{
		SInstanceUpdateBuffer<float> sizes(runtime, m_scale);
		float avg = (sizes.Range().start + sizes.Range().end) * 0.5f;
		for (uint i = 0; i < runtime.GetNumInstances(); ++i)
		{
			// Increase each dimension by 1 to include boundaries; works properly for boxes, rects, and lines
			const float s = abs(scales[i] * sizes[i] * avg);
			extents[i] += (m_box.x * s + 1.0f) * (m_box.y * s + 1.0f) * (m_box.z * s + 1.0f);
		}
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& parentContainer = runtime.GetParentContainer();
		CParticleContainer& container = runtime.GetContainer();
		const Quat defaultQuat = runtime.GetEmitter()->GetLocation().q;
		IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		STempInitBuffer<float> scales(runtime, m_scale);

		for (auto particleId : runtime.SpawnedRange())
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const float scale = scales.SafeLoad(particleId);
			const Vec3 wPosition0 = positions.Load(particleId);
			const Quat wQuat = parentQuats.SafeLoad(parentId);
			const Vec3 oOffset = Vec3(
			  runtime.Chaos().RandSNorm() * m_box.x,
			  runtime.Chaos().RandSNorm() * m_box.y,
			  runtime.Chaos().RandSNorm() * m_box.z);
			const Vec3 wOffset = wQuat * oOffset;
			const Vec3 wPosition1 = wPosition0 + wOffset * scale;
			positions.Store(particleId, wPosition1);
		}
	}

	virtual void UpdateGPUParams(CParticleComponentRuntime& runtime, gpu_pfx2::SUpdateParams& params) override
	{
		params.box = m_box;
		params.scale.x = m_scale.GetValueRange(runtime)(0.5f);
		params.initFlags |= gpu_pfx2::eFeatureInitializationFlags_LocationBox;
	}

private:
	Vec3                                 m_box = ZERO;
	CParamMod<EDD_PerParticle, UFloat10> m_scale;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationBox, "Location", "Box", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationSphere

class CFeatureLocationSphere : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureLocationSphere()
		: m_radius(0.0f)
		, m_velocity(0.0f)
		, m_axisScale(1.0f, 1.0f, 1.0f)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->InitParticles.add(this);
		pComponent->GetSpatialExtents.add(this);
		pComponent->UpdateGPUParams.add(this);
		m_radius.AddToComponent(pComponent, this);
		m_velocity.AddToComponent(pComponent, this);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_radius, "Radius", "Radius");
		ar(m_velocity, "Velocity", "Velocity");
		ar(m_axisScale, "AxisScale", "Axis Scale");
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		const float EPSILON = 1.0f / 2048.0f;
		const bool useRadius = m_radius.GetBaseValue() > EPSILON;
		const bool useVelocity = abs(m_velocity.GetBaseValue()) > EPSILON;

		if (useRadius && useVelocity)
			SphericalDist<true, true>(runtime);
		else if (useRadius)
			SphericalDist<true, false>(runtime);
		else if (useVelocity)
			SphericalDist<false, true>(runtime);
	}

	virtual void UpdateGPUParams(CParticleComponentRuntime& runtime, gpu_pfx2::SUpdateParams& params) override
	{
		params.scale = m_axisScale;
		params.radius = m_radius.GetValueRange(runtime)(0.5f);
		params.velocity = m_velocity.GetValueRange(runtime)(0.5f);
		params.initFlags |= gpu_pfx2::eFeatureInitializationFlags_LocationSphere;
	}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) override
	{
		SInstanceUpdateBuffer<float> sizes(runtime, m_radius);
		for (uint i = 0; i < runtime.GetNumInstances(); ++i)
		{
			// Increase each dimension by 1 to include sphere bounds; works properly for spheres and circles
			const Vec3 axisMax = m_axisScale * abs(scales[i] * sizes[i] * sizes.Range().end) + Vec3(1.0f),
			           axisMin = m_axisScale * abs(scales[i] * sizes[i] * sizes.Range().start);
			const float v = (axisMax.x * axisMax.y * axisMax.z - axisMin.x * axisMin.y * axisMin.z) * (gf_PI * 4.0f / 3.0f);
			extents[i] += v;
		}
	}

private:
	template<const bool UseRadius, const bool UseVelocity>
	void SphericalDist(CParticleComponentRuntime& runtime)
	{
		CParticleContainer& container = runtime.GetContainer();
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);

		STempInitBuffer<float> radii(runtime, m_radius);
		STempInitBuffer<float> velocityMults(runtime, m_velocity);

		for (auto particleId : runtime.SpawnedRange())
		{
			const Vec3 sphere = runtime.Chaos().RandSphere();
			const Vec3 sphereDist = sphere.CompMul(m_axisScale);

			if (UseRadius)
			{
				const float radius = radii.SafeLoad(particleId);
				const Vec3 wPosition0 = positions.Load(particleId);
				const Vec3 wPosition1 = wPosition0 + sphereDist * radius;
				positions.Store(particleId, wPosition1);
			}

			if (UseVelocity)
			{
				const float velocityMult = velocityMults.SafeLoad(particleId);
				const Vec3 wVelocity0 = velocities.Load(particleId);
				const Vec3 wVelocity1 = wVelocity0 + sphereDist * velocityMult;
				velocities.Store(particleId, wVelocity1);
			}
		}
	}

	CParamMod<EDD_PerParticle, UFloat10> m_radius;
	CParamMod<EDD_PerParticle, SFloat10> m_velocity;
	Vec3                                 m_axisScale;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationSphere, "Location", "Sphere", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationDisc

class CFeatureLocationCircle : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureLocationCircle()
		: m_radius(0.0f)
		, m_velocity(0.0f)
		, m_axisScale(1.0f, 1.0f)
		, m_axis(0.0f, 0.0f, 1.0f)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->InitParticles.add(this);
		pComponent->GetSpatialExtents.add(this);
		pComponent->UpdateGPUParams.add(this);
		m_radius.AddToComponent(pComponent, this);
		m_velocity.AddToComponent(pComponent, this);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_radius, "Radius", "Radius");
		ar(m_velocity, "Velocity", "Velocity");
		ar(m_axisScale, "AxisScale", "Axis Scale");
		ar(m_axis, "Axis", "Axis");
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		const float EPSILON = 1.0f / 2048.0f;
		const bool useRadius = m_radius.GetBaseValue() > EPSILON;
		const bool useVelocity = abs(m_velocity.GetBaseValue()) > EPSILON;

		if (useRadius && useVelocity)
			CircularDist<true, true>(runtime);
		else if (useRadius)
			CircularDist<true, false>(runtime);
		else if (useVelocity)
			CircularDist<false, true>(runtime);
	}

	virtual void UpdateGPUParams(CParticleComponentRuntime& runtime, gpu_pfx2::SUpdateParams& params) override
	{
		params.scale.x = m_axisScale.x;
		params.scale.y = m_axisScale.y;
		params.radius = m_radius.GetValueRange(runtime)(0.5f);
		params.velocity = m_velocity.GetValueRange(runtime)(0.5f);
		params.initFlags |= gpu_pfx2::eFeatureInitializationFlags_LocationCircle;
	}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) override
	{
		SInstanceUpdateBuffer<float> sizes(runtime, m_radius);
		for (uint i = 0; i < runtime.GetNumInstances(); ++i)
		{
			const Vec2 axisMax = m_axisScale * abs(scales[i] * sizes[i] * sizes.Range().end) + Vec2(1.0f),
			           axisMin = m_axisScale * abs(scales[i] * sizes[i] * sizes.Range().start);
			const float v = (axisMax.x * axisMax.y - axisMin.x * axisMin.y) * gf_PI;
			extents[i] += v;
		}
	}

private:
	template<const bool UseRadius, const bool UseVelocity>
	void CircularDist(CParticleComponentRuntime& runtime)
	{
		CParticleContainer& parentContainer = runtime.GetParentContainer();
		CParticleContainer& container = runtime.GetContainer();
		const Quat defaultQuat = runtime.GetEmitter()->GetLocation().q;
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		const IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, defaultQuat);
		const Quat axisQuat = Quat::CreateRotationV0V1(Vec3(0.0f, 0.0f, 1.0f), m_axis.GetNormalizedSafe());
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);

		STempInitBuffer<float> radii(runtime, m_radius);
		STempInitBuffer<float> velocityMults(runtime, m_velocity);

		for (auto particleId : runtime.SpawnedRange())
		{
			TParticleId parentId = parentIds.Load(particleId);
			const Quat wQuat = parentQuats.SafeLoad(parentId);

			const Vec2 disc2 = runtime.Chaos().RandCircle();
			const Vec3 disc3 = axisQuat * Vec3(disc2.x * m_axisScale.x, disc2.y * m_axisScale.y, 0.0f);

			if (UseRadius)
			{
				const float radius = radii.SafeLoad(particleId);
				const Vec3 oPosition = disc3 * radius;
				const Vec3 wPosition0 = positions.Load(particleId);
				const Vec3 wPosition1 = wPosition0 + wQuat * oPosition;
				positions.Store(particleId, wPosition1);
			}

			if (UseVelocity)
			{
				const float velocityMult = velocityMults.SafeLoad(particleId);
				const Vec3 wVelocity0 = velocities.Load(particleId);
				const Vec3 wVelocity1 = wVelocity0 + wQuat * disc3 * velocityMult;
				velocities.Store(particleId, wVelocity1);
			}
		}
	}

private:
	CParamMod<EDD_PerParticle, UFloat10> m_radius;
	CParamMod<EDD_PerParticle, SFloat10> m_velocity;
	Vec3                                 m_axis;
	Vec2                                 m_axisScale;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationCircle, "Location", "Circle", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationGeometry

extern TDataType<IMeshObj*>        EPDT_MeshGeometry;
extern TDataType<IPhysicalEntity*> EPDT_PhysicalEntity;

SERIALIZATION_DECLARE_ENUM(EGeometrySource,
                           Render = GeomType_Render,
                           Physics = GeomType_Physics
                           )

SERIALIZATION_DECLARE_ENUM(EGeometryLocation,
                           Vertices = GeomForm_Vertices,
                           Edges = GeomForm_Edges,
                           Surface = GeomForm_Surface,
                           Volume = GeomForm_Volume
                           )

class CFeatureLocationGeometry : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->MainPreUpdate.add(this);
		pComponent->GetSpatialExtents.add(this);
		m_offset.AddToComponent(pComponent, this);
		m_velocity.AddToComponent(pComponent, this);
		if (m_orientToNormal)
			pComponent->AddParticleData(EPQF_Orientation);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		SERIALIZE_VAR(ar, m_source);
		SERIALIZE_VAR(ar, m_location);
		SERIALIZE_VAR(ar, m_offset);
		SERIALIZE_VAR(ar, m_velocity);
		SERIALIZE_VAR(ar, m_orientToNormal);
		SERIALIZE_VAR(ar, m_augmentLocation);
	}

	virtual void MainPreUpdate(CParticleComponentRuntime& runtime) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		if (CParticleComponent* pParentComponent = runtime.GetComponent()->GetParentComponent())
		{
			if (IMeshObj* pMesh = pParentComponent->GetComponentParams().m_pMesh)
				pMesh->GetExtent((EGeomForm)m_location);
		}
		else if (CParticleEmitter* pEmitter = runtime.GetEmitter())
		{
			pEmitter->UpdateEmitGeomFromEntity();
			const GeomRef& emitterGeometry = pEmitter->GetEmitterGeometry();
			const EGeomType geomType = (EGeomType)m_source;
			const EGeomForm geomForm = (EGeomForm)m_location;
			emitterGeometry.GetExtent(geomType, geomForm);
		}
	}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) override
	{
		if (!runtime.GetEmitter())
			return;

		GeomRef emitterGeometry = runtime.GetEmitter()->GetEmitterGeometry();
		CParticleComponent* pParentComponent = runtime.GetComponent()->GetParentComponent();
		if (pParentComponent)
		{
			if (IMeshObj* pMesh = pParentComponent->GetComponentParams().m_pMesh)
				emitterGeometry.Set(pMesh);
		}
		auto parentMeshes = runtime.GetParentContainer().IStream(EPDT_MeshGeometry, +emitterGeometry.m_pMeshObj);
		auto parentPhysics = runtime.GetParentContainer().IStream(EPDT_PhysicalEntity);

		uint numInstances = runtime.GetNumInstances();
		for (uint i = 0; i < numInstances; ++i)
		{
			if (pParentComponent)
			{
				TParticleId parentId = runtime.GetInstance(i).m_parentId;
				if (IMeshObj* mesh = parentMeshes.Load(parentId))
					emitterGeometry.Set(mesh);
				if (m_source == EGeometrySource::Physics)
				{
					if (IPhysicalEntity* pPhysics = parentPhysics.Load(parentId))
						emitterGeometry.Set(pPhysics);
				}
			}
			float extent = emitterGeometry.GetExtent((EGeomType)m_source, (EGeomForm)m_location);
			for (int dim = (int)m_location; dim > 0; --dim)
				extent *= scales[i];
			extents[i] += extent;
		}
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		const float EPSILON = 1.0f / 2048.0f;
		const bool useVelocity = abs(m_velocity.GetBaseValue()) > EPSILON;

		const CParticleEmitter* pEmitter = runtime.GetEmitter();
		const CParticleComponent* pParentComponent = runtime.GetComponent()->GetParentComponent();

		GeomRef emitterGeometry = pEmitter->GetEmitterGeometry();
		bool geometryCentered = false;
		if (pParentComponent)
		{
			IMeshObj* pMesh = pParentComponent->GetComponentParams().m_pMesh;
			if (pMesh)
				emitterGeometry.Set(pMesh);
			geometryCentered = pParentComponent->GetComponentParams().m_meshCentered;
		}
		if (!emitterGeometry)
			return;

		const EGeomType geomType = (EGeomType)m_source;
		const EGeomForm geomForm = (EGeomForm)m_location;
		QuatTS geomLocation = pEmitter->GetEmitterGeometryLocation();
		const QuatTS emitterLocation = pEmitter->GetLocation();
		CParticleContainer& container = runtime.GetContainer();
		const CParticleContainer& parentContainer = runtime.GetParentContainer();
		const IPidStream parentIds = container.GetIPidStream(EPDT_ParentId);
		const IVec3Stream parentPositions = parentContainer.GetIVec3Stream(EPVF_Position, geomLocation.t);
		const IQuatStream parentQuats = parentContainer.GetIQuatStream(EPQF_Orientation, geomLocation.q);
		const IFStream parentSizes = parentContainer.GetIFStream(EPDT_Size, geomLocation.s);
		const TIStream<IMeshObj*> parentMeshes = parentContainer.IStream(EPDT_MeshGeometry, +emitterGeometry.m_pMeshObj);
		const TIStream<IPhysicalEntity*> parentPhysics = parentContainer.IStream(EPDT_PhysicalEntity);
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);
		IOVec3Stream velocities = container.GetIOVec3Stream(EPVF_Velocity);
		IOQuatStream orientations = container.GetIOQuatStream(EPQF_Orientation);

		STempInitBuffer<float> offsets(runtime, m_offset);
		STempInitBuffer<float> velocityMults(runtime, m_velocity);

		auto spawnRange = container.GetSpawnedRange();
		TParticleHeap::Array<PosNorm> randomPoints(runtime.MemHeap(), spawnRange.size());

		// Count children for each parent attachment object
		struct GeomChildren
		{
			GeomRef geom;
			uint pos;
		};
		THeapArray<GeomChildren*> parentGeom(runtime.MemHeap());
		THeapArray<GeomChildren> mapGeomChildren(runtime.MemHeap());

		if (!pParentComponent)
		{
			emitterGeometry.GetRandomPoints(
				randomPoints,
				runtime.Chaos().Rand(), geomType, geomForm,
				&geomLocation, geometryCentered);
		}
		else
		{
			auto FindGeomChildren = [&mapGeomChildren](const GeomRef& geom) -> GeomChildren&
			{
				for (auto& elem : mapGeomChildren)
					if (elem.geom == geom)
						return elem;
				mapGeomChildren.push_back(GeomChildren{geom, 0});
				return mapGeomChildren.back();
			};

			parentGeom.resize(spawnRange.size());
			mapGeomChildren.reserve(parentContainer.GetNumParticles());
			for (auto particleId : runtime.SpawnedRange())
			{
				GeomRef particleGeometry = emitterGeometry;
				TParticleId parentId = parentIds.Load(particleId);
				if (IMeshObj* mesh = parentMeshes.SafeLoad(parentId))
					particleGeometry.Set(mesh);
				if (m_source == EGeometrySource::Physics)
				{
					if (IPhysicalEntity* pPhysics = parentPhysics.SafeLoad(parentId))
						particleGeometry.Set(pPhysics);
				}

				auto& geom = FindGeomChildren(particleGeometry);

				parentGeom[particleId - spawnRange.m_begin] = &geom;
				geom.pos++;
			}

			// Assign geom position, and get random points
			uint pos = 0;
			for (auto& elem : mapGeomChildren)
			{
				uint count = elem.pos;
				elem.pos = pos;

				elem.geom.GetRandomPoints(
					randomPoints(pos, count),
					runtime.Chaos().Rand(), geomType, geomForm,
					nullptr, geometryCentered);
				pos += count;
			}
		}

		for (auto particleId : runtime.SpawnedRange())
		{
			PosNorm randPositionNormal;
			
			if (!pParentComponent)
			{
				randPositionNormal = randomPoints[particleId - spawnRange.m_begin];
				if (m_augmentLocation)
				{
					Vec3 wPosition = positions.Load(particleId);
					randPositionNormal.vPos += wPosition - emitterLocation.t;
				}
			}
			else
			{
				TParticleId parentId = parentIds.Load(particleId);
				geomLocation.t = parentPositions.SafeLoad(parentId);
				geomLocation.q = parentQuats.SafeLoad(parentId);
				geomLocation.s = parentSizes.SafeLoad(parentId);

				auto& geom = *parentGeom[particleId - spawnRange.m_begin];
				randPositionNormal = randomPoints[geom.pos++];
				randPositionNormal <<= geomLocation;
			}
			
			const float offset = offsets.SafeLoad(particleId);
			const Vec3 wPosition = randPositionNormal.vPos + randPositionNormal.vNorm * offset;
			assert((wPosition - geomLocation.t).len() < 100000);
			positions.Store(particleId, wPosition);

			if (useVelocity)
			{
				const float velocityMult = velocityMults.SafeLoad(particleId);
				const Vec3 wVelocity0 = velocities.Load(particleId);
				const Vec3 wVelocity1 = wVelocity0 + randPositionNormal.vNorm * velocityMult;
				velocities.Store(particleId, wVelocity1);
			}

			if (m_orientToNormal)
			{
				const Quat wOrient0 = orientations.Load(particleId);
				const Quat oOrient = Quat::CreateRotationV0V1(randPositionNormal.vNorm, Vec3(0.0f, 0.0f, 1.0f));
				const Quat wOrient1 = wOrient0 * oOrient;
				orientations.Store(particleId, wOrient1);
			}
		}
	}

	EGeometrySource                      m_source          = EGeometrySource::Render;
	EGeometryLocation                    m_location        = EGeometryLocation::Surface;
	CParamMod<EDD_PerParticle, SFloat10> m_offset          = 0;
	CParamMod<EDD_PerParticle, SFloat10> m_velocity        = 0;
	bool                                 m_orientToNormal  = true;
	bool                                 m_augmentLocation = false;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationGeometry, "Location", "Geometry", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationNoise

class CFeatureLocationNoise : public CParticleFeature
{
private:
	typedef TValue<THardLimits<uint, 1, 6>> UIntOctaves;

public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureLocationNoise()
		: m_amplitude(1.0f)
		, m_size(1.0f)
		, m_rate(0.0f)
		, m_octaves(1)
	{}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->InitParticles.add(this);
		pComponent->UpdateGPUParams.add(this);
		m_amplitude.AddToComponent(pComponent, this);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_amplitude, "Amplitude", "Amplitude");
		ar(m_size, "Size", "Size");
		ar(m_rate, "Rate", "Rate");
		ar(m_octaves, "Octaves", "Octaves");
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		const float maxSize = (float)(1 << 12);
		const float minSize = rcp_fast(maxSize); // small enough and prevents SIMD exceptions
		const float invSize = rcp_fast(max(minSize, +m_size));
		const float time = mod(runtime.GetEmitter()->GetTime() * m_rate * minSize, 1.0f) * maxSize;
		const float delta = m_rate * runtime.DeltaTime();
		CParticleContainer& container = runtime.GetContainer();
		const IFStream ages = container.GetIFStream(EPDT_NormalAge);
		const IFStream lifeTimes = container.GetIFStream(EPDT_LifeTime);
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);

		STempInitBuffer<float> sizes(runtime, m_amplitude);

		for (auto particleId : runtime.SpawnedRange())
		{
			const float amplitude = sizes.SafeLoad(particleId);
			const Vec3 wPosition0 = positions.Load(particleId);
			const float age = ages.Load(particleId);
			const float lifeTime = lifeTimes.Load(particleId);
			Vec4 sample;
			sample.x = wPosition0.x * invSize;
			sample.y = wPosition0.y * invSize;
			sample.z = wPosition0.z * invSize;
			sample.w = StartTime(time, delta, age * lifeTime);
			const Vec3 potential = Fractal(sample, m_octaves);
			const Vec3 wPosition1 = potential * amplitude + wPosition0;
			positions.Store(particleId, wPosition1);
		}
	}

	virtual void UpdateGPUParams(CParticleComponentRuntime& runtime, gpu_pfx2::SUpdateParams& params) override
	{
		params.amplitude = m_amplitude.GetValueRange(runtime)(0.5f);
		params.noiseSize = m_size;
		params.rate = m_rate;
		params.octaves = m_octaves;
		params.initFlags |= gpu_pfx2::eFeatureInitializationFlags_LocationNoise;
	}
	
private:
	// non-inline version with Vec4f conversion
	static float SNoise(const Vec4& v)
	{
		return ::SNoise(static_cast<const Vec4f&>(v));
	}

	ILINE static Vec3 Potential(const Vec4 sample)
	{
		const Vec4 offy = Vec4(149, 311, 191, 491);
		const Vec4 offz = Vec4(233, 197, 43, 59);
		const Vec3 potential = Vec3(
		  SNoise(sample),
		  SNoise(sample + offy),
		  SNoise(sample + offz));
		return potential;
	}

	ILINE static Vec3 Fractal(const Vec4 sample, const uint octaves)
	{
		Vec3 total = Vec3(ZERO);
		float mult = 1.0f;
		float totalMult = 0.0f;
		for (uint i = 0; i < octaves; ++i)
		{
			totalMult += mult;
			mult *= 0.5f;
		}
		mult = rcp_fast(totalMult);
		float size = 1.0f;
		for (uint i = 0; i < octaves; ++i)
		{
			total += Potential(sample * size) * mult;
			size *= 2.0f;
			mult *= 0.5f;
		}
		return total;
	}

	CParamMod<EDD_PerParticle, SFloat10> m_amplitude;
	UFloat10                             m_size;
	UFloat10                             m_rate;
	UIntOctaves                          m_octaves;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationNoise, "Location", "Noise", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationBeam

class CFeatureLocationBeam : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureLocationBeam()
		: m_source(ETargetSource::Parent)
		, m_destination(ETargetSource::Target) {}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->InitParticles.add(this);
		pComponent->GetSpatialExtents.add(this);
		pComponent->AddParticleData(EPDT_SpawnFraction);
		m_source.AddToComponent(pComponent);
		m_destination.AddToComponent(pComponent);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(m_source, "Source", "Source");
		if (ar.isInput() && GetVersion(ar) <= 5)
			ar(m_destination, "Destiny", "Destination");
		ar(m_destination, "Destination", "Destination");
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		CParticleContainer& container = runtime.GetContainer();
		const IFStream fractions = container.GetIFStream(EPDT_SpawnFraction);
		IOVec3Stream positions = container.GetIOVec3Stream(EPVF_Position);

		for (auto particleId : runtime.SpawnedRange())
		{
			const Vec3 wSource = m_source.GetTarget(runtime, particleId);
			const Vec3 wDestination = m_destination.GetTarget(runtime, particleId);
			const float fraction = fractions.SafeLoad(particleId);
			const Vec3 wPosition = wSource + (wDestination - wSource) * fraction;
			positions.Store(particleId, wPosition);
		}
	}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) override
	{
		uint numInstances = runtime.GetNumInstances();
		for (uint i = 0; i < numInstances; ++i)
		{
			TParticleId parentId = runtime.GetInstance(i).m_parentId;
			const Vec3 wSource = m_source.GetTarget(runtime, parentId, true);
			const Vec3 wDestination = m_destination.GetTarget(runtime, parentId, true);
			extents[i] += (wSource - wDestination).GetLengthFast() * scales[i];
		}
	}

private:
	CTargetSource m_source;
	CTargetSource m_destination;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationBeam, "Location", "Beam", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationBindToCamera

class CFeatureLocationBindToCamera : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->PostInitParticles.add(this);
		if (!m_spawnOnly)
			pComponent->PreUpdateParticles.add(this);
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		SERIALIZE_VAR(ar, m_spawnOnly);
	}

	virtual void PostInitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		const CCamera& camera = gEnv->p3DEngine->GetRenderingCamera();
		const QuatT wCameraPose = QuatT(camera.GetMatrix());

		CParticleContainer& container = runtime.GetContainer();
		const CParticleContainer& parentContainer = runtime.GetParentContainer();
		const auto parentIds = container.IStream(EPDT_ParentId);
		const auto parentPositions = parentContainer.IStream(EPVF_Position);
		const auto parentOrientations = parentContainer.IStream(EPQF_Orientation);
		auto positions = container.IOStream(EPVF_Position);
		auto orientations = container.IOStream(EPQF_Orientation);
		auto velocities = container.IOStream(EPVF_Velocity);

		for (auto particleId : runtime.SpawnedRange())
		{
			const TParticleId parentId = parentIds.Load(particleId);
			const Vec3 wParentPosition = parentPositions.Load(parentId);
			const Quat wParentOrientation = parentOrientations.Load(parentId);
			const QuatT worldToParent = QuatT(wParentPosition, wParentOrientation).GetInverted();

			Vec3 wPosition = positions.Load(particleId);
			wPosition = wCameraPose * (worldToParent * wPosition);
			positions.Store(particleId, wPosition);

			if (container.HasData(EPQF_Orientation))
			{
				Quat wOrientation = orientations.Load(particleId);
				wOrientation = wCameraPose.q * (worldToParent.q * wOrientation);
				orientations.Store(particleId, wOrientation);
			}

			Vec3 wVelocity = velocities.Load(particleId);
			wVelocity = wCameraPose.q * (worldToParent.q * wVelocity);
			velocities.Store(particleId, wVelocity);
		}
	}

	virtual void PreUpdateParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		const CCamera& camera = gEnv->p3DEngine->GetRenderingCamera();
		const QuatT wCurCameraPose = QuatT(camera.GetMatrix());
		const QuatT wPrevCameraPose = GetPSystem()->GetLastCameraPose();
		const Quat cameraRotation = wCurCameraPose.q * wPrevCameraPose.q.GetInverted();

		CParticleContainer& container = runtime.GetContainer();
		auto positions = container.IOStream(EPVF_Position);
		auto orientations = container.IOStream(EPQF_Orientation);
		auto velocities = container.IOStream(EPVF_Velocity);

		for (auto particleId : container.GetNonSpawnedRange())
		{
			Vec3 wPosition = positions.Load(particleId);
			wPosition = cameraRotation * (wPosition - wPrevCameraPose.t) + wCurCameraPose.t;
			positions.Store(particleId, wPosition);

			if (container.HasData(EPQF_Orientation))
			{
				Quat wOrientation = orientations.Load(particleId);
				wOrientation = cameraRotation * wOrientation;
				orientations.Store(particleId, wOrientation);
			}

			Vec3 wVelocity = velocities.Load(particleId);
			wVelocity = cameraRotation * wVelocity;
			velocities.Store(particleId, wVelocity);
		}
	}

private:
	bool m_spawnOnly = false;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationBindToCamera, "Location", "BindToCamera", colorLocation);

//////////////////////////////////////////////////////////////////////////
// CFeatureLocationOmni

extern TDataType<Vec3> EPVF_PositionPrev;

using Matrix34v = Matrix34_tpl<floatv>;

static CubeRootApprox cubeRootApprox(0.125f);

ILINE void DoElements(int mask, const Vec3v& vv, std::function<void(const Vec3& v)> func)
{
#ifdef CRY_PFX2_USE_SSE
	static_assert(CRY_PFX2_GROUP_STRIDE == 4, "Particle data vectorization != 4");
	if (mask & 1)
		func(Vec3(get_element<0>(vv.x), get_element<0>(vv.y), get_element<0>(vv.z)));
	if (mask & 2)
		func(Vec3(get_element<1>(vv.x), get_element<1>(vv.y), get_element<1>(vv.z)));
	if (mask & 4)
		func(Vec3(get_element<2>(vv.x), get_element<2>(vv.y), get_element<2>(vv.z)));
	if (mask & 8)
		func(Vec3(get_element<3>(vv.x), get_element<3>(vv.y), get_element<3>(vv.z)));
#else
	if (mask)
		func(vv);
#endif
}

class CFeatureLocationOmni : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pComponent->OnPreRun.add(this);
		pComponent->CullSubInstances.add(this);
		pComponent->GetSpatialExtents.add(this);
		pComponent->KillParticles.add(this);
		pComponent->SpawnParticles.add(this);
		pComponent->InitParticles.add(this);
		m_visibilityRange.AddToComponent(pComponent, this);

		if (m_spawnOutsideView)
			pParams->m_isPreAged = true;

		if (!m_useEmitterLocation)
		{
			// auto-set distance culling
			const float visibility = m_visibilityRange.GetValueRange().end;
			auto& maxCamDistance = pParams->m_visibility.m_maxCameraDistance;
			if (visibility < maxCamDistance)
				maxCamDistance = visibility;
		}
	}

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		SERIALIZE_VAR(ar, m_visibilityRange);
		if (ar.isInput() && GetVersion(ar) < 14)
			ar(m_visibilityRange, "Visibility", "Visibility");
		SERIALIZE_VAR(ar, m_spawnOutsideView);
	#ifndef _RELEASE
		SERIALIZE_VAR(ar, m_useEmitterLocation);
	#endif
	}

	virtual void OnPreRun(CParticleComponentRuntime& runtime, uint numParticles) override
	{
		CParticleContainer& container = runtime.GetContainer();
		auto positions = container.IStream(EPVF_Position);
		auto positionsPrev = container.IStream(EPVF_PositionPrev, runtime.GetEmitter()->GetLocation().t);
		auto velocities = container.IStream(EPVF_Velocity);

		m_averageData.numParticles = numParticles;
		m_averageData.velocityFinal.zero();
		m_averageData.vectorTravel.zero();
		for (auto particleId : runtime.FullRange())
		{
			m_averageData.velocityFinal += velocities.Load(particleId);
			m_averageData.vectorTravel += positions.Load(particleId) - positionsPrev.SafeLoad(particleId);
		}
		m_averageData.velocityFinal /= (float)container.GetNumParticles();
		m_averageData.vectorTravel /= (float)container.GetNumParticles();
	}

	virtual void CullSubInstances(CParticleComponentRuntime& runtime, TDynArray<SInstance>& instances) override
	{
		// Allow only one instance
		uint numAllowed = 1 - runtime.GetNumInstances();
		if (numAllowed < instances.size())
			instances.resize(numAllowed);
	}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) override
	{
		UpdateCameraData(runtime);

		const uint numInstances = runtime.GetNumInstances();
		for (uint i = 0; i < numInstances; ++i)
		{
			float scale = m_camData.maxDistance * scales[i];
			const Vec3 boxUnit(m_camData.scrWidth.x, m_camData.scrWidth.y, 1.0f);
			float capHeight = 1.0f - boxUnit.GetInvLength();
			float extent = (capHeight * scale) * sqr(scale + 1.0f) * 4.0f / 3.0f;
			extents[i] += extent;
		}
	}

	virtual void KillParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& container = runtime.GetContainer();
		auto ages = container.IOStream(EPDT_NormalAge);
		auto positions = container.IStream(EPVF_Position);

		UpdateCameraData(runtime);

		// All particles no longer in sector are killed (age -> 1)
		Matrix34v fromWorld = m_camData.fromWorld;
		for (auto particleId : runtime.FullRangeV())
		{
			floatv age = ages.Load(particleId);
			Vec3v pos = positions.Load(particleId);
			Vec3v posCam = fromWorld * pos;
			age = if_else(InSector(posCam), age, convert<floatv>(1.0f));
			ages.Store(particleId, age);
		}
	}

	virtual void SpawnParticles(CParticleComponentRuntime& runtime, TDynArray<SSpawnEntry>& spawnEntries) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		if (runtime.IsPreRunning())
			return;

		UpdateCameraData(runtime);

		Vec3 travelPrev = m_averageData.velocityFinal * m_camData.deltaTimePrev;
		Matrix34v toPrev = m_camData.fromWorldPrev * Matrix34::CreateTranslationMat(-travelPrev) * m_camData.toWorld;
		Matrix34v toWorld = m_camData.toWorld;

		// Randomly generate positions in current sector; only those not in previous sector spawn as particles
		for (uint i = CRY_PFX2_GROUP_ALIGN(m_averageData.numParticles); i > 0; i -= CRY_PFX2_GROUP_STRIDE)
		{
			Vec3v posCam = RandomSector<floatv>(runtime.ChaosV());
			Vec3v posCamPrev = toPrev * posCam;
			int mask = Any(~InSector(posCamPrev));
			if (mask)
			{
				Vec3v posv = toWorld * posCam;
				DoElements(mask, posv, [this](const Vec3& v) { m_newPositions.push_back(v); });
			}
		}
		
		if (m_newPositions.size())
		{
			const float life = runtime.ComponentParams().m_maxParticleLife;
			float fracNewSpawned = runtime.DeltaTime() / life;
			SSpawnEntry spawn = {};
			spawn.m_count = m_newPositions.size();
			spawn.m_count = uint(spawn.m_count * (1.0f - fracNewSpawned));
			spawn.m_ageBegin = 0.0f;
			spawn.m_ageIncrement = life / float(spawn.m_count);
			spawnEntries.push_back(spawn);
		}
	}

	virtual void InitParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PFX2_PROFILE_DETAIL;

		if (runtime.IsPreRunning())
			return;

		CParticleContainer& container = runtime.GetContainer();
		auto positions = container.IOStream(EPVF_Position);
		auto positionsPrev = container.IOStream(EPVF_PositionPrev);
		auto velocities = container.IOStream(EPVF_Velocity);
		auto normAges = container.IOStream(EPDT_NormalAge);

		const bool hasPositionPrev = container.HasData(EPVF_PositionPrev);

		UpdateCameraData(runtime);

		float lifeFraction = 1.5f * m_camData.maxDistance * m_averageData.vectorTravel.GetInvLengthSafe();
		float curAge = max(1.0f - 2.0f * lifeFraction, 0.0f);
		float ageInc = lifeFraction / runtime.SpawnedRange().size();

		uint nNew = 0;

		for (auto particleId : runtime.SpawnedRange())
		{
			Vec3 pos;
			if (nNew < m_newPositions.size())
			{
				pos = m_newPositions[nNew++];
			}
			else
			{
				Vec3 posCam = RandomSector<float>(runtime.Chaos());
				pos = m_camData.toWorld * posCam;
			}
			positions.Store(particleId, pos);
			if (m_spawnOutsideView)
			{
				if (hasPositionPrev)
					positionsPrev.Store(particleId, pos - m_averageData.vectorTravel);

				Vec3 vel = velocities.Load(particleId);
				vel += m_averageData.velocityFinal;
				velocities.Store(particleId, vel);

				normAges[particleId] = curAge;
				curAge += ageInc;
			}
		}
		m_newPositions.resize(0);

		m_camData.deltaTimePrev = runtime.DeltaTime();
	}


private:

	// Camera data is shared per-effect, based on the global render camera and effect params
	struct SCameraData
	{
		int32     nFrameId      {-1};
		Vec2      scrWidth      {0};
		float     maxDistance   {0};
		Matrix34  toWorld       {IDENTITY};
		Matrix34  fromWorld     {IDENTITY};
		Matrix34  fromWorldPrev {IDENTITY};
		float     deltaTimePrev {0};
	};
	SCameraData m_camData;

	struct SComponentData
	{
		uint numParticles;
		Vec3 velocityFinal;
		Vec3 vectorTravel;
	};
	SComponentData  m_averageData;

	TDynArray<Vec3> m_newPositions;

	void UpdateCameraData(const CParticleComponentRuntime& runtime)
	{
		if (gEnv->nMainFrameID == m_camData.nFrameId)
			return;
		CCamera cam = GetEffectCamera(runtime);
		m_camData.maxDistance = m_visibilityRange.GetValueRange(runtime).end;
		m_camData.maxDistance = min(m_camData.maxDistance, +runtime.ComponentParams().m_visibility.m_maxCameraDistance);

		// Clamp view distance based on particle size
		const float maxSize = runtime.ComponentParams().m_maxParticleSize;
		const float maxParticleSize = maxSize * runtime.ComponentParams().m_visibility.m_viewDistanceMultiple;
		const float maxAngularDensity = GetPSystem()->GetMaxAngularDensity(cam);
		const float maxDistance = maxAngularDensity * maxParticleSize * runtime.GetEmitter()->GetViewDistRatio();
		if (maxDistance < m_camData.maxDistance)
			m_camData.maxDistance = maxDistance;

		m_camData.fromWorldPrev = m_camData.fromWorld;
		m_camData.scrWidth = ScreenWidth(cam);

		// Matrix rotates Z to Y, scales by range, and offsets backward by particle size
		const Matrix34 toCam(
			m_camData.maxDistance * m_camData.scrWidth.x, 0, 0, 0,
			0, 0, m_camData.maxDistance, -maxSize,
			0, -m_camData.maxDistance * m_camData.scrWidth.y, 0, 0
		);
		// Concatenate with camera world location
		m_camData.toWorld = cam.GetMatrix() * toCam;
		m_camData.fromWorld = m_camData.toWorld.GetInverted();
		if (m_camData.nFrameId == -1)
			m_camData.fromWorldPrev = m_camData.fromWorld;
		m_camData.nFrameId = gEnv->nMainFrameID;
		m_camData.deltaTimePrev = runtime.DeltaTime();
	}

	CCamera GetEffectCamera(const CParticleComponentRuntime& runtime) const
	{
		if (!m_useEmitterLocation)
		{
			return gEnv->p3DEngine->GetRenderingCamera();
		}
		else
		{
			CCamera camera = gEnv->p3DEngine->GetRenderingCamera();
			const CParticleEmitter& emitter = *runtime.GetEmitter();
			Matrix34 matEmitter = Matrix34(emitter.GetLocation());
			camera.SetMatrix(matEmitter);
			return camera;
		}
	}

	static Vec2 ScreenWidth(const CCamera& cam)
	{
		Vec3 corner = cam.GetEdgeF();
		return Vec2(abs(corner.x), abs(corner.z)) / corner.y;
	}

	uint InSector(const Vec3& pos) const
	{
		return max(abs(pos.x), abs(pos.y)) <= pos.z
			&& pos.len2() <= 1.0f;
	}

#ifdef CRY_PFX2_USE_SSE
	mask32v4 InSector(const Vec3v& pos) const
	{
		return max(abs(pos.x), abs(pos.y)) <= pos.z
			& pos.len2() <= convert<floatv>(1.0f);
	}

	template<int elem>
	void AddElem(int mask, const Vec3v& v)
	{
		if (mask & (1<<elem))
		{
			m_newPositions.emplace_back(get_element<elem>(v.x), get_element<elem>(v.y), get_element<elem>(v.z));
		}
	}
#endif

	template<typename T, typename TChaos>
	Vec3_tpl<T> RandomSector(TChaos& chaos) const
	{
		Vec3_tpl<T> pos(
			chaos.RandSNorm(),
			chaos.RandSNorm(),
			convert<T>(1.0f));
		T r = cubeRootApprox(chaos.RandUNorm());
		pos *= r * pos.GetInvLengthFast();
		return pos;
	}

	// Parameters
	CParamMod<EDD_InstanceUpdate, UFloat100> m_visibilityRange;
	bool                                     m_spawnOutsideView = false;

	// Debugging options
	bool                                     m_useEmitterLocation = false;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureLocationOmni, "Location", "Omni", colorLocation);

}
