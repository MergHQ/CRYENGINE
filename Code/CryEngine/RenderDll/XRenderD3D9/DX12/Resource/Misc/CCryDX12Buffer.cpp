// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CCryDX12Buffer.hpp"
#include "../GI/CCryDX12SwapChain.hpp"

CCryDX12Buffer* CCryDX12Buffer::Create(CCryDX12Device* pDevice)
{
	D3D12_RESOURCE_DESC desc12;
	D3D11_BUFFER_DESC desc11;

	ZeroStruct(desc12);
	ZeroStruct(desc11);

	desc12.Format = DXGI_FORMAT_UNKNOWN;
	desc12.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc11.Usage = D3D11_USAGE_DEFAULT;

	CCryDX12Buffer* result = DX12_NEW_RAW(CCryDX12Buffer(pDevice, desc11, nullptr, D3D12_RESOURCE_STATE_COMMON, CD3DX12_RESOURCE_DESC(desc12), NULL));

	return result;
}

CCryDX12Buffer* CCryDX12Buffer::Create(CCryDX12Device* pDevice, ID3D12Resource* pResource, D3D12_RESOURCE_STATES initialState)
{
	DX12_ASSERT(pResource);

	D3D12_RESOURCE_DESC desc12 = pResource->GetDesc();

	D3D11_BUFFER_DESC desc11;
	desc11.ByteWidth = 0;           // static_cast<UINT>(desc12.ByteWidth);
	desc11.StructureByteStride = 0; // static_cast<UINT>(desc12.StructureByteStride);
	desc11.Usage = D3D11_USAGE_DEFAULT;
	desc11.CPUAccessFlags = 0;
	desc11.BindFlags = 0;
	desc11.MiscFlags = 0;

	if (desc12.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		desc11.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	if (desc12.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		desc11.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}

	if (desc12.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		desc11.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}

	CCryDX12Buffer* result = DX12_NEW_RAW(CCryDX12Buffer(pDevice, desc11, pResource, initialState, CD3DX12_RESOURCE_DESC(desc12), NULL));

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Buffer* CCryDX12Buffer::Create(CCryDX12Device* pDevice, CCryDX12SwapChain* pSwapChain, ID3D12Resource* pResource)
{
	CCryDX12Buffer* result = Create(pDevice, pResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
	result->GetDX12Resource().SetDX12SwapChain(pSwapChain->GetDX12SwapChain());

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Buffer* CCryDX12Buffer::Create(CCryDX12Device* pDevice, const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData)
{
	DX12_ASSERT(pDesc, "CreateBuffer() called without description!");

	D3D12_RESOURCE_DESC desc12 = CD3DX12_RESOURCE_DESC::Buffer((pDesc->ByteWidth + 255) & ~255);

	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, pDevice->GetCreationMask(false), pDevice->GetVisibilityMask(false));
	D3D12_RESOURCE_STATES resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D11_SUBRESOURCE_DATA sInitialData;
	if (pInitialData)
	{
		sInitialData = *pInitialData;
		sInitialData.SysMemPitch = pDesc->ByteWidth;
		sInitialData.SysMemSlicePitch = pDesc->ByteWidth;
		pInitialData = &sInitialData;
	}

	if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
	{
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, pDevice->GetCreationMask(false), pDevice->GetVisibilityMask(false));
		resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	else if (pDesc->Usage == D3D11_USAGE_STAGING)
	{
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK, pDevice->GetCreationMask(true), pDevice->GetVisibilityMask(true));
		resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	else if (pDesc->Usage == D3D11_USAGE_DYNAMIC)
	{
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, pDevice->GetCreationMask(true), pDevice->GetVisibilityMask(true));
		resourceUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	if (pDesc->CPUAccessFlags != 0)
	{
		// TODO: Check feasibility and performance of allocating staging textures as row major buffers
		// and using CopyTextureRegion instead of ReadFromSubresource/WriteToSubresource

		if (pDesc->CPUAccessFlags == D3D11_CPU_ACCESS_WRITE)
		{
			heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, pDevice->GetCreationMask(true), pDevice->GetVisibilityMask(true));
			resourceUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (pDesc->CPUAccessFlags == D3D11_CPU_ACCESS_READ)
		{
			heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK, pDevice->GetCreationMask(true), pDevice->GetVisibilityMask(true));
			resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else
		{
			DX12_NOT_IMPLEMENTED;
			return nullptr;
		}
	}

	if (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS)
	{
		desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		resourceUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	if (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		resourceUsage = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	if (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)
	{
		desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		resourceUsage = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	ID3D12Resource* resource = NULL;

	if (S_OK != pDevice->GetD3D12Device()->CreateCommittedResource(
	      &heapProperties,
	      D3D12_HEAP_FLAG_NONE,
	      &desc12,
	      resourceUsage,
	      NULL,
	      IID_PPV_ARGS(&resource)
	      ) || !resource)
	{
		DX12_ASSERT(0, "Could not create buffer resource!");
		return NULL;
	}

	auto result = DX12_NEW_RAW(CCryDX12Buffer(pDevice, *pDesc, resource, resourceUsage, CD3DX12_RESOURCE_DESC(resource->GetDesc()), pInitialData, 1));
	resource->Release();

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Buffer::CCryDX12Buffer(CCryDX12Device* pDevice, const D3D11_BUFFER_DESC& desc11, ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState, const D3D12_RESOURCE_DESC& desc12, const D3D11_SUBRESOURCE_DATA* pInitialData, size_t numInitialData)
	: Super(pDevice, pResource, eInitialState, desc12, pInitialData, numInitialData)
	, m_Desc11(desc11)
{
	m_Desc11.StructureByteStride = (m_Desc11.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) ? m_Desc11.StructureByteStride : 0U;
	m_DX12View.Init(m_DX12Resource, NCryDX12::EVT_ConstantBufferView, m_Desc11.ByteWidth);
}

CCryDX12Buffer::~CCryDX12Buffer()
{

}

void CCryDX12Buffer::MapDiscard()
{
	Super::MapDiscard();
	m_DX12View.Init(m_DX12Resource, NCryDX12::EVT_ConstantBufferView, m_Desc11.ByteWidth);
}

void CCryDX12Buffer::CopyDiscard()
{
	Super::CopyDiscard();
	m_DX12View.Init(m_DX12Resource, NCryDX12::EVT_ConstantBufferView, m_Desc11.ByteWidth);
}

#pragma region /* ID3D11Buffer implementation */

void STDMETHODCALLTYPE CCryDX12Buffer::GetDesc(
  _Out_ D3D11_BUFFER_DESC* pDesc)
{
	if (pDesc)
	{
		*pDesc = m_Desc11;
	}
}

#pragma endregion
