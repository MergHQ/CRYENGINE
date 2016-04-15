// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <array>

class CCompiledRenderPrimitive
{
	friend class CPrimitiveRenderPass;

public:
	enum EPrimitiveType
	{
		ePrim_Triangle,
		ePrim_Box,
		ePrim_Custom,

		ePrim_Count,
		ePrim_First = ePrim_Triangle
	};

	enum EDirtyFlags
	{
		eDirty_Technique   = BIT(0),
		eDirty_RenderState = BIT(1),
		eDirty_Resources   = BIT(2),
		eDirty_Geometry    = BIT(3),

		eDirty_None        = 0,
		eDirty_All         = eDirty_Technique | eDirty_RenderState | eDirty_Resources | eDirty_Geometry
	};

	typedef SDeviceObjectHelpers::SConstantBufferBindInfo InlineConstantBuffer;

public:
	CCompiledRenderPrimitive();

	void                               SetRenderState(int state);
	void                               SetCullMode(ECull cullMode);
	void                               SetTechnique(CShader* pShader, CCryNameTSCRC& techName, uint64 rtMask);
	void                               SetTexture(uint32 shaderSlot, CTexture* pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView);
	void                               SetSampler(uint32 shaderSlot, int32 sampler);
	void                               SetInlineConstantBuffers(std::vector<InlineConstantBuffer>&& inlineConstantBuffers);
	void                               SetPrimitiveType(EPrimitiveType primitiveType);
	void                               SetCustomVertexStream(buffer_handle_t vertexBuffer, EVertexFormat vertexFormat, uint32 vertexStride);
	void                               SetCustomIndexStream(buffer_handle_t indexBuffer, uint32 indexStride);
	void                               SetDrawInfo(uint32 vertexOrIndexCount);

	bool                               IsDirty() const;

	CShader*                           GetShader()          const { return m_pShader; }
	CCryNameTSCRC                      GetShaderTechnique() const { return m_techniqueName; }
	uint64                             GetShaderRtMask()    const { return m_rtMask; }
	std::vector<InlineConstantBuffer>& GetInlineConstantBuffers() { return m_inlineConstantBuffers; }

	static void                        AddPrimitiveGeometryCacheUser();
	static void                        RemovePrimitiveGeometryCacheUser();

private:
	EDirtyFlags Compile(CDeviceGraphicsPSODesc partialPsoDesc);

private:

	struct SPrimitiveGeometry
	{
		SStreamInfo   vertexStream;
		SStreamInfo   indexStream;

		EVertexFormat vertexFormat;
		uint32        vertexOrIndexCount;

		SPrimitiveGeometry();
	};

private:
	EDirtyFlags                       m_dirtyMask;
	uint32                            m_renderState;
	ECull                             m_cullMode;
	CShader*                          m_pShader;
	CCryNameTSCRC                     m_techniqueName;
	uint64                            m_rtMask;

	EPrimitiveType                    m_primitiveType;
	SPrimitiveGeometry                m_primitiveGeometry;

	std::vector<InlineConstantBuffer> m_inlineConstantBuffers;

	int                               m_currentPsoUpdateCount;
	CDeviceGraphicsPSOPtr             m_pPipelineState;
	CDeviceResourceLayoutPtr          m_pResourceLayout;
	CDeviceResourceSetPtr             m_pResources;
	CDeviceInputStream*               m_pVertexInputSet;
	CDeviceInputStream*               m_pIndexInputSet;

	static SPrimitiveGeometry         s_primitiveGeometryCache[ePrim_Count];
	static int                        s_nPrimitiveGeometryCacheUsers;
};

DEFINE_ENUM_FLAG_OPERATORS(CCompiledRenderPrimitive::EDirtyFlags);

class CPrimitiveRenderPass
{
public:
	CPrimitiveRenderPass();
	~CPrimitiveRenderPass();

	bool IsDirty() const { return m_bDirty; }

	void SetRenderTarget(uint32 slot, CTexture* pRenderTarget, SResourceView::KeyType rendertargetView = SResourceView::DefaultRendertargtView);
	void SetDepthTarget(SDepthTexture* pDepthTarget);
	void SetViewport(const D3DViewPort& viewport);

	void ClearPrimitives() { m_primitiveCount = 0; }
	void AddPrimitive(const CCompiledRenderPrimitive& primitive);

	void Execute();

protected:
	bool CompileResources();

protected:
	std::array<CTexture*, 4>              m_pRenderTargets;
	std::array<SResourceView::KeyType, 4> m_renderTargetViews;
	SDepthTexture*                        m_pDepthTarget;
	uint32                                m_numRenderTargets;
	D3DViewPort                           m_viewport;

	std::vector<CCompiledRenderPrimitive> m_primitives;
	uint32                                m_primitiveCount;

	bool m_bDirty;
};
