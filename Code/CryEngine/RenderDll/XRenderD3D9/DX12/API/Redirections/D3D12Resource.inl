// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include <array>
#include <d3d12.h>

template<const int numTargets>
class BroadcastableD3D12Resource : public ID3D12Resource, public Handable<BroadcastableD3D12Resource<numTargets>*, numTargets>
{
	template<const int numTargets> friend class BroadcastableD3D12Device;
	template<const int numTargets> friend class BroadcastableD3D12CommandQueue;
	template<const int numTargets> friend class BroadcastableD3D12GraphicsCommandList;
	friend Handable;

	int m_RefCount;
	std::array<ID3D12Resource*, numTargets> m_Targets;
	ID3D12Resource* const* operator[](int i) const { return &m_Targets[i]; }

	typedef BroadcastableD3D12Resource<numTargets> self;
public:
	BroadcastableD3D12Resource(
	  _In_ ID3D12Device* pDevice,
	  _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
	  D3D12_HEAP_FLAGS HeapFlags,
	  _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
	  D3D12_RESOURCE_STATES InitialResourceState,
	  _In_opt_ const D3D12_CLEAR_VALUE* pOptimizedClearValue,
	  REFIID riid) : m_RefCount(1), Handable(this)
	{
		D3D12_GPU_DELTA_ADDRESS deltaGPUAddresses = { 0ULL, 0ULL };
		D3D12_HEAP_PROPERTIES Properties = *pHeapProperties;
		D3D12_RESOURCE_DESC Desc = *pResourceDesc;

		DX12_ASSERT(pHeapProperties->CreationNodeMask != 0, "0 is not allowed in the broadcaster!");
		for (int i = 0; i < numTargets; ++i)
		{
			m_Targets[i] = nullptr;

			if (Properties.CreationNodeMask = (pHeapProperties->CreationNodeMask & (1 << i)))
			{
				Properties.VisibleNodeMask = pHeapProperties->VisibleNodeMask | Properties.CreationNodeMask;
				HRESULT ret = pDevice->CreateCommittedResource(
				  &Properties, HeapFlags, &Desc, InitialResourceState, pOptimizedClearValue, riid, (void**)&m_Targets[i]);
				DX12_ASSERT(ret == S_OK, "Failed to create comitted resource!");
			}
		}

		for (int i = 0; i < numTargets; ++i)
		{
			if (Properties.VisibleNodeMask & (1 << i))
			{
				if (!m_Targets[i])
					m_Targets[i] = m_Targets[__lzcnt(pHeapProperties->CreationNodeMask)];
			}

			// ERROR #745: GetGPUVirtualAddress returns NULL for non-buffer resources.
			if (m_Targets[i] && (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER))
			{
				deltaGPUAddresses[i] = m_Targets[i]->GetGPUVirtualAddress() - (UINT64(m_Handle) << 32);
			}
		}

		AssignGPUDeltasToHandle(m_Handle, deltaGPUAddresses);
	}

	BroadcastableD3D12Resource(
	  _In_ ID3D12Device* pDevice,
	  _In_ ID3D12Resource* pResource,
	  REFIID riid) : m_RefCount(1), Handable(this)
	{
		D3D12_GPU_DELTA_ADDRESS deltaGPUAddresses = { 0ULL, 0ULL };
		D3D12_HEAP_PROPERTIES Properties;
		pResource->GetHeapProperties(&Properties, nullptr);
		D3D12_RESOURCE_DESC Desc = pResource->GetDesc();

		DX12_ASSERT(Properties.CreationNodeMask != 0, "0 is not allowed in the broadcaster!");
		for (int i = 0; i < numTargets; ++i)
		{
			m_Targets[i] = nullptr;

			if (Properties.CreationNodeMask & (1 << i))
			{
				m_Targets[i] = pResource;
			}
		}

		for (int i = 0; i < numTargets; ++i)
		{
			if (Properties.VisibleNodeMask & (1 << i))
			{
				m_Targets[i] = m_Targets[__lzcnt(Properties.CreationNodeMask)];
			}

			// ERROR #745: GetGPUVirtualAddress returns NULL for non-buffer resources.
			if (m_Targets[i] && (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER))
			{
				deltaGPUAddresses[i] = m_Targets[i]->GetGPUVirtualAddress() - (UINT64(m_Handle) << 32);
			}
		}

		AssignGPUDeltasToHandle(m_Handle, deltaGPUAddresses);
	}

	BroadcastableD3D12Resource(
	  _In_ ID3D12Device* pDevice,
	  _In_ ID3D12Resource* pResource0,
	  _In_ ID3D12Resource* pResourceR,
	  REFIID riid) : m_RefCount(1), Handable(this)
	{
		D3D12_GPU_DELTA_ADDRESS deltaGPUAddresses = { 0ULL, 0ULL };
		D3D12_HEAP_PROPERTIES Properties0;
		pResource0->GetHeapProperties(&Properties0, nullptr);
		D3D12_HEAP_PROPERTIES PropertiesR;
		pResourceR->GetHeapProperties(&PropertiesR, nullptr);
		D3D12_RESOURCE_DESC Desc0 = pResource0->GetDesc();
		D3D12_RESOURCE_DESC DescR = pResourceR->GetDesc();

		for (int i = 0; i < numTargets; ++i)
		{
			m_Targets[i] = nullptr;

			if (Properties0.CreationNodeMask & (1 << i))
			{
				m_Targets[i] = pResource0;
			}
			else if (PropertiesR.CreationNodeMask & (1 << i))
			{
				m_Targets[i] = pResourceR;
			}
		}

		for (int i = 0; i < numTargets; ++i)
		{
			if (!i && (Properties0.VisibleNodeMask & (1 << i)))
			{
				m_Targets[i] = m_Targets[__lzcnt(Properties0.CreationNodeMask)];
			}

			if (i && (PropertiesR.VisibleNodeMask & (1 << i)))
			{
				m_Targets[i] = m_Targets[__lzcnt(PropertiesR.CreationNodeMask)];
			}

			// ERROR #745: GetGPUVirtualAddress returns NULL for non-buffer resources.
			if (m_Targets[i] && (Desc0.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER))
			{
				deltaGPUAddresses[i] = m_Targets[i]->GetGPUVirtualAddress() - (UINT64(m_Handle) << 32);
			}
		}

		AssignGPUDeltasToHandle(m_Handle, deltaGPUAddresses);
	}

	bool IsShared(UINT pos)
	{
		if (pos && (m_Targets[pos] == m_Targets[0 /*TODO:__lzcnt(pHeapProperties->CreationNodeMask)*/]))
		{
			return true;
		}

		return false;
	}

#define Pick(i, func) \
  m_Targets[i]->func;

#define Broadcast(func)                  \
  if (numTargets > 2)                    \
  {                                      \
    for (int i = 0; i < numTargets; ++i) \
      if (m_Targets[i])                  \
        m_Targets[i]->func;              \
  }                                      \
  else                                   \
  {                                      \
    m_Targets[0]->func;                  \
    m_Targets[1]->func;                  \
  }

	static HRESULT STDMETHODCALLTYPE CreateResource(
	  _In_ ID3D12Device* pDevice,
	  _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
	  D3D12_HEAP_FLAGS HeapFlags,
	  _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
	  D3D12_RESOURCE_STATES InitialResourceState,
	  _In_opt_ const D3D12_CLEAR_VALUE* pOptimizedClearValue,
	  REFIID riid,
	  _COM_Outptr_ void** ppvResource)
	{
		return (*ppvResource = new self(pDevice, pHeapProperties, HeapFlags, pResourceDesc, InitialResourceState, pOptimizedClearValue, riid)) ? S_OK : E_FAIL;
	}

	#pragma region /* IUnknown implementation */

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
	  /* [in] */ REFIID riid,
	  /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) final
	{
		abort();
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void) final
	{
		Broadcast(AddRef());
		return CryInterlockedIncrement(&m_RefCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release(void) final
	{
		Broadcast(Release());
		ULONG count = CryInterlockedDecrement(&m_RefCount);
		if (!count) delete this; return count;
	}

	#pragma endregion

	#pragma region /* ID3D12Object implementation */

	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
	  _In_ REFGUID guid,
	  _Inout_ UINT* pDataSize,
	  _Out_writes_bytes_opt_(*pDataSize)  void* pData) final
	{
		return Pick(0, GetPrivateData(guid, pDataSize, pData));
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
	  _In_ REFGUID guid,
	  _In_ UINT DataSize,
	  _In_reads_bytes_opt_(DataSize)  const void* pData) final
	{
		Broadcast(SetPrivateData(guid, DataSize, pData));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
	  _In_ REFGUID guid,
	  _In_opt_ const IUnknown* pData) final
	{
		Broadcast(SetPrivateDataInterface(guid, pData));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetName(
	  _In_z_ LPCWSTR Name) final
	{
		Broadcast(SetName(Name));
		return S_OK;
	}

	#pragma endregion

	#pragma region /* ID3D12DeviceChild implementation */

	virtual HRESULT STDMETHODCALLTYPE GetDevice(
	  REFIID riid,
	  _COM_Outptr_opt_ void** ppvDevice) final
	{
		return Pick(0, GetDevice(riid, ppvDevice));
	}

	#pragma endregion

	#pragma region /* ID3D12Resource implementation */

	virtual HRESULT STDMETHODCALLTYPE Map(
	  UINT Subresource,
	  _In_opt_ const D3D12_RANGE* pReadRange,
	  _Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void** ppData) final
	{
		for (int i = 1; i < numTargets; ++i)
		{
			DX12_ASSERT(IsShared(i), "Other cases not yet implemented!");
		}

		return Pick(0, Map(Subresource, pReadRange, ppData));
	}

	virtual void STDMETHODCALLTYPE Unmap(
	  UINT Subresource,
	  _In_opt_ const D3D12_RANGE* pWrittenRange) final
	{
		for (int i = 1; i < numTargets; ++i)
		{
			DX12_ASSERT(IsShared(i), "Other cases not yet implemented!");
		}

		return Pick(0, Unmap(Subresource, pWrittenRange));
	}

	virtual D3D12_RESOURCE_DESC STDMETHODCALLTYPE GetDesc(void) final
	{
		return Pick(0, GetDesc());
	}

	virtual D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE GetGPUVirtualAddress(void) final
	{
		return D3D12_GPU_VIRTUAL_ADDRESS(m_Handle) << 32;
	}

	virtual HRESULT STDMETHODCALLTYPE WriteToSubresource(
	  UINT DstSubresource,
	  _In_opt_ const D3D12_BOX* pDstBox,
	  _In_ const void* pSrcData,
	  UINT SrcRowPitch,
	  UINT SrcDepthPitch) final
	{
		abort();
		return E_FAIL;
	}

	virtual HRESULT STDMETHODCALLTYPE ReadFromSubresource(
	  _Out_ void* pDstData,
	  UINT DstRowPitch,
	  UINT DstDepthPitch,
	  UINT SrcSubresource,
	  _In_opt_ const D3D12_BOX* pSrcBox) final
	{
		abort();
		return E_FAIL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetHeapProperties(
	  _Out_opt_ D3D12_HEAP_PROPERTIES* pHeapProperties,
	  _Out_opt_ D3D12_HEAP_FLAGS* pHeapFlags) final
	{
		D3D12_HEAP_PROPERTIES Properties;
		D3D12_HEAP_FLAGS Flags;

		HRESULT res = Pick(0, GetHeapProperties(&Properties, &Flags));
		if (res == S_OK)
		{
			for (int i = 1; i < numTargets; ++i)
			{
				Properties.CreationNodeMask += UINT(!!m_Targets[i]) << i;
				//	Properties.VisibleNodeMask += UINT(!!m_Targets[i]) << i;
			}

			if (pHeapProperties)
				*pHeapProperties = Properties;
			if (pHeapFlags)
				*pHeapFlags = Flags;
		}

		return res;
	}

	#pragma endregion
};

#ifdef INCLUDE_STATICS
// For all utilized templates
static const Handable<BroadcastableD3D12Resource<2>*, 2>::D3D12_CPU_DELTA_ADDRESS emptyCPUDeltaR = { 0ULL, 0ULL };
static const Handable<BroadcastableD3D12Resource<2>*, 2>::D3D12_GPU_DELTA_ADDRESS emptyGPUDeltaR = { 0ULL, 0ULL };
UINT32 Handable<BroadcastableD3D12Resource<2>*, 2 >::m_HighHandle = 0UL;
std::vector<BroadcastableD3D12Resource<2>*> Handable<BroadcastableD3D12Resource<2>*, 2 >::m_AddressTableLookUp(DX12_MULTIGPU_NUM_RESOURCES, 0ULL);
std::vector<Handable<BroadcastableD3D12Resource<2>*, 2>::D3D12_CPU_DELTA_ADDRESS> Handable<BroadcastableD3D12Resource<2>*, 2 >::m_DeltaCPUAddressTableLookUp(DX12_MULTIGPU_NUM_RESOURCES, emptyCPUDeltaR);
std::vector<Handable<BroadcastableD3D12Resource<2>*, 2>::D3D12_GPU_DELTA_ADDRESS> Handable<BroadcastableD3D12Resource<2>*, 2 >::m_DeltaGPUAddressTableLookUp(DX12_MULTIGPU_NUM_RESOURCES, emptyGPUDeltaR);
ConcQueue<BoundMPMC, UINT32> Handable<BroadcastableD3D12Resource<2>*, 2 >::m_FreeHandles;
#endif
