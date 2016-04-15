// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX12QueryHeap.hpp"

namespace NCryDX12
{

//---------------------------------------------------------------------------------------------------------------------
CQueryHeap::CQueryHeap(CDevice* device)
	: CDeviceObject(device)
{

}

//---------------------------------------------------------------------------------------------------------------------
CQueryHeap::~CQueryHeap()
{

}

//---------------------------------------------------------------------------------------------------------------------
bool CQueryHeap::Init(CDevice* device, const D3D12_QUERY_HEAP_DESC& desc)
{
	if (!IsInitialized())
	{
		SetDevice(device);

		ID3D12QueryHeap* heap;
		GetDevice()->GetD3D12Device()->CreateQueryHeap(&desc, IID_PPV_ARGS(&heap));

		m_pQueryHeap = heap;
		heap->Release();

		m_Desc12 = desc /*m_pQueryHeap->GetDesc()*/;

		IsInitialized(true);
	}

	Reset();
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
void CQueryHeap::Reset()
{
}

}
