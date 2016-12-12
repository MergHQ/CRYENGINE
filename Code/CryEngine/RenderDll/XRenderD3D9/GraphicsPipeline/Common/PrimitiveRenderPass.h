// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <array>

#include "../../DeviceManager/DeviceObjects.h" // SDeviceObjectHelpers

struct SCompiledRenderPrimitive : private NoCopy
{
	SCompiledRenderPrimitive();
	SCompiledRenderPrimitive(SCompiledRenderPrimitive&& other);

	void Reset();

	struct SInstanceInfo
	{
		std::vector<SDeviceObjectHelpers::SConstantBufferBindInfo>  constantBuffers;

		uint32        vertexOrIndexCount;
		uint32        vertexOrIndexOffset;
		uint32        vertexBaseOffset;
	};

	CDeviceGraphicsPSOPtr      m_pPipelineState;
	CDeviceResourceLayoutPtr   m_pResourceLayout;
	CDeviceResourceSetPtr      m_pResources;
	const CDeviceInputStream*  m_pVertexInputSet;
	const CDeviceInputStream*  m_pIndexInputSet;

	uint8                      m_stencilRef;
	std::vector<SInstanceInfo> m_instances;
};


class CRenderPrimitive : public SCompiledRenderPrimitive
{
public:
	enum EPrimitiveType
	{
		ePrim_Triangle,
		ePrim_UnitBox,                // axis aligned box ( 0, 0, 0) - (1,1,1)
		ePrim_CenteredBox,            // axis aligned box (-1,-1,-1) - (1,1,1)
		ePrim_Projector,              // pyramid shape with sparsely tessellated ground plane
		ePrim_Projector1,             // pyramid shape with semi densely tessellated ground plane
		ePrim_Projector2,             // pyramid shape with densely tessellated ground plane
		ePrim_ClipProjector,          // same as ePrim_Projector  but with even denser tessellation
		ePrim_ClipProjector1,         // same as ePrim_Projector1 but with even denser tessellation
		ePrim_ClipProjector2,         // same as ePrim_Projector2 but with even denser tessellation
		ePrim_FullscreenQuad,         // fullscreen quad             ( 0  0, 0) - ( 1  1, 0)
		ePrim_FullscreenQuadCentered, // fullscreen quad             (-1,-1, 0) - ( 1, 1, 0)
		ePrim_FullscreenQuadTess,     // tessellated fullscreen quad (-1,-1, 0) - ( 1, 1, 0)
		ePrim_Custom,

		ePrim_Count,
		ePrim_First = ePrim_Triangle
	};

	enum EDirtyFlags
	{
		eDirty_Technique      = BIT(0),
		eDirty_RenderState    = BIT(1),
		eDirty_Resources      = BIT(2),
		eDirty_Geometry       = BIT(3),
		eDirty_ResourceLayout = BIT(4),
		eDirty_InstanceData   = BIT(5),
		eDirty_Topology       = BIT(6),

		eDirty_None        = 0,
		eDirty_All         = eDirty_Technique | eDirty_RenderState | eDirty_Resources | eDirty_Geometry | eDirty_ResourceLayout | eDirty_InstanceData | eDirty_Topology
	};

	enum EPrimitiveFlags
	{
		eFlags_None                             = 0,
		eFlags_ReflectConstantBuffersFromShader = BIT(0)
	};

	typedef SDeviceObjectHelpers::CShaderConstantManager ConstantManager;

public:
	CRenderPrimitive(CRenderPrimitive&& other);
	CRenderPrimitive(EPrimitiveFlags flags = eFlags_None);

	void          Reset(EPrimitiveFlags flags = eFlags_None);

	void          AllocateTypedConstantBuffer(EConstantBufferShaderSlot shaderSlot, int size, EShaderStage shaderStages);

	void          SetFlags(EPrimitiveFlags flags);
	void          SetRenderState(int state);
	void          SetStencilState(int state, uint8 stencilRef, uint8 stencilReadMask = 0xFF, uint8 stencilWriteMask = 0xFF);
	void          SetCullMode(ECull cullMode);
	void          SetEnableDepthClip(bool bEnable);
	void          SetTechnique(CShader* pShader, const CCryNameTSCRC& techName, uint64 rtMask);
	void          SetTexture(uint32 shaderSlot, CTexture* pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView, EShaderStage shaderStages = EShaderStage_Pixel);
	void          SetSampler(uint32 shaderSlot, int32 sampler, EShaderStage shaderStages = EShaderStage_Pixel);
	void          SetConstantBuffer(uint32 shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages = EShaderStage_Pixel);
	void          SetBuffer(uint32 shaderSlot, const CGpuBuffer& buffer, bool bUnorderedAccess = false, EShaderStage shaderStages = EShaderStage_Pixel);
	void          SetInlineConstantBuffer(EConstantBufferShaderSlot shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages = EShaderStage_Pixel);
	void          SetPrimitiveType(EPrimitiveType primitiveType);
	void          SetCustomVertexStream(buffer_handle_t vertexBuffer, EVertexFormat vertexFormat, uint32 vertexStride);
	void          SetCustomIndexStream(buffer_handle_t indexBuffer, RenderIndexType indexType);
	void          SetDrawInfo(ERenderPrimitiveType primType, uint32 vertexBaseOffset, uint32 vertexOrIndexOffset, uint32 vertexOrIndexCount);
	void          SetDrawTopology(ERenderPrimitiveType primType);

	bool          IsDirty() const;

	CShader*      GetShader()             const { return m_pShader; }
	CCryNameTSCRC GetShaderTechnique()    const { return m_techniqueName; }
	uint64        GetShaderRtMask()       const { return m_rtMask; }

	ConstantManager&       GetConstantManager()       { return m_constantManager; }
	const ConstantManager& GetConstantManager() const { return m_constantManager; }

	static void      AddPrimitiveGeometryCacheUser();
	static void      RemovePrimitiveGeometryCacheUser();

	EDirtyFlags Compile(uint32 renderTargetCount, CTexture* const* pRenderTargets, const SDepthTexture* pDepthTarget, SResourceView::KeyType* pRenderTargetViews = nullptr, CDeviceResourceSetPtr pOutputResources = nullptr, uint64 extraRtMask = 0);

private:

	struct SPrimitiveGeometry
	{
		ERenderPrimitiveType primType;
		EVertexFormat        vertexFormat;

		uint32        vertexBaseOffset;
		uint32        vertexOrIndexCount;
		uint32        vertexOrIndexOffset;

		SStreamInfo   vertexStream;
		SStreamInfo   indexStream;

		SPrimitiveGeometry();
	};

private:
	EPrimitiveFlags           m_flags;
	EDirtyFlags               m_dirtyMask;
	uint32                    m_renderState;
	uint32                    m_stencilState;
	uint8                     m_stencilReadMask;
	uint8                     m_stencilWriteMask;
	ECull                     m_cullMode;
	bool                      m_bDepthClip;
	CShader*                  m_pShader;
	CCryNameTSCRC             m_techniqueName;
	uint64                    m_rtMask;

	EPrimitiveType            m_primitiveType;
	SPrimitiveGeometry        m_primitiveGeometry;

	ConstantManager           m_constantManager;

	int                       m_currentPsoUpdateCount;

	static SPrimitiveGeometry s_primitiveGeometryCache[ePrim_Count];
	static int                s_nPrimitiveGeometryCacheUsers;
};

DEFINE_ENUM_FLAG_OPERATORS(CRenderPrimitive::EDirtyFlags);

struct VrProjectionInfo;

class CPrimitiveRenderPass : private NoCopy
{
public:
	enum EPrimitivePassFlags
	{
		ePassFlags_None = 0,
		ePassFlags_UseVrProjectionState = BIT(0),
		ePassFlags_RequireVrProjectionConstants = BIT(1),
		ePassFlags_VrProjectionPass = ePassFlags_UseVrProjectionState | ePassFlags_RequireVrProjectionConstants // for convenience
	};

	CPrimitiveRenderPass(bool createGeometryCache = true);
	CPrimitiveRenderPass(CPrimitiveRenderPass&& other);
	~CPrimitiveRenderPass();
	CPrimitiveRenderPass& operator=(CPrimitiveRenderPass&& other);

	void   SetFlags(EPrimitivePassFlags flags) { m_passFlags = flags; }
	void   SetRenderTarget(uint32 slot, CTexture* pRenderTarget, SResourceView::KeyType rendertargetView = SResourceView::DefaultRendertargetView);
	CTexture* GetRenderTarget(uint32 slot) { return m_pRenderTargets[slot]; }
	void   SetOutputUAV(uint32 slot, CGpuBuffer* pBuffer);
	void   SetDepthTarget(SDepthTexture* pDepthTarget);
	void   SetViewport(const D3DViewPort& viewport);
	void   SetScissor(bool bEnable, const D3DRectangle& scissor);

	void   ClearPrimitives()   { m_compiledPrimitives.clear(); }
	bool   AddPrimitive(CRenderPrimitive* pPrimitive);
	bool   AddPrimitive(SCompiledRenderPrimitive* pPrimitive);
	void   UndoAddPrimitive()  { CRY_ASSERT(!m_compiledPrimitives.empty()); m_compiledPrimitives.pop_back(); }

	uint32 GetPrimitiveCount() const { return m_compiledPrimitives.size();  }

	void   Execute();

protected:
	void   Prepare(CDeviceCommandListRef RESTRICT_REFERENCE commandList);

protected:
	EPrimitivePassFlags                   m_passFlags;
	std::array<CTexture*, 4>              m_pRenderTargets;
	std::array<SResourceView::KeyType, 4> m_renderTargetViews;
	CDeviceResourceSetPtr                 m_pOutputResources;
	CDeviceResourceSetPtr                 m_pOutputNULLResources;
	SDepthTexture*                        m_pDepthTarget;
	uint32                                m_numRenderTargets;
	D3DViewPort                           m_viewport;
	bool                                  m_scissorEnabled;
	D3DRectangle                          m_scissor;

	std::vector<SCompiledRenderPrimitive*> m_compiledPrimitives;
};


///////////////////////////////////// Inline functions for CRenderPrimitive /////////////////////////////////////

#define ASSIGN_VALUE(dst, src, dirtyFlag)                     \
  m_dirtyMask |= !((dst)==(src)) ? (dirtyFlag) : eDirty_None; \
  (dst) = (src);

inline void CRenderPrimitive::SetFlags(EPrimitiveFlags flags)
{
	ASSIGN_VALUE(m_flags, flags, eDirty_ResourceLayout);
}

inline void CRenderPrimitive::SetRenderState(int state)
{
	ASSIGN_VALUE(m_renderState, state, eDirty_RenderState);
}

inline void CRenderPrimitive::SetStencilState(int state, uint8 stencilRef, uint8 stencilReadMask, uint8 stencilWriteMask)
{
	ASSIGN_VALUE(m_stencilState, state, eDirty_RenderState);
	ASSIGN_VALUE(m_stencilReadMask, stencilReadMask, eDirty_RenderState);
	ASSIGN_VALUE(m_stencilWriteMask, stencilWriteMask, eDirty_RenderState);
	m_stencilRef = stencilRef;
}

inline void CRenderPrimitive::SetCullMode(ECull cullMode)
{
	ASSIGN_VALUE(m_cullMode, cullMode, eDirty_RenderState);
}

inline void CRenderPrimitive::SetEnableDepthClip(bool bEnable)
{
	ASSIGN_VALUE(m_bDepthClip, bEnable, eDirty_RenderState);
}

inline void CRenderPrimitive::SetTechnique(CShader* pShader, const CCryNameTSCRC& techName, uint64 rtMask)
{
	ASSIGN_VALUE(m_pShader, pShader, eDirty_Technique);
	ASSIGN_VALUE(m_techniqueName, techName, eDirty_Technique);
	ASSIGN_VALUE(m_rtMask, rtMask, eDirty_Technique);

	if (m_pPipelineState && m_currentPsoUpdateCount != m_pPipelineState->GetUpdateCount())
		m_dirtyMask |= eDirty_Technique;

	if (m_flags & eFlags_ReflectConstantBuffersFromShader)
	{
		if (m_dirtyMask & eDirty_Technique)
		{
			m_constantManager.InitShaderReflection(pShader, techName, rtMask);
		}
	}
}

inline void CRenderPrimitive::SetTexture(uint32 shaderSlot, CTexture* pTexture, SResourceView::KeyType resourceViewID, EShaderStage shaderStages)
{
	m_pResources->SetTexture(shaderSlot, pTexture, resourceViewID, shaderStages);
	m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
}

inline void CRenderPrimitive::SetSampler(uint32 shaderSlot, int32 sampler, EShaderStage shaderStages)
{
	m_pResources->SetSampler(shaderSlot, sampler, shaderStages);
	m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
}

inline void CRenderPrimitive::SetConstantBuffer(uint32 shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages)
{
	m_pResources->SetConstantBuffer(shaderSlot, pBuffer, shaderStages);
	m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
}

inline void CRenderPrimitive::SetBuffer(uint32 shaderSlot, const CGpuBuffer& buffer, bool bUnorderedAccess, EShaderStage shaderStages)
{
	m_pResources->SetBuffer(shaderSlot, buffer, bUnorderedAccess, shaderStages);
	m_dirtyMask |= m_pResources->IsDirty() ? eDirty_Resources : eDirty_None;
}

inline void CRenderPrimitive::SetInlineConstantBuffer(EConstantBufferShaderSlot shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages)
{
	if (m_constantManager.SetTypedConstantBuffer(shaderSlot, pBuffer, shaderStages))
		m_dirtyMask |= eDirty_ResourceLayout;
}

inline void CRenderPrimitive::SetPrimitiveType(EPrimitiveType primitiveType)
{
	ASSIGN_VALUE(m_primitiveType, primitiveType, eDirty_Geometry);
}

inline void CRenderPrimitive::SetCustomVertexStream(buffer_handle_t vertexBuffer, EVertexFormat vertexFormat, uint32 vertexStride)
{
	ASSIGN_VALUE(m_primitiveGeometry.vertexStream.hStream, vertexBuffer, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveGeometry.vertexStream.nStride, vertexStride, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveGeometry.vertexFormat, vertexFormat, eDirty_Geometry | eDirty_Topology);
	ASSIGN_VALUE(m_primitiveType, ePrim_Custom, eDirty_Geometry);
}

inline void CRenderPrimitive::SetCustomIndexStream(buffer_handle_t indexBuffer, RenderIndexType indexType)
{
	ASSIGN_VALUE(m_primitiveGeometry.indexStream.hStream, indexBuffer, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveGeometry.indexStream.nStride, indexType, eDirty_Geometry);
	ASSIGN_VALUE(m_primitiveType, ePrim_Custom, eDirty_Geometry);
}

inline void CRenderPrimitive::SetDrawInfo(ERenderPrimitiveType primType, uint32 vertexBaseOffset, uint32 vertexOrIndexOffset, uint32 vertexOrIndexCount)
{
	ASSIGN_VALUE(m_primitiveGeometry.primType, primType, eDirty_Topology);
	ASSIGN_VALUE(m_primitiveGeometry.vertexBaseOffset, vertexBaseOffset, eDirty_InstanceData);
	ASSIGN_VALUE(m_primitiveGeometry.vertexOrIndexOffset, vertexOrIndexOffset, eDirty_InstanceData);
	ASSIGN_VALUE(m_primitiveGeometry.vertexOrIndexCount, vertexOrIndexCount, eDirty_InstanceData);
}

inline void CRenderPrimitive::SetDrawTopology(ERenderPrimitiveType primType)
{
	ASSIGN_VALUE(m_primitiveGeometry.primType, primType, eDirty_Topology);
}

#undef ASSIGN_VALUE
