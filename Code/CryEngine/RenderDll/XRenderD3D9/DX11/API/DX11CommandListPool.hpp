// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DX11Base.hpp"
#include "DX11CommandListFence.hpp"

namespace NCryDX11
{

class CCommandList;
class CSwapChain;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CCommandListPool
{
public:
	CCommandListPool(CDevice* device);
	~CCommandListPool();

	bool Init();
	void Clear();

	void Flush();

	void AcquireCommandList(DX11_PTR(CCommandList) & result) threadsafe;
	void ForfeitCommandList(DX11_PTR(CCommandList) & result) threadsafe;
	void AcquireCommandLists(uint32 numCLs, DX11_PTR(CCommandList) * results) threadsafe;
	void ForfeitCommandLists(uint32 numCLs, DX11_PTR(CCommandList) * results) threadsafe;

	ILINE CCommandList* GetCoreCommandList() threadsafe_const
	{
		return m_pCoreCommandList;
	}

	ILINE CDevice* GetDevice() threadsafe_const
	{
		return m_pDevice;
	}
	
private:
	void TrimLiveCommandLists();
	void CreateOrReuseCommandList(DX11_PTR(CCommandList) & result);

	CDevice*              m_pDevice;

	typedef std::deque<DX11_PTR(CCommandList)> TCommandLists;
	TCommandLists m_LiveCommandLists;
	TCommandLists m_FreeCommandLists;

	DX11_PTR(CCommandList) m_pCoreCommandList;

	#ifdef DX11_STATS
	size_t m_PeakNumCommandListsAllocated;
	size_t m_PeakNumCommandListsInFlight;
	#endif // DX11_STATS
};

}
