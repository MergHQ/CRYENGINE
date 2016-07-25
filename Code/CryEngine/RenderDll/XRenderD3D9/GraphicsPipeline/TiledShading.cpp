// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TiledShading.h"

#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "D3D_SVO.h"


struct STiledLightVolumeInfo
{
	Matrix44 worldMat;
	Vec4     volumeTypeInfo;
	Vec4     volumeParams0;
	Vec4     volumeParams1;
	Vec4     volumeParams2;
};

CTiledShadingStage::CTiledShadingStage()
	: m_passCullingShading(CComputeRenderPass::eFlags_ReflectConstantBuffersFromShader)
{
	STATIC_ASSERT(sizeof(STiledLightVolumeInfo) % 16 == 0, "STiledLightVolumeInfo should be 16 byte aligned for GPU performance");
	STATIC_ASSERT(MaxNumTileLights <= 256 && LightTileSizeX == 8 && LightTileSizeY == 8, "Volumes don't support other settings");
}

CTiledShadingStage::~CTiledShadingStage()
{
	m_lightVolumeInfoBuf.Release();
	
	for (uint32 i = 0; i < eVolumeType_Count; i++)
	{
		m_volumeMeshes[i].vertexDataBuf.Release();
		gRenDev->m_DevBufMan.Destroy(m_volumeMeshes[i].vertexBuffer);
		gRenDev->m_DevBufMan.Destroy(m_volumeMeshes[i].indexBuffer);
	}
}


void CTiledShadingStage::Init()
{
	CD3D9Renderer* pRenderer = gcpRendD3D;
	
	STexState ts0(FILTER_TRILINEAR, true);
	STexState ts1(FILTER_LINEAR, true);
	ts1.SetComparisonFilter(true);
	m_samplerTrilinearClamp = CTexture::GetTexState(ts0);
	m_samplerCompare = CTexture::GetTexState(ts1);

	m_lightVolumeInfoBuf.Create(MaxNumTileLights, sizeof(STiledLightVolumeInfo), DXGI_FORMAT_UNKNOWN, DX11BUF_DYNAMIC | DX11BUF_STRUCTURED | DX11BUF_BIND_SRV, NULL);
	
	// Create geometry for light volumes
	{
		t_arrDeferredMeshVertBuff vertices;
		t_arrDeferredMeshIndBuff indices;
		Vec3 vertexData[256];
		
		for (uint32 i = 0; i < eVolumeType_Count; i++)
		{
			switch (i)
			{
			case eVolumeType_Box:
				CDeferredRenderUtils::CreateUnitBox(indices, vertices);
				for (uint32 i = 0; i < vertices.size(); i++)
				{
					vertices[i].xyz = vertices[i].xyz * 2.0f - Vec3(1.0f, 1.0f, 1.0f);  // Centered box
				}
				break;
			case eVolumeType_Sphere:
				CDeferredRenderUtils::CreateUnitSphere(0, indices, vertices);
				break;
			case eVolumeType_Cone:
				CDeferredRenderUtils::CreateUnitSphere(0, indices, vertices);  // Currently we use a sphere proxy for cones
				break;
			}

			assert(vertices.size() < CRY_ARRAY_COUNT(vertexData));
			
			SVolumeGeometry& volumeMesh = m_volumeMeshes[i];

			volumeMesh.numIndices = indices.size();
			volumeMesh.numVertices = vertices.size();
			indices.resize(volumeMesh.numIndices * 256);
			for (uint32 i = 0; i < 256; i++)
			{
				for (uint32 j = 0; j < volumeMesh.numIndices; j++)
				{
					indices[i * volumeMesh.numIndices + j] = indices[j] + i * volumeMesh.numVertices;
				}
			}

			volumeMesh.indexBuffer = pRenderer->m_DevBufMan.Create(BBT_INDEX_BUFFER, BU_STATIC, volumeMesh.numIndices * sizeof(uint16) * 256);
			pRenderer->m_DevBufMan.UpdateBuffer(volumeMesh.indexBuffer, indices);
			volumeMesh.vertexBuffer = pRenderer->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_STATIC, volumeMesh.numVertices * sizeof(vertices[0]));
			pRenderer->m_DevBufMan.UpdateBuffer(volumeMesh.vertexBuffer, vertices);

			for (uint32 i = 0; i < volumeMesh.numVertices; i++)
			{
				vertexData[i] = vertices[i].xyz;
			}
			volumeMesh.vertexDataBuf.Create(volumeMesh.numVertices, sizeof(Vec3), DXGI_FORMAT_R32G32B32_FLOAT, DX11BUF_BIND_SRV, vertexData);
		}
	}
}


void CTiledShadingStage::PrepareLightVolumeInfo()
{
	const uint32 kNumPasses = eVolumeType_Count * 2;
	uint16 volumeLightIndices[kNumPasses][MaxNumTileLights];
	
	for (uint32 i = 0; i < kNumPasses; i++)
		m_numVolumesPerPass[i] = 0;

	CD3D9Renderer* pRenderer = gcpRendD3D;
	CTiledShading* pTiledShading = &pRenderer->GetTiledShading();
	STiledLightCullInfo* pLightCullInfo = pTiledShading->GetTiledLightCullInfo();

	Vec3 camPos = pRenderer->GetRCamera().vOrigin;
	Matrix44A matView = pRenderer->m_ViewMatrix;
	matView.m02 *= -1; matView.m12 *= -1; matView.m22 *= -1; matView.m32 *= -1;
	Matrix44A matViewInv = matView.GetInverted();
	
	float tileCoverageX = (float)LightTileSizeX / (float)pRenderer->GetWidth();
	float tileCoverageY = (float)LightTileSizeY / (float)pRenderer->GetHeight();
	float camFovScaleX = 0.5f * pRenderer->m_ProjMatrix.m00;
	float camFovScaleY = 0.5f * pRenderer->m_ProjMatrix.m11;

	// Classify volume type
	uint32 numLights = pTiledShading->GetValidLightCount();
	for (uint32 lightIdx = 0; lightIdx < numLights; ++lightIdx)
	{
		STiledLightCullInfo& lightCullInfo = pLightCullInfo[lightIdx];

		bool bInsideVolume = false;
		Vec3 lightPos = Vec3(Vec4(Vec3(lightCullInfo.posRad), 1) * matViewInv);
		if (lightPos.GetDistance(camPos) < lightCullInfo.posRad.w * 1.5f)
		{
			bInsideVolume = true;
		}

		uint32 pass = eVolumeType_Sphere;
		if (lightCullInfo.volumeType == CTiledShading::tlVolumeOBB)
			pass = eVolumeType_Box;
		else if (lightCullInfo.volumeType == CTiledShading::tlVolumeCone)
			pass = eVolumeType_Cone;

		pass += bInsideVolume ? 0 : eVolumeType_Count;
		volumeLightIndices[pass][m_numVolumesPerPass[pass]++] = lightIdx;
	}

	// Prepare buffer with light volume info
	uint32 curIndex = 0;
	STiledLightVolumeInfo volumeInfo[MaxNumTileLights];
	for (uint32 i = 0; i < kNumPasses; i++)
	{
		bool bInsideVolume = i < eVolumeType_Count; 
		
		for (uint32 j = 0; j < m_numVolumesPerPass[i]; j++)
		{	
			STiledLightCullInfo& lightCullInfo = pLightCullInfo[volumeLightIndices[i][j]];

			// To ensure conservative rasterization, we need to enlarge the projected volume by at least half a tile
			// Compute the size of a tile in view space at the position of the volume
			//     ScreenCoverage = FovScale * ObjectSize / Distance  =>  ObjectSize = ScreenCoverage * Distance / FovScale
			float distance = (bInsideVolume ? lightCullInfo.depthBounds.y : lightCullInfo.depthBounds.x) * pRenderer->GetRCamera().fFar;
			float tileSizeVS = std::max(tileCoverageX * distance / camFovScaleX, tileCoverageY * distance / camFovScaleY);
			tileSizeVS = sqrtf(sqr(tileCoverageX * distance / camFovScaleX) + sqr(tileCoverageY * distance / camFovScaleY));
			
			Vec3 lightPos = Vec3(Vec4(Vec3(lightCullInfo.posRad), 1) * matViewInv);
			
			Matrix44 worldMat;
			Vec4 volumeParams0(0, 0, 0, 0);
			Vec4 volumeParams1(0, 0, 0, 0);
			Vec4 volumeParams2(0, 0, 0, 0);

			if (lightCullInfo.volumeType == CTiledShading::tlVolumeOBB)
			{
				float minBoxSize = std::min(std::min(lightCullInfo.volumeParams0.w, lightCullInfo.volumeParams1.w), lightCullInfo.volumeParams2.w);
				float volumeScale = (minBoxSize + tileSizeVS) / minBoxSize;

				volumeParams0 = Vec4(Vec3(lightCullInfo.volumeParams0), 0) * matViewInv; volumeParams0.w = lightCullInfo.volumeParams0.w * volumeScale;
				volumeParams1 = Vec4(Vec3(lightCullInfo.volumeParams1), 0) * matViewInv; volumeParams1.w = lightCullInfo.volumeParams1.w * volumeScale;
				volumeParams2 = Vec4(Vec3(lightCullInfo.volumeParams2), 0) * matViewInv; volumeParams2.w = lightCullInfo.volumeParams2.w * volumeScale;
				
				Matrix33 rotScaleMat(Vec3(volumeParams0) * volumeParams0.w, Vec3(volumeParams1) * volumeParams1.w, Vec3(volumeParams2) * volumeParams2.w);
				Matrix34 unitVolumeToWorldMat = Matrix34::CreateTranslationMat(lightPos) * rotScaleMat;
				worldMat = unitVolumeToWorldMat.GetTransposed();

				volumeParams0 = Vec4(Vec3(volumeParams0) / volumeParams0.w, lightPos.x - camPos.x);
				volumeParams1 = Vec4(Vec3(volumeParams1) / volumeParams1.w, lightPos.y - camPos.y);
				volumeParams2 = Vec4(Vec3(volumeParams2) / volumeParams2.w, lightPos.z - camPos.z);
			}
			else if (lightCullInfo.volumeType == CTiledShading::tlVolumeSun)
			{
				volumeParams0 = Vec4(camPos - camPos, pRenderer->GetRCamera().fFar - 1.0f);
				
				Matrix34 unitVolumeToWorldMat = Matrix34::CreateScale(Vec3(volumeParams0.w), camPos);
				worldMat = unitVolumeToWorldMat.GetTransposed();
			}
			else if (lightCullInfo.volumeType == CTiledShading::tlVolumeCone)
			{
				float volumeScale = (lightCullInfo.posRad.w + tileSizeVS) / lightCullInfo.posRad.w;
				volumeParams0 = Vec4(lightPos - camPos, lightCullInfo.posRad.w * volumeScale);
				volumeParams1 = Vec4(Vec3(Vec4(Vec3(lightCullInfo.volumeParams0), 0) * matViewInv), cosf(lightCullInfo.volumeParams1.w + 0.20f));
				
				float volumeScaleLowRes = (lightCullInfo.posRad.w + (2.0f + tileSizeVS)) / lightCullInfo.posRad.w;
				Matrix34 unitVolumeToWorldMat = Matrix34::CreateScale(Vec3(lightCullInfo.posRad.w * volumeScaleLowRes), lightPos);
				worldMat = unitVolumeToWorldMat.GetTransposed();
			}
			else
			{
				float volumeScale = (lightCullInfo.posRad.w + tileSizeVS) / lightCullInfo.posRad.w;
				volumeParams0 = Vec4(lightPos - camPos, lightCullInfo.posRad.w * volumeScale);
				
				float volumeScaleLowRes = (lightCullInfo.posRad.w + (2.0f + tileSizeVS)) / lightCullInfo.posRad.w;
				Matrix34 unitVolumeToWorldMat = Matrix34::CreateScale(Vec3(lightCullInfo.posRad.w * volumeScaleLowRes), lightPos);
				worldMat = unitVolumeToWorldMat.GetTransposed();
			}

			volumeInfo[curIndex].worldMat = worldMat;
			volumeInfo[curIndex].volumeTypeInfo = Vec4((float)lightCullInfo.volumeType, 0, 0, (float)volumeLightIndices[i][j]);
			volumeInfo[curIndex].volumeParams0 = volumeParams0;
			volumeInfo[curIndex].volumeParams1 = volumeParams1;
			volumeInfo[curIndex].volumeParams2 = volumeParams2;
			curIndex += 1;
		}
	}

	m_lightVolumeInfoBuf.UpdateBufferContent(volumeInfo, sizeof(volumeInfo));
}


void CTiledShadingStage::PrepareResources()
{
	// On PS4 the DMA operation for ClearUAV is not blocking, so it needs to be scheduled early enough to avoid race conditions with the first pass accessing the UAV
	if (CRenderer::CV_r_DeferredShadingTiled == 3)
	{
		uint clearNull[4] = { 0 };
		CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList()->GetComputeInterface()->ClearUAV(gcpRendD3D->GetTiledShading().m_tileOpaqueLightMaskBuf.GetDeviceUAV(), clearNull, 0, nullptr);
	}
}


bool CTiledShadingStage::ExecuteVolumeListGen(uint32 dispatchSizeX, uint32 dispatchSizeY)
{
	if (CRenderer::CV_r_DeferredShadingTiled < 3)
		return false;
	
	CD3D9Renderer* pRenderer = gcpRendD3D;
	CTiledShading* pTiledShading = &pRenderer->GetTiledShading();

	CTexture* pDepthRT = CTexture::s_ptexDepthBufferHalfQuarter;
	assert(pDepthRT->GetWidth() == dispatchSizeX && pDepthRT->GetHeight() == dispatchSizeY);
	
	SDepthTexture depthTarget;
	depthTarget.nWidth = pDepthRT->GetWidth();
	depthTarget.nHeight = pDepthRT->GetHeight();
	depthTarget.nFrameAccess = -1;
	depthTarget.bBusy = false;
	depthTarget.pTexture = pDepthRT;
	depthTarget.pTarget = pDepthRT->GetDevTexture()->Get2DTexture();
	depthTarget.pSurface = pDepthRT->GetDeviceDepthStencilView();

	{
		PROFILE_LABEL_SCOPE("COPY_DEPTH");

		static CCryNameTSCRC techCopy("CopyToDeviceDepth");
		m_passCopyDepth.SetTechnique(CShaderMan::s_shPostEffects, techCopy, 0);
		m_passCopyDepth.SetRequirePerViewConstantBuffer(true);
		m_passCopyDepth.SetDepthTarget(&depthTarget);
		m_passCopyDepth.SetState(GS_DEPTHWRITE | GS_DEPTHFUNC_NOTEQUAL);
		m_passCopyDepth.SetTexture(0, CTexture::s_ptexZTargetScaled3);
		
		m_passCopyDepth.BeginConstantUpdate();
		m_passCopyDepth.Execute();
	}

	PROFILE_LABEL_SCOPE("TILED_VOLUMES");

	PrepareLightVolumeInfo();
	
	Vec4r wBasisX, wBasisY, wBasisZ, camPos;
	CShadowUtils::ProjectScreenToWorldExpansionBasis(
		pRenderer->m_IdentityMatrix, pRenderer->GetCamera(), pRenderer->m_vProjMatrixSubPixoffset,
		(float)pDepthRT->GetWidth(), (float)pDepthRT->GetHeight(), wBasisX, wBasisY, wBasisZ, camPos, true, NULL);
	
	{
		D3DViewPort viewport;
		viewport.TopLeftX = viewport.TopLeftY = 0.0f;
		viewport.Width = (float)pDepthRT->GetWidth();
		viewport.Height = (float)pDepthRT->GetHeight();
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		
		m_passLightVolumes.SetDepthTarget(&depthTarget);
		m_passLightVolumes.SetViewport(viewport);
		m_passLightVolumes.ClearPrimitives();
		m_passLightVolumes.SetOutputUAV(0, &pTiledShading->m_tileOpaqueLightMaskBuf);
		m_passLightVolumes.SetOutputUAV(1, &pTiledShading->m_tileTranspLightMaskBuf);
		
		uint32 curIndex = 0;
		for (uint32 pass = 0; pass < eVolumeType_Count * 2; pass++)
		{
			uint32 volumeType = pass % eVolumeType_Count;
			bool bInsideVolume = pass < eVolumeType_Count;
			
			if (m_numVolumesPerPass[pass] > 0)
			{
				CRenderPrimitive& primitive = m_volumePasses[pass];

				static CCryNameTSCRC techLightVolume("VolumeLightListGen");
				primitive.SetFlags(CRenderPrimitive::eFlags_ReflectConstantBuffersFromShader);
				primitive.SetTechnique(CShaderMan::s_shDeferredShading, techLightVolume, 0);
				primitive.SetRenderState(bInsideVolume ? GS_NODEPTHTEST : GS_DEPTHFUNC_GEQUAL);
				primitive.SetCullMode(bInsideVolume ? eCULL_Front : eCULL_Back);
				primitive.SetTexture(3, CTexture::s_ptexZTargetScaled3, SResourceView::DefaultView, EShaderStage_Vertex | EShaderStage_Pixel);
				primitive.SetBuffer(1, m_lightVolumeInfoBuf, false, EShaderStage_Vertex | EShaderStage_Pixel);

				uint32 numIndices = m_volumeMeshes[volumeType].numIndices;
				uint32 numVertices = m_volumeMeshes[volumeType].numVertices;
				uint32 numInstances = m_numVolumesPerPass[pass];
			
				primitive.SetCustomVertexStream(m_volumeMeshes[volumeType].vertexBuffer, eVF_P3F_C4B_T2F, sizeof(SVF_P3F_C4B_T2F));
				primitive.SetCustomIndexStream(m_volumeMeshes[volumeType].indexBuffer, Index16);
				primitive.SetDrawInfo(eptTriangleList, 0, 0, numIndices * numInstances);
				primitive.SetBuffer(0, m_volumeMeshes[volumeType].vertexDataBuf, false, EShaderStage_Vertex | EShaderStage_Pixel);
				
				auto& constantManager = primitive.GetConstantManager();
				if (constantManager.IsShaderReflectionValid())
				{
					static CCryNameR viewProjName("g_mViewProj");
					static CCryNameR volumeToWorldName("g_mUnitLightVolumeToWorld");
					static CCryNameR screenScaleName("g_ScreenScale");
					static CCryNameR volumeParamsName("VolumeParams");
					static CCryNameR wBasisXName("vWBasisX");
					static CCryNameR wBasisYName("vWBasisY");
					static CCryNameR wBasisZName("vWBasisZ");
			
					Matrix44 viewProjMat = pRenderer->m_ViewMatrix * pRenderer->m_ProjMatrix;
					Vec4 screenScale((float)pDepthRT->GetWidth(), (float)pDepthRT->GetHeight(), 0, 0);

					CStandardGraphicsPipeline::SViewInfo viewInfo[2];
					int viewInfoCount = gcpRendD3D->GetGraphicsPipeline().GetViewInfo(viewInfo);
					const bool bReverseDepth = (viewInfo[0].flags & CStandardGraphicsPipeline::SViewInfo::eFlags_ReverseDepth) != 0;
					Vec4 volumeParams(0, 0, (float)curIndex, (float)numVertices);
					float zn = viewInfo[0].pRenderCamera->fNear;
					float zf = viewInfo[0].pRenderCamera->fFar;
					volumeParams.x = bReverseDepth ? zn / (zn - zf) : zf / (zf - zn);
					volumeParams.y = bReverseDepth ? zn / (zf - zn) : zn / (zn - zf);
			
					constantManager.BeginNamedConstantUpdate();
					constantManager.SetNamedConstantArray(eHWSC_Vertex, viewProjName, (Vec4*)viewProjMat.GetData(), 4);
					constantManager.SetNamedConstant(eHWSC_Vertex, volumeParamsName, volumeParams);
					constantManager.SetNamedConstant(eHWSC_Pixel, screenScaleName, screenScale);
					constantManager.SetNamedConstant(eHWSC_Pixel, volumeParamsName, volumeParams);
					constantManager.SetNamedConstant(eHWSC_Pixel, wBasisXName, (Vec4)wBasisX);
					constantManager.SetNamedConstant(eHWSC_Pixel, wBasisYName, (Vec4)wBasisY);
					constantManager.SetNamedConstant(eHWSC_Pixel, wBasisZName, (Vec4)wBasisZ);
					constantManager.EndNamedConstantUpdate();
				}
				
				m_passLightVolumes.AddPrimitive(&primitive);
			}

			curIndex += m_numVolumesPerPass[pass];
		}
	}

	m_passLightVolumes.Execute();

	// TODO: Workaround, UAV is not getting unbound
	pRenderer->GetGraphicsPipeline().SwitchToLegacyPipeline();
	pRenderer->GetGraphicsPipeline().SwitchFromLegacyPipeline();

	return true;
}


void CTiledShadingStage::Execute()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;
	CTiledShading* pTiledShading = &rd->GetTiledShading();

	// Make sure HDR target is not bound as RT any more
	rd->FX_PushRenderTarget(0, CTexture::s_ptexSceneSpecularAccMap, NULL);
	rd->FX_Commit();
	rd->FX_PopRenderTarget(0);

	uint32 screenWidth = rd->GetWidth();
	uint32 screenHeight = rd->GetHeight();
	uint32 dispatchSizeX = screenWidth / LightTileSizeX + (screenWidth % LightTileSizeX > 0 ? 1 : 0);
	uint32 dispatchSizeY = screenHeight / LightTileSizeY + (screenHeight % LightTileSizeY > 0 ? 1 : 0);

	bool bSeparateCullingPass = ExecuteVolumeListGen(dispatchSizeX, dispatchSizeY);
	
	uint64 rtFlags = 0;
	if (CRenderer::CV_r_DeferredShadingTiled > 1)   // Tiled deferred
		rtFlags |= g_HWSR_MaskBit[HWSR_SAMPLE0];
	if (CRenderer::CV_r_DeferredShadingTiledDebug == 2)  // Light coverage visualization
		rtFlags |= g_HWSR_MaskBit[HWSR_SAMPLE2];
	if (bSeparateCullingPass)
		rtFlags |= g_HWSR_MaskBit[HWSR_SAMPLE1];
	if (CRenderer::CV_r_SSReflections)
		rtFlags |= g_HWSR_MaskBit[HWSR_SAMPLE3];
	if (CRenderer::CV_r_DeferredShadingSSS)
		rtFlags |= g_HWSR_MaskBit[HWSR_SAMPLE4];
	if (CRenderer::CV_r_DeferredShadingAreaLights > 0)
		rtFlags |= g_HWSR_MaskBit[HWSR_SAMPLE5];

#if defined(FEATURE_SVO_GI)
	if (CSvoRenderer::GetInstance()->IsActive())
	{
		int nModeGI = CSvoRenderer::GetInstance()->GetIntegratioMode();

		if (nModeGI == 0 && CSvoRenderer::GetInstance()->GetUseLightProbes())
		{
			// AO modulates diffuse and specular
			rtFlags |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
		}
		else if (nModeGI <= 1)
		{
			// GI replaces diffuse and modulates specular
			rtFlags |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
		}
		else if (nModeGI == 2)
		{
			// GI replaces diffuse and specular
			rtFlags |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
			rtFlags |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
		}
	}
#endif

	CTexture* texClipVolumeIndex = CTexture::s_ptexVelocity;
	CTexture* pTexCaustics = pTiledShading->m_bApplyCaustics ? CTexture::s_ptexSceneTargetR11G11B10F[1] : CTexture::s_ptexBlack;
	CTexture* pTexAOColorBleed = CRenderer::CV_r_ssdoColorBleeding ? CTexture::s_ptexAOColorBleed : CTexture::s_ptexBlack;

	CTexture* pTexGiDiff = CTexture::s_ptexBlack;
	CTexture* pTexGiSpec = CTexture::s_ptexBlack;

#if defined(FEATURE_SVO_GI)
	if (CSvoRenderer::GetInstance()->IsActive() && CSvoRenderer::GetInstance()->GetSpecularFinRT())
	{
		pTexGiDiff = (CTexture*)CSvoRenderer::GetInstance()->GetDiffuseFinRT();
		pTexGiSpec = (CTexture*)CSvoRenderer::GetInstance()->GetSpecularFinRT();
	}
#endif

	static int s_prevTexAOColorBleed = pTexAOColorBleed->GetID();

	if (m_passCullingShading.InputChanged(int(rtFlags >> 32), int(rtFlags), pTexCaustics->GetID(), pTexGiDiff->GetID()) || s_prevTexAOColorBleed != pTexAOColorBleed->GetID())
	{
		static CCryNameTSCRC techTiledShading("TiledDeferredShading");
		m_passCullingShading.SetTechnique(CShaderMan::s_shDeferredShading, techTiledShading, rtFlags);

		m_passCullingShading.SetOutputUAV(0, &pTiledShading->m_tileTranspLightMaskBuf);
		m_passCullingShading.SetOutputUAV(1, CTexture::s_ptexHDRTarget);
		m_passCullingShading.SetOutputUAV(2, CTexture::s_ptexSceneTargetR11G11B10F[0]);

		m_passCullingShading.SetSampler(0, m_samplerTrilinearClamp);

		m_passCullingShading.SetTexture(0, CTexture::s_ptexZTarget);
		m_passCullingShading.SetTexture(1, CTexture::s_ptexSceneNormalsMap);
		m_passCullingShading.SetTexture(2, CTexture::s_ptexSceneSpecular);
		m_passCullingShading.SetTexture(3, CTexture::s_ptexSceneDiffuse);
		m_passCullingShading.SetTexture(4, CTexture::s_ptexShadowMask);
		m_passCullingShading.SetTexture(5, CTexture::s_ptexSceneNormalsBent);
		m_passCullingShading.SetTexture(6, CTexture::s_ptexHDRTargetScaledTmp[0]);
		m_passCullingShading.SetTexture(7, CTexture::s_ptexEnvironmentBRDF);
		m_passCullingShading.SetTexture(8, texClipVolumeIndex);
		m_passCullingShading.SetTexture(9, pTexAOColorBleed);
		m_passCullingShading.SetTexture(10, pTexGiDiff);
		m_passCullingShading.SetTexture(11, pTexGiSpec);
		m_passCullingShading.SetTexture(12, pTexCaustics);

		m_passCullingShading.SetBuffer(16, &pTiledShading->m_LightShadeInfoBuf);
		m_passCullingShading.SetTexture(17, pTiledShading->m_specularProbeAtlas.texArray);
		m_passCullingShading.SetTexture(18, pTiledShading->m_diffuseProbeAtlas.texArray);
		m_passCullingShading.SetTexture(19, pTiledShading->m_spotTexAtlas.texArray);
		m_passCullingShading.SetTexture(20, CTexture::s_ptexRT_ShadowPool);
		m_passCullingShading.SetTexture(21, CTexture::s_ptexShadowJitterMap);
		m_passCullingShading.SetBuffer(22, bSeparateCullingPass ? &pTiledShading->m_tileOpaqueLightMaskBuf : &pTiledShading->m_lightCullInfoBuf);
		m_passCullingShading.SetBuffer(23, &pTiledShading->m_clipVolumeInfoBuf);

		s_prevTexAOColorBleed = pTexAOColorBleed->GetID();
	}

	m_passCullingShading.BeginConstantUpdate();
	{
		static CCryNameR projParamsName("ProjParams");
		Vec4 projParams(rd->m_ProjMatrix.m00, rd->m_ProjMatrix.m11, rd->m_ProjMatrix.m20, rd->m_ProjMatrix.m21);
		m_passCullingShading.SetConstant(projParamsName, projParams);

		static CCryNameR screenSizeName("ScreenSize");
		float fScreenWidth = (float)screenWidth;
		float fScreenHeight = (float)screenHeight;
		Vec4 screenSize(fScreenWidth, fScreenHeight, 1.0f / fScreenWidth, 1.0f / fScreenHeight);
		m_passCullingShading.SetConstant(screenSizeName, screenSize);

		static CCryNameR worldViewPosName("WorldViewPos");
		Vec4 worldViewPosParam(rd->GetRCamera().vOrigin, 0);
		m_passCullingShading.SetConstant(worldViewPosName, worldViewPosParam);

		SD3DPostEffectsUtils::UpdateFrustumCorners();
		static CCryNameR frustumTLName("FrustumTL");
		Vec4 frustumTL(SD3DPostEffectsUtils::m_vLT.x, SD3DPostEffectsUtils::m_vLT.y, SD3DPostEffectsUtils::m_vLT.z, 0);
		m_passCullingShading.SetConstant(frustumTLName, frustumTL);
		static CCryNameR frustumTRName("FrustumTR");
		Vec4 frustumTR(SD3DPostEffectsUtils::m_vRT.x, SD3DPostEffectsUtils::m_vRT.y, SD3DPostEffectsUtils::m_vRT.z, 0);
		m_passCullingShading.SetConstant(frustumTRName, frustumTR);
		static CCryNameR frustumBLName("FrustumBL");
		Vec4 frustumBL(SD3DPostEffectsUtils::m_vLB.x, SD3DPostEffectsUtils::m_vLB.y, SD3DPostEffectsUtils::m_vLB.z, 0);
		m_passCullingShading.SetConstant(frustumBLName, frustumBL);

		static CCryNameR sunDirName("SunDir");
		Vec4 sunDirParam(gEnv->p3DEngine->GetSunDirNormalized(), TiledShading_SunDistance);
		m_passCullingShading.SetConstant(sunDirName, sunDirParam);

		static CCryNameR sunColorName("SunColor");
		Vec3 sunColor;
		gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SUN_COLOR, sunColor);
		sunColor *= rd->m_fAdaptedSceneScaleLBuffer;  // Apply LBuffers range rescale
		Vec4 sunColorParam(sunColor.x, sunColor.y, sunColor.z, gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SUN_SPECULAR_MULTIPLIER));
		m_passCullingShading.SetConstant(sunColorName, sunColorParam);

		static CCryNameR ssdoParamsName("SSDOParams");
		Vec4 ssdoParams(CRenderer::CV_r_ssdoAmountDirect, CRenderer::CV_r_ssdoAmountAmbient, CRenderer::CV_r_ssdoAmountReflection, 0);
#if defined(FEATURE_SVO_GI)
		if (CSvoRenderer::GetInstance()->IsActive())
			ssdoParams *= CSvoRenderer::GetInstance()->GetSsaoAmount();
#endif
		Vec4 ssdoNullParams(0, 0, 0, 0);
		m_passCullingShading.SetConstant(ssdoParamsName, CRenderer::CV_r_ssdo ? ssdoParams : ssdoNullParams);
	}

	m_passCullingShading.SetDispatchSize(dispatchSizeX, dispatchSizeY, 1);

	{
		PROFILE_LABEL_SCOPE("SHADING");		
		// Prepare buffers and textures which have been used in the z-pass and post-processes for use in the compute queue
		// Reduce resource state switching by requesting the most inclusive resource state
		m_passCullingShading.PrepareResourcesForUse(*CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList(), EShaderStage_All);

		{
			const bool bAsynchronousCompute = CRenderer::CV_r_D3D12AsynchronousCompute & BIT((eStage_TiledShading - eStage_FIRST_ASYNC_COMPUTE)) ? true : false;
			SScopedComputeCommandList computeCommandList(bAsynchronousCompute);

			m_passCullingShading.Execute(computeCommandList, EShaderStage_All);
		}
	}

	// Output debug information
	if (CRenderer::CV_r_DeferredShadingTiledDebug == 1)
	{
		rd->Draw2dLabel(20, 60, 2.0f, Col_Blue, false, "Tiled Shading Debug");
		rd->Draw2dLabel(20, 95, 1.7f, pTiledShading->m_numSkippedLights > 0 ? Col_Red : Col_Blue, false, "Skipped Lights: %i", pTiledShading->m_numSkippedLights);
		rd->Draw2dLabel(20, 120, 1.7f, Col_Blue, false, "Atlas Updates: %i", pTiledShading->m_numAtlasUpdates);
	}

	pTiledShading->m_bApplyCaustics = false;  // Reset flag
}