// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryRenderer/RenderElements/CREVolumeObject.h>
#include <CryEntitySystem/IEntityRenderState.h> // <> required for Interfuscator

#include "../../XRenderD3D9/DriverD3D.h"

//////////////////////////////////////////////////////////////////////////
//

class CVolumeTexture : public CREVolumeObject::IVolumeTexture
{
public:
	virtual void Release();
	virtual bool Create(unsigned int width, unsigned int height, unsigned int depth, unsigned char* pData);
	virtual bool Update(unsigned int width, unsigned int height, unsigned int depth, const unsigned char* pData);
	virtual int  GetTexID() const;

	CVolumeTexture();
	~CVolumeTexture();

private:
	unsigned int m_width;
	unsigned int m_height;
	unsigned int m_depth;
	CTexture*    m_pTex;
};

CVolumeTexture::CVolumeTexture()
	: m_width(0)
	, m_height(0)
	, m_depth(0)
	, m_pTex(0)
{
}

CVolumeTexture::~CVolumeTexture()
{
	if (m_pTex)
		gRenDev->RemoveTexture(m_pTex->GetTextureID());
}

void CVolumeTexture::Release()
{
	delete this;
}

bool CVolumeTexture::Create(unsigned int width, unsigned int height, unsigned int depth, unsigned char* pData)
{
	assert(!m_pTex);
	if (!m_pTex)
	{
		char name[128];
		cry_sprintf(name, "$VolObj_%d", gRenDev->m_TexGenID++);

		int flags(FT_DONT_STREAM);
		m_pTex = CTexture::Create3DTexture(name, width, height, depth, 1, flags, pData, eTF_A8, eTF_A8);

		m_width = width;
		m_height = height;
		m_depth = depth;
	}
	return m_pTex != 0;
}

bool CVolumeTexture::Update(unsigned int width, unsigned int height, unsigned int depth, const unsigned char* pData)
{
	if (!CTexture::IsTextureExist(m_pTex))
		return false;

	unsigned int cpyWidth = min(width, m_width);
	unsigned int cpyHeight = min(height, m_height);
	unsigned int cpyDepth = min(depth, m_depth);

	D3D11_BOX box;
	box.left = 0;
	box.right = cpyWidth;
	box.top = 0;
	box.bottom = cpyHeight;
	box.front = 0;
	box.back = cpyDepth;

	D3DVolumeTexture* pVolTex = m_pTex->GetDevTexture()->GetVolumeTexture();
	gcpRendD3D->GetDeviceContext().UpdateSubresource(pVolTex, 0, &box, pData, width, height * width);

	return true;
}

int CVolumeTexture::GetTexID() const
{
	return m_pTex ? m_pTex->GetTextureID() : 0;
}

//////////////////////////////////////////////////////////////////////////
//

CREVolumeObject::CREVolumeObject()
	: CRendElementBase()
	, m_center(0, 0, 0)
	, m_matInv()
	, m_eyePosInWS(0, 0, 0)
	, m_eyePosInOS(0, 0, 0)
	, m_volumeTraceStartPlane(Vec3(0, 0, 1), 0)
	, m_renderBoundsOS(Vec3(-1, -1, -1), Vec3(1, 1, 1))
	, m_viewerInsideVolume(false)
	, m_nearPlaneIntersectsVolume(false)
	, m_alpha(1)
	, m_scale(1)
	, m_pDensVol(0)
	, m_pShadVol(0)
	, m_pHullMesh(0)
{
	mfSetType(eDATA_VolumeObject);
	mfUpdateFlags(FCEF_TRANSFORM);
	m_matInv.SetIdentity();
}

CREVolumeObject::~CREVolumeObject()
{
}

void CREVolumeObject::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

CREVolumeObject::IVolumeTexture* CREVolumeObject::CreateVolumeTexture() const
{
	return new CVolumeTexture();
}
