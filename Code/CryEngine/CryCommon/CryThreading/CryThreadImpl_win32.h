// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Platform/platform.h>
#include "CryThread_win32.h"
#include <CryCore/Platform/CryWindows.h>

struct SThreadNameDesc
{
	DWORD  dwType;
	LPCSTR szName;
	DWORD  dwThreadID;
	DWORD  dwFlags;
};

//////////////////////////////////////////////////////////////////////////
CryEvent::CryEvent()
{
	m_handle = (void*)CreateEvent(NULL, FALSE, FALSE, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryEvent::~CryEvent()
{
	CloseHandle(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Reset()
{
	ResetEvent(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Set()
{
	SetEvent(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Wait() const
{
	WaitForSingleObject(m_handle, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
bool CryEvent::Wait(const uint32 timeoutMillis) const
{
	if (WaitForSingleObject(m_handle, timeoutMillis) == WAIT_TIMEOUT)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CryLock_WinMutex
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CryLock_WinMutex::CryLock_WinMutex() : m_hdl(CreateMutex(NULL, FALSE, NULL)) {}
CryLock_WinMutex::~CryLock_WinMutex()
{
	CloseHandle(m_hdl);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_WinMutex::Lock()
{
	WaitForSingleObject(m_hdl, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_WinMutex::Unlock()
{
	ReleaseMutex(m_hdl);
}

//////////////////////////////////////////////////////////////////////////
bool CryLock_WinMutex::TryLock()
{
	return WaitForSingleObject(m_hdl, 0) != WAIT_TIMEOUT;
}

//////////////////////////////////////////////////////////////////////////
// CryLock_CritSection
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CryLock_CritSection::CryLock_CritSection()
{
	InitializeCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
CryLock_CritSection::~CryLock_CritSection()
{
	DeleteCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_CritSection::Lock()
{
	EnterCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_CritSection::Unlock()
{
	LeaveCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
bool CryLock_CritSection::TryLock()
{
	return TryEnterCriticalSection((CRITICAL_SECTION*)&m_cs) != FALSE;
}

//////////////////////////////////////////////////////////////////////////
// Most of this is taken from http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
//////////////////////////////////////////////////////////////////////////
CryConditionVariable::CryConditionVariable()
{
	m_waitersCount = 0;
	m_wasBroadcast = 0;
	m_sema = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
	InitializeCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	m_waitersDone = CreateEvent(NULL, FALSE, FALSE, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryConditionVariable::~CryConditionVariable()
{
	CloseHandle(m_sema);
	DeleteCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	CloseHandle(m_waitersDone);
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::Wait(LockType& lock)
{
	EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	m_waitersCount++;
	LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

	SignalObjectAndWait(lock._get_win32_handle(), m_sema, INFINITE, FALSE);

	EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	m_waitersCount--;
	bool lastWaiter = m_wasBroadcast && m_waitersCount == 0;
	LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

	if (lastWaiter)
		SignalObjectAndWait(m_waitersDone, lock._get_win32_handle(), INFINITE, FALSE);
	else
		WaitForSingleObject(lock._get_win32_handle(), INFINITE);
}

//////////////////////////////////////////////////////////////////////////
bool CryConditionVariable::TimedWait(LockType& lock, uint32 millis)
{
	EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	m_waitersCount++;
	LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

	bool ok = true;
	if (WAIT_TIMEOUT == SignalObjectAndWait(lock._get_win32_handle(), m_sema, millis, FALSE))
		ok = false;

	EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	m_waitersCount--;
	bool lastWaiter = m_wasBroadcast && m_waitersCount == 0;
	LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

	if (lastWaiter)
		SignalObjectAndWait(m_waitersDone, lock._get_win32_handle(), INFINITE, FALSE);
	else
		WaitForSingleObject(lock._get_win32_handle(), INFINITE);

	return ok;
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::NotifySingle()
{
	EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	bool haveWaiters = m_waitersCount > 0;
	LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	if (haveWaiters)
		ReleaseSemaphore(m_sema, 1, 0);
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::Notify()
{
	EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	bool haveWaiters = false;
	if (m_waitersCount > 0)
	{
		m_wasBroadcast = 1;
		haveWaiters = true;
	}
	if (haveWaiters)
	{
		ReleaseSemaphore(m_sema, m_waitersCount, 0);
		LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
		WaitForSingleObject(m_waitersDone, INFINITE);
		m_wasBroadcast = 0;
	}
	else
	{
		LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
	}
}

//////////////////////////////////////////////////////////////////////////
CrySemaphore::CrySemaphore(int nMaximumCount, int nInitialCount)
{
	m_Semaphore = (void*)CreateSemaphore(NULL, nInitialCount, nMaximumCount, NULL);
}

//////////////////////////////////////////////////////////////////////////
CrySemaphore::~CrySemaphore()
{
	CloseHandle((HANDLE)m_Semaphore);
}

//////////////////////////////////////////////////////////////////////////
void CrySemaphore::Acquire()
{
	WaitForSingleObject((HANDLE)m_Semaphore, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
void CrySemaphore::Release()
{
	ReleaseSemaphore((HANDLE)m_Semaphore, 1, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryFastSemaphore::CryFastSemaphore(int nMaximumCount, int nInitialCount) :
	m_Semaphore(nMaximumCount),
	m_nCounter(nInitialCount)
{
}

//////////////////////////////////////////////////////////////////////////
CryFastSemaphore::~CryFastSemaphore()
{
}

//////////////////////////////////////////////////////////////////////////
void CryFastSemaphore::Acquire()
{
	int nCount = ~0;
	do
	{
		nCount = *const_cast<volatile int*>(&m_nCounter);
	}
	while (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nCounter), nCount - 1, nCount) != nCount);

	// if the count would have been 0 or below, go to kernel semaphore
	if ((nCount - 1) < 0)
		m_Semaphore.Acquire();
}

//////////////////////////////////////////////////////////////////////////
void CryFastSemaphore::Release()
{
	int nCount = ~0;
	do
	{
		nCount = *const_cast<volatile int*>(&m_nCounter);
	}
	while (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nCounter), nCount + 1, nCount) != nCount);

	// wake up kernel semaphore if we have waiter
	if (nCount < 0)
		m_Semaphore.Release();
}

//////////////////////////////////////////////////////////////////////////
CryRWLock::CryRWLock()
{
	STATIC_ASSERT(sizeof(m_Lock) == sizeof(PSRWLOCK), "RWLock-pointer has invalid size");
	InitializeSRWLock(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
CryRWLock::~CryRWLock()
{
}

//////////////////////////////////////////////////////////////////////////
void CryRWLock::RLock()
{
	AcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
bool CryRWLock::TryRLock()
{
	return TryAcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Lock)) != 0;
}
#endif

//////////////////////////////////////////////////////////////////////////
void CryRWLock::RUnlock()
{
	ReleaseSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
void CryRWLock::WLock()
{
	AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
bool CryRWLock::TryWLock()
{
	return TryAcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Lock)) != 0;
}
#endif

//////////////////////////////////////////////////////////////////////////
void CryRWLock::WUnlock()
{
	ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
void CryRWLock::Lock()
{
	WLock();
}

//////////////////////////////////////////////////////////////////////////
#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
bool CryRWLock::TryLock()
{
	return TryWLock();
}
#endif

//////////////////////////////////////////////////////////////////////////
void CryRWLock::Unlock()
{
	WUnlock();
}

///////////////////////////////////////////////////////////////////////////////
namespace CryMT {
namespace detail {
void CryMemoryBarrier()
{
	MemoryBarrier();
}

} // namespace detail
} // namespace CryMT
