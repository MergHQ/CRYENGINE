// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryRenderer/RenderElements/CREVolumeObject.h>

#include "XRenderD3D9/DriverD3D.h"

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

	CDeviceTexture* pVolDevTex = m_pTex->GetDevTexture();
	GPUPIN_DEVICE_TEXTURE(gcpRendD3D->GetPerformanceDeviceContext(), pVolDevTex);

	D3D11_BOX box;
	box.left = 0;
	box.right = cpyWidth;
	box.top = 0;
	box.bottom = cpyHeight;
	box.front = 0;
	box.back = cpyDepth;

	D3DVolumeTexture* pVolTex = pVolDevTex->GetVolumeTexture();
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
	: CRenderElement()
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

bool CREVolumeObject::mfDraw(CShader* ef, SShaderPass* sfm)
{
	CD3D9Renderer* rd(gcpRendD3D);
	I3DEngine* p3DEngine(gEnv->p3DEngine);

	uint32 nFlagsPS2 = rd->m_RP.m_PersFlags2;
	rd->m_RP.m_PersFlags2 &= ~(RBPF2_COMMIT_PF | RBPF2_COMMIT_CM);

	// render
	uint32 nPasses(0);
	ef->FXBegin(&nPasses, 0);
	if (!nPasses)
		return false;

	ef->FXBeginPass(0);

	if (m_nearPlaneIntersectsVolume)
	{
		rd->SetCullMode(R_CULL_FRONT);
		rd->FX_SetState(GS_COLMASK_RGB | GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
	}
	else
	{
		rd->SetCullMode(R_CULL_BACK);
		rd->FX_SetState(GS_COLMASK_RGB | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
	}

	// set vs constants
	static CCryNameR invObjSpaceMatrixName("invObjSpaceMatrix");
	ef->FXSetVSFloat(invObjSpaceMatrixName, (const Vec4*)&m_matInv.m00, 3);

	const Vec4 cEyePosVec(m_eyePosInWS, 0);
	static CCryNameR eyePosInWSName("eyePosInWS");
	ef->FXSetVSFloat(eyePosInWSName, &cEyePosVec, 1);

	const Vec4 cViewerOutsideVec(!m_viewerInsideVolume ? 1.f : 0, m_nearPlaneIntersectsVolume ? 1.f : 0, 0, 0);
	static CCryNameR viewerIsOutsideName("viewerIsOutside");
	ef->FXSetVSFloat(viewerIsOutsideName, &cViewerOutsideVec, 1);

	const Vec4 cEyePosInOSVec(m_eyePosInOS, 0);
	static CCryNameR eyePosInOSName("eyePosInOS");
	ef->FXSetVSFloat(eyePosInOSName, &cEyePosInOSVec, 1);

	// set ps constants
	const Vec4 cEyePosInWSVec(m_eyePosInWS, 0);
	ef->FXSetPSFloat(eyePosInWSName, &cEyePosInWSVec, 1);

	ColorF specColor(1, 1, 1, 1);
	ColorF diffColor(1, 1, 1, 1);

	CShaderResources* pRes(rd->m_RP.m_pShaderResources);
	if (pRes && pRes->HasLMConstants())
	{
		specColor = pRes->GetColorValue(EFTT_SPECULAR);
		diffColor = pRes->GetColorValue(EFTT_DIFFUSE);
	}

	Vec3 cloudShadingMultipliers;
	p3DEngine->GetGlobalParameter(E3DPARAM_CLOUDSHADING_MULTIPLIERS, cloudShadingMultipliers);

	Vec3 brightColor(p3DEngine->GetSunColor() * cloudShadingMultipliers.x);
	brightColor = brightColor.CompMul(Vec3(specColor.r, specColor.g, specColor.b));

	Vec3 darkColor(p3DEngine->GetSkyColor() * cloudShadingMultipliers.y);
	darkColor = darkColor.CompMul(Vec3(diffColor.r, diffColor.g, diffColor.b));

	{
		static CCryNameR darkColorName("darkColor");
		const Vec4 data(darkColor, m_alpha);
		ef->FXSetPSFloat(darkColorName, &data, 1);
	}
	{
		static CCryNameR brightColorName("brightColor");
		const Vec4 data(brightColor, m_alpha);
		ef->FXSetPSFloat(brightColorName, &data, 1);
	}

	const Vec4 cVolumeTraceStartPlane(m_volumeTraceStartPlane.n, m_volumeTraceStartPlane.d);
	static CCryNameR volumeTraceStartPlaneName("volumeTraceStartPlane");
	ef->FXSetPSFloat(volumeTraceStartPlaneName, &cVolumeTraceStartPlane, 1);

	const Vec4 cScaleConsts(m_scale, 0, 0, 0);
	static CCryNameR scaleConstsName("scaleConsts");
	ef->FXSetPSFloat(scaleConstsName, &cScaleConsts, 1);

	// TODO: optimize shader and remove need to pass inv obj space matrix
	ef->FXSetPSFloat(invObjSpaceMatrixName, (const Vec4*)&m_matInv.m00, 3);

	// commit all render changes
	rd->FX_Commit();

	// set vertex declaration and streams
	if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
	{
		CRenderMesh* pHullMesh = static_cast<CRenderMesh*>(m_pHullMesh.get());

		// set vertex and index buffer
		pHullMesh->CheckUpdate(pHullMesh->_GetVertexFormat(), 0);
		size_t vbOffset(0);
		size_t ibOffset(0);
		D3DVertexBuffer* pVB = rd->m_DevBufMan.GetD3DVB(pHullMesh->_GetVBStream(VSF_GENERAL), &vbOffset);
		D3DIndexBuffer* pIB = rd->m_DevBufMan.GetD3DIB(pHullMesh->_GetIBStream(), &ibOffset);
		assert(pVB);
		assert(pIB);
		if (!pVB || !pIB)
			return false;

		rd->FX_SetVStream(0, pVB, vbOffset, pHullMesh->GetStreamStride(VSF_GENERAL));
		rd->FX_SetIStream(pIB, ibOffset, (sizeof(vtx_idx) == 2 ? Index16 : Index32));
		// draw
		rd->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, pHullMesh->_GetNumVerts(), 0, pHullMesh->_GetNumInds());
	}

	ef->FXEndPass();
	ef->FXEnd();

	rd->FX_ResetPipe();
	rd->m_RP.m_PersFlags2 = nFlagsPS2;

	return true;
}