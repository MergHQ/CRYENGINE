// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX12Resource.hpp"
#include "DX12Device.hpp"

#define DX12_BARRIER_SPLIT        2 // mark barriers after and before resources are changed and used
#define DX12_BARRIER_EARLY        1 // enabled barriers immediately after resources have changed (larger time to transition, but possibly double-transitions)
#define DX12_BARRIER_LATE         0 // enabled barriers immediately before resources are used (smaller amount of transitions)
#define DX12_BARRIER_MODE         DX12_BARRIER_SPLIT

#define DX12_BARRIER_VERIFICATION // verify and repair wrong barriers
#define DX12_BARRIER_RELAXATION   // READ-barriers are not changed to more specific subsets like INDEX_BUFFER

#ifdef DX12_BARRIER_RELAXATION
	#define IsIncompatibleState(currentState, desiredState) ((currentState & desiredState) != desiredState || (!currentState != !desiredState))
#else
	#define IsIncompatibleState(currentState, desiredState) ((currentState) != desiredState)
#endif

// https://msdn.microsoft.com/de-de/library/windows/desktop/dn899217%28v=vs.85%29.aspx
// TODO: mask + popcnt == 1
#define IsPromotableState(currentState, desiredState) ( \
  false)

#if 0
(pCmdList->GetD3D12ListType() == D3D12_COMMAND_LIST_TYPE_COPY) && (           \
  ((currentState == D3D12_RESOURCE_STATE_COMMON) && (                         \
     /*			(desiredState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) */\
     /*			(desiredState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) */   \
     (desiredState == D3D12_RESOURCE_STATE_COPY_DEST) ||                      \
     (desiredState == D3D12_RESOURCE_STATE_COPY_SOURCE))) ||                  \
  ((desiredState == D3D12_RESOURCE_STATE_COMMON) && (                         \
     /*			(desiredState == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) */\
     /*			(desiredState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) */   \
     (currentState == D3D12_RESOURCE_STATE_COPY_DEST) ||                      \
     (currentState == D3D12_RESOURCE_STATE_COPY_SOURCE)))))
#endif

// TODO: repair all wrong barrier-transitions
#if 0
	#define DX12_BARRIER_ERROR(...) DX12_ERROR(__VA_ARGS__)
#else
	#define DX12_BARRIER_ERROR(...)
#endif

namespace NCryDX12
{

CResource::SCpuAccessibleData::SubresourceData& CResource::SCpuAccessibleData::GetData(UINT Subresource)
{
	for (auto& data : m_SubResourceData)
	{
		if (data.Subresource == Subresource)
		{
			return data;
		}
	}

	m_SubResourceData.push_back(SCpuAccessibleData::SubresourceData());
	auto& result = m_SubResourceData.back();

	ZeroStruct(result);
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
CResource::CResource(CDevice* pDevice)
	: CDeviceObject(pDevice)
	, m_HeapType(D3D12_HEAP_TYPE_DEFAULT)
	, m_GPUVirtualAddress(DX12_GPU_VIRTUAL_ADDRESS_NULL)
	, m_pD3D12Resource(nullptr)
	, m_pD3D12UploadBuffer(nullptr)
	, m_pD3D12DownloadBuffer(nullptr)
	, m_CurrentState(D3D12_RESOURCE_STATE_COMMON)
	, m_AnnouncedState((D3D12_RESOURCE_STATES)-1)
	, m_InitialData(nullptr)
	, m_CpuAccessibleData(nullptr)
	, m_pSwapChainOwner(nullptr)
	, m_bCompressed(false)
	, m_PlaneCount(0)
	, m_bConcurrentWritable(false)
{
	memset(&m_FenceValues, 0, sizeof(m_FenceValues));
}

CResource::CResource(CResource&& r)
	: CDeviceObject(std::move(r))
	, m_HeapType(std::move(r.m_HeapType))
	, m_GPUVirtualAddress(std::move(r.m_GPUVirtualAddress))
	, m_pD3D12Resource(std::move(r.m_pD3D12Resource))
	, m_pD3D12UploadBuffer(std::move(r.m_pD3D12UploadBuffer))
	, m_pD3D12DownloadBuffer(std::move(r.m_pD3D12DownloadBuffer))
	, m_CurrentState(std::move(r.m_CurrentState))
	, m_AnnouncedState(std::move(r.m_AnnouncedState))
	, m_InitialData(std::move(r.m_InitialData))
	, m_CpuAccessibleData(std::move(r.m_CpuAccessibleData))
	, m_pSwapChainOwner(std::move(r.m_pSwapChainOwner))
	, m_bCompressed(std::move(r.m_bCompressed))
	, m_PlaneCount(std::move(r.m_PlaneCount))
	, m_bConcurrentWritable(std::move(r.m_bConcurrentWritable))
	, m_SubresourceStates(std::move(r.m_SubresourceStates))
{
	memcpy(&m_FenceValues, &r.m_FenceValues, sizeof(m_FenceValues));

	r.m_pD3D12Resource = nullptr;
	r.m_pD3D12UploadBuffer = nullptr;
	r.m_pD3D12DownloadBuffer = nullptr;
	r.m_InitialData = nullptr;
	r.m_CpuAccessibleData = nullptr;
	r.m_pSwapChainOwner = nullptr;
}

CResource& CResource::operator=(CResource&& r)
{
	CDeviceObject::operator=(std::move(r));

	m_HeapType = std::move(r.m_HeapType);
	m_GPUVirtualAddress = std::move(r.m_GPUVirtualAddress);
	m_pD3D12Resource = std::move(r.m_pD3D12Resource);
	m_pD3D12UploadBuffer = std::move(r.m_pD3D12UploadBuffer);
	m_pD3D12DownloadBuffer = std::move(r.m_pD3D12DownloadBuffer);
	m_CurrentState = std::move(r.m_CurrentState);
	m_AnnouncedState = std::move(r.m_AnnouncedState);
	m_InitialData = std::move(r.m_InitialData);
	m_CpuAccessibleData = std::move(r.m_CpuAccessibleData);
	m_pSwapChainOwner = std::move(r.m_pSwapChainOwner);
	m_bCompressed = std::move(r.m_bCompressed);
	m_PlaneCount = std::move(r.m_PlaneCount);
	m_bConcurrentWritable = std::move(r.m_bConcurrentWritable);
	m_SubresourceStates = std::move(r.m_SubresourceStates);

	memcpy(&m_FenceValues, &r.m_FenceValues, sizeof(m_FenceValues));

	r.m_pD3D12Resource = nullptr;
	r.m_pD3D12UploadBuffer = nullptr;
	r.m_pD3D12DownloadBuffer = nullptr;
	r.m_InitialData = nullptr;
	r.m_CpuAccessibleData = nullptr;
	r.m_pSwapChainOwner = nullptr;

	return *this;
}

//---------------------------------------------------------------------------------------------------------------------
CResource::~CResource()
{
	GetDevice()->ReleaseLater(GetFenceValues(CMDTYPE_ANY), m_pD3D12Resource, !m_pSwapChainOwner);
	GetDevice()->ReleaseLater(GetFenceValues(CMDTYPE_ANY), m_pD3D12UploadBuffer);
	GetDevice()->ReleaseLater(GetFenceValues(CMDTYPE_ANY), m_pD3D12DownloadBuffer);

	DiscardInitialData();

	if (m_CpuAccessibleData)
	{
		for (auto& subresourceData : m_CpuAccessibleData->m_SubResourceData)
		{
			if (subresourceData.Data.RowPitch)
			{
				CryModuleMemalignFree((void*)subresourceData.Data.pData);
			}
		}

		SAFE_DELETE(m_CpuAccessibleData);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void CResource::DiscardInitialData()
{
	if (m_InitialData)
	{
		for (size_t i = 0, S = m_InitialData->m_SubResourceData.size(); i < S; ++i)
		{
			CryModuleMemalignFree((void*)m_InitialData->m_SubResourceData[i].pData);
		}

		SAFE_DELETE(m_InitialData);
	}
}

//---------------------------------------------------------------------------------------------------------------------
bool CResource::Init(ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState, const D3D12_RESOURCE_DESC& desc)
{
	m_pD3D12Resource = pResource;
	m_Desc = desc;
	m_PlaneCount = D3D12GetFormatPlaneCount(GetDevice()->GetD3D12Device(), m_Desc.Format);

	ZeroStruct(m_NodeMasks);

	if (m_pD3D12Resource)
	{
		D3D12_HEAP_PROPERTIES sHeap;

		if (m_pD3D12Resource->GetHeapProperties(&sHeap, nullptr) == S_OK)
		{
			m_HeapType = sHeap.Type;
			m_CurrentState = eInitialState;

			// Certain heaps are restricted to certain D3D12_RESOURCE_STATES states, and cannot be changed.
			// D3D12_HEAP_TYPE_UPLOAD requires D3D12_RESOURCE_STATE_GENERIC_READ.
			if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
			{
				m_CurrentState = D3D12_RESOURCE_STATE_GENERIC_READ;
			}
			else if (m_HeapType == D3D12_HEAP_TYPE_READBACK)
			{
				m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
			}

			m_NodeMasks.creationMask = sHeap.CreationNodeMask;
			m_NodeMasks.visibilityMask = sHeap.VisibleNodeMask;
		}

#if CRY_USE_DX12_MULTIADAPTER_SIMULATION
		// Always allow getting GPUAddress (CreationMask == VisibilityMask), if running simulation
		if ((m_NodeMasks.creationMask == m_NodeMasks.visibilityMask) || (CRenderer::CV_r_StereoEnableMgpu < 0))
#else
		// Don't get GPU-addresses for shared staging resources
		if ((m_NodeMasks.creationMask == m_NodeMasks.visibilityMask))
#endif
		{
			if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				m_GPUVirtualAddress = m_pD3D12Resource->GetGPUVirtualAddress();
			}
		}
	}
	else
	{
		m_HeapType = D3D12_HEAP_TYPE_UPLOAD; // Null resource put on the UPLOAD heap to prevent attempts to transition the resource.
	}

	m_pSwapChainOwner = NULL;
	m_bCompressed = IsDXGIFormatCompressed(desc.Format);
	m_bConcurrentWritable = false;

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
void CResource::InitDeferred(CCommandList* pCmdList, D3D12_RESOURCE_STATES eAnnouncedState)
{
	DX12_LOG(g_nPrintDX12, "Uploading initial data");
	DX12_ASSERT(!IsUsed<PERSP_CPU>(pCmdList->GetCommandListPool()), "Resource is used prior to initialization!");

	SInitialData* InitialData = nullptr;
	std::swap(InitialData, m_InitialData);

	pCmdList->UpdateSubresources(*this, eAnnouncedState,
	                             InitialData->m_Size,
	                             static_cast<UINT>(InitialData->m_SubResourceData.size()),
	                             &(InitialData->m_SubResourceData[0]));

	std::swap(InitialData, m_InitialData);
	DiscardInitialData();
}

//---------------------------------------------------------------------------------------------------------------------
CResource::SInitialData* CResource::GetOrCreateInitialData()
{
	if (!m_InitialData)
	{
		m_InitialData = new SInitialData();
	}

	return m_InitialData;
}

//---------------------------------------------------------------------------------------------------------------------
CResource::SCpuAccessibleData::SubresourceData& CResource::GetOrCreateCpuAccessibleData(UINT Subresource, bool bAllocate)
{
	if (!m_CpuAccessibleData)
	{
		m_CpuAccessibleData = new SCpuAccessibleData();
	}

	auto& result = m_CpuAccessibleData->GetData(Subresource);
	if (!result.Data.pData)
	{
		ID3D12Device* pDevice12 = GetDevice()->GetD3D12Device();

		UINT64 requiredSize, rowPitch;
		UINT rowCount;
		pDevice12->GetCopyableFootprints(&m_Desc, Subresource, 1, 0, nullptr, &rowCount, &rowPitch, &requiredSize);

		result.Subresource = Subresource;
		result.Size = requiredSize;
		result.IsDirty = false;

		if (bAllocate)
		{
			result.Data.RowPitch = SIZE_T(rowPitch);
			result.Data.SlicePitch = SIZE_T(requiredSize);
			result.Data.pData = CryModuleMemalign(size_t(requiredSize), CRY_PLATFORM_ALIGNMENT);
		}
		else
		{
			result.Data.RowPitch = 0;
			result.Data.SlicePitch = 0;
			result.Data.pData = nullptr;
		}
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
void CResource::DiscardCpuAccessibleData(UINT Subresource)
{
	// NOTE: I'm deliberately not freeing the CPU cached data here as there's a strong possibility that it will be reused
	// next frame. We should probably build some buffer management system if this ever becomes a problem.
}

//---------------------------------------------------------------------------------------------------------------------
const char* StateToString(D3D12_RESOURCE_STATES state)
{
	static int buf = 0;
	static char str[2][512], * ret;

	ret = str[buf ^= 1];
	*ret = '\0';

	if (!state)
	{
		strcat(ret, " Common/Present");
		return ret;
	}

	if ((state & D3D12_RESOURCE_STATE_GENERIC_READ) == D3D12_RESOURCE_STATE_GENERIC_READ)
	{
		strcat(ret, " Generic Read");
		return ret;
	}

	if (state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
	{
		strcat(ret, " V/C Buffer");
	}
	if (state & D3D12_RESOURCE_STATE_INDEX_BUFFER)
	{
		strcat(ret, " I Buffer");
	}
	if (state & D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		strcat(ret, " RT");
	}
	if (state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		strcat(ret, " UA");
	}
	if (state & D3D12_RESOURCE_STATE_DEPTH_WRITE)
	{
		strcat(ret, " DepthW");
	}
	if (state & D3D12_RESOURCE_STATE_DEPTH_READ)
	{
		strcat(ret, " DepthR");
	}
	if (state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	{
		strcat(ret, " NoPixelR");
	}
	if (state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		strcat(ret, " PixelR");
	}
	if (state & D3D12_RESOURCE_STATE_STREAM_OUT)
	{
		strcat(ret, " Stream Out");
	}
	if (state & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
	{
		strcat(ret, " Indirect Arg");
	}
	if (state & D3D12_RESOURCE_STATE_COPY_DEST)
	{
		strcat(ret, " CopyD");
	}
	if (state & D3D12_RESOURCE_STATE_COPY_SOURCE)
	{
		strcat(ret, " CopyS");
	}
	if (state & D3D12_RESOURCE_STATE_RESOLVE_DEST)
	{
		strcat(ret, " ResolveD");
	}
	if (state & D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
	{
		strcat(ret, " ResolveS");
	}

	return ret;
}

D3D12_RESOURCE_STATES CResource::TransitionBarrier(CCommandList* pCmdList, D3D12_RESOURCE_STATES desiredState)
{
	if (m_InitialData)
	{
		InitDeferred(pCmdList, desiredState);
	}

	if (IsOffCard())
	{
		return m_CurrentState;
	}

	if (HasSubresourceTransitionBarriers()) // Switch from subresource barrier mode to shared barrier mode
	{
		DisableSubresourceTransitionBarriers(pCmdList, desiredState);
		return (D3D12_RESOURCE_STATES)-1;
	}

#if defined(DX12_BARRIER_VERIFICATION) && (DX12_BARRIER_MODE == DX12_BARRIER_SPLIT)
	if (m_AnnouncedState != (D3D12_RESOURCE_STATES)-1)
	{
		DX12_BARRIER_ERROR("A resource has been marked for a transition which never happend: %s", StateToString(m_AnnouncedState));
		DX12_BARRIER_ERROR("Please clean-up the high-level layer. [Example: SetRenderTarget() without rendering to it.]");

		D3D12_RESOURCE_STATES bak1 = desiredState;
		D3D12_RESOURCE_STATES bak2 = m_AnnouncedState;
		D3D12_RESOURCE_STATES bak3 = m_CurrentState;

		EndTransitionBarrier(pCmdList, m_AnnouncedState);

		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting begin without end: %s", StateToString(m_AnnouncedState));
	}
#endif

	D3D12_RESOURCE_STATES ePreviousState = m_CurrentState;
	if (IsPromotableState(m_CurrentState, desiredState))
	{
		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier change %s (ID3D12: %p): %s ->%s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState), StateToString(desiredState));
		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting begin without end: %s", StateToString(m_AnnouncedState));

		m_CurrentState = desiredState;
		//		m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
	else if (IsIncompatibleState(m_CurrentState, desiredState))
	{
		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier change %s (ID3D12: %p): %s ->%s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState), StateToString(desiredState));
		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting begin without end: %s", StateToString(m_AnnouncedState));

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_pD3D12Resource;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = m_CurrentState;
		barrierDesc.Transition.StateAfter = desiredState;

		pCmdList->ResourceBarrier(1, &barrierDesc);
		m_CurrentState = desiredState;
		//		m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
	else if ((desiredState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) && !IsConcurrentWritable())
	{
		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier block %s (ID3D12: %p): %s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState));
		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting begin without end: %s", StateToString(m_AnnouncedState));

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrierDesc.UAV.pResource = m_pD3D12Resource;

		pCmdList->ResourceBarrier(1, &barrierDesc);
		//		m_CurrentState = desiredState;
		//		m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}

	return ePreviousState;
}

D3D12_RESOURCE_STATES CResource::TransitionBarrier(CCommandList* pCmdList, const CView& view, D3D12_RESOURCE_STATES desiredState)
{
	// Use shared barrier mode if possible
	if (view.MapsFullResource())
	{
		return TransitionBarrier(pCmdList, desiredState);
	}

	if (!HasSubresourceTransitionBarriers()) // Switch to subresource barrier mode
	{
		EnableSubresourceTransitionBarriers();
	}

	CD3DX12_RESOURCE_DESC resourceDesc(m_Desc);
	const auto& slices = view.GetSlices();
	const auto& mips = view.GetMips();

	std::vector<D3D12_RESOURCE_BARRIER> barriers;
	barriers.reserve(slices.Length() * mips.Length() * GetPlaneCount());

	for (int slice = slices.start; slice < slices.end; ++slice)
	{
		for (int mip = mips.start; mip < mips.end; ++mip)
		{
			for (int plane = 0; plane < GetPlaneCount(); ++plane)
			{
				uint subresource = resourceDesc.CalcSubresource(mip, slice, plane);
				auto subresourceState = m_SubresourceStates[subresource];

				if (subresourceState != desiredState)
				{
					D3D12_RESOURCE_BARRIER barrierDesc = {};
					barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrierDesc.Transition.pResource = m_pD3D12Resource;
					barrierDesc.Transition.Subresource = subresource;
					barrierDesc.Transition.StateBefore = subresourceState;
					barrierDesc.Transition.StateAfter = desiredState;

					barriers.push_back(barrierDesc);
					m_SubresourceStates[subresource] = desiredState;
				}
			}
		}
	}

	if (!barriers.empty())
	{
		pCmdList->ResourceBarrier(barriers.size(), &barriers[0]);
	}

	return D3D12_RESOURCE_STATES(-1);
}

int CResource::SelectQueueForTransitionBarrier(int altQueue, int desiredQueue, D3D12_RESOURCE_STATES desiredState) const
{
	static bool validTable[4][14] =
	{
		// CMDLIST_TYPE_DIRECT
		{ true , true , true , true , true , true , true , true , true , true , true, true, true , true  },
		// CMDLIST_TYPE_BUNDLE
		{ false },
		// CMDLIST_TYPE_COMPUTE
		{ true , false, false, true , false, false, true , false, false, true , true, true, false, false },
		// CMDLIST_TYPE_COPY
		{ false, false, false, false, false, false, false, false, false, false, true, true, false, false },
	};

	bool (&    altTable)[14] = validTable[CCommandListPool::m_MapQueueType[    altQueue]];
	bool (&desiredTable)[14] = validTable[CCommandListPool::m_MapQueueType[desiredQueue]];

	D3D12_RESOURCE_STATES mergedState = desiredState;
	if (!HasSubresourceTransitionBarriers())
	{
		mergedState |= m_CurrentState;
	}
	else
	{
		for (auto subresourceState : m_SubresourceStates)
			mergedState |= subresourceState;
	}

	uint32 stateOffset = 0;
	while (mergedState)
	{
		// Move the cursor after the set bit
		uint32 z = countTrailingZeroes(uint32(mergedState)) + 1;
		
		mergedState = D3D12_RESOURCE_STATES(mergedState >> z);
		stateOffset = stateOffset + z;

		if (!desiredTable[stateOffset])
		{
			if (!altTable[stateOffset])
			{
				return CMDQUEUE_GRAPHICS;
			}

			return altQueue;
		}
	}

	return desiredQueue;
}

bool CResource::NeedsTransitionBarrier(CCommandList* pCmdList, D3D12_RESOURCE_STATES desiredState, bool bPrepare) const
{
	if (IsOffCard())
	{
		// Never issues barriers
		return false;
	}

	if (HasSubresourceTransitionBarriers())
	{
		for (auto subresourceState : m_SubresourceStates)
		{
			if (subresourceState != desiredState)
				return true;
		}
		return false;
	}

#if (DX12_BARRIER_MODE == DX12_BARRIER_SPLIT)
	if (m_AnnouncedState != (D3D12_RESOURCE_STATES)-1)
	{
		// Needs to end begun barrier
		return true;
	}
#endif

	// Needs a barrier when not compatible
	return
		IsIncompatibleState(m_CurrentState, desiredState) ||
		(bPrepare && (desiredState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) && !IsConcurrentWritable());
}

bool CResource::NeedsTransitionBarrier(CCommandList* pCmdList, const CView& view, D3D12_RESOURCE_STATES desiredState, bool bPrepare) const
{
	if (view.MapsFullResource())
	{
		if (!HasSubresourceTransitionBarriers())
		{
			return NeedsTransitionBarrier(pCmdList, desiredState, bPrepare);
		}
		else
		{
			for (auto subresourceState : m_SubresourceStates)
			{
				if (subresourceState != desiredState)
					return true;
			}

			return false;
		}
	}
	else
	{
		if (!HasSubresourceTransitionBarriers())
		{
			return m_CurrentState != desiredState;
		}
		else
		{
			CD3DX12_RESOURCE_DESC desc(m_Desc);
			const auto& slices = view.GetSlices();
			const auto& mips = view.GetMips();

			for (int slice = slices.start; slice < slices.end; ++slice)
			{
				for (int mip = mips.start; mip < mips.end; ++mip)
				{
					for (int plane = 0; plane < m_PlaneCount; ++plane)
					{
						uint32 subresource = desc.CalcSubresource(mip, slice, plane);
						if (m_SubresourceStates[subresource] != desiredState)
						{
							return true;
						}
					}
				}
			}

			return false;
		}
	}
}

D3D12_RESOURCE_STATES CResource::DecayTransitionBarrier(CCommandList* pCmdList, D3D12_RESOURCE_STATES desiredState)
{
	if (IsOffCard())
	{
		return m_CurrentState;
	}

	D3D12_RESOURCE_STATES ePreviousState = m_CurrentState;

	m_CurrentState = desiredState;
	m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

	return ePreviousState;
}

D3D12_RESOURCE_STATES CResource::BeginTransitionBarrier(CCommandList* pCmdList, D3D12_RESOURCE_STATES desiredState)
{
	if (HasSubresourceTransitionBarriers())
	{
		DisableSubresourceTransitionBarriers(pCmdList, desiredState);
		return m_CurrentState;
	}

#if (DX12_BARRIER_MODE == DX12_BARRIER_EARLY)
	return TransitionBarrier(pCmdList, desiredState);
#endif

	if (m_InitialData)
	{
#if defined(DX12_BARRIER_VERIFICATION)
		if ((m_AnnouncedState != (D3D12_RESOURCE_STATES)-1) && (m_AnnouncedState != desiredState))
		{
			DX12_BARRIER_ERROR("A resource has been uploaded, but the first use of the resource is NOT a read!");
			DX12_BARRIER_ERROR("Please clean-up the high-level layer. [Example: CopyResource() without using the initial data.]");

			if (IsIncompatibleState(m_AnnouncedState, desiredState))
			{
				EndTransitionBarrier(pCmdList, m_AnnouncedState);
				DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier not ended: %s", StateToString(m_AnnouncedState));
			}
			else
			{
				desiredState = m_AnnouncedState;
			}
		}
#endif

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");

		InitDeferred(pCmdList, desiredState);
		return m_CurrentState;
	}

#if (DX12_BARRIER_MODE == DX12_BARRIER_LATE)
	return m_CurrentState;
#endif

	if (IsOffCard())
	{
		return m_CurrentState;
	}

#if defined(DX12_BARRIER_VERIFICATION) && (DX12_BARRIER_MODE == DX12_BARRIER_SPLIT)
	if ((m_AnnouncedState != (D3D12_RESOURCE_STATES)-1) && (m_AnnouncedState != desiredState))
	{
		DX12_BARRIER_ERROR("A resource has been marked for a transition which never happend: %s", StateToString(m_AnnouncedState));
		DX12_BARRIER_ERROR("Please clean-up the high-level layer. [Example: SetRenderTarget() without rendering to it.]");

		if (IsIncompatibleState(m_AnnouncedState, desiredState))
		{
			EndTransitionBarrier(pCmdList, m_AnnouncedState);
			DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier not ended: %s", StateToString(m_AnnouncedState));
		}
		else
		{
			desiredState = m_AnnouncedState;
		}

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
#endif

	D3D12_RESOURCE_STATES ePreviousState = m_CurrentState;
	if (IsPromotableState(m_CurrentState, desiredState))
	{
		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier begin %s (ID3D12: %p): %s ->%s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState), StateToString(desiredState));
		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting begin without end: %s", StateToString(m_AnnouncedState));

		//		m_CurrentState = (D3D12_RESOURCE_STATES)-1;
		m_AnnouncedState = desiredState;
	}
	else if (IsIncompatibleState(m_CurrentState, desiredState))
	{
#if defined(DX12_BARRIER_VERIFICATION)
		if (m_AnnouncedState == desiredState)
		{
			DX12_BARRIER_ERROR("A resource has been bound at least TWICE! Please clean-up the high-level layer.");
			return m_CurrentState;
		}
#endif

		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier begin %s (ID3D12: %p): %s ->%s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState), StateToString(desiredState));
		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting initialization: %s", StateToString(m_AnnouncedState));

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_pD3D12Resource;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = m_CurrentState;
		barrierDesc.Transition.StateAfter = desiredState;

		pCmdList->ResourceBarrier(1, &barrierDesc);
		//		m_CurrentState = (D3D12_RESOURCE_STATES)-1;
		m_AnnouncedState = desiredState;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
	else if ((desiredState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) && !IsConcurrentWritable())
	{
		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier block %s (ID3D12: %p): %s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState));
		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting begin without end: %s", StateToString(m_AnnouncedState));

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrierDesc.UAV.pResource = m_pD3D12Resource;

		pCmdList->ResourceBarrier(1, &barrierDesc);
		//		m_CurrentState = (D3D12_RESOURCE_STATES)-1;
		m_AnnouncedState = desiredState;

		//		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
	else
	{
		bool omition = true;
	}

	return ePreviousState;
}

D3D12_RESOURCE_STATES CResource::BeginTransitionBarrier(CCommandList* pCmdList, const CView& view, D3D12_RESOURCE_STATES desiredState)
{
	if (view.MapsFullResource())
	{
		return BeginTransitionBarrier(pCmdList, desiredState);
	}
	else
	{
		return desiredState;
	}
}

D3D12_RESOURCE_STATES CResource::EndTransitionBarrier(CCommandList* pCmdList, D3D12_RESOURCE_STATES desiredState)
{
	if (HasSubresourceTransitionBarriers())
	{
		DisableSubresourceTransitionBarriers(pCmdList, desiredState);
		return m_CurrentState;
	}

#if (DX12_BARRIER_MODE == DX12_BARRIER_EARLY || DX12_BARRIER_MODE == DX12_BARRIER_LATE)
	return TransitionBarrier(pCmdList, desiredState);
#endif

#if (DX12_BARRIER_MODE == DX12_BARRIER_EARLY)
	return m_CurrentState;
#endif

	if (IsOffCard())
	{
		return m_CurrentState;
	}

#if defined(DX12_BARRIER_VERIFICATION)
	if ((m_AnnouncedState != (D3D12_RESOURCE_STATES)-1) && (m_AnnouncedState != desiredState))
	{
		DX12_BARRIER_ERROR("A resource has been marked for a transition which end in a different state: %s", StateToString(m_AnnouncedState), StateToString(desiredState));
		DX12_BARRIER_ERROR("Please clean-up the high-level layer. [Example: SetRenderTarget() without rendering to it.]");

		if (IsIncompatibleState(m_AnnouncedState, desiredState))
		{
			EndTransitionBarrier(pCmdList, m_AnnouncedState);
			return TransitionBarrier(pCmdList, desiredState);
		}
		else
		{
			desiredState = m_AnnouncedState;
		}

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
#endif

	D3D12_RESOURCE_STATES ePreviousState = m_CurrentState;
	if (IsPromotableState(m_CurrentState, desiredState))
	{
		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier end %s (ID3D12: %p): %s ->%s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState), StateToString(desiredState));
		DX12_ASSERT(m_AnnouncedState == desiredState, "Resource barrier has conflicting begin: %s", StateToString(m_AnnouncedState));

		m_CurrentState = desiredState;
		m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
	else if (IsIncompatibleState(m_CurrentState, desiredState))
	{
#if defined(DX12_BARRIER_VERIFICATION)
		if (m_AnnouncedState == (D3D12_RESOURCE_STATES)-1)
		{
			DX12_BARRIER_ERROR("A resource wanted to end a transition which was never announced: %s", StateToString(desiredState));
			DX12_BARRIER_ERROR("Please clean-up the high-level layer. [Example: SetRenderTarget() without rendering to it.]");

			return TransitionBarrier(pCmdList, desiredState);
		}
#endif

		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier end %s (ID3D12: %p): %s ->%s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState), StateToString(desiredState));
		DX12_ASSERT(m_AnnouncedState == desiredState, "Resource barrier has conflicting begin: %s", StateToString(m_AnnouncedState));

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_pD3D12Resource;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = m_CurrentState;
		barrierDesc.Transition.StateAfter = desiredState;

		pCmdList->ResourceBarrier(1, &barrierDesc);
		m_CurrentState = desiredState;
		m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}
	else if ((desiredState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) && !IsConcurrentWritable())
	{
#if defined(DX12_BARRIER_VERIFICATION)
		if (m_AnnouncedState == (D3D12_RESOURCE_STATES)-1)
		{
			DX12_BARRIER_ERROR("A resource wanted to end a transition which was never announced: %s", StateToString(desiredState));
			DX12_BARRIER_ERROR("Please clean-up the high-level layer. [Example: SetRenderTarget() without rendering to it.]");

			return TransitionBarrier(pCmdList, desiredState);
		}
#endif

#if 0
		DX12_LOG(DX12_BARRIER_ANALYZER, "Resource barrier block %p (ID3D12: %p): %s", GetName().c_str(), m_pD3D12Resource.get(), StateToString(m_CurrentState));
		DX12_ASSERT(m_AnnouncedState == (D3D12_RESOURCE_STATES)-1, "Resource barrier has conflicting begin without end: %s", StateToString(m_AnnouncedState));

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrierDesc.UAV.pResource = m_pD3D12Resource;

		pCmdList->ResourceBarrier(1, &barrierDesc);
		//		m_CurrentState = desiredState;
#endif
		m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;

		DX12_ASSERT(m_CurrentState != m_AnnouncedState, "Resource barrier corruption detected!");
	}

	return ePreviousState;
}

D3D12_RESOURCE_STATES CResource::EndTransitionBarrier(CCommandList* pCmdList, const CView& view, D3D12_RESOURCE_STATES desiredState)
{
	if (view.MapsFullResource())
	{
		return EndTransitionBarrier(pCmdList, desiredState);
	}
	else
	{
		return TransitionBarrier(pCmdList, view, desiredState);
	}
}

void CResource::EnableSubresourceTransitionBarriers()
{
	CRY_ASSERT(!HasSubresourceTransitionBarriers());
	CRY_ASSERT(m_SubresourceStates.empty());

	uint32 subresourceCount = CD3DX12_RESOURCE_DESC(m_Desc).Subresources(GetPlaneCount());
	m_SubresourceStates.resize(subresourceCount, m_CurrentState);

	m_CurrentState = (D3D12_RESOURCE_STATES)-1;
	m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;
}

void CResource::DisableSubresourceTransitionBarriers(CCommandList* pCmdList, D3D12_RESOURCE_STATES desiredCommonState)
{
	CRY_ASSERT(HasSubresourceTransitionBarriers());

	std::vector<D3D12_RESOURCE_BARRIER> barriers;
	barriers.reserve(m_SubresourceStates.size());

	for (int subresource = 0; subresource < m_SubresourceStates.size(); ++subresource)
	{
		auto subresourceState = m_SubresourceStates[subresource];
		if (subresourceState != desiredCommonState)
		{
			D3D12_RESOURCE_BARRIER barrierDesc = {};
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierDesc.Transition.pResource = m_pD3D12Resource;
			barrierDesc.Transition.Subresource = subresource;
			barrierDesc.Transition.StateBefore = subresourceState;
			barrierDesc.Transition.StateAfter = desiredCommonState;

			barriers.push_back(barrierDesc);
		}
	}

	if (!barriers.empty())
	{
		pCmdList->ResourceBarrier(barriers.size(), &barriers[0]);
	}

	m_CurrentState = desiredCommonState;
	m_AnnouncedState = (D3D12_RESOURCE_STATES)-1;
	m_SubresourceStates.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void CResource::MapDiscard()
{
	GetDevice()->ReleaseLater(GetFenceValues(CMDTYPE_ANY), m_pD3D12Resource);

	// NOTE: this might not be a staging resource, duplicate incoming nodeMasks
	ID3D12Resource* resource = NULL;
	if (S_OK != GetDevice()->DuplicateCommittedResource(
	      m_pD3D12Resource,
		  D3D12_RESOURCE_STATE_GENERIC_READ,
		  &resource
	      ) || !resource)
	{
		DX12_ASSERT(0, "Could not create buffer resource!");
		return;
	}

	m_pD3D12Resource = resource;
	resource->Release();

	m_CurrentState = D3D12_RESOURCE_STATE_GENERIC_READ;
	m_SubresourceStates.clear();

	// Don't get GPU-addresses for shared staging resources
	if (m_NodeMasks.creationMask == m_NodeMasks.visibilityMask)
	{
		if (m_pD3D12Resource && m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			m_GPUVirtualAddress = m_pD3D12Resource->GetGPUVirtualAddress();
		}
	}
}

void CResource::CopyDiscard()
{
	// TODO: clone the surface
	__debugbreak();
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT CResource::MappedWriteToSubresource(UINT Subresource, const D3D12_RANGE* Range, const void* pInData)
{
	const D3D12_RANGE sNoRead = { 0, 0 }; // It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin

	void* pOutData;
	GetD3D12Resource()->Map(Subresource, &sNoRead, &pOutData);
	StreamBufferData(reinterpret_cast<uint8*>(pOutData) + Range->Begin, pInData, Range->End - Range->Begin);
	GetD3D12Resource()->Unmap(Subresource, Range);

	return S_OK;
}

HRESULT CResource::MappedReadFromSubresource(UINT Subresource, const D3D12_RANGE* Range, void* pOutData)
{
	const D3D12_RANGE sNoWrite = { 0, 0 }; // It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin

	void* pInData;
	GetD3D12Resource()->Map(Subresource, Range, &pInData);
	FetchBufferData(pOutData, reinterpret_cast<uint8*>(pInData) + Range->Begin, Range->End - Range->Begin);
	GetD3D12Resource()->Unmap(Subresource, &sNoWrite);

	return S_OK;
}

}
