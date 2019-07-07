// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DX11Base.hpp"
#include "DX11CommandListFence.hpp"

class CDeviceStatesManagerDX11;

namespace NCryDX11
{
class CCommandListPool;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CCommandList : public CDeviceObject
{
	friend class CCommandListPool;

public:
	virtual ~CCommandList();

	bool Init();
	bool Reset();

	ILINE D3DDeviceContext* GetD3D11DeviceContext() threadsafe_const
	{
		return m_pD3D11DeviceContext;
	}

#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	ILINE ID3DXboxPerformanceContext* GetD3D11PerformanceContext() threadsafe_const
	{
		return m_pD3D11PerformanceContext;
	}
#endif

	ILINE ID3D11CommandList* GetD3D11CommandList() threadsafe_const
	{
		return m_pD3D11CommandList;
	}

	ILINE CCommandListPool& GetCommandListPool() threadsafe_const
	{
		return m_rPool;
	}

	ILINE bool IsImmediate()
	{
		return m_pD3D11DeviceContext->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE;
	}

	//---------------------------------------------------------------------------------------------------------------------
	// Stage 1
	void       Begin();
	void       End();

	// Stage 2
	void       Submit();

	// Stage 3
	void       Clear();

protected:
	CCommandList(CCommandListPool& pPool);
	CCommandListPool& m_rPool;

	D3DDevice* m_pD3D11Device;
	DX11_PTR(D3DDeviceContext) m_pD3D11DeviceContext;
#if defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	DX11_PTR(ID3DXboxPerformanceContext) m_pD3D11PerformanceContext;
#endif
	DX11_PTR(ID3D11CommandList) m_pD3D11CommandList;

	enum
	{
		CLSTATE_UNINITIALIZED,
		CLSTATE_FREE,
		CLSTATE_STARTED,
		CLSTATE_COMPLETED,
		CLSTATE_SUBMITTED,
	}
	m_eState;
};

}
