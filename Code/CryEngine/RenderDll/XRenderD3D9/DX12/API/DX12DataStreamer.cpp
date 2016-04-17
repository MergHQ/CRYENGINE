// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX12DataStreamer.hpp"
#include "DX12Device.hpp"
#include "DX12Resource.hpp"

namespace NCryDX12 {

//---------------------------------------------------------------------------------------------------------------------
CDataStreamer::CDataStreamer()
	: m_pDevice(NULL)
{

}

//---------------------------------------------------------------------------------------------------------------------
CDataStreamer::~CDataStreamer()
{

}

//---------------------------------------------------------------------------------------------------------------------
bool CDataStreamer::Init(CDevice* pDevice)
{
	m_pDevice = pDevice;
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
void CDataStreamer::Flush(UINT64 fenceValue)
{

}

//---------------------------------------------------------------------------------------------------------------------
UINT64 CDataStreamer::RequestUpload(CResource* destResource, UINT subresource, const D3D12_SUBRESOURCE_DATA& desc, UINT64 size)
{
	SRequest request;

	if (S_OK == m_pDevice->GetD3D12Device()->CreateCommittedResource(
	      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, 0, 0),
	      D3D12_HEAP_FLAG_NONE,
	      &CD3DX12_RESOURCE_DESC::Buffer(size),
	      D3D12_RESOURCE_STATE_GENERIC_READ,
	      nullptr,
	      IID_PPV_ARGS(&request.m_pTemporaryResource)))
	{

	}

	m_Requests.push_back(request);

	return 0;
}

}
