// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "FullscreenPass.h"

struct IUtilityRenderPass
{
	virtual ~IUtilityRenderPass() {};
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CStretchRectPass : public IUtilityRenderPass
{
public:
	void Execute(CTexture* pSrcRT, CTexture* pDestRT);

protected:
	CFullscreenPass m_pass;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CStretchRegionPass : public IUtilityRenderPass
{
public:
	CStretchRegionPass() {};
	~CStretchRegionPass() {};

	void Execute(CTexture* pSrcRT, CTexture* pDestRT, const RECT *pSrcRect=NULL, const RECT *pDstRect=NULL, bool bBigDownsample=false);
	void PreparePrimitive(CRenderPrimitive& prim, const RECT& rcS, int renderState, const D3DViewPort& targetViewport, bool bResample, bool bBigDownsample, CTexture *pSrcRT, CTexture *pDstRT);

	static CStretchRegionPass &GetPass();
	static void Shutdown();

protected:
	CPrimitiveRenderPass m_pass;

	CRenderPrimitive m_Primitive;

private:
	static CStretchRegionPass *s_pPass;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CStableDownsamplePass : public IUtilityRenderPass
{
public:
	void Execute(CTexture* pSrcRT, CTexture* pDestRT, bool bKillFireflies);

private:
	CFullscreenPass m_pass;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDepthDownsamplePass : public IUtilityRenderPass
{
public:
	void Execute(CTexture* pSrcRT, CTexture* pDestRT, bool bLinearizeSrcDepth, bool bFromSingleChannel);

private:
	CFullscreenPass m_pass;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CGaussianBlurPass : public IUtilityRenderPass
{
public:
	CGaussianBlurPass()
		: m_scale(-FLT_MIN)
		, m_distribution(-FLT_MIN)
	{
	}
	void Execute(CTexture* pScrDestRT, CTexture* pTempRT, float scale, float distribution, bool bAlphaOnly = false);

protected:
	float GaussianDistribution1D(float x, float rho);
	void  ComputeParams(int texWidth, int texHeight, int numSamples, float scale, float distribution);

protected:
	float           m_scale;
	float           m_distribution;
	Vec4            m_paramsH[16];
	Vec4            m_paramsV[16];
	Vec4            m_weights[16];

	CFullscreenPass m_passH;
	CFullscreenPass m_passV;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CMipmapGenPass : public IUtilityRenderPass
{
public:
	void Execute(CTexture* pScrDestRT, int mipCount = 0);

protected:
	std::array<CFullscreenPass, 13> m_downsamplePasses;
};

class CClearRegionPass : public IUtilityRenderPass
{
public:
	CClearRegionPass();
	virtual ~CClearRegionPass();

	void Execute(SDepthTexture* pDepthTex, const int nFlags, const float cDepth, const uint8 cStencil, const uint numRects, const RECT* pRects);
	void Execute(CTexture* pTex, const ColorF& cClear, const uint numRects, const RECT* pRects);

protected:
	void PreparePrimitive(CRenderPrimitive& prim, int renderState, int stencilState, const ColorF& cClear, float cDepth, int stencilRef, const RECT& rect, const D3DViewPort& targetViewport);

	CPrimitiveRenderPass          m_clearPass;
	std::vector<CRenderPrimitive> m_clearPrimitives;
	buffer_handle_t               m_quadVertices;
};
