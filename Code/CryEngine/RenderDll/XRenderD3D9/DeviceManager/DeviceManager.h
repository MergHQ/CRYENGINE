// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _DeviceManager_H_
#define _DeviceManager_H_

#define DEVMAN_USE_STAGING_POOL

// Might be used in future for defragging
#if CRY_PLATFORM_DURANGO
	#define DEVMAN_USE_PINNING
#endif

#if CRY_PLATFORM_ORBIS
	#define DEVICE_MANAGER_IMMEDIATE_STATE_WRITE 1
#else
	#define DEVICE_MANAGER_IMMEDIATE_STATE_WRITE 0
#endif

class SGPUMemHdl
{
public:
	SGPUMemHdl()
		: m_handleAndFlags(0)
	{
	}

	explicit SGPUMemHdl(IDefragAllocator::Hdl hdl)
		: m_handleAndFlags((UINT_PTR)hdl << FlagsShift)
	{
	}

	explicit SGPUMemHdl(void* pFixed)
		: m_handleAndFlags((UINT_PTR)pFixed | IsFixedFlag)
	{
	}

	ILINE bool IsValid() const
	{
		return m_handleAndFlags != 0;
	}

	ILINE int IsFixed() const
	{
		return m_handleAndFlags & IsFixedFlag;
	}

	void* GetFixedAddress() const
	{
		assert(IsFixed());
		return (void*)(m_handleAndFlags & ~FlagsMask);
	}

	IDefragAllocator::Hdl GetHandle() const
	{
		assert(!IsFixed());
		return (IDefragAllocator::Hdl)(m_handleAndFlags >> FlagsShift);
	}

	friend bool operator == (const SGPUMemHdl& a, const SGPUMemHdl& b)
	{
		return a.m_handleAndFlags == b.m_handleAndFlags;
	}

	friend bool operator < (const SGPUMemHdl& a, const SGPUMemHdl& b)
	{
		return a.m_handleAndFlags < b.m_handleAndFlags;
	}

private:
	enum
	{
		IsFixedFlag = 0x1,
		FlagsMask = 0x1,
		FlagsShift = 1,
	};

private:
	UINT_PTR m_handleAndFlags;
};

class CDeviceTexture;
class CDeviceVertexBuffer;
class CDeviceIndexBuffer;
class CCryDeviceContextWrapper;
class CConstantBuffer;

typedef CDeviceTexture* LPDEVICETEXTURE;
typedef UINT_PTR        DeviceFenceHandle;

struct STextureInfoData
{
	const void*   pSysMem;
	uint32        SysMemPitch;
	uint32        SysMemSlicePitch;
	ETEX_TileMode SysMemTileMode;
};

struct STextureInfo
{
	uint8             m_nMSAASamples;
	uint8             m_nMSAAQuality;
	STextureInfoData* m_pData;

	STextureInfo()
	{
		m_nMSAASamples = 1;
		m_nMSAAQuality = 0;
		m_pData = NULL;
	}
};

#ifdef CRY_USE_DX12
#else
	#if CRY_PLATFORM_DURANGO
		#include "D3D11/DeviceManager_D3D11_Durango.h"
	#endif

	#if defined(USE_NV_API)
		#include "D3D11/DeviceManager_D3D11_NVAPI.h"
	#endif
#endif

//===============================================================================================================

class CDeviceManager
{
	friend class CCryDX12DeviceContext;

public:
	enum SHADER_TYPE
	{
		TYPE_VS = 0,
		TYPE_PS,
		TYPE_GS,
		TYPE_DS,
		TYPE_HS,
		TYPE_CS,
		MAX_TYPE,
	};

	enum
	{
		MAX_BOUND_CB       = 16,
		MAX_BOUND_VBS      = 16,
		MAX_BOUND_SRVS     = 128,
		MAX_BOUND_UAVS     = 64,
		MAX_BOUND_SAMPLERS = 16,
		MAX_SRV_DIRTY      = MAX_BOUND_SRVS / 32,
		MAX_UAV_DIRTY      = MAX_BOUND_UAVS / 32,
		SRV_DIRTY_SHIFT    = 5,
		SRV_DIRTY_MASK     = 31,
		UAV_DIRTY_SHIFT    = 5,
		UAV_DIRTY_MASK     = 31
	};

private:
	friend class CD3DRenderer;
	friend class CDeviceTexture;

#if BUFFER_ENABLE_DIRECT_ACCESS && CRY_PLATFORM_DURANGO
	static const GUID BufferPointerGuid;
#endif

private:

#if !DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	// Constant Buffers
	struct
	{
		D3DBuffer* buffers[MAX_BOUND_CB];
		D3DBuffer* buffers1[MAX_BOUND_CB];
		uint32     offsets[MAX_BOUND_CB];
		uint32     sizes[MAX_BOUND_CB];
		uint32     dirty;
		uint32     dirty1;
	} m_CB[MAX_TYPE];

	// Shader Resource Views
	struct
	{
		D3DShaderResource* views[MAX_BOUND_SRVS];
		uint32             dirty[MAX_SRV_DIRTY];
	} m_SRV[MAX_TYPE];

	// Unordered Access Views
	struct
	{
		D3DUAV* views[MAX_BOUND_UAVS];
		uint32  counts[MAX_BOUND_UAVS];
		uint32  dirty[MAX_UAV_DIRTY];
	} m_UAV[MAX_TYPE];

	// SamplerStates
	struct
	{
		D3DSamplerState* samplers[MAX_BOUND_SAMPLERS];
		uint32           dirty;
	} m_Samplers[MAX_TYPE];

	// VertexBuffers
	struct
	{
		D3DBuffer* buffers[MAX_BOUND_VBS];
		uint32     offsets[MAX_BOUND_VBS];
		uint32     strides[MAX_BOUND_VBS];
		uint32     dirty;
	} m_VBs;

	// IndexBuffer
	struct
	{
		D3DBuffer* buffer;
		uint32     offset;
		int        format;
		uint32     dirty;
	} m_IB;

	struct
	{
		D3DVertexDeclaration* decl;
		bool                  dirty;
	} m_VertexDecl;

	struct
	{
		D3D11_PRIMITIVE_TOPOLOGY topology;
		bool                     dirty;
	} m_Topology;

	struct
	{
		ID3D11DepthStencilState* dss;
		uint32                   stencilref;
		bool                     dirty;
	} m_DepthStencilState;

	struct
	{
		ID3D11BlendState* pBlendState;
		float             BlendFactor[4];
		uint32            SampleMask;
		bool              dirty;
	} m_BlendState;

	struct
	{
		ID3D11RasterizerState* pRasterizerState;
		bool                   dirty;
	} m_RasterState;

	struct
	{
		ID3D11Resource* shader;
		bool            dirty;
	} m_Shaders[MAX_TYPE];

	// The buffer invalidations
	struct SBufferInvalidation
	{
		D3DBuffer* buffer;
		void*      base_ptr;
		size_t     offset;
		size_t     size;
		bool operator<(const SBufferInvalidation& other) const
		{
			if (buffer == other.buffer)
			{
				return offset < other.offset;
			}
			return buffer < other.buffer;
		}
		bool operator!=(const SBufferInvalidation& other) const
		{
			return buffer != other.buffer
	#if CRY_PLATFORM_DURANGO // Should be removed when we have range based invalidations
			       && offset != other.offset
	#endif
			;
		}
	};

	typedef std::vector<SBufferInvalidation> BufferInvalidationsT;
	BufferInvalidationsT m_buffer_invalidations[2];
#endif

	uint32 m_numInvalidDrawcalls;

#if DURANGO_ENABLE_ASYNC_DIPS
	// counter to cap the number of device calls which are executed as a AsyncDIP Job
	uint32 m_nAsyncDipJobCounter;

	void BindConstantBuffersAsync(SHADER_TYPE type);
	void BindOffsetConstantBuffersAsync(SHADER_TYPE type);
	void BindSRVsAsync(SHADER_TYPE type);
	void BindUAVsAsync(SHADER_TYPE type);
	void BindSamplersAsync(SHADER_TYPE type);
	void BindIAAsync();
	void BindShaderAsync(SHADER_TYPE type);
	void BindStateAsync();
	void CommitDeviceStatesAsync();
#endif

	// Internal handle for debugging
	DeviceFenceHandle m_fence_handle;

	void BindConstantBuffers(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext);
	void BindOffsetConstantBuffers(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext);
	void BindSRVs(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext);
	void BindUAVs(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext);
	void BindSamplers(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext);
	void BindShader(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext);
	void BindState(CCryDeviceContextWrapper& rDeviceContext);
	void BindIA(CCryDeviceContextWrapper& rDeviceContext);

	bool ValidateDrawcall();

public:
	enum
	{
		USAGE_DIRECT_ACCESS              = BIT(0),
		USAGE_DIRECT_ACCESS_CPU_COHERENT = BIT(1),
		USAGE_DIRECT_ACCESS_GPU_COHERENT = BIT(2),

		USAGE_DEPTH_STENCIL              = BIT(17),
		USAGE_RENDER_TARGET              = BIT(18),
		USAGE_DYNAMIC                    = BIT(19),
		USAGE_AUTOGENMIPS                = BIT(20),
		USAGE_STREAMING                  = BIT(21),
		USAGE_STAGE_ACCESS               = BIT(22),
		USAGE_STAGING                    = BIT(23),
		USAGE_IMMUTABLE                  = BIT(24),
		USAGE_DEFAULT                    = BIT(25),
		USAGE_CPU_READ                   = BIT(26),
		USAGE_CPU_WRITE                  = BIT(27),
		USAGE_CPU_CACHED_MEMORY          = BIT(28), // This flag is now redundant
		USAGE_UNORDERED_ACCESS           = BIT(29),
		USAGE_ETERNAL                    = BIT(30),
		USAGE_UAV_RWTEXTURE              = BIT(31),

		USAGE_CUSTOM                     = (USAGE_DEPTH_STENCIL | USAGE_RENDER_TARGET | USAGE_DYNAMIC | USAGE_AUTOGENMIPS)
	};

	// Resource Usage	| Default	| Dynamic	| Immutable	| Staging
	// -----------------+-----------+-----------+-----------+--------
	// GPU - Read       | yes       | yes1      | yes       | yes1, 2
	// GPU - Write      | yes1      |           |           | yes1, 2
	// CPU - Read       |           |           |           | yes1, 2
	// CPU - Write      |           | yes       |           | yes1, 2
	//
	// 1 - This is restricted to ID3D11DeviceContext::CopySubresourceRegion, ID3D11DeviceContext::CopyResource,
	//     ID3D11DeviceContext::UpdateSubresource, and ID3D11DeviceContext::CopyStructureCount.
	// 2 - Cannot be a depth - stencil buffer or a multi-sampled render target.

	enum
	{
		BIND_VERTEX_BUFFER    = BIT(0),
		BIND_INDEX_BUFFER     = BIT(1),
		BIND_CONSTANT_BUFFER  = BIT(2),
		BIND_SHADER_RESOURCE  = BIT(3),
		BIND_STREAM_OUTPUT    = BIT(4),
		BIND_RENDER_TARGET    = BIT(5),
		BIND_DEPTH_STENCIL    = BIT(6),
		BIND_UNORDERED_ACCESS = BIT(7)
	};

	CDeviceManager();
	~CDeviceManager();

	void           SyncToGPU();

	void           Init();
	void           RT_Tick();

	HRESULT        Create2DTexture(uint32 nWidth, uint32 nHeight, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = nullptr, int32 nESRAMOffset = -1);
	HRESULT        CreateCubeTexture(uint32 nSize, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = nullptr);
	HRESULT        CreateVolumeTexture(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = nullptr);

	HRESULT        Create2DTexture(const D3D11_TEXTURE2D_DESC& Desc, const ColorF& cClearValue, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = nullptr);
	HRESULT        CreateCubeTexture(const D3D11_TEXTURE2D_DESC& Desc, const ColorF& cClearValue, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = nullptr);
	HRESULT        CreateVolumeTexture(const D3D11_TEXTURE3D_DESC& Desc, const ColorF& cClearValue, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI = nullptr);

	HRESULT        CreateBuffer(uint32 nSize, uint32 elemSize, int32 nUsage, int32 nBindFlags, D3DBuffer** ppBuff);

	HRESULT        CreateFence(DeviceFenceHandle& query);
	HRESULT        ReleaseFence(DeviceFenceHandle query);
	HRESULT        IssueFence(DeviceFenceHandle query);
	HRESULT        SyncFence(DeviceFenceHandle query, bool block, bool flush = true);

	static HRESULT InvalidateGpuCache(D3DBuffer* buffer, void* base_ptr, size_t size, size_t offset);
	static HRESULT InvalidateCpuCache(void* base_ptr, size_t size, size_t offset);
	HRESULT        InvalidateResourceGpuCache(D3DBuffer* buffer);
	void           InvalidateBuffer(D3DBuffer* buffer, void* base_ptr, size_t offset, size_t size, uint32 id);

	inline void    BindConstantBuffer(SHADER_TYPE type, const CConstantBuffer* Buffer, uint32 slot);
	inline void    BindConstantBuffer(SHADER_TYPE type, const CConstantBuffer* Buffer, uint32 slot, uint32 offset, uint32 size);
	inline void    BindCBs(SHADER_TYPE type, const CConstantBuffer* const* Buffer, uint32 start_slot, uint32 count);
	inline void    BindConstantBuffer(SHADER_TYPE type, D3DBuffer* Buffer, uint32 slot);
	inline void    BindConstantBuffer(SHADER_TYPE type, D3DBuffer* Buffer, uint32 slot, uint32 offset, uint32 size);
	inline void    BindCBs(SHADER_TYPE type, D3DBuffer* const* Buffer, uint32 start_slot, uint32 count);
	inline void    BindSRV(SHADER_TYPE type, D3DShaderResource* SRV, uint32 slot);
	inline void    BindSRV(SHADER_TYPE type, D3DShaderResource* const* SRV, uint32 start_slot, uint32 count);
	inline void    BindUAV(SHADER_TYPE type, D3DUAV* UAV, uint32 counts, uint32 slot);
	inline void    BindUAV(SHADER_TYPE type, D3DUAV* const* UAV, const uint32* counts, uint32 start_slot, uint32 count);
	inline void    BindSampler(SHADER_TYPE type, D3DSamplerState* Sampler, uint32 slot);
	inline void    BindSampler(SHADER_TYPE type, D3DSamplerState* const* Samplers, uint32 start_slot, uint32 count);
	inline void    BindVB(D3DBuffer* VB, uint32 slot, uint32 offset, uint32 stride);
	inline void    BindVB(uint32 start, uint32 count, D3DBuffer* const* Buffers, const uint32* offset, const uint32* stride);
	inline void    BindIB(D3DBuffer* Buffer, uint32 offset, DXGI_FORMAT format);
	inline void    BindVtxDecl(D3DVertexDeclaration* decl);
	inline void    BindTopology(D3D11_PRIMITIVE_TOPOLOGY top);
	inline void    BindShader(SHADER_TYPE type, ID3D11Resource* shader);
	inline void    SetDepthStencilState(ID3D11DepthStencilState* dss, uint32 stencilref);
	inline void    SetBlendState(ID3D11BlendState* pBlendState, float* BlendFactor, uint32 SampleMask);
	inline void    SetRasterState(ID3D11RasterizerState* pRasterizerState);

	void           CommitDeviceStates();
	void           Draw(uint32 nVerticesCount, uint32 nStartVertex);
	void           DrawInstanced(uint32 nInstanceVerts, uint32 nInsts, uint32 nStartVertex, uint32 nStartInstance);
	void           DrawIndexed(uint32, uint32, uint32);
	void           DrawIndexedInstanced(uint32 numIndices, uint32 nInsts, uint32 startIndex, uint32 v0, uint32 v1);
	void           DrawIndexedInstancedIndirect(ID3D11Buffer*, uint32);
	void           Dispatch(uint32, uint32, uint32);
	void           DispatchIndirect(ID3D11Buffer*, uint32);

#if CRY_PLATFORM_DURANGO
	IDefragAllocatorStats GetTexturePoolStats();

	size_t GetTexturePoolSize() const { return m_texturePool.GetPoolSize(); }
	size_t GetTexturePoolAllocated() const { return m_texturePool.GetPoolAllocated(); }
#endif

	uint32 GetNumInvalidDrawcalls()
	{
		return m_numInvalidDrawcalls;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	static uint8* Map(D3DBuffer* buffer, uint32 subresource, size_t offset, size_t size, D3D11_MAP mode);
	static void Unmap(D3DBuffer* buffer, uint32 subresource, size_t offset, size_t size, D3D11_MAP mode);

	// NOTE: Standard behaviour in the presence of multiple GPUs is to make the same data available to all
	// GPUs. If data should diverge per GPU, it can be uploaded by concatenating multiple divergent data-blocks
	// and passing the appropriate "numDataBlocks". Each GPU will then receive it's own version of the data.
	template<const bool bDirectAccess = false>
	static void UploadContents(D3DBuffer* buffer, uint32 subresource, size_t offset, size_t size, D3D11_MAP mode, const void* pInDataCPU, void* pOutDataGPU = nullptr, UINT numDataBlocks = 1);
	template<const bool bDirectAccess = false>
	static void DownloadContents(D3DBuffer* buffer, uint32 subresource, size_t offset, size_t size, D3D11_MAP mode, void* pOutDataCPU, const void* pInDataGPU = nullptr, UINT numDataBlocks = 1);

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// On UMA system, return the pointer to the start of the buffer
	static inline void ExtractBasePointer(D3DBuffer* buffer, D3D11_MAP mode, uint8*& base_ptr)
	{
#if BUFFER_ENABLE_DIRECT_ACCESS
	#if CRY_PLATFORM_ORBIS
		base_ptr = (uint8*)buffer->GetData();
	#endif
	#if CRY_PLATFORM_DURANGO
		// Note: temporary solution, this should be removed as soon as the device
		// layer for Durango is available
		void* data;
		unsigned size = sizeof(data);
		HRESULT hr = buffer->GetPrivateData(BufferPointerGuid, &size, &data);
		CHECK_HRESULT(hr);
		base_ptr = reinterpret_cast<uint8*>(data);
	#endif
	#if defined(CRY_USE_DX12)
		base_ptr = CDeviceManager::Map(buffer, 0, 0, 0, mode /* MAP_DISCARD could affect the ptr */);
	#endif
#else
		base_ptr = NULL;
#endif
	}

	static inline void ReleaseBasePointer(D3DBuffer* buffer)
	{
#if BUFFER_ENABLE_DIRECT_ACCESS
#if defined(CRY_USE_DX12)
		CDeviceManager::Unmap(buffer, 0, 0, 0, D3D11_MAP(0));
#endif
#endif
	}

	static inline uint8 MarkReadRange(D3DBuffer* buffer, size_t offset, size_t size, D3D11_MAP mode)
	{
#if defined(CRY_USE_DX12)
		DX12_ASSERT(mode == D3D11_MAP_READ || mode == D3D11_MAP_WRITE, "No other access specifier than READ/WRITE allowed for marking!");
		CDeviceManager::Map(buffer, 0, offset, (mode & D3D11_MAP_READ ? size : 0), D3D11_MAP(0));
#endif

		return uint8(mode);
	}

	static inline uint8 MarkWriteRange(D3DBuffer* buffer, size_t offset, size_t size, uint8 marker)
	{
#if defined(CRY_USE_DX12)
		DX12_ASSERT(marker == D3D11_MAP_READ || marker == D3D11_MAP_WRITE, "No other access specifier than READ/WRITE allowed for marking!");
		CDeviceManager::Unmap(buffer, 0, offset, (marker & D3D11_MAP_WRITE ? size : 0), D3D11_MAP(0));
#endif

		return uint8(marker);
	}

#if CRY_PLATFORM_DURANGO
	// Note: temporary solution, this should be removed as soon as the device
	// layer for Durango is available
	static void* GetBackingStorage(D3DBuffer* buffer);
	static void  FreebackingStorage(void* base_ptr);

	struct STileRequest
	{
		STileRequest()
			: pLinSurfaceSrc(NULL)
			, nDstSubResource(0)
			, bSrcInGPUMemory(false)
		{
		}

		const void* pLinSurfaceSrc;
		int         nDstSubResource;
		bool        bSrcInGPUMemory;
	};

	HRESULT BeginTileFromLinear2D(CDeviceTexture* pDst, const STileRequest* pSubresources, size_t nSubresources, UINT64& fenceOut);
#endif

	D3DResource* AllocateNullResource(D3D11_RESOURCE_DIMENSION eType);
	void         ReleaseNullResource(D3DResource* pNullResource);

private:
	D3DResource* m_NullResources[D3D11_RESOURCE_DIMENSION_TEXTURE3D + 1];

#if defined(DEVMAN_USE_STAGING_POOL)
public:
	D3DResource* AllocateStagingResource(D3DResource* pForTex, bool bUpload);
	void         ReleaseStagingResource(D3DResource* pStagingTex);

	#if defined(DEVMAN_USE_STAGING_POOL) && !defined(CRY_USE_DX12)
private:
	struct StagingTextureDef
	{
		D3D11_TEXTURE2D_DESC desc;
		D3DTexture*          pStagingResource;

		friend bool operator==(const StagingTextureDef& a, const D3D11_TEXTURE2D_DESC& b)
		{
			return memcmp(&a.desc, &b, sizeof(b)) == 0;
		}
	};

	typedef std::vector<StagingTextureDef, stl::STLGlobalAllocator<StagingTextureDef>> StagingPoolVec;

private:
	StagingPoolVec m_stagingPool;
	#endif
#endif

#if CRY_PLATFORM_DURANGO
private:
	typedef std::map<SMinimisedTexture2DDesc, SDeviceTextureDesc, std::less<SMinimisedTexture2DDesc>, stl::STLGlobalAllocator<std::pair<SMinimisedTexture2DDesc, SDeviceTextureDesc>>> TLayoutTableMap;

	static bool               InPlaceConstructable(const D3D11_TEXTURE2D_DESC& Desc);
	HRESULT                   CreateInPlaceTexture2D(const D3D11_TEXTURE2D_DESC& D3DDesc, const STextureInfoData* pSRD, CDeviceTexture*& pDevTexOut, bool bDeferD3DConstruction);
	const SDeviceTextureDesc* Find2DResourceLayout(const D3D11_TEXTURE2D_DESC& desc, ETEX_TileMode tileMode);

	CDurangoGPUMemoryManager       m_texturePool;
	CDurangoGPURingMemAllocator    m_textureStagingRing;
	CryCriticalSectionNonRecursive m_layoutTableLock;
	TLayoutTableMap                m_layoutTable;

	#if DURANGO_ENABLE_ASYNC_DIPS == 1
	// fields for the AsyncDIP Job queue
	enum { nQueueSize = 2048 }; // size of the queue to store the job parameters

	// submit new AsyncDIP packages to the queue
	void SubmitAsyncDipJob(char* start, char* end, volatile int* nSyncState);

	// struct to define the job parameters
	struct SParam
	{
		char*         start;
		char*         end;
		volatile int* nSyncState;
	};

	CRY_ALIGN(64) uint32 m_nPush;
	CRY_ALIGN(64) uint32 m_nPull;
	CRY_ALIGN(64) SParam m_arrParams[nQueueSize];
public:

	// execute async dip jobs (should be called by the job system during idle time)
	void ExecuteAsyncDIP();
	#endif // DURANGO_ENABLE_ASYNC_DIPS
#endif   // CRY_PLATFORM_DURANGO
};

//====================================================================================================

struct STexturePairBlock;

class CDeviceTexture
{
	friend class CDeviceManager;

	D3DBaseTexture* m_pD3DTexture;
	// for native hand-made textures
	size_t          m_nBaseAllocatedSize;

	bool            m_bNoDelete;
	bool            m_bCube;

#ifdef DEVMAN_USE_STAGING_POOL
	D3DResource* m_pStagingResource[2];
	void*        m_pStagingMemory[2];
#endif
#ifdef DEVMAN_USE_PINNING
	SGPUMemHdl m_gpuHdl;
#endif

#if defined(USE_NV_API)
	void* m_handleMGPU;
#endif
#if CRY_PLATFORM_DURANGO
	const SDeviceTextureDesc* m_pLayout;
	uint32                    m_nBaseAddressInvalidated;
#endif

#if defined(DEVICE_TEXTURE_STORE_OWNER)
	CTexture* m_pDebugOwner;
#endif

public:

	inline D3DBaseTexture* GetBaseTexture()
	{
		return m_pD3DTexture;
	}
	D3DLookupTexture* GetLookupTexture()
	{
		return (D3DLookupTexture*)GetBaseTexture();
	}
	D3DTexture* Get2DTexture()
	{
		return (D3DTexture*)GetBaseTexture();
	}
	D3DCubeTexture* GetCubeTexture()
	{
		return (D3DCubeTexture*)GetBaseTexture();
	}
	D3DVolumeTexture* GetVolumeTexture()
	{
		return (D3DVolumeTexture*)GetBaseTexture();
	}

	bool IsCube() const
	{
		return m_bCube;
	}

	void Unbind();

#if defined(DEVICE_TEXTURE_STORE_OWNER)
	void SetOwner(CTexture* pOwner) { m_pDebugOwner = pOwner; }
#else
	void SetOwner(CTexture* pOwner) {}
#endif

#if defined(USE_NV_API)
	void DisableMgpuSync();
	void MgpuResourceUpdate(bool bUpdating = true);
#endif

	CDeviceTexture() : m_pD3DTexture(NULL), m_nBaseAllocatedSize(0), m_bNoDelete(false), m_bCube(false)
#if defined(USE_NV_API)
		, m_handleMGPU(NULL)
#endif
#if CRY_PLATFORM_DURANGO
		, m_pLayout(NULL)
		, m_nBaseAddressInvalidated(0)
#endif
#if defined(DEVICE_TEXTURE_STORE_OWNER)
		, m_pDebugOwner(NULL)
#endif
	{
#ifdef DEVMAN_USE_STAGING_POOL
		m_pStagingResource[0] = m_pStagingResource[1] = nullptr;
		m_pStagingMemory[0] = m_pStagingMemory[1] = nullptr;
#endif
	}

	CDeviceTexture(D3DBaseTexture* pBaseTexture) : m_pD3DTexture(pBaseTexture), m_nBaseAllocatedSize(0), m_bNoDelete(false), m_bCube(false)
#if defined(USE_NV_API)
		, m_handleMGPU(NULL)
#endif
#if CRY_PLATFORM_DURANGO
		, m_pLayout(NULL)
		, m_nBaseAddressInvalidated(0)
#endif
#if defined(DEVICE_TEXTURE_STORE_OWNER)
		, m_pDebugOwner(NULL)
#endif
	{
#ifdef DEVMAN_USE_STAGING_POOL
		m_pStagingResource[0] = m_pStagingResource[1] = nullptr;
		m_pStagingMemory[0] = m_pStagingMemory[1] = nullptr;
#endif
	}

	CDeviceTexture(D3DCubeTexture* pBaseTexture) : m_pD3DTexture(pBaseTexture), m_nBaseAllocatedSize(0), m_bNoDelete(false), m_bCube(true)
#if defined(USE_NV_API)
		, m_handleMGPU(NULL)
#endif
#if CRY_PLATFORM_DURANGO
		, m_pLayout(NULL)
		, m_nBaseAddressInvalidated(0)
#endif
#if defined(DEVICE_TEXTURE_STORE_OWNER)
		, m_pDebugOwner(NULL)
#endif
	{
#ifdef DEVMAN_USE_STAGING_POOL
		m_pStagingResource[0] = m_pStagingResource[1] = nullptr;
		m_pStagingMemory[0] = m_pStagingMemory[1] = nullptr;
#endif
	}

	~CDeviceTexture();

	typedef std::function<bool (void*, uint32, uint32)> StagingHook;

	void   DownloadToStagingResource(uint32 nSubRes, StagingHook cbTransfer);
	void   DownloadToStagingResource(uint32 nSubRes);
	void   UploadFromStagingResource(uint32 nSubRes, StagingHook cbTransfer);
	void   UploadFromStagingResource(uint32 nSubRes);
	void   AccessCurrStagingResource(uint32 nSubRes, bool forUpload, StagingHook cbTransfer);

	size_t GetDeviceSize() const
	{
		return m_nBaseAllocatedSize;
	}

	static uint32 TextureDataSize(D3DBaseView* pView);
	static uint32 TextureDataSize(D3DBaseView* pView, const uint numRects, const RECT* pRects);

#if CRY_PLATFORM_DURANGO
	static uint32 TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF, ETEX_TileMode eTM = eTM_Optimal);
#else
	static uint32 TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF);
#endif

#ifdef DEVMAN_USE_PINNING
	void* WeakPin();
	void* Pin();
	void Unpin();
	bool IsMoveable() const { return m_gpuHdl.IsValid() && !m_gpuHdl.IsFixed(); }

#if CRY_PLATFORM_DURANGO
	void GpuPin();
	void GpuUnpin(ID3DXboxPerformanceContext* pCtx);
	void GpuUnpin(ID3D11DmaEngineContextX* pCtx);
#endif

#endif

#if CRY_PLATFORM_DURANGO
	void InitD3DTexture();
	bool IsInPool() const { return m_gpuHdl.IsValid(); }
	size_t GetSurfaceOffset(int nMip, int nSlice) const
	{
		const XG_RESOURCE_LAYOUT& l = m_pLayout->layout;
		return l.Plane[0].MipLayout[nMip].OffsetBytes + l.Plane[0].MipLayout[nMip].Slice2DSizeBytes * nSlice;
	}
	size_t GetSurfaceSize(int nMip) const
	{
		const XG_RESOURCE_LAYOUT& l = m_pLayout->layout;
		return l.Plane[0].MipLayout[nMip].Slice2DSizeBytes;
	}
	XG_TILE_MODE GetXGTileMode() const
	{
		const XG_RESOURCE_LAYOUT& l = m_pLayout->layout;
		return l.Plane[0].MipLayout[0].TileMode;
	}
	void GPUFlush();
	const SDeviceTextureDesc* GetLayout() const { return m_pLayout; }

	void ReplaceTexture(ID3D11Texture2D* pReplacement);
	uint32 GetBaseAddressInvalidated() const { return m_nBaseAddressInvalidated; }
#endif

	void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	int32 Release();
	void  SetNoDelete(bool noDelete)
	{
		m_bNoDelete = noDelete;
	}

private:
	CDeviceTexture(const CDeviceTexture&);
	CDeviceTexture& operator=(const CDeviceTexture&);

private:
	int Cleanup();
};

#ifdef DEVMAN_USE_PINNING
class CDeviceTexturePin
{
public:
	CDeviceTexturePin(CDeviceTexture* pDevTex)
		: m_pTexture(pDevTex)
	{
		m_pBaseAddress = pDevTex->Pin();
	}

	~CDeviceTexturePin()
	{
		m_pTexture->Unpin();
	}

	void* GetBaseAddress() const { return m_pBaseAddress; }

#if CRY_PLATFORM_DURANGO
	void* GetSurfaceAddress(int nMip, int nSlice) const
	{
		return (char*)m_pBaseAddress + m_pTexture->GetSurfaceOffset(nMip, nSlice);
	}
#endif

private:
	CDeviceTexturePin(const CDeviceTexturePin&);
	CDeviceTexturePin& operator=(const CDeviceTexturePin&);

private:
	CDeviceTexture* m_pTexture;
	void* m_pBaseAddress;
};

#if CRY_PLATFORM_DURANGO

class CDeviceTextureGpuPin
{
public:
	CDeviceTextureGpuPin(ID3DXboxPerformanceContext* pCtx, CDeviceTexture* pDevTex)
		: m_pCtx(pCtx)
		, m_pTexture(pDevTex)
	{
		pDevTex->GpuPin();
	}

	CDeviceTextureGpuPin(CCryPerformanceDeviceContextWrapper& ctx, CDeviceTexture* pDevTex)
		: m_pCtx(ctx.GetRealPerformanceDeviceContext())
		, m_pTexture(pDevTex)
	{
		pDevTex->GpuPin();
	}

	~CDeviceTextureGpuPin()
	{
		m_pTexture->GpuUnpin(m_pCtx);
	}

private:
	CDeviceTextureGpuPin(const CDeviceTextureGpuPin&);
	CDeviceTextureGpuPin& operator = (const CDeviceTextureGpuPin&);

private:
	ID3DXboxPerformanceContext* m_pCtx;
	CDeviceTexture* m_pTexture;
};


class CDeviceTextureDmaPin
{
public:
	CDeviceTextureDmaPin(ID3D11DmaEngineContextX* pDmaCtx, CDeviceTexture* pDevTex)
		: m_pCtx(pDmaCtx)
		, m_pTexture(pDevTex)
	{
		m_pTexture->GpuPin();
	}

	~CDeviceTextureDmaPin()
	{
		m_pTexture->GpuUnpin(m_pCtx);
	}

private:
	CDeviceTextureDmaPin(const CDeviceTextureDmaPin&);
	CDeviceTextureDmaPin& operator = (const CDeviceTextureDmaPin&);

private:
	ID3D11DmaEngineContextX* m_pCtx;
	CDeviceTexture* m_pTexture;
};

#define PIN_CONCAT_(a,b) a ## b
#define PIN_CONCAT(a,b) PIN_CONCAT_(a,b)
#define GPUPIN_DEVICE_TEXTURE(...) CDeviceTextureGpuPin PIN_CONCAT(gpupin, __LINE__) (__VA_ARGS__)
#define DMAPIN_DEVICE_TEXTURE(...) CDeviceTextureDmaPin PIN_CONCAT(dmapin, __LINE__) (__VA_ARGS__)

#ifndef _RELEASE
//#define ASSERT_DEVICE_TEXTURE_IS_FIXED(tex) do { if (tex->IsMoveable()) __debugbreak(); } while (0)
#endif

#endif

#endif

#ifndef GPUPIN_DEVICE_TEXTURE
#define GPUPIN_DEVICE_TEXTURE(...) do {} while (0)
#endif

#ifndef DMAPIN_DEVICE_TEXTURE
#define DMAPIN_DEVICE_TEXTURE(...) do {} while (0)
#endif

#ifndef ASSERT_DEVICE_TEXTURE_IS_FIXED
#define ASSERT_DEVICE_TEXTURE_IS_FIXED(...) do {} while(0)
#endif

class CDeviceVertexBuffer
{

};

class CDeviceIndexBuffer
{

};

////////////////////////////////////////////////////////////////////////////////////////
class SUsageTrackedItem
{
public:
	SUsageTrackedItem() { MarkUsed(); }
	SUsageTrackedItem(uint32 lastUseFrame) : m_lastUseFrame(lastUseFrame) {}

	void MarkUsed();
	bool IsInUse() const;

protected:
	uint32 m_lastUseFrame;
};

////////////////////////////////////////////////////////////////////////////////////////
// NOTE: this container is expected to hold very few (<3) items usually, so a linear search during allocate should be fine here
template<typename T>
class CTrackedItemAllocatorBase
{
public:
	CTrackedItemAllocatorBase()
	{}

	T* FindUnusedItem()
	{
		for (auto& item : m_items)
		{
			if (item.bAvailable && !item.pItem->IsInUse())
			{
				item.bAvailable = false;
				return item.pItem;
			}
		}
		return nullptr;
	}

	void Release(T*& pItem)
	{
		for (auto& storedItem : m_items)
		{
			if (storedItem.pItem == pItem)
			{
				CRY_ASSERT(!storedItem.bAvailable); // check for double release
				storedItem.bAvailable = true;

				break;
			}
		}

		pItem = nullptr;
	}

	size_t GetItemCount() const
	{
		return m_items.size();
	}

protected:
	T* AddItem(T* pItem)
	{
		m_items.emplace_back(pItem);
		return pItem;
	}

	void ResetBase(std::function<void(T*&)> deleteItem)
	{
		for (auto& storedItem : m_items)
			deleteItem(storedItem.pItem);

		m_items.clear();
	}

	struct SItem
	{
		SItem(T* pItem)
			: bAvailable(false)
			, pItem(pItem)
		{}

		bool bAvailable;
		T*   pItem;
	};

	std::vector<SItem>   m_items;

private:
	CTrackedItemAllocatorBase(const CTrackedItemAllocatorBase<T>&);
	CTrackedItemAllocatorBase<T>& operator=(const CTrackedItemAllocatorBase<T>&);
};

template<typename T>
class CTrackedItemAllocator : public CTrackedItemAllocatorBase<T>
{
public:
	CTrackedItemAllocator() {}
	~CTrackedItemAllocator() { Reset(); }

	void Reset()
	{
		auto deleteLambda = [](T*& pItem) { delete pItem; };
		this->ResetBase(deleteLambda);
	}

	// workaround for the lack of variadic templates in visual studio 2012
#define TRY_RETURN_UNUSED_BUFFER            \
  if (auto* pItem = this->FindUnusedItem()) \
  {                                         \
    if (pNewAlloc) * pNewAlloc = false;     \
    return pItem;                           \
  }                                         \

#define RETURN_NEW_BUFFER(item)      \
  if (pNewAlloc) * pNewAlloc = true; \
  return this->AddItem((item));      \

	T*                                    Allocate(bool* pNewAlloc = nullptr)                   { TRY_RETURN_UNUSED_BUFFER; RETURN_NEW_BUFFER(new T()); }
	template<typename A0> T*              Allocate(A0&& a0, bool* pNewAlloc = nullptr)          { TRY_RETURN_UNUSED_BUFFER; RETURN_NEW_BUFFER(new T(std::forward<A0>(a0))); }
	template<typename A0, typename A1> T* Allocate(A0&& a0, A1&& a1, bool* pNewAlloc = nullptr) { TRY_RETURN_UNUSED_BUFFER; RETURN_NEW_BUFFER(new T(std::forward<A0>(a0), std::forward<A1>(a1))); }

#undef TRY_RETURN_UNUSED_BUFFER
#undef RETURN_NEW_BUFFER

};

template<typename T>
class CTrackedItemCustomAllocator : public CTrackedItemAllocatorBase<T>
{
public:
	typedef std::function<T*()>      CustomAllocFunction;
	typedef std::function<void(T*&)> CustomDeleteFunction;

	CTrackedItemCustomAllocator(CustomAllocFunction customAlloc, CustomDeleteFunction customDelete)
		: m_customAllocate(customAlloc)
		, m_customDelete(customDelete)
	{}

	~CTrackedItemCustomAllocator()
	{
		Reset();
	}
	
	void Reset()
	{
		ResetBase(m_customDelete);
	}

	T* Allocate(bool* pNewAlloc = nullptr)
	{
		if (auto* pItem = this->FindUnusedItem())
		{
			if (pNewAlloc) *pNewAlloc = false;
			return pItem;
		}

		if (pNewAlloc) *pNewAlloc = true;
		T* pItem = m_customAllocate();
		return AddItem(pItem);
	}

	void SetAllocFunctions(CustomAllocFunction customAllocate, CustomDeleteFunction customDelete)
	{
		m_customAllocate = customAllocate;
		m_customDelete = customDelete;
	}
protected:
	CustomAllocFunction  m_customAllocate;
	CustomDeleteFunction m_customDelete;
};
#endif  // _DeviceManager_H_
