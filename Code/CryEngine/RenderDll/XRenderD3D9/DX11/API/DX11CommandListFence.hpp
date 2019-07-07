// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <atomic>
#include <array>
#include <CryThreading/CryThread.h>

// *INDENT-OFF*
namespace std
{
ILINE void upr(UINT64& RESTRICT_REFERENCE a, const UINT64 b)
{
	// Generate conditional moves instead of branches
	a = a >= b ? a : b;
}
}

namespace NCryDX11
{

typedef std::array<ID3D11Fence*, 64> DX11NaryFence;
typedef std::atomic<UINT64> FVAL64;

ILINE UINT64 MaxFenceValue(FVAL64& a, const UINT64& b)
{
	UINT64 utilizedValue = b;
	UINT64 previousValue = a;
	while (previousValue < utilizedValue &&
	       !a.compare_exchange_weak(previousValue, utilizedValue))
		;

	return previousValue;
}

ILINE bool SmallerEqualFenceValues(const UINT64 a, const UINT64 b)
{
	return (a <= b);
}
// *INDENT-ON*

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CCommandListFenceSet
{
public:
	CCommandListFenceSet(CDevice* device);
	~CCommandListFenceSet();

	bool                Init();

	ILINE const DX11NaryFence* GetD3D11Fence() threadsafe_const
	{
		return &m_Fence;
	}

	ILINE UINT64 GetSignalledValue() threadsafe_const
	{
		return m_SignalledValue;
	}

	ILINE void SetSignalledValue(const UINT64 fenceValue) threadsafe
	{
	#ifdef DX11_IN_ORDER_SUBMISSION
		DX11_ASSERT(m_SignalledValue <= fenceValue, "Setting new fence which is older than the signalled!");
		// We do not allow smaller fences being signalled, and fences always signal in-order, no max() neccessary
		m_SignalledValue = fenceValue;
	#else
		// CLs may signal in any order. Is it higher than last known signalled fence? If so, update it!
		MaxFenceValue(m_SignalledValue, fenceValue);
	#endif
	}

	ILINE UINT64 GetSubmittedValue() threadsafe_const
	{
		return m_SubmittedValue;
	}

	ILINE void SetSubmittedValue(const UINT64 fenceValue) threadsafe
	{
	#ifdef DX11_IN_ORDER_SUBMISSION
		DX11_ASSERT(m_SubmittedValue <= fenceValue, "Setting new fence which is older than the submitted!");
		// We do not allow smaller fences being submitted, and fences always submit in-order, no max() neccessary
		m_SubmittedValue = fenceValue;
	#else
		// CLs may submit in any order. Is it higher than last known submitted fence? If so, update it!
		MaxFenceValue(m_SubmittedValue, fenceValue);
	#endif
	}

	ILINE UINT64 GetCurrentValue() threadsafe_const
	{
		return m_CurrentValue;
	}

	// thread-save
	ILINE void SetCurrentValue(const UINT64 fenceValue) threadsafe
	{
	#ifdef DX11_IN_ORDER_ACQUIRATION
		DX11_ASSERT(m_CurrentValue <= fenceValue, "Setting new fence which is older than the current!");
		// We do not allow smaller fences being submitted, and fences always submit in-order, no max() neccessary
		m_CurrentValue = fenceValue;
	#else
		// CLs may submit in any order. Is it higher than last known completed fence? If so, update it!
		MaxFenceValue(m_CurrentValue, fenceValue);
	#endif
	}

	ILINE bool IsCompleted(const UINT64 fenceValue) threadsafe_const
	{
		// Check against last known completed value first to avoid unnecessary fence read
		return (m_LastCompletedValue >= fenceValue) || (AdvanceCompletion() >= fenceValue);
	}

	void SignalFence(const UINT64 fenceValue) threadsafe_const;
	void WaitForFence(const UINT64 fenceValue) threadsafe_const;

	UINT64 ProbeCompletion() threadsafe_const;

	UINT64 AdvanceCompletion() threadsafe_const
	{
		// Check current completed fence
		UINT64 currentCompletedValue = ProbeCompletion();

		if (m_LastCompletedValue < currentCompletedValue)
		{
			DX11_LOG(DX11_FENCE_ANALYZER, "Completed GPU fence: %lld to %lld", m_LastCompletedValue + 1, currentCompletedValue);
		}

#ifdef DX11_IN_ORDER_TERMINATION
		DX11_ASSERT(m_LastCompletedValue <= currentCompletedValue, "Getting new fence which is older than the last!");
		// We do not allow smaller fences being submitted, and fences always complete in-order, no max() neccessary
		m_LastCompletedValue = currentCompletedValue;
#else
		// CLs may terminate in any order. Is it higher than last known completed fence? If so, update it!
		MaxFenceValue(m_LastCompletedValue, currentCompletedValue);
#endif

		return currentCompletedValue;
	}

	ILINE UINT64 GetLastCompletedFenceValue() threadsafe_const
	{
		return m_LastCompletedValue;
	}

private:
	CDevice*        m_pDevice;
	DX11NaryFence   m_Fence;

	// Maximum fence-value of all command-lists currently in flight (allocated, running or free)
	FVAL64 m_CurrentValue;
	// Maximum fence-value of all command-lists passed to the driver (running only)
	FVAL64 m_SubmittedValue; // scheduled
	FVAL64 m_SignalledValue; // processed

	// Maximum fence-value of all command-lists executed by the driver (free only)
	mutable FVAL64 m_LastCompletedValue;

	// Need to synchronize calls to GetData
	mutable CryRWLock m_FenceAccessLock;
};
}
