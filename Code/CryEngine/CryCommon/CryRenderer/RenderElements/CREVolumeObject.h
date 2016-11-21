// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _CREVOLUMEOBJECT_
#define _CREVOLUMEOBJECT_

#pragma once

#include <CryRenderer/VertexFormats.h>

struct IVolumeObjectRenderNode;

class CREVolumeObject : public CRenderElement
{
public:
	struct IVolumeTexture
	{
	public:
		virtual void Release() = 0;
		virtual bool Create(unsigned int width, unsigned int height, unsigned int depth, unsigned char* pData) = 0;
		virtual bool Update(unsigned int width, unsigned int height, unsigned int depth, const unsigned char* pData) = 0;
		virtual int  GetTexID() const = 0;

	protected:
		virtual ~IVolumeTexture() {}
	};

public:
	CREVolumeObject();

	virtual ~CREVolumeObject();
	virtual void            mfPrepare(bool bCheckOverflow);
	virtual bool            mfDraw(CShader* ef, SShaderPass* sfm);

	virtual IVolumeTexture* CreateVolumeTexture() const;

	virtual void            GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	Vec3                    m_center;
	Matrix34                m_matInv;
	Vec3                    m_eyePosInWS;
	Vec3                    m_eyePosInOS;
	Plane                   m_volumeTraceStartPlane;
	AABB                    m_renderBoundsOS;
	bool                    m_viewerInsideVolume;
	bool                    m_nearPlaneIntersectsVolume;
	float                   m_alpha;
	float                   m_scale;

	IVolumeTexture*         m_pDensVol;
	IVolumeTexture*         m_pShadVol;
	_smart_ptr<IRenderMesh> m_pHullMesh;
};

#endif // #ifndef _CREVOLUMEOBJECT_
