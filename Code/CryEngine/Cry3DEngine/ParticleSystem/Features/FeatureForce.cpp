// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

#include "StdAfx.h"
#include "FeatureCommon.h"


namespace pfx2
{

//////////////////////////////////////////////////////////////////////////
// CFeatureForce

// Generates a physical force
// PFX2_TODO: Force Manual mode does not work per-spawner, because there are no spawner-field modifiers.
//   Either support spawner-field modifiers, such as Age;
//   Or implement per-particle forces (as with lights and audio).

SERIALIZATION_ENUM_DECLARE(EForceMode,,
	AutoFromParticles,
	Manual
);
SERIALIZATION_ENUM_DECLARE(EForceType,,
	Wind,
	Gravity
);
SERIALIZATION_ENUM_DECLARE(EShapeType,,
	Box,
	Sphere
);
SERIALIZATION_ENUM_DECLARE(EIgnore,,
	None,
	ThisComponent,
	ThisEffect
);

// Per-spawner data
using PPhysEntity = IPhysicalEntity*;

MakeDataType(ESDT_ForceEntity, PPhysEntity, EDD_Spawner);
MakeDataType(ESDT_ForceMag, float, EDD_SpawnerUpdate);

struct SForceParams
{
	Vec3       center;
	Vec3       size;
	UUnitFloat innerFallof;
	Vec4       vector;
};

// Data for auto mode
MakeDataType(ESDT_ForceParams, SForceParams, EDD_Spawner);

// Data for manual mode
MakeDataType(ESDT_ForceSize, float, EDD_SpawnerUpdate);

class CFeatureForce : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE;

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		// SERIALIZE_VAR(ar, m_mode);
		SERIALIZE_VAR(ar, m_type);
		SERIALIZE_VAR(ar, m_ignore);
		SERIALIZE_VAR(ar, m_magnitude);

		if (m_mode == EForceMode::AutoFromParticles)
		{
			SERIALIZE_VAR(ar, m_scaleByCount);
		}
		else
		{
			SERIALIZE_VAR(ar, m_shape);
			if (m_shape == EShapeType::Box)
				SERIALIZE_VAR(ar, m_boxSize);
			SERIALIZE_VAR(ar, m_size);
			SERIALIZE_VAR(ar, m_innerFallof);
			SERIALIZE_VAR(ar, m_direction);
		}
	}

	virtual void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		pParams->m_keepParentAlive = true;
		pComponent->AddParticleData(ESDT_ForceEntity);
		pComponent->MainPreUpdate.add(this);
		pComponent->InitSpawners.add(this);
		pComponent->DestroySpawners.add(this);

		if (m_mode == EForceMode::AutoFromParticles)
		{
			pComponent->AddParticleData(EPDT_SpawnerId);
			pComponent->AddParticleData(ESDT_ForceParams);
			m_magnitude.AddToComponent(pComponent, ESDT_ForceMag);
			pComponent->PostUpdateParticles.add(this);
		}
		else
		{
			m_size.AddToComponent(pComponent, ESDT_ForceSize);
		}
	}

	void InitSpawners(CParticleComponentRuntime& runtime) override
	{
		auto forces = runtime.IOStream(ESDT_ForceEntity);
		forces.Fill(runtime.SpawnedRange(EDD_Spawner), nullptr);
	}

	void DestroySpawners(CParticleComponentRuntime& runtime, TConstArray<TParticleId> ids) override
	{
		auto forces = runtime.IStream(ESDT_ForceEntity);
		for (auto sid : ids)
		{
			if (auto pForce = forces.Load(sid))
			{
				gEnv->pPhysicalWorld->DestroyPhysicalEntity(pForce);
			}
		}
	}

	virtual void PostUpdateParticles(CParticleComponentRuntime& runtime) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		auto const& spawners = runtime.Container(EDD_Spawner);
		if (spawners.Size() == 0)
			return;

		// Accumulate data for each force
		struct SForceData
		{
			AABB bbInner {AABB::RESET}, bbOuter {AABB::RESET};
			Vec4 velSum {0};
			uint count = 0;
		};
		THeapArray<SForceData> forceData(runtime.MemHeap(), spawners.Size());
		THeapArray<QuatT> locations(runtime.MemHeap(), spawners.Size());
		runtime.GetEmitLocations(locations);
		THeapArray<Matrix34> invLocations(runtime.MemHeap(), spawners.Size());
		for (uint sid = 0; sid < locations.size(); ++sid)
			invLocations[sid] = Matrix34(locations[sid].GetInverted());

		auto spawnerIds = runtime.IStream(EPDT_SpawnerId);
		auto positions = runtime.IStream(EPVF_Position);
		auto sizes = runtime.IStream(EPDT_Size);
		auto velocities = runtime.IStream(EPVF_Velocity);

		// Get local bounds
		for (auto id : runtime.FullRange(EDD_Particle))
		{
			const TParticleId sid = spawnerIds[id];
			if (!spawners.IdIsValid(sid))
				continue;
			SForceData& data = forceData[sid];

			const float size = sizes.Load(id);
			const Vec3 posW = positions.Load(id);
			const Vec3 pos = invLocations[sid] * posW;
			data.bbInner.Add(pos);
			data.bbOuter.Add(pos, size);

			const Vec3 vel = velocities.Load(id);
			const float posSq = pos.GetLengthSquared();
			const float radialSpeed = posSq != 0.0f ? (vel | pos) * rsqrt_fast(posSq) : 0.0f;

			data.velSum += Vec4(vel, radialSpeed);

			data.count++;
		}

		// Process force data
		auto forceParams = runtime.IOStream(ESDT_ForceParams);
		auto mags = runtime.IStream(m_magnitude.DataType());

		for (auto sid : spawners.FullRange())
		{
			const auto& data = forceData[sid];
			auto& params = forceParams[sid];

			if (data.count)
			{
				params.vector = Vec4(invLocations[sid].TransformVector(data.velSum), data.velSum.w);
				if (!m_scaleByCount)
					params.vector /= float(data.count);
				params.vector *= mags[sid];

				params.center = data.bbOuter.GetCenter();
				params.size = data.bbOuter.GetSize() * 0.5f;

				const float inner = data.bbInner.GetRadius(), outer = data.bbOuter.GetRadius();
				params.innerFallof = max(inner + inner - outer, 0.0f) / outer;
			}
			else
			{
				params.vector = Vec4(0);
			}
		}
	}

	virtual void MainPreUpdate(CParticleComponentRuntime& runtime) override
	{
		CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

		auto& container = runtime.Container(EDD_Spawner);

		auto forces = container.IOStream(ESDT_ForceEntity);

		THeapArray<QuatT> locations(runtime.MemHeap(), container.Size());
		runtime.GetEmitLocations(locations, 0);

		if (m_mode == EForceMode::AutoFromParticles)
		{
			// Use values computed during last PostUpdateParticles
			auto forceParams = runtime.IStream(ESDT_ForceParams);
			for (auto sid : container.FullRange())
			{
				UpdateForce(runtime, forces[sid], locations[sid], forceParams[sid]);
			}
		}
		else
		{
			SForceParams force;
			force.center.zero();
			force.innerFallof = m_innerFallof;

			auto mags = container.IStream(m_magnitude.DataType());
			auto sizes = container.IStream(m_size.DataType());

			for (auto sid : container.FullRange())
			{
				force.size = m_boxSize * sizes[sid];
				force.vector = m_direction * mags[sid];

				UpdateForce(runtime, forces[sid], locations[sid], force);
			}
		}
	}

protected:

	// Apply force params to update a physical force
	void UpdateForce(CParticleComponentRuntime& runtime, PPhysEntity& pEntity, const QuatT& location, const SForceParams& force) const
	{
		if (force.vector.GetLengthSquared() == 0.0f)
		{
			// No force.
			if (pEntity)
			{
				gEnv->pPhysicalWorld->DestroyPhysicalEntity(pEntity);
				pEntity = nullptr;
			}
			return;
		}

		//
		// Create physical area for force.
		//

		primitives::primitive* pPrim = nullptr;
		int primType = 0;
		primitives::box geomBox;
		primitives::sphere geomSphere;

		switch (m_shape)
		{
		case EShapeType::Box:
			primType = geomBox.type;
			pPrim = &geomBox;
			geomBox.Basis.SetIdentity();
			geomBox.bOriented = 0;
			geomBox.center = force.center;
			geomBox.size = force.size;
			break;
		case EShapeType::Sphere:
			primType = geomSphere.type;
			pPrim = &geomSphere;
			geomSphere.center = force.center;
			geomSphere.r = force.size.len();
			break;
		}

		pe_params_area area;

		if (!pEntity)
		{
			// Make new entity
			area.pGeom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primType, pPrim);
			pEntity = gEnv->pPhysicalWorld->AddArea(area.pGeom, location.t, location.q, 1.0f);
			if (!pEntity)
				return;

			pEntity->GetParams(&area);

			if (m_ignore != EIgnore::None)
			{
				// Tag area with this emitter, so we can ignore it in the emitter family.
				pe_params_foreign_data fd;
				fd.iForeignData = fd.iForeignFlags = 0;
				if (m_ignore == EIgnore::ThisComponent)
					fd.pForeignData = &runtime;
				else if (m_ignore == EIgnore::ThisEffect)
					fd.pForeignData = runtime.GetEmitter();
				pEntity->SetParams(&fd);
			}
		}
		else
		{
			// Update location
			pe_params_pos pos;
			pEntity->GetParams(&pos);
			if (!IsEquivalent(pos.pos, location.t, 0.01f)
				|| !IsEquivalent(pos.q, location.q))
			{
				pos.pos = location.t;
				pos.q = location.q;
				pos.scale = 1.0f;
				pEntity->SetParams(&pos);
			}

			// Update geometry shape
			pEntity->GetParams(&area);
			CRY_ASSERT(area.pGeom);
			primitives::box curBox;
			area.pGeom->GetBBox(&curBox);

			if (!IsEquivalent(curBox.center, geomBox.center, 0.01f))
				area.pGeom->SetData(pPrim);
			else if (m_shape == EShapeType::Box)
			{
				if (!IsEquivalent(curBox.size, geomBox.size, 0.01f))
					area.pGeom->SetData(pPrim);
			}
			else if (m_shape == EShapeType::Sphere)
			{
				if (!IsEquivalent(curBox.size.len(), geomSphere.r, 0.01f))
					area.pGeom->SetData(pPrim);
			}
		}

		area.falloff0 = force.innerFallof;

		// Determine whether force should be directional or radial
		// TODO: Support both in one area
		Vec3 forceVec;
		float dirLenSqr = force.vector.GetLengthSquared();
		area.bUniform = (dirLenSqr > 2.0f * sqr(force.vector.w)) * 2;
		if (area.bUniform)
		{
			// Directional
			forceVec = force.vector;
		}
		else
		{
			// Radial
			forceVec.z = force.vector.w;
			forceVec.x = forceVec.y = 0.f;
		}

		if (m_type == EForceType::Gravity)
			area.gravity = forceVec;
		pEntity->SetParams(&area);

		if (m_type == EForceType::Wind)
		{
			const SPhysEnviron& physEnv = runtime.GetEmitter()->GetPhysicsEnv();
			pe_params_buoyancy buoy;
			buoy.iMedium = 1; // air
			buoy.waterDensity = buoy.waterResistance = 1;
			buoy.waterFlow = forceVec;
			buoy.waterPlane.n = -physEnv.m_UniformForces.plWater.n;
			buoy.waterPlane.origin = physEnv.m_UniformForces.plWater.n * physEnv.m_UniformForces.plWater.d;
			pEntity->SetParams(&buoy);
		}
	}

	// Options
	EForceMode m_mode   = EForceMode::AutoFromParticles;
	EForceType m_type   = EForceType::Wind;
	EIgnore    m_ignore = EIgnore::ThisComponent;

	// Data param
	CParamData<EDD_SpawnerUpdate, SFloat> m_magnitude = 1;

	// For mode = AutoFromParticles
	bool m_scaleByCount = false;

	// For mode = Manual
	EShapeType m_shape         = EShapeType::Box;
	Vec3       m_boxSize       {0};
	UUnitFloat m_innerFallof;
	Vec4       m_direction     {0,0,0,1};

	// Data params
	CParamData<EDD_SpawnerUpdate, UFloat> m_size;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureForce, "Aux", "Force", colorAux);

}
