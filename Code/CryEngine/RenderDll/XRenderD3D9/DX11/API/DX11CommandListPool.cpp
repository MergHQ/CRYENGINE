// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX11CommandListPool.hpp"

namespace NCryDX11
{
CCommandListPool::CCommandListPool(CDevice* device)
	: m_pDevice(device)
	, m_pCoreCommandList(nullptr)
#ifdef DX11_STATS
	, m_PeakNumCommandListsAllocated(0)
	, m_PeakNumCommandListsInFlight(0)
#endif // DX11_STATS
{
}

//---------------------------------------------------------------------------------------------------------------------
CCommandListPool::~CCommandListPool()
{
	Clear();
}

//---------------------------------------------------------------------------------------------------------------------
bool CCommandListPool::Init()
{
	D3DDeviceContext* pImmediateContext = m_pDevice->GetD3D11ImmediateDeviceContext();

#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	ID3DXboxPerformanceContext* pPerformanceDeviceContext = nullptr;
	const HRESULT hr = pImmediateContext->QueryInterface(__uuidof(ID3DXboxPerformanceContext), (void**)&pPerformanceDeviceContext);
	if (hr != S_OK)
	{
		DX11_ERROR("Could not create command list");
		return false;
}
#endif

	m_pCoreCommandList = new CCommandList(*this);
	m_pCoreCommandList->m_pD3D11DeviceContext = pImmediateContext;
#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	m_pCoreCommandList->m_pD3D11PerformanceContext = pPerformanceDeviceContext;
#endif
	m_pCoreCommandList->Init();

	pImmediateContext->Release();
#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	pPerformanceDeviceContext->Release();
#endif

#ifdef DX11_STATS
	m_PeakNumCommandListsAllocated = 0;
	m_PeakNumCommandListsInFlight = 0;
#endif // DX11_STATS

	return true;
}

void CCommandListPool::Clear()
{
	// No running command lists of any type allowed:
	// * trigger Live-to-Free transitions
	TrimLiveCommandLists();
	CRY_ASSERT(m_LiveCommandLists.empty());

	m_FreeCommandLists.clear();

	m_pCoreCommandList = nullptr;
}

void CCommandListPool::Flush()
{
	bool bSubmitted = false;
	// Submit completed but not yet submitted command-lists from the head of the live-list
	for (uint32 t = 0; t < m_LiveCommandLists.size(); ++t)
	{
		DX11_PTR(CCommandList) pCmdList = m_LiveCommandLists[t];

		if (pCmdList->m_eState == CCommandList::CLSTATE_COMPLETED)
		{
			pCmdList->Submit();
			bSubmitted = true;
		}

		if (pCmdList->m_eState < CCommandList::CLSTATE_SUBMITTED)
		{
			break;
		}
	}

	if (bSubmitted)
	{
		TrimLiveCommandLists();
		GetDeviceObjectFactory().GetCoreCommandList().Reset();
	}

	CRY_ASSERT(m_LiveCommandLists.empty());
}

//---------------------------------------------------------------------------------------------------------------------
void CCommandListPool::TrimLiveCommandLists()
{
	// Remove finished command-lists from the head of the live-list
	while (m_LiveCommandLists.size())
	{
		DX11_PTR(CCommandList) pCmdList = m_LiveCommandLists.front();

		if (pCmdList->m_eState == CCommandList::CLSTATE_SUBMITTED)
		{
			m_LiveCommandLists.pop_front();
			m_FreeCommandLists.push_back(pCmdList);

			pCmdList->Clear();
		}
		else
		{
			break;
		}
	}
}

void CCommandListPool::CreateOrReuseCommandList(DX11_PTR(CCommandList)& result)
{
	if (m_FreeCommandLists.empty())
	{
		result = new CCommandList(*this);
		const bool bInitResult =  result->Init();
		CRY_ASSERT(bInitResult);
	}
	else
	{
		result = m_FreeCommandLists.front();

		m_FreeCommandLists.pop_front();
	}
	
	m_LiveCommandLists.push_back(result);

#ifdef DX11_STATS
	m_PeakNumCommandListsAllocated = std::max(m_PeakNumCommandListsAllocated, m_LiveCommandLists.size() + m_FreeCommandLists.size());
	m_PeakNumCommandListsInFlight = std::max(m_PeakNumCommandListsInFlight, m_LiveCommandLists.size());
#endif // DX11_STATS
}

void CCommandListPool::AcquireCommandList(DX11_PTR(CCommandList)& result) threadsafe
{
	static CryCriticalSectionNonRecursive csThreadSafeScope;
	CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);

	CreateOrReuseCommandList(result);
}

void CCommandListPool::ForfeitCommandList(DX11_PTR(CCommandList)& result) threadsafe
{
	static CryCriticalSectionNonRecursive csThreadSafeScope;
	CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);

	DX11_ASSERT(result->IsImmediate() || result->m_eState == CCommandList::CLSTATE_COMPLETED, "It's not possible to forfeit an unclosed command list!");
	result = nullptr;
}

void CCommandListPool::AcquireCommandLists(uint32 numCLs, DX11_PTR(CCommandList)* results) threadsafe
{
	static CryCriticalSectionNonRecursive csThreadSafeScope;
	CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);

	results[0] = GetCoreCommandList();
	for (uint32 i = 1; i < numCLs; ++i)
		CreateOrReuseCommandList(results[i]);
}

void CCommandListPool::ForfeitCommandLists(uint32 numCLs, DX11_PTR(CCommandList)* results) threadsafe
{
	static CryCriticalSectionNonRecursive csThreadSafeScope;
	CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(csThreadSafeScope);

	int32 i = numCLs;
	while (--i >= 0)
	{
		DX11_ASSERT(results[i]->IsImmediate() || results[i]->m_eState == CCommandList::CLSTATE_COMPLETED, "It's not possible to forfeit an unclosed command list!");
		results[i] = nullptr;
	}
}
}
