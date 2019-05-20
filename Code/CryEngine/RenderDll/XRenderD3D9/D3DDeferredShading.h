// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/RenderPipeline.h"
#include "Common/Textures/Texture.h"
#include "Common/Textures/PowerOf2BlockPacker.h"
#include "Common/Shadow_Renderer.h"

struct IVisArea;

enum EDecalType
{
	DTYP_DARKEN,
	DTYP_BRIGHTEN,
	DTYP_ALPHABLEND,
	DTYP_ALPHABLEND_AND_BUMP,
	DTYP_ALPHABLEND_SPECULAR,
	DTYP_DARKEN_LIGHTBUF,
	DTYP_NUM,
};

#define MAX_DEFERRED_CLIP_VOLUMES 64

class CTexPoolAtlas;

class CDeferredShading
{
public:

	static inline bool IsValid()
	{
		return m_pInstance != nullptr;
	}

	static CDeferredShading& Instance()
	{
		return (*m_pInstance);
	}

	void SetupPasses(CRenderView* pRenderView);
	void SetupGlobalConsts(CRenderView* pRenderView);

	void Release();

	// called in between levels to free up memory
	void        ReleaseData(std::shared_ptr<CGraphicsPipeline> pGraphicsPipeline);

	void        GetLightRenderSettings(const CRenderView* pRenderView, const SRenderLight* const __restrict pDL, bool& bStencilMask, bool& bUseLightVolumes, EShapeMeshType& meshType);

	const Matrix44A&                GetCameraProjMatrix() const    { return m_mViewProj; }

	CPowerOf2BlockPacker&           GetBlockPacker()               { return m_blockPack; }
	const CPowerOf2BlockPacker&     GetBlockPacker() const         { return m_blockPack; }
	TArray<SShadowAllocData>&       GetShadowPoolAllocator()       { return m_shadowPoolAlloc; }
	const TArray<SShadowAllocData>& GetShadowPoolAllocator() const { return m_shadowPoolAlloc; }

private:

	CDeferredShading()
		: m_pDiffuseRT(CRendererResources::s_ptexSceneDiffuse)
		, m_blockPack(0, 0)
	{
		for (int i = 0; i < MAX_GPU_NUM; ++i)
		{
			m_prevViewProj[i].SetIdentity();
		}

		for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
		{
			for (int j = 0; j < MAX_REND_RECURSION_LEVELS; ++j)
			{
				m_nVisAreasGIRef[i][j] = 0;
			}
		}
	}

	~CDeferredShading()
	{
		Release();
	}

private:
	uint32 m_nVisAreasGIRef[RT_COMMAND_BUF_COUNT][MAX_REND_RECURSION_LEVELS];

	struct SClipShape
	{
		IRenderMesh* pShape;
		Matrix34     mxTransform;
		SClipShape() : pShape(NULL), mxTransform(Matrix34::CreateIdentity()) {}
		SClipShape(IRenderMesh* _pShape, const Matrix34& _mxTransform) : pShape(_pShape), mxTransform(_mxTransform) {}
	};

	Vec3               m_pCamPos;
	Vec3               m_pCamFront;
	float              m_fCamFar;
	float              m_fCamNear;

	Matrix44A          m_mViewProj;
	Matrix44A          m_pView;

	Matrix44A          m_prevViewProj[MAX_GPU_NUM];

	Vec4               vWorldBasisX, vWorldBasisY, vWorldBasisZ;

	CTexture*          m_pDiffuseRT;

	uint               m_nShadowPoolSize = 0;

	short              m_nCurTargetWidth;
	short              m_nCurTargetHeight;

	static CDeferredShading* m_pInstance;

	friend class CD3D9Renderer;
	friend class CRenderer;
	friend class CTiledLightVolumesStage;
	friend class CShadowMapStage;
	friend class CVolumetricFogStage; // for access to m_nCurrentShadowPoolLight and m_nFirstCandidateShadowPoolLight.

	CPowerOf2BlockPacker     m_blockPack;
	TArray<SShadowAllocData> m_shadowPoolAlloc;

public:
	static CDeferredShading* CreateDeferredShading()
	{
		m_pInstance = new CDeferredShading();

		return m_pInstance;
	}

	static void DestroyDeferredShading()
	{
		SAFE_DELETE(m_pInstance);
	}
};
