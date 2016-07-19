// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryRenderer/VertexFormats.h>
#include <CryRenderer/ITexture.h>
#include <array>
#include <bitset>

#include "../../Common/Textures/Texture.h" // SResourceView

class CHWShader_D3D;
class CShader;
class CTexture;
class CCryNameTSCRC;
class CCryDeviceWrapper;
class CConstantBuffer;
class CShaderResources;
struct SGraphicsPipelineStateDescription;
struct SComputePipelineStateDescription;

/////////////////////////////////////////////////////////////////////////////////
//// include concrete device/context implementations
#if defined(CRY_USE_DX12)
	#define CRY_USE_DX12_NATIVE
	#include "XRenderD3D9/DX12/API/DX12CommandList.hpp"
#endif

enum EConstantBufferShaderSlot
{
	// Scaleform
	eConstantBufferShaderSlot_ScaleformMeshAttributes   = 0,
	eConstantBufferShaderSlot_ScaleformRenderParameters = 0,

	// Z/G-Buffer
	eConstantBufferShaderSlot_PerBatch          = 0,
	eConstantBufferShaderSlot_PerInstanceLegacy = 1,
	eConstantBufferShaderSlot_PerFrame          = 2,
	eConstantBufferShaderSlot_PerMaterial       = 3,
	eConstantBufferShaderSlot_PerLight          = 4,
	eConstantBufferShaderSlot_PerPass           = 5,
	eConstantBufferShaderSlot_SkinData          = 6,
	eConstantBufferShaderSlot_InstanceData      = 7,
	eConstantBufferShaderSlot_SPI               = 8,
	eConstantBufferShaderSlot_SkinQuat          = 9,
	eConstantBufferShaderSlot_SkinQuatPrev      = 10,
	eConstantBufferShaderSlot_SPIIndex          = 11,
	eConstantBufferShaderSlot_PerInstance       = 12,
	eConstantBufferShaderSlot_PerView           = 13,

	eConstantBufferShaderSlot_Count
};

enum EResourceLayoutSlot
{
	EResourceLayoutSlot_PerInstanceCB      = 0,
	EResourceLayoutSlot_PerMaterialRS      = 1,
	EResourceLayoutSlot_PerInstanceExtraRS = 2,
	EResourceLayoutSlot_PerPassRS          = 3,

	EResourceLayoutSlot_Max                = 7
};

enum EReservedTextureSlot
{
	EReservedTextureSlot_SkinExtraWeights = 14,
	EReservedTextureSlot_AdjacencyInfo    = 15,
	EReservedTextureSlot_PatchID          = 15,
	EReservedTextureSlot_ComputeSkinVerts = 16,
	EReservedTextureSlot_TerrainBaseMap   = 29,
};

enum EShaderStage
{
	EShaderStage_Vertex            = BIT(eHWSC_Vertex),
	EShaderStage_Pixel             = BIT(eHWSC_Pixel),
	EShaderStage_Geometry          = BIT(eHWSC_Geometry),
	EShaderStage_Compute           = BIT(eHWSC_Compute),
	EShaderStage_Domain            = BIT(eHWSC_Domain),
	EShaderStage_Hull              = BIT(eHWSC_Hull),

	EShaderStage_Count             = eHWSC_Num,
	EShaderStage_None              = 0,
	EShaderStage_All               = EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Geometry | EShaderStage_Domain | EShaderStage_Hull | EShaderStage_Compute,
	EShaderStage_AllWithoutCompute = EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Geometry | EShaderStage_Domain | EShaderStage_Hull
};
DEFINE_ENUM_FLAG_OPERATORS(EShaderStage)
#define SHADERSTAGE_FROM_SHADERCLASS(SHADERCLASS) ::EShaderStage(BIT(SHADERCLASS))

enum { InlineConstantsShaderSlot = eConstantBufferShaderSlot_PerInstance };
const int ResourceSetBufferCount = 8;

typedef std::bitset<EResourceLayoutSlot_Max + 1> UsedBindSlotSet;

struct SProfilingStats
{
	int  numPSOSwitches;
	int  numLayoutSwitches;
	int  numResourceSetSwitches;
	int  numInlineSets;
	int  numPolygons;
	int  numDIPs;
	int  numInvalidDIPs;

	void Reset()
	{
		ZeroStruct(*this);
	}

	void Merge(const SProfilingStats& other)
	{
		CryInterlockedAdd(&numPSOSwitches, other.numPSOSwitches);
		CryInterlockedAdd(&numLayoutSwitches, other.numLayoutSwitches);
		CryInterlockedAdd(&numResourceSetSwitches, other.numResourceSetSwitches);
		CryInterlockedAdd(&numInlineSets, other.numInlineSets);
		CryInterlockedAdd(&numPolygons, other.numPolygons);
		CryInterlockedAdd(&numDIPs, other.numDIPs);
		CryInterlockedAdd(&numInvalidDIPs, other.numInvalidDIPs);
	}
};
////////////////////////////////////////////////////////////////////////////

struct CDeviceInputStream
{
	friend class CDeviceObjectFactory;
	friend class CDeviceGraphicsCommandList;
	friend class CDeviceGraphicsCommandInterfaceImpl;

	/*explicit*/ operator SStreamInfo() const
	{
		SStreamInfo stream;
		stream.hStream = hStream;
		stream.nStride = nStride;
		stream.nSlot = nSlot;
		return stream;
	}

	bool operator!() const
	{
		return hStream == ~0u;
	}

	const CDeviceInputStream& operator[](uint32 offset) const
	{
		return *(this + offset);
	}

private:
	buffer_handle_t hStream;
	uint32          nStride; // NOTE: needs to contain index format for index buffers
	uint32          nSlot;

	CDeviceInputStream() { hStream = ~0u; nStride = 0; nStride = 0; nSlot = 0; }
	CDeviceInputStream(const SStreamInfo& stream) { *this = stream; }
	CDeviceInputStream(buffer_handle_t stream, uint32 stride, uint32 slot) : hStream(stream), nStride(stride), nSlot(slot) {}

	CDeviceInputStream& operator=(const SStreamInfo& stream)
	{
		hStream = stream.hStream;
		nStride = stream.nStride;
		nSlot = stream.nSlot;
		return *this;
	}

	inline bool operator==(const CDeviceInputStream& other) const
	{
		return (hStream == other.hStream) & (nStride == other.nStride) & (nSlot == other.nSlot);
	}
};

////////////////////////////////////////////////////////////////////////////
namespace DeviceResourceBinding
{
enum EShaderSlotType : uint8
{
	eShaderSlotType_ConstantBuffer      = 0,          // hlsl b slot
	eShaderSlotType_TextureAndBuffer    = 1,          // hlsl t slot
	eShaderSlotType_UnorderedAccessView = 2,          // hlsl u slot
	eShaderSlotType_Sampler             = 3,          // hlsl s slot

	eShaderSlotType_Count
};

struct SShaderSlot
{
	bool operator<(const SShaderSlot& other) const
	{
		if (type != other.type) return type < other.type;
		else                    return slotNumber < other.slotNumber;
	}

	EShaderSlotType type;
	uint16          slotNumber;
};

template<typename T>
struct SResourceData
{
	SResourceData() : resource(), view(SResourceView::DefaultView), shaderStages(EShaderStage_None) {}
	SResourceData(const T& _resource, SResourceView::KeyType _view, ::EShaderStage _shaderStages) : resource(_resource), view(_view), shaderStages(_shaderStages) {}
	SResourceData(T&& _resource, SResourceView::KeyType _view, ::EShaderStage _shaderStages) : resource(std::move(_resource)), view(_view), shaderStages(_shaderStages) {}

	bool operator==(const SResourceData<T>& other) const
	{
		return resource == other.resource && shaderStages == other.shaderStages && view == other.view;
	}

	void swap(SResourceData& other)
	{
		std::swap(resource, other.resource);
		std::swap(view, other.view);
		std::swap(shaderStages, other.shaderStages);
	}

	T                      resource;
	SResourceView::KeyType view;
	::EShaderStage         shaderStages;
};
};

class CDeviceResourceSet
{
	friend class CDeviceObjectFactory;
	friend struct SDeviceResourceLayoutDesc;
	friend class CDeviceResourceLayout;
	friend class CDeviceResourceLayout_DX12;

public:
	enum EFlags
	{
		EFlags_None                    = 0,
		EFlags_ForceSetAllState        = BIT(0), // Dx11 only: don't rely on shader reflection, set all resources contained in the resource set
		EFlags_ContainsVolatileBuffers = BIT(1), // Resource set contains buffers that are frequently written on the cpu and consumed by the gpu
		EFlags_DynamicUpdates          = BIT(2), // Internal use only: resource set contains CDynTextureSource
		EFlags_PendingAllocation       = BIT(3), // Internal use only: resource set contains CDynTextureSource with deferred allocation
		EFlags_Multibuffered           = BIT(4), // Internal use only: resource set is multibuffered to enable modification while in use by gpu
	};

	CDeviceResourceSet(EFlags flags);
	CDeviceResourceSet(const CDeviceResourceSet& other);
	virtual ~CDeviceResourceSet();

	bool         IsValid() const { return m_bValid; }
	bool         IsEmpty() const { return m_bEmpty; }
	bool         IsDirty() const { return m_bDirty; }
	void         SetDirty(bool bDirty);
	bool         IsLayoutDirty() const { return m_bDirtyLayout; }

	void         Clear(bool bTextures = true);

	void         SetTexture(int shaderSlot, CTexture* pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView, EShaderStage shaderStages = EShaderStage_Pixel);
	void         SetSampler(int shaderSlot, int sampler, EShaderStage shaderStages = EShaderStage_Pixel);
	void         SetConstantBuffer(int shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages = EShaderStage_Pixel);
	void         SetBuffer(int shaderSlot, const CGpuBuffer& buffer, bool bUnorderedAccess = false, EShaderStage shaderStages = EShaderStage_Pixel);

	EShaderStage GetShaderStages() const;
	EFlags       GetFlags() const { return m_Flags; }

	bool         Fill(CShader* pShader, CShaderResources* pResources, EShaderStage shaderStages = EShaderStage_Pixel);

	void Build();

public:
	typedef DeviceResourceBinding::SResourceData<_smart_ptr<CTexture>>        STextureData;
	typedef DeviceResourceBinding::SResourceData<int>                         SSamplerData;
	typedef DeviceResourceBinding::SResourceData<_smart_ptr<CConstantBuffer>> SConstantBufferData;
	typedef DeviceResourceBinding::SResourceData<CGpuBuffer>                  SBufferData;

protected:
	virtual bool BuildImpl(EFlags updatedFlags) = 0;
	void OnTextureChanged(uint32 dirtyFlags);

	VectorMap<DeviceResourceBinding::SShaderSlot, STextureData>        m_Textures;
	VectorMap<DeviceResourceBinding::SShaderSlot, SSamplerData>        m_Samplers;
	VectorMap<DeviceResourceBinding::SShaderSlot, SBufferData>         m_Buffers;
	VectorMap<DeviceResourceBinding::SShaderSlot, SConstantBufferData> m_ConstantBuffers;

	EFlags m_Flags;

private:
	bool   m_bValid;
	bool   m_bDirty;
	bool   m_bEmpty;
	bool   m_bDirtyLayout;
};
DEFINE_ENUM_FLAG_OPERATORS(CDeviceResourceSet::EFlags)

typedef std::shared_ptr<CDeviceResourceSet> CDeviceResourceSetPtr;

////////////////////////////////////////////////////////////////////////////

struct SDeviceResourceLayoutDesc
{
	SDeviceResourceLayoutDesc() : m_InlineConstantCount(0) {}

	void            SetInlineConstants(uint32 numConstants);
	void            SetResourceSet(uint32 bindSlot, CDeviceResourceSetPtr pResourceSet);
	void            SetConstantBuffer(uint32 bindSlot, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages);

	bool            IsValid() const;
	UsedBindSlotSet GetRequiredResourceBindings() const;

	struct SInlineConstantBuffer
	{
		inline operator DeviceResourceBinding::SShaderSlot() const
		{
			DeviceResourceBinding::SShaderSlot slot = { DeviceResourceBinding::eShaderSlotType_ConstantBuffer, shaderSlot };
			return slot;
		}

		EConstantBufferShaderSlot shaderSlot;
		EShaderStage              shaderStages;
	};

	uint32 m_InlineConstantCount;
	VectorMap<uint32, SInlineConstantBuffer> m_ConstantBuffers;
	VectorMap<uint32, CDeviceResourceSetPtr> m_ResourceSets;

	bool operator<(const SDeviceResourceLayoutDesc& other) const;
};

class CDeviceResourceLayout
{
public:
	CDeviceResourceLayout(UsedBindSlotSet requiredBindings)
		: m_requiredResourceBindings(requiredBindings)
	{}

	UsedBindSlotSet GetRequiredResourceBindings() const { return m_requiredResourceBindings; }

protected:
	UsedBindSlotSet m_requiredResourceBindings;
};

typedef std::shared_ptr<const CDeviceResourceLayout> CDeviceResourceLayoutConstPtr;
typedef std::weak_ptr<const CDeviceResourceLayout>   CDeviceResourceLayoutConstWPtr;

typedef std::shared_ptr<CDeviceResourceLayout>       CDeviceResourceLayoutPtr;
typedef std::weak_ptr<CDeviceResourceLayout>         CDeviceResourceLayoutWPtr;

////////////////////////////////////////////////////////////////////////////
class CDeviceGraphicsPSODesc
{
public:
	CDeviceGraphicsPSODesc(const CDeviceGraphicsPSODesc& other);
	CDeviceGraphicsPSODesc(CDeviceResourceLayout* pResourceLayout, const SGraphicsPipelineStateDescription& pipelineDesc);
	CDeviceGraphicsPSODesc(uint32 renderTargetCount, CTexture* const* pRenderTargets, const SDepthTexture* pDepthTarget, SResourceView::KeyType* pRenderTargetViews = nullptr);

	CDeviceGraphicsPSODesc& operator=(const CDeviceGraphicsPSODesc& other);
	bool                    operator==(const CDeviceGraphicsPSODesc& other) const;

	uint64                  GetHash() const;

public:
	void  InitWithDefaults();

	void  FillDescs(D3D11_RASTERIZER_DESC& rasterizerDesc, D3D11_BLEND_DESC& blendDesc, D3D11_DEPTH_STENCIL_DESC& depthStencilDesc) const;
	uint8 CombineVertexStreamMasks(uint8 fromShader, uint8 fromObject) const;

public:
	_smart_ptr<CShader>        m_pShader;
	CCryNameTSCRC              m_technique;
	bool                       m_bAllowTesselation;
	uint64                     m_ShaderFlags_RT;
	uint32                     m_ShaderFlags_MD;
	uint32                     m_ShaderFlags_MDV;
	EShaderQuality             m_ShaderQuality;
	uint32                     m_RenderState;
	uint32                     m_StencilState;
	uint8                      m_StencilReadMask;
	uint8                      m_StencilWriteMask;
	EVertexFormat              m_VertexFormat;
	uint8                      m_ObjectStreamMask;
	std::array<ETEX_Format, 4> m_RenderTargetFormats;
	ETEX_Format                m_DepthStencilFormat;
	ECull                      m_CullMode;
	int32                      m_DepthBias;
	f32                        m_DepthBiasClamp;
	f32                        m_SlopeScaledDepthBias;
	ERenderPrimitiveType       m_PrimitiveType;
	CDeviceResourceLayout*     m_pResourceLayout;
};

class CDeviceComputePSODesc
{
public:
	CDeviceComputePSODesc(const CDeviceComputePSODesc& other);
	CDeviceComputePSODesc(CDeviceResourceLayout* pResourceLayout, const SComputePipelineStateDescription& pipelineDesc);
	CDeviceComputePSODesc(CDeviceResourceLayout* pResourceLayout, CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags);

	CDeviceComputePSODesc& operator=(const CDeviceComputePSODesc& other);
	bool                   operator==(const CDeviceComputePSODesc& other) const;

	uint64                 GetHash() const;

public:
	void InitWithDefaults();

public:
	_smart_ptr<CShader>    m_pShader;
	CCryNameTSCRC          m_technique;
	uint64                 m_ShaderFlags_RT;
	uint32                 m_ShaderFlags_MD;
	uint32                 m_ShaderFlags_MDV;
	CDeviceResourceLayout* m_pResourceLayout;
};

namespace std
{
template<>
struct hash<CDeviceGraphicsPSODesc>
{
	uint64 operator()(const CDeviceGraphicsPSODesc& psoDesc) const
	{
		return psoDesc.GetHash();
	}
};

template<>
struct equal_to<CDeviceGraphicsPSODesc>
{
	bool operator()(const CDeviceGraphicsPSODesc& psoDesc1, const CDeviceGraphicsPSODesc& psoDesc2) const
	{
		return psoDesc1 == psoDesc2;
	}
};

template<>
struct hash<CDeviceComputePSODesc>
{
	uint64 operator()(const CDeviceComputePSODesc& psoDesc) const
	{
		return psoDesc.GetHash();
	}
};

template<>
struct equal_to<CDeviceComputePSODesc>
{
	bool operator()(const CDeviceComputePSODesc& psoDesc1, const CDeviceComputePSODesc& psoDesc2) const
	{
		return psoDesc1 == psoDesc2;
	}
};

template<>
struct less<SDeviceResourceLayoutDesc>
{
	bool operator()(const SDeviceResourceLayoutDesc& layoutDesc1, const SDeviceResourceLayoutDesc& layoutDesc2) const
	{
		return layoutDesc1 < layoutDesc2;
	}
};
}

class CDeviceGraphicsPSO
{
public:
	CDeviceGraphicsPSO()
		: m_bValid(false)
		, m_nUpdateCount(0)
	{}

	virtual bool Init(const CDeviceGraphicsPSODesc& psoDesc) = 0;
	bool         IsValid() const        { return m_bValid; }
	uint32       GetUpdateCount() const { return m_nUpdateCount;  }

	std::array<void*, eHWSC_Num>          m_pHwShaderInstances; // TODO: remove once we don't need shader reflection anymore
	std::array<CHWShader_D3D*, eHWSC_Num> m_pHwShaders;         // TODO: remove once we don't need shader reflection anymore

#if defined(ENABLE_PROFILING_CODE)
	ERenderPrimitiveType m_PrimitiveTypeForProfiling;
#endif

protected:
	bool   m_bValid;
	uint32 m_nUpdateCount;
};

class CDeviceComputePSO
{
public:
	CDeviceComputePSO()
		: m_bValid(false)
		, m_nUpdateCount(0)
		, m_pHwShaderInstance(nullptr)
		, m_pHwShader(nullptr)
	{}

	virtual bool Init(const CDeviceComputePSODesc& psoDesc) = 0;
	bool         IsValid() const { return m_bValid; }
	uint32       GetUpdateCount() const { return m_nUpdateCount; }

	void*          m_pHwShaderInstance; // TODO: remove once we don't need shader reflection anymore
	CHWShader_D3D* m_pHwShader;         // TODO: remove once we don't need shader reflection anymore

protected:
	bool m_bValid;
	uint32 m_nUpdateCount;
};

typedef std::shared_ptr<const CDeviceGraphicsPSO> CDeviceGraphicsPSOConstPtr;
typedef std::weak_ptr<const CDeviceGraphicsPSO>   CDeviceGraphicsPSOConstWPtr;

typedef std::shared_ptr<const CDeviceComputePSO>  CDeviceComputePSOConstPtr;
typedef std::weak_ptr<const CDeviceComputePSO>    CDeviceComputePSOConstWPtr;

typedef std::shared_ptr<CDeviceGraphicsPSO>       CDeviceGraphicsPSOPtr;
typedef std::weak_ptr<CDeviceGraphicsPSO>         CDeviceGraphicsPSOWPtr;

typedef std::shared_ptr<CDeviceComputePSO>        CDeviceComputePSOPtr;
typedef std::weak_ptr<CDeviceComputePSO>          CDeviceComputePSOWPtr;

////////////////////////////////////////////////////////////////////////////

template<class Impl>
class CDeviceTimestampGroup_Base
{
public:
	enum { kMaxTimestamps = 1024 };

	void   Init();

	void   BeginMeasurement();
	void   EndMeasurement();

	uint32 IssueTimestamp();
	bool   ResolveTimestamps();

	float  GetTimeMS(uint32 timestamp0, uint32 timestamp1);
};

////////////////////////////////////////////////////////////////////////////
#include "DeviceObjects.inl"
#include "D3D11/DeviceObjects_D3D11.inl"
#include "D3D12/DeviceObjects_D3D12.inl"
#if CRY_PLATFORM_ORBIS
	#include "GNM/DeviceObjects_GNM.inl"
#endif

////////////////////////////////////////////////////////////////////////////
	template<typename T>
	struct SCachedValue
	{
		T cachedValue;
		SCachedValue() {}
		SCachedValue(const T& value) : cachedValue(value) {}

		template<typename U>
	ILINE bool Set(U newValue)
		{
			if (cachedValue == newValue)
				return false;

			cachedValue = newValue;
			return true;
		}

		template<typename U>
		inline bool operator!=(U otherValue) const
		{
			return !(cachedValue == otherValue);
		}
	};

template<typename CustomSharedState, typename CustomGraphicsState, typename CustomComputeState>
class CDeviceCommandListCommon
{
protected:
	struct SCachedResourceState
	{
		SCachedValue<const CDeviceResourceLayout*> pResourceLayout;
		SCachedValue<const void*>                  pResources[EResourceLayoutSlot_Max + 1];

		UsedBindSlotSet                            requiredResourceBindings;
		UsedBindSlotSet                            validResourceBindings;
};

	struct SCachedGraphicsState : SCachedResourceState
{
		SCachedValue<const CDeviceGraphicsPSO*> pPipelineState;
		SCachedValue<const CDeviceInputStream*> vertexStreams;
		SCachedValue<const CDeviceInputStream*> indexStream;
		SCachedValue<int32>                     stencilRef;
		CustomGraphicsState                     custom;
	};

	struct SCachedComputeState : SCachedResourceState
	{
		SCachedValue<const CDeviceComputePSO*> pPipelineState;
		CustomComputeState                     custom;
	};

protected:
	SCachedGraphicsState m_graphicsState;
	SCachedComputeState  m_computeState;
	CustomSharedState    m_sharedState;

#if defined(ENABLE_PROFILING_CODE)
	ERenderPrimitiveType m_primitiveTypeForProfiling;
	SProfilingStats      m_profilingStats;
#endif
};

enum EResourceTransitionType
{
	eResTransition_TextureRead
};

#include "D3D11/DeviceCommandList_D3D11.h"
#include "D3D12/DeviceCommandList_D3D12.h"
#if CRY_USE_GNM_RENDERER
	#include "GNM/DeviceCommandList_GNM.h"
#endif

class CDeviceGraphicsCommandInterface : public CDeviceGraphicsCommandInterfaceImpl
{
public:
	void PrepareUAVsForUse(uint32 viewCount, CGpuBuffer** pViews) const;
	void PrepareRenderTargetsForUse(uint32 targetCount, CTexture** pTargets, SDepthTexture* pDepthTarget, const SResourceView::KeyType* pRenderTargetViews = nullptr) const;
	void         PrepareResourcesForUse(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const;
	void         PrepareInlineConstantBufferForUse(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass) const;
	void PrepareInlineConstantBufferForUse(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages) const;
	void PrepareVertexBuffersForUse(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams) const;
	void         PrepareIndexBufferForUse(const CDeviceInputStream* indexStream) const;
	void BeginResourceTransitions(uint32 numTextures, CTexture** pTextures, EResourceTransitionType type);

	void SetRenderTargets(uint32 targetCount, CTexture* const* pTargets, const SDepthTexture* pDepthTarget, const SResourceView::KeyType* pRenderTargetViews = nullptr);
	void         SetViewports(uint32 vpCount, const D3DViewPort* pViewports);
	void         SetScissorRects(uint32 rcCount, const D3DRectangle* pRects);
	void SetPipelineState(const CDeviceGraphicsPSO* devicePSO);
	void SetResourceLayout(const CDeviceResourceLayout* resourceLayout);
	void SetResources(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage);
	void         SetInlineConstantBuffer(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass);
	void SetInlineConstantBuffer(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, ::EShaderStage shaderStages);
	void SetVertexBuffers(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams);
	void         SetIndexBuffer(const CDeviceInputStream* indexStream); // NOTE: Take care with PSO strip cut/restart value and 32/16 bit indices
	void         SetInlineConstants(uint32 bindSlot, uint32 constantCount, float* pConstants);
	void         SetStencilRef(uint8 stencilRefValue);

	void         Draw(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation);
	void         DrawIndexed(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation);

	void ClearSurface(D3DSurface* pView, const float color[4], uint32 numRects, const D3D11_RECT* pRects);
};

STATIC_ASSERT(sizeof(CDeviceGraphicsCommandInterface) == sizeof(CDeviceCommandListImpl), "CDeviceGraphicsCommandInterface cannot contain data members");

class CDeviceComputeCommandInterface : public CDeviceComputeCommandInterfaceImpl
{
public:
	void PrepareUAVsForUse(uint32 viewCount, CGpuBuffer** pViews) const;
	void PrepareResourcesForUse(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const;
	void PrepareInlineConstantBufferForUse(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlots) const;

	void SetPipelineState(const CDeviceComputePSO* pDevicePSO);
	void SetResourceLayout(const CDeviceResourceLayout* pResourceLayout);
	void SetResources(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage);
	void SetInlineConstantBuffer(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot);
	void         SetInlineConstants(uint32 bindSlot, uint32 constantCount, float* pConstants);

	void         Dispatch(uint32 X, uint32 Y, uint32 Z);

	void ClearUAV(D3DUAV* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRects);
	void ClearUAV(D3DUAV* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRects);
};

STATIC_ASSERT(sizeof(CDeviceGraphicsCommandInterface) == sizeof(CDeviceCommandListImpl), "CDeviceComputeCommandInterface cannot contain data members");

class CDeviceCommandList : public CDeviceCommandListImpl
{
	friend class CDeviceObjectFactory;

public:
	CDeviceCommandList() { Reset(); }

	CDeviceGraphicsCommandInterface* GetGraphicsInterface();
	CDeviceComputeCommandInterface*  GetComputeInterface();

	void                             Reset();
	void                             LockToThread();
	void                             Close();

#if defined(ENABLE_PROFILING_CODE)
	void                   BeginProfilingSection();
	const SProfilingStats& EndProfilingSection();
#endif
};

STATIC_ASSERT(sizeof(CDeviceCommandList) == sizeof(CDeviceCommandListImpl), "CDeviceCommandList cannot contain data members");

typedef std::shared_ptr<CDeviceCommandList> CDeviceCommandListPtr;
typedef CDeviceCommandList&                 CDeviceCommandListRef;
typedef std::unique_ptr<CDeviceCommandList> CDeviceCommandListUPtr;

////////////////////////////////////////////////////////////////////////////
// Device Object Factory

class CDeviceObjectFactory
{
public:
	CDeviceObjectFactory();

	enum EQueueType
	{
		eQueue_Graphics = 0,
		eQueue_Compute = 1,
		eQueue_Copy = 2,
		eQueue_Bundle = 3,
	};

	CDeviceResourceSetPtr    CreateResourceSet(CDeviceResourceSet::EFlags flags = CDeviceResourceSet::EFlags_None) const;
	CDeviceResourceSetPtr    CloneResourceSet(const CDeviceResourceSetPtr pSrcResourceSet) const; // NOTE: does not copy device dependent resources. be sure to call Build() before usage

	CDeviceResourceLayoutPtr CreateResourceLayout(const SDeviceResourceLayoutDesc& resourceLayoutDesc);
	CDeviceInputStream*      CreateVertexStreamSet(uint32 numStreams, const SStreamInfo* streams);
	CDeviceInputStream*      CreateIndexStreamSet(const SStreamInfo* stream);

	CDeviceGraphicsPSOPtr    CreateGraphicsPSO(const CDeviceGraphicsPSODesc& psoDesc);
	CDeviceComputePSOPtr     CreateComputePSO(const CDeviceComputePSODesc& psoDesc);

	// Get a pointer to the core graphics command-list, which runs on the command-queue assigned to Present().
	// Only the allocating thread is allowed to call functions on this command-list (DX12 restriction).
	CDeviceCommandListPtr GetCoreCommandList() const;

	// Acquire one or more command-lists which are independent of the core command-list
	// Only one thread is allowed to call functions on this command-list (DX12 restriction).
	// The thread that gets the permition is the one calling Begin() on it AFAICS
	CDeviceCommandListUPtr AcquireCommandList(EQueueType eQueueType = eQueue_Graphics);
	std::vector<CDeviceCommandListUPtr> AcquireCommandLists(uint32 listCount, EQueueType eQueueType = eQueue_Graphics);

	// Command-list sinks, will automatically submit command-lists in [global] allocation-order
	void ForfeitCommandList(CDeviceCommandListUPtr pCommandList, EQueueType eQueueType = eQueue_Graphics);
	void ForfeitCommandLists(std::vector<CDeviceCommandListUPtr> pCommandLists, EQueueType eQueueType = eQueue_Graphics);

	void ReleaseResources();
	void ReloadPipelineStates();

	void UpdatePipelineStates();

private:
	CDeviceGraphicsPSOPtr    CreateGraphicsPSOImpl(const CDeviceGraphicsPSODesc& psoDesc) const;
	CDeviceComputePSOPtr     CreateComputePSOImpl(const CDeviceComputePSODesc& psoDesc) const;

	CDeviceResourceLayoutPtr CreateResourceLayoutImpl(const SDeviceResourceLayoutDesc& resourceLayoutDesc) const;

	void                     ReleaseResourcesImpl();

	CDeviceCommandListPtr m_pCoreCommandList;

	std::unordered_map<CDeviceGraphicsPSODesc, CDeviceGraphicsPSOPtr>  m_GraphicsPsoCache;
	std::unordered_map<CDeviceGraphicsPSODesc, CDeviceGraphicsPSOWPtr> m_InvalidGraphicsPsos;

	std::unordered_map<CDeviceComputePSODesc, CDeviceComputePSOPtr>    m_ComputePsoCache;
	std::unordered_map<CDeviceComputePSODesc, CDeviceComputePSOWPtr>   m_InvalidComputePsos;

	VectorMap<SDeviceResourceLayoutDesc, CDeviceResourceLayoutPtr>     m_ResourceLayoutCache;

	template<class T, const size_t N> struct SDeviceStreamInfoHash
	{
		size_t operator()(const T& x) const
		{
			return size_t(XXH64(x, sizeof(CDeviceInputStream) * N, 61));
		}
	};

	template<class T, const size_t N> struct SDeviceStreamInfoEquality
	{
		bool operator()(const T& x, const T& y) const
		{
			return !memcmp(x, y, sizeof(CDeviceInputStream) * N);
		}
	};

	std::unordered_set<CDeviceInputStream*, SDeviceStreamInfoHash<CDeviceInputStream*, 1>, SDeviceStreamInfoEquality<CDeviceInputStream*, 1>>             m_UniqueIndexStreams;
	std::unordered_set<CDeviceInputStream*, SDeviceStreamInfoHash<CDeviceInputStream*, VSF_NUM>, SDeviceStreamInfoEquality<CDeviceInputStream*, VSF_NUM>> m_UniqueVertexStreams;
};

struct SDeviceObjectHelpers
{
	struct SShaderInstanceInfo
	{
		SShaderInstanceInfo() : pHwShader(nullptr), pHwShaderInstance(nullptr), pDeviceShader(nullptr) {}

		CHWShader_D3D* pHwShader;
		CCryNameTSCRC  technique; // temp
		void*          pHwShaderInstance;
		void*          pDeviceShader;
	};

	struct SConstantBufferBindInfo
	{
		EConstantBufferShaderSlot   shaderSlot;
		EShaderStage                shaderStages;
		_smart_ptr<CConstantBuffer> pBuffer;

		void                        swap(SConstantBufferBindInfo& other)
		{
			std::swap(shaderSlot, other.shaderSlot);
			std::swap(shaderStages, other.shaderStages);
			std::swap(pBuffer, other.pBuffer);
		}

	};

	template<typename T, size_t Alignment = CRY_PLATFORM_ALIGNMENT>
	class STypedConstants : private NoCopy
	{
	public:
		STypedConstants(CConstantBufferPtr pBuffer)
			: m_pBuffer(pBuffer)
			, m_pCachedData(AlignCachedData())
			, m_CurrentBufferIndex(0)
		{
			ZeroStruct(m_pCachedData[0]);
		}

		STypedConstants(STypedConstants&& other)
			: m_pBuffer(std::move(other.m_pBuffer))
			, m_pCachedData(AlignCachedData())
			, m_CurrentBufferIndex(std::move(other.m_CurrentBufferIndex))
		{
			*m_pCachedData = *other.m_pCachedData;
		}

		void CommitChanges()
		{
			if (m_pBuffer)
			{
				m_pBuffer->UpdateBuffer(m_pCachedData, Align(sizeof(T), Alignment), m_CurrentBufferIndex + 1);
			}
		}

		void BeginStereoOverride(bool bCopyLeftEyeData = true)
		{
			CRY_ASSERT(m_CurrentBufferIndex < CCamera::eEye_eCount - 1);

			if (bCopyLeftEyeData)
			{
				m_pCachedData[m_CurrentBufferIndex + 1] = m_pCachedData[m_CurrentBufferIndex];
			}

			++m_CurrentBufferIndex;
		}

		T*   operator->() { return &m_pCachedData[m_CurrentBufferIndex]; }

	private:
		// NOTE: enough memory to hold an aligned struct size + the adjustment of a possible unaligned start
		uint8              m_CachedMemory[((CCamera::eEye_eCount * sizeof(T) + (Alignment - 1)) & (~(Alignment - 1))) + (Alignment - 1)];
		T* AlignCachedData() { return reinterpret_cast<T*>(Align(uintptr_t(m_CachedMemory), Alignment)); }

	private:
		T*                 m_pCachedData;
		CConstantBufferPtr m_pBuffer;
		int                m_CurrentBufferIndex;
	};

	class CShaderConstantManager
	{
	public:
		CShaderConstantManager();
		CShaderConstantManager(CShaderConstantManager&& other);
		~CShaderConstantManager() {};

		////////// Constant update via shader reflection //////////
		bool InitShaderReflection(::CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags);
		bool IsShaderReflectionValid() const { return m_pShaderReflection && m_pShaderReflection->bValid; }
		void ReleaseShaderReflection();

		void BeginNamedConstantUpdate();
		void EndNamedConstantUpdate();

		bool SetNamedConstant(EHWShaderClass shaderClass, const CCryNameR& paramName, const Vec4 param);
		bool SetNamedConstantArray(EHWShaderClass shaderClass, const CCryNameR& paramName, const Vec4 params[], uint32 numParams);

		////////// Manual constant update via typed buffers //////////
		bool SetTypedConstantBuffer(EConstantBufferShaderSlot shaderSlot, CConstantBuffer* pBuffer, EShaderStage shaderStages = EShaderStage_Pixel);

		template<typename T>
		STypedConstants<T> BeginTypedConstantUpdate(EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages = EShaderStage_Pixel) const;

		template<typename T>
		void EndTypedConstantUpdate(STypedConstants<T>& constants) const;
		//////////////////////////////////////////////////////////////

		// Returns all managed constant buffers
		const std::vector<SConstantBufferBindInfo>& GetBuffers() const { return m_constantBuffers; }

	private:

		struct SReflectedBufferUpdateContext
		{
			SReflectedBufferUpdateContext() : shaderClass(eHWSC_Num), vectorCount(0) {}

			EHWShaderClass              shaderClass; // somewhat redundant but avoids a loop over shader stages in Begin
			int                         vectorCount;
			_smart_ptr<CConstantBuffer> pPreviousBuffer;
			SShaderInstanceInfo         shaderInfo;
		};

		struct SShaderReflection
		{
			SShaderReflection() : pShader(nullptr), bValid(false) {}

			CShader*      pShader;
			CCryNameTSCRC technique;
			bool          bValid;

			VectorMap<int, SReflectedBufferUpdateContext> bufferUpdateContexts;
		};

	protected:
		void ReleaseReflectedBuffers();

		std::vector<SConstantBufferBindInfo> m_constantBuffers;
		std::unique_ptr<SShaderReflection>   m_pShaderReflection;
	};

	// Get shader instances for each shader stage
	static std::array<SShaderInstanceInfo, eHWSC_Num> GetShaderInstanceInfo(::CShader* pShader, const CCryNameTSCRC& technique, uint64 rtFlags, uint32 mdFlags, uint32 mdvFlags, UPipelineState pipelineState[eHWSC_Num], bool bAllowTesselation);

	// Check if shader has tessellation support
	static bool CheckTessellationSupport(SShaderItem& shaderItem);
};

template<typename T>
SDeviceObjectHelpers::STypedConstants<T> SDeviceObjectHelpers::CShaderConstantManager::BeginTypedConstantUpdate(EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages) const
{
	for (int i = 0, end = m_constantBuffers.size(); i < end; ++i)
	{
		auto& cb = m_constantBuffers[i];

		if (cb.shaderSlot == shaderSlot && cb.shaderStages == shaderStages)
		{
			CRY_ASSERT(!m_pShaderReflection || m_pShaderReflection->bufferUpdateContexts.find(i) == m_pShaderReflection->bufferUpdateContexts.end());
			return std::move(SDeviceObjectHelpers::STypedConstants<T>(cb.pBuffer));
		}
	}

	CRY_ASSERT(false);
	return std::move(SDeviceObjectHelpers::STypedConstants<T>(nullptr));
}

template<typename T>
void SDeviceObjectHelpers::CShaderConstantManager::EndTypedConstantUpdate(SDeviceObjectHelpers::STypedConstants<T>& constants) const
{
	constants.CommitChanges();
}

////////////////////////////////////////////////////////////////////////////
struct SScopedComputeCommandList
{
	CDeviceCommandListUPtr m_pCommandList;
	CDeviceComputeCommandInterface* m_pComputeInterface;
	bool m_bAsynchronousCommandList;

	SScopedComputeCommandList(bool bAsynchronousCommandList)
	{
		CDeviceObjectFactory& pFactory = CCryDeviceWrapper::GetObjectFactory();

		if ((m_bAsynchronousCommandList = bAsynchronousCommandList))
		{
			m_pCommandList = pFactory.AcquireCommandList(CDeviceObjectFactory::eQueue_Compute);
			m_pCommandList->LockToThread();
			m_pComputeInterface = m_pCommandList->GetComputeInterface();
		}
		else
		{
			auto pCommandList = pFactory.GetCoreCommandList();
			m_pComputeInterface = pCommandList->GetComputeInterface();
		}
	}

	~SScopedComputeCommandList()
	{
		CDeviceObjectFactory& pFactory = CCryDeviceWrapper::GetObjectFactory();

		if (m_bAsynchronousCommandList)
		{
			m_pCommandList->Close();
			pFactory.ForfeitCommandList(std::move(m_pCommandList), CDeviceObjectFactory::eQueue_Compute);
		}
	}

	operator CDeviceComputeCommandInterface*() const
	{
		return m_pComputeInterface;
	}

	operator CDeviceCommandListRef() const
	{
		if (m_bAsynchronousCommandList)
			return *m_pCommandList.get();
		else
			return *CCryDeviceWrapper::GetObjectFactory().GetCoreCommandList();
	}
};

#include "DeviceCommandList.inl"
