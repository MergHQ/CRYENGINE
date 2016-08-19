// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/CREWaterOcean.h>
#include <Cry3DEngine/I3DEngine.h>
#include "XRenderD3D9/DriverD3D.h"

CREWaterOcean::CREWaterOcean() : CRendElementBase()
{
	mfSetType(eDATA_WaterOcean);
	mfUpdateFlags(FCEF_TRANSFORM);

	m_nVerticesCount = 0;
	m_nIndicesCount = 0;

	m_pVertDecl = 0;
	m_pVertices = 0;
	m_pIndices = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

CREWaterOcean::~CREWaterOcean()
{
	ReleaseOcean();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::mfGetPlane(Plane& pl)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::mfPrepare(bool bCheckOverflow)
{
	if (bCheckOverflow)
		gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

Vec3 CREWaterOcean::GetPositionAt(float x, float y) const
{
	//assert( m_pWaterSim );
	if (WaterSimMgr())
		return WaterSimMgr()->GetPositionAt((int)x, (int)y);

	return Vec3(0, 0, 0);
}

Vec4* CREWaterOcean::GetDisplaceGrid() const
{
	//assert( m_pWaterSim );
	if (WaterSimMgr())
		return WaterSimMgr()->GetDisplaceGrid();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::UpdateFFT()
{

}


void CREWaterOcean::Create(uint32 nVerticesCount, SVF_P3F_C4B_T2F* pVertices, uint32 nIndicesCount, const void* pIndices, uint32 nIndexSizeof)
{
	if (!nVerticesCount || !pVertices || !nIndicesCount || !pIndices || (nIndexSizeof != 2 && nIndexSizeof != 4))
		return;

	CD3D9Renderer* rd(gcpRendD3D);
	ReleaseOcean();

	m_nVerticesCount = nVerticesCount;
	m_nIndicesCount = nIndicesCount;
	m_nIndexSizeof = nIndexSizeof;
	HRESULT hr(S_OK);
	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Create vertex buffer
	//////////////////////////////////////////////////////////////////////////////////////////////////
	{
		D3DVertexBuffer* pVertexBuffer = 0;
		D3D11_BUFFER_DESC BufDesc;
		SVF_P3F_C4B_T2F* dst = 0;

		uint32 size = nVerticesCount * sizeof(SVF_P3F_C4B_T2F);
		BufDesc.ByteWidth = size;
		BufDesc.Usage = D3D11_USAGE_DEFAULT;
		BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufDesc.CPUAccessFlags = 0;
		BufDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA pInitData;
		pInitData.pSysMem = pVertices;
		pInitData.SysMemPitch = 0;
		pInitData.SysMemSlicePitch = 0;

		gcpRendD3D->GetDevice().CreateBuffer(&BufDesc, &pInitData, &pVertexBuffer);
		m_pVertices = pVertexBuffer;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Create index buffer
	//////////////////////////////////////////////////////////////////////////////////////////////////
	{
		D3DIndexBuffer* pIndexBuffer = 0;
		const uint32 size = nIndicesCount * m_nIndexSizeof;

		D3D11_BUFFER_DESC BufDesc;
		BufDesc.ByteWidth = size;
		BufDesc.Usage = D3D11_USAGE_DEFAULT;
		BufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		BufDesc.CPUAccessFlags = 0;
		BufDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA pInitData;
		pInitData.pSysMem = pIndices;
		pInitData.SysMemPitch = 0;
		pInitData.SysMemSlicePitch = 0;

		gcpRendD3D->GetDevice().CreateBuffer(&BufDesc, &pInitData, &pIndexBuffer);
		m_pIndices = pIndexBuffer;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::FrameUpdate()
{
	// Update Water simulator
	//  UpdateFFT();

	static bool bInitialize = true;
	static Vec4 pParams0(0, 0, 0, 0), pParams1(0, 0, 0, 0);

	Vec4 pCurrParams0, pCurrParams1;
	gEnv->p3DEngine->GetOceanAnimationParams(pCurrParams0, pCurrParams1);

	// why no comparison operator on Vec4 ??
	if (bInitialize || pCurrParams0.x != pParams0.x || pCurrParams0.y != pParams0.y ||
		pCurrParams0.z != pParams0.z || pCurrParams0.w != pParams0.w || pCurrParams1.x != pParams1.x ||
		pCurrParams1.y != pParams1.y || pCurrParams1.z != pParams1.z || pCurrParams1.w != pParams1.w)
	{
		pParams0 = pCurrParams0;
		pParams1 = pCurrParams1;
		WaterSimMgr()->Create(1.0, pParams0.x, pParams0.z, 1.0f, 1.0f);
		bInitialize = false;
	}

	const int nGridSize = 64;

	// Update Vertex Texture
	if (!CTexture::IsTextureExist(CTexture::s_ptexWaterOcean))
	{
		CTexture::s_ptexWaterOcean->Create2DTexture(nGridSize, nGridSize, 1,
			FT_DONT_RELEASE | FT_NOMIPS | FT_STAGE_UPLOAD,
			0, eTF_R32G32B32A32F, eTF_R32G32B32A32F);
		//	CTexture::s_ptexWaterOcean->SetVertexTexture(true);
	}

	CTexture* pTexture = CTexture::s_ptexWaterOcean;

	// Copy data..
	if (CTexture::IsTextureExist(pTexture))
	{
		const float fUpdateTime = 0.125f * gEnv->pTimer->GetCurrTime();// / clamp_tpl<float>(pParams1.x, 0.55f, 1.0f);
		int nFrameID = gRenDev->GetFrameID();
		void* pRawPtr = NULL;
		WaterSimMgr()->Update(nFrameID, fUpdateTime, false, pRawPtr);

		Vec4* pDispGrid = WaterSimMgr()->GetDisplaceGrid();
		if (pDispGrid == NULL)
			return;

		const uint32 pitch = 4 * sizeof(f32) * nGridSize;
		const uint32 width = nGridSize;
		const uint32 height = nGridSize;

		STALL_PROFILER("update subresource")

			CDeviceTexture * pDevTex = pTexture->GetDevTexture();
		pDevTex->UploadFromStagingResource(0, [=](void* pData, uint32 rowPitch, uint32 slicePitch)
		{
			cryMemcpy(pData, pDispGrid, 4 * width * height * sizeof(f32));
			return true;
		});
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CREWaterOcean::ReleaseOcean()
{
	ID3D11Buffer* pVertices = (ID3D11Buffer*)m_pVertices;
	ID3D11Buffer* pIndices = (ID3D11Buffer*)m_pIndices;

	SAFE_RELEASE(pVertices);
	SAFE_RELEASE(pIndices);

	m_pVertices = nullptr;
	m_pIndices = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREWaterOcean::mfDraw(CShader* ef, SShaderPass* sfm)
{
	if (!m_nVerticesCount || !m_nIndicesCount || !m_pVertices || !m_pIndices)
		return false;

	CD3D9Renderer* rd(gcpRendD3D);

	FrameUpdate();

	int32 nGpuID = 0;// gRenDev->RT_GetCurrGpuID();
	if (CTexture::s_ptexWaterOcean)
	{
		CTexture::s_ptexWaterOcean->SetFilterMode(FILTER_LINEAR);
		CTexture::s_ptexWaterOcean->SetClampingMode(0, 0, 1);
		CTexture::s_ptexWaterOcean->UpdateTexStates();
	}

	if (CTexture::s_ptexWaterRipplesDDN)
	{
		CTexture::s_ptexWaterRipplesDDN->SetVertexTexture(true);
		CTexture::s_ptexWaterRipplesDDN->SetFilterMode(FILTER_LINEAR);
		CTexture::s_ptexWaterRipplesDDN->SetClampingMode(0, 0, 1);
		CTexture::s_ptexWaterRipplesDDN->UpdateTexStates();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////

	uint64 nFlagsShaderRTprev = rd->m_RP.m_FlagsShader_RT;
	uint32 nFlagsPF2prev = rd->m_RP.m_PersFlags2;

	//  rd->m_RP.m_PersFlags2 &= ~(RBPF2_COMMIT_PF|RBPF2_COMMIT_CM);

	// render
	uint32 nPasses(0);

	// set streams
	HRESULT hr(S_OK);

	STexStageInfo pPrevTexState0 = CTexture::s_TexStages[0];
	STexStageInfo pPrevTexState1 = CTexture::s_TexStages[1];

	STexState pState(FILTER_BILINEAR, false);
	const int texStateID(CTexture::GetTexState(pState));

	N3DEngineCommon::SOceanInfo& OceanInfo = gRenDev->m_p3DEngineCommon.m_OceanInfo;
	Vec4& pParams = OceanInfo.m_vMeshParams;

	uint32 nPrevStateOr = rd->m_RP.m_StateOr;
	uint32 nPrevStateAnd = rd->m_RP.m_StateAnd;

	ef->FXSetTechnique("Water");
	ef->FXBegin(&nPasses, 0);

	if (0 == nPasses)
		return false;

	if (gRenDev->GetRCamera().vOrigin.z > OceanInfo.m_fWaterLevel)
	{
		rd->m_RP.m_StateAnd |= GS_DEPTHFUNC_MASK;
		rd->m_RP.m_StateOr |= GS_DEPTHWRITE | GS_DEPTHFUNC_LEQUAL;
	}

	ef->FXBeginPass(0);

	if (CTexture::s_ptexWaterOcean)
	{
		// Set on vertex and pixel shader
		CTexture::s_ptexWaterOcean->SetVertexTexture(true);
		CTexture::s_ptexWaterOcean->Apply(0, texStateID);
		CTexture::s_ptexWaterOcean->SetVertexTexture(false);
	}

	if (CTexture::s_ptexWaterRipplesDDN)
	{
		// Set on vertex and pixel shader
		CTexture::s_ptexWaterRipplesDDN->SetVertexTexture(true);
		CTexture::s_ptexWaterRipplesDDN->Apply(1, texStateID);
		CTexture::s_ptexWaterRipplesDDN->SetVertexTexture(false);
	}

	hr = rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F);
	if (!FAILED(hr))
	{
		// commit all render changes
		rd->FX_Commit();

		rd->FX_SetVStream(0, m_pVertices, 0, sizeof(SVF_P3F_C4B_T2F));
		rd->FX_SetIStream(m_pIndices, 0, (m_nIndexSizeof == 2 ? Index16 : Index32));

		ERenderPrimitiveType eType = rd->m_bUseWaterTessHW ? eptTriangleList : eptTriangleStrip;
#ifdef WATER_TESSELLATION_RENDERER
		if (CHWShader_D3D::s_pCurInstHS)
			eType = ept3ControlPointPatchList;
#endif
		rd->FX_DrawIndexedPrimitive(eType, 0, 0, m_nVerticesCount, 0, m_nIndicesCount);
	}

	ef->FXEndPass();
	ef->FXEnd();

	rd->m_RP.m_StateOr = nPrevStateOr;
	rd->m_RP.m_StateAnd = nPrevStateAnd;

	CTexture::s_TexStages[0] = pPrevTexState0;
	CTexture::s_TexStages[1] = pPrevTexState1;

	gcpRendD3D->FX_ResetPipe();

	rd->m_RP.m_FlagsShader_RT = nFlagsShaderRTprev;
	rd->m_RP.m_PersFlags2 = nFlagsPF2prev;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
