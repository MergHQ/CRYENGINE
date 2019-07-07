// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "BreezeGenerator.h"

namespace
{
static inline Vec3 RandomPosition(const Vec3& centre, const Vec3& wind_speed, float life_time, float radius, float extents)
{
	Vec3 result;
	result.x = centre.x - (wind_speed.x * life_time * 0.5f) + cry_random(-radius, radius);
	result.y = centre.y - (wind_speed.y * life_time * 0.5f) + cry_random(-radius, radius);
	result.z = centre.z - (wind_speed.z * life_time * 0.5f) + cry_random(-radius, radius);
	return result;
}

static inline Vec3 RandomDirection(const Vec3& dir, float spread)
{
	return Matrix33::CreateRotationZ(DEG2RAD(cry_random(-spread, spread))) * dir;
}
}

struct SBreeze
{
	IPhysicalEntity* wind_area;
	Vec3             position;
	Vec3             direction;
	float            lifetime;

	SBreeze()
		: wind_area()
		, position()
		, direction()
		, lifetime(-1.f)
	{}
};

CBreezeGenerator::CBreezeGenerator()
	: m_breezes(nullptr)
{
	m_params.breezeSpawnRadius = 0.0f;
	m_params.breezeSpread = 0.0f;
	m_params.breezeCount = 0;
	m_params.breezeRadius = 0.0f;
	m_params.breezeLifeTime = 0.0f;
	m_params.breezeVariance = 0.0f;
	m_params.breezeStrength = 0.0f;
	m_params.breezeMovementSpeed = 0.0f;
	m_params.windVector = Vec3(ZERO);
	m_params.breezeFixedHeight = -1.0f;
	m_params.breezeAwakeThreshold = 0.0f;	
	m_params.breezeGenerationEnabled = false;
}

CBreezeGenerator::~CBreezeGenerator()
{
	Shutdown();
}

void CBreezeGenerator::SetParams(const BreezeGeneratorParams& params)
{
	const bool wasEnabled = m_params.breezeGenerationEnabled;
	const bool shouldEnable = params.breezeGenerationEnabled;

	if (!shouldEnable)
	{
		Shutdown();
	}

	m_params = params;
	
	if (!wasEnabled && shouldEnable)
	{
		Initialize();
	}
}

BreezeGeneratorParams CBreezeGenerator::GetParams() const
{
	return m_params;
}

void CBreezeGenerator::Initialize()
{
	if (m_params.breezeGenerationEnabled && m_params.breezeCount)
	{
		m_breezes = new SBreeze[m_params.breezeCount];
		for (uint32 i = 0; i < m_params.breezeCount; ++i)
		{
			primitives::box geomBox;
			geomBox.Basis.SetIdentity();
			geomBox.center = Vec3(0, 0, 0);
			geomBox.size = Vec3(
			  cry_random(m_params.breezeRadius, m_params.breezeRadius + m_params.breezeVariance),
			  cry_random(m_params.breezeRadius, m_params.breezeRadius + m_params.breezeVariance),
			  cry_random(m_params.breezeRadius, m_params.breezeRadius + m_params.breezeVariance));
			geomBox.bOriented = 0;
			IGeometry* pGeom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::box::type, &geomBox);
			m_breezes[i].wind_area = gEnv->pPhysicalWorld->AddArea(pGeom, Vec3(0, 0, 0), Quat::CreateIdentity(), 1.f);

			pe_params_buoyancy buoyancy;
			buoyancy.waterDensity = 1.f;
			buoyancy.waterResistance = 1.f;
			buoyancy.waterFlow = m_params.windVector * cry_random(0.0f, m_params.breezeStrength);
			buoyancy.iMedium = 1;
			buoyancy.waterPlane.origin.Set(0, 0, 1e6f);
			buoyancy.waterPlane.n.Set(0, 0, -1);
			m_breezes[i].wind_area->SetParams(&buoyancy);

			pe_params_area area;
			area.bUniform = 1;
			area.falloff0 = 0.80f;
			m_breezes[i].wind_area->SetParams(&area);

			pe_params_foreign_data fd;
			fd.iForeignFlags = PFF_OUTDOOR_AREA;
			m_breezes[i].wind_area->SetParams(&fd);
		}
	}
}

void CBreezeGenerator::Reset()
{
	Shutdown();
	Initialize();
}

void CBreezeGenerator::Shutdown()
{
	if (m_params.breezeGenerationEnabled)
		for (uint32 i = 0; i < m_params.breezeCount; ++i)
			gEnv->pPhysicalWorld->DestroyPhysicalEntity(m_breezes[i].wind_area);

	delete[] m_breezes;
	m_breezes = NULL;
	m_params.breezeCount = 0;
}

void CBreezeGenerator::Update()
{
	CRY_PROFILE_FUNCTION(PROFILE_3DENGINE);
	if (!m_params.breezeGenerationEnabled)
		return;

	pe_params_buoyancy buoyancy;
	buoyancy.waterDensity = 1.f;
	buoyancy.waterResistance = 1.f;
	buoyancy.iMedium = 1;

	pe_params_pos pp;
	const float frame_time = GetTimer()->GetFrameTime();
	const Vec3 centre = Get3DEngine()->GetRenderingCamera().GetPosition();
	for (uint32 i = 0; i < m_params.breezeCount; ++i)
	{
		SBreeze& breeze = m_breezes[i];
		breeze.lifetime -= frame_time;
		if (breeze.lifetime < 0.f)
		{
			Vec3 pos = RandomPosition(centre, m_params.windVector, m_params.breezeLifeTime, m_params.breezeSpawnRadius, m_params.breezeRadius);
			if (m_params.breezeFixedHeight != -1.f)
				pos.z = m_params.breezeFixedHeight;
			else
				pos.z = Get3DEngine()->GetTerrainElevation(pos.x, pos.y) - m_params.breezeRadius * 0.5f;
			breeze.position = pp.pos = pos;
			buoyancy.waterFlow = breeze.direction = RandomDirection(m_params.windVector, m_params.breezeSpread) * cry_random(0.0f, m_params.breezeStrength);
			breeze.wind_area->SetParams(&pp);
			breeze.wind_area->SetParams(&buoyancy);
			breeze.lifetime = m_params.breezeLifeTime * cry_random(0.5f, 1.0f);
		}
		else
		{
			pp.pos = (breeze.position += breeze.direction * m_params.breezeMovementSpeed * frame_time);
			if (m_params.breezeFixedHeight != -1.f)
				pp.pos.z = m_params.breezeFixedHeight;
			else
				pp.pos.z = Get3DEngine()->GetTerrainElevation(pp.pos.x, pp.pos.y) - m_params.breezeRadius * 0.5f;
			breeze.wind_area->SetParams(&pp);

			if (m_params.breezeAwakeThreshold)
			{
				pe_params_bbox pbb;
				breeze.wind_area->GetParams(&pbb);
				Vec3 sz = (pbb.BBox[1] - pbb.BBox[0]) * 0.5f;
				float kv = m_params.breezeStrength * buoyancy.waterResistance;
				IPhysicalEntity* pentBuf[128], ** pents = pentBuf;
				pe_params_part ppart;
				pe_action_awake aa;
				for (int j = gEnv->pPhysicalWorld->GetEntitiesInBox(pp.pos - sz, pp.pos + sz, pents, ent_sleeping_rigid | ent_allocate_list, 128) - 1; j >= 0; j--)
					for (ppart.ipart = 0; pents[j]->GetParams(&ppart); ppart.ipart++)
						if (sqr(ppart.pPhysGeom->V * cube(ppart.scale)) * cube(kv) > cube(ppart.mass * m_params.breezeAwakeThreshold))
						{
							pents[j]->Action(&aa);
							break;
						}
				if (pents != pentBuf)
					gEnv->pPhysicalWorld->GetPhysUtils()->DeletePointer(pents);
			}
		}
	}
}
