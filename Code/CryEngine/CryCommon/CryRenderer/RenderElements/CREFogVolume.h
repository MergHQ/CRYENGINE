// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _CREFOGVOLUME_
#define _CREFOGVOLUME_

#pragma once

#include <CryRenderer/VertexFormats.h>

struct IFogVolumeRenderNode;

class CREFogVolume : public CRendElementBase
{
public:
	CREFogVolume();

	virtual ~CREFogVolume();
	virtual void mfPrepare(bool bCheckOverflow);
	virtual bool mfDraw(CShader* ef, SShaderPass* sfm);

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	Vec3     m_center;
	uint32   m_viewerInsideVolume  : 1;
	uint32   m_affectsThisAreaOnly : 1;
	uint32   m_stencilRef          : 8;
	uint32   m_volumeType          : 1;
	uint32   m_reserved            : 21;
	AABB     m_localAABB;
	Matrix34 m_matWSInv;
	float    m_globalDensity;
	float    m_densityOffset;
	float    m_nearCutoff;
	Vec2     m_softEdgesLerp;
	ColorF   m_fogColor;              //!< Color already combined with fHDRDynamic.
	Vec3     m_heightFallOffDirScaled;
	Vec3     m_heightFallOffBasePoint;
	Vec3     m_eyePosInWS;
	Vec3     m_eyePosInOS;
	Vec3     m_rampParams;
	Vec3     m_windOffset;
	float    m_noiseScale;
	Vec3     m_noiseFreq;
	float    m_noiseOffset;
	float    m_noiseElapsedTime;
	Vec3     m_emission;
};

#endif // #ifndef _CREFOGVOLUME_
