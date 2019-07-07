// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX11CommandListFence.hpp"
#include "DriverD3D.h"

namespace NCryDX11
{
//---------------------------------------------------------------------------------------------------------------------
CCommandListFenceSet::CCommandListFenceSet(CDevice* device)
	: m_pDevice(device)
	, m_LastCompletedValue(0)
	, m_SubmittedValue(0)
	, m_SignalledValue(0)
	, m_CurrentValue(0)
{
}

//---------------------------------------------------------------------------------------------------------------------
CCommandListFenceSet::~CCommandListFenceSet()
{
}

//---------------------------------------------------------------------------------------------------------------------
bool CCommandListFenceSet::Init()
{
	D3D11_QUERY_DESC FenceInfo;

	ZeroStruct(FenceInfo);

	FenceInfo.Query = D3D11_QUERY_EVENT;
	FenceInfo.MiscFlags = 0; // D3D11_QUERY_MISC_PREDICATEHINT;

	for (int f = 0, n = m_Fence.size(); f < n; ++f)
	{
		ID3D11Query* fence;
		if (S_OK != m_pDevice->GetD3D11Device()->CreateQuery(&FenceInfo, &fence))
		{
			DX11_ERROR("Could not create fence object!");
			return false;
		}

		m_Fence[f] = fence;
	}

	return true;
}

void CCommandListFenceSet::SignalFence(const UINT64 fenceValue) threadsafe_const
{
	DX11_ERROR("Fences can't be signaled by the CPU under DX11");
}

void CCommandListFenceSet::WaitForFence(const UINT64 fenceValue) threadsafe_const
{
	DX11_LOG(DX11_FENCE_ANALYZER, "Waiting CPU for fence: %lld (is %lld currently)",
	       fenceValue,
	       ProbeCompletion());

	{
		if (m_LastCompletedValue < fenceValue)
		{
			DX11_ASSERT(m_CurrentValue   >= fenceValue, "Fence to be tested not allocated!");
			DX11_ASSERT(m_SubmittedValue >= fenceValue, "Fence to be tested not submitted!");
			DX11_ASSERT(m_SignalledValue >= fenceValue, "Fence to be tested not signalled!");

			BOOL bQuery = false;

			const int size  = int(m_Fence.size());
			const int start = int((m_LastCompletedValue      + 1) % size);
			const int end   = int((fenceValue                + 1) % size);
			const int last  = int((fenceValue                + 0) % size);

			D3DDeviceContext* const pContext = m_pDevice->GetD3D11ImmediateDeviceContext();

			// NOTE: optimal
			if (end > start)
			{
				while ((pContext->GetData(m_Fence[last - 0], (void*)&bQuery, sizeof(BOOL), 0)) != S_OK);
			}
			else
			{
				while ((pContext->GetData(m_Fence[size - 1], (void*)&bQuery, sizeof(BOOL), 0)) != S_OK);
				while ((pContext->GetData(m_Fence[last - 0], (void*)&bQuery, sizeof(BOOL), 0)) != S_OK);
			}

		}
	}

	DX11_LOG(DX11_FENCE_ANALYZER, "Completed CPU on fence: %lld",
	       ProbeCompletion());

	AdvanceCompletion();
}

UINT64 CCommandListFenceSet::ProbeCompletion() threadsafe_const
{
	// Check current completed fence
	UINT64 currentCompletedValue = m_LastCompletedValue;

	if (m_LastCompletedValue < m_SignalledValue)
	{
		BOOL bQuery = false;

		const int size  = int(m_Fence.size());
		const int start = int(m_LastCompletedValue + 1);
		const int end   = int(m_SignalledValue     + 1);
		
		D3DDeviceContext* const pContext = m_pDevice->GetD3D11ImmediateDeviceContext();

		for (int i = start; i < end; ++i)
		{
			if ((pContext->GetData(m_Fence[i % size], (void*)&bQuery, sizeof(BOOL), D3D11_ASYNC_GETDATA_DONOTFLUSH)) != S_OK)
				break;

			++currentCompletedValue;
		}
	}

	return currentCompletedValue;
}

}
