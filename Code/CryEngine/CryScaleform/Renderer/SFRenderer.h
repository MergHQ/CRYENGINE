// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Renderer/SFConfig.h"
#include "Renderer/SFMesh.h"
#include "Renderer/SFTechnique.h"
#include "Renderer/SFTexture.h"
#include "Renderer/SFRenderCache.h"
#include <Render/Render_HAL.h>
#include <Render/Render_ShaderHAL.h>
#include <Render/Render_Profiler.h>

namespace Scaleform {
namespace Render {

class CSFRenderTargets : public Render::RenderTargetData
{
public:
	CSFRenderTargets(RenderBuffer* buffer, CTexture* prt, DepthStencilBuffer* pdsb, CTexture* pdss);
	virtual ~CSFRenderTargets();

	static void  SetTextures(RenderBuffer* buffer, CTexture* prt, DepthStencilBuffer* pdsb, CTexture* pdss);
	virtual bool ReplaceDepthStencilBuffer(DepthStencilBuffer* pdsb);

	CTexturePtr pRenderTargetTexture;
	CTexturePtr pDepthStencilTexture;

	friend class CSFRenderer;
};

class CSFRenderer : public Render::ShaderHAL<CSFTechniqueManager, CSFTechniqueInterface>
{
	struct SSFRenderState
	{
		CSFRenderCache::SResourceSetCacheEntry resourceSet;
		CConstantBuffer*                       pVertexConstants;
		CConstantBuffer*                       pPixelConstants;
		uint32 vertexStride;
		uint32 vertexOffset;
		int32  blendState;
		int32  rasterState;
	};

public:
	typedef Render::ShaderHAL<CSFTechniqueManager, CSFTechniqueInterface> BaseHAL;

	CSFRenderer(ThreadCommandQueue* pCommandQueue);
	virtual ~CSFRenderer();

	virtual bool                    InitHAL(const Render::HALInitParams& params);
	virtual bool                    ShutdownHAL();
	bool                            RestoreAfterReset();

	virtual Render::RenderTarget*   CreateRenderTarget(CTexture* pRenderTarget, CTexture* pDepthStencil);
	virtual Render::RenderTarget*   CreateRenderTarget(Render::Texture* texture, bool needsStencil);
	virtual Render::RenderTarget*   CreateTempRenderTarget(const ImageSize& size, bool needsStencil);

	virtual Render::TextureManager* GetTextureManager() { return m_pTextureManager.GetPtr(); }
	virtual Render::MeshCache&      GetMeshCache()      { return m_meshCache; }
	virtual Render::RenderEvents&   GetEvents()         { return m_events; }

	virtual bool                    Submit();
	virtual bool                    BeginScene();
	virtual bool                    BeginFrame();
	virtual void                    EndFrame();
	virtual bool                    IsRasterModeSupported(RasterModeType mode) const;

	virtual void                    MapVertexFormat(PrimitiveFillType fill, const VertexFormat* sourceFormat,
	                                                const VertexFormat** single,
	                                                const VertexFormat** batch,
	                                                const VertexFormat** instanced, unsigned = 0);

	void SetVertexShaderConstants(const float* pData, const buffer_size_t size) { m_rs.pVertexConstants = CreateConstantBuffer(pData, size); }
	void SetPixelShaderConstants(const float* pData, const buffer_size_t size)  { m_rs.pPixelConstants = CreateConstantBuffer(pData, size); }
	void SetSamplerState(uint32 stage, uint32 viewCount, CTexture** views, SamplerStateHandle state);

	void SetRenderStatesCache(GPtr<CSFRenderCache> pCache)
	{
		m_pRenderCache = pCache;
		m_rs.resourceSet.pCache = pCache;
	}

protected:
	virtual void     updateViewport();
	virtual void     setRenderTargetImpl(Render::RenderTargetData* phd, unsigned flags, const Color& clearColor);
	virtual bool     checkDepthStencilBufferCaps();
	virtual void     applyDepthStencilMode(DepthStencilMode mode, unsigned stencilRef);
	virtual void     applyRasterModeImpl(RasterModeType mode);
	virtual void     applyBlendModeImpl(BlendMode mode, bool sourceAc = false, bool forceAc = false);

	virtual UPInt    setVertexArray(PrimitiveBatch* pbatch, MeshCacheItem* pmesh);
	virtual UPInt    setVertexArray(const ComplexMesh::FillRecord& fr, unsigned formatIndex, MeshCacheItem* pmesh);
	virtual void     setBatchUnitSquareVertexStream();
	virtual void     drawPrimitive(unsigned indexCount, unsigned meshCount);
	virtual void     drawIndexedPrimitive(unsigned indexCount, unsigned vertexCount, unsigned meshCount, UPInt indexPtr, UPInt vertexOffset);
	virtual void     drawIndexedInstanced(unsigned indexCount, unsigned vertexCount, unsigned meshCount, UPInt indexPtr, UPInt vertexOffset);
	virtual void     drawUncachedFilter(const FilterStackEntry&);

	static int32     GetBlendType(BlendMode blendMode, bool colorWrite, bool sourceAc);
	CConstantBuffer* CreateConstantBuffer(const float* pData, const buffer_size_t m_size);
	void             Render(unsigned vertexBase, unsigned vertexOrIndexOffset, unsigned vertexOrIndexCount, unsigned instanceCount);
	bool             CreateBlendStates();

	static const uint32     s_blendTypeCount = Blend_Count * 2 + 1;
	static const int        s_depthStencilStates[DepthStencil_Count];
	static const int        s_depthStencilFunctions[DepthStencil_Count];

	SSFRenderState          m_rs;
	SStreamInfo             m_vertexStream;
	SStreamInfo             m_indexStream;
	CPrimitiveRenderPass*   m_pRenderPass;
	GPtr<CSFRenderCache>    m_pRenderCache;
	CSFMeshCache            m_meshCache;
	GPtr<CSFTextureManager> m_pTextureManager;
	Render::RenderEvents    m_events;
	int                     m_blendStates[s_blendTypeCount];
	CConstantBufferPtr      m_pEmptyConstants;

	friend class CSFTechniqueInterface;
};

} // ~Render namespace
} // ~Scaleform namespace
