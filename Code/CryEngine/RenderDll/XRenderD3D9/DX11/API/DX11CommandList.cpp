// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX11CommandList.hpp"
#include "DX11CommandListPool.hpp"

namespace NCryDX11
{
//---------------------------------------------------------------------------------------------------------------------
CCommandList::CCommandList(CCommandListPool& rPool)
	: CDeviceObject(rPool.GetDevice())
	, m_rPool(rPool)
	, m_pD3D11DeviceContext(nullptr)
#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	, m_pD3D11PerformanceContext(nullptr)
#endif
	, m_pD3D11CommandList(nullptr)
	, m_pD3D11Device(nullptr)
	, m_eState(CLSTATE_UNINITIALIZED)
{
}

//---------------------------------------------------------------------------------------------------------------------
CCommandList::~CCommandList()
{
	Reset();
}

//---------------------------------------------------------------------------------------------------------------------

bool CCommandList::Init()
{
	CRY_ASSERT(m_eState == CLSTATE_UNINITIALIZED);

	m_pD3D11Device = GetDevice()->GetD3D11Device();

	if (!m_pD3D11DeviceContext)
	{
		D3DDeviceContext* pContext = nullptr;
#if (CRY_RENDERER_DIRECT3D >= 111)
		if (S_OK != m_pD3D11Device->CreateDeferredContext1(0, &pContext))
#else
		if (S_OK != m_pD3D11Device->CreateDeferredContext(0, &pContext))
#endif
		{
			DX11_ERROR("Could not create command list");
			return false;
		}

		m_pD3D11DeviceContext = pContext;
		pContext->Release();
	}
	
#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	if (!m_pD3D11PerformanceContext)
	{
		ID3DXboxPerformanceContext* pPerformanceDeviceContext = nullptr;
		const HRESULT hr = m_pD3D11DeviceContext->QueryInterface(__uuidof(ID3DXboxPerformanceContext), (void**)&pPerformanceDeviceContext);
		if (hr != S_OK)
		{
			DX11_ERROR("Could not create command list");
			return false;
		}
		m_pD3D11PerformanceContext = pPerformanceDeviceContext;
		pPerformanceDeviceContext->Release();
	}
#endif

	m_eState = CLSTATE_FREE;

	return true;
}

void CCommandList::Begin()
{
	if (!IsImmediate())
	{
		CRY_ASSERT(m_eState == CLSTATE_FREE);
		m_eState = CLSTATE_STARTED;
	}
}

void CCommandList::End()
{
	if (!IsImmediate())
	{
		CRY_ASSERT(m_eState == CLSTATE_STARTED);
		CRY_ASSERT(m_pD3D11CommandList == nullptr);
		ID3D11CommandList* pD3D11CommandList = nullptr;
		const HRESULT result = m_pD3D11DeviceContext->FinishCommandList(false, &pD3D11CommandList);
		CRY_ASSERT(result == S_OK);
		m_pD3D11CommandList = pD3D11CommandList;
		pD3D11CommandList->Release();
		m_eState = CLSTATE_COMPLETED;
	}
}

void CCommandList::Submit()
{
	CRY_ASSERT(!IsImmediate());
	CRY_ASSERT(m_eState == CLSTATE_COMPLETED);
	CRY_ASSERT(gcpRendD3D->m_pRT->IsRenderThread());

	m_rPool.GetCoreCommandList()->GetD3D11DeviceContext()->ExecuteCommandList(m_pD3D11CommandList, false);
	
	m_eState = CLSTATE_SUBMITTED;
}

void CCommandList::Clear()
{
	m_pD3D11CommandList = nullptr;
	m_eState = CLSTATE_FREE;
}

bool CCommandList::Reset()
{
	CRY_ASSERT(m_eState == CLSTATE_FREE);

	m_pD3D11Device = nullptr;
	m_pD3D11DeviceContext = nullptr;
#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	m_pD3D11PerformanceContext = nullptr;
#endif
	m_pD3D11CommandList = nullptr;

	m_eState = CLSTATE_UNINITIALIZED;

	return true;
}
}
