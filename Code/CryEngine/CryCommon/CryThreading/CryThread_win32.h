// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <process.h>

//! CryEvent represent a synchronization event.
class CryEvent
{
public:
	CryEvent();
	~CryEvent();

	//! Reset the event to the unsignalled state.
	void Reset();

	//! Set the event to the signalled state.
	void Set();

	//! Access a HANDLE to wait on.
	void* GetHandle() const { return m_handle; };

	//! Wait indefinitely for the object to become signalled.
	void Wait() const;

	//! Wait, with a time limit, for the object to become signalled.
	bool Wait(const uint32 timeoutMillis) const;

private:
	CryEvent(const CryEvent&);
	CryEvent& operator=(const CryEvent&);

private:
	void* m_handle;
};

typedef CryEvent CryEventTimed;

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//! From winnt.h.
struct CRY_CRITICAL_SECTION
{
	void*          DebugInfo;
	long           LockCount;
	long           RecursionCount;
	void*          OwningThread;
	void*          LockSemaphore;
	unsigned long* SpinCount;        //!< Force size on 64-bit systems when packed.
};

//////////////////////////////////////////////////////////////////////////

//! Kernel mutex - don't use... use CryMutex instead.
class CryLock_WinMutex
{
public:
	CryLock_WinMutex();
	~CryLock_WinMutex();

	void Lock();
	void Unlock();
	bool TryLock();
#ifndef _RELEASE
	//! Deprecated: do not use this function - its return value might already be wrong the moment it is returned.
	bool IsLocked()
	{
		if (TryLock())
		{
			Unlock();
			return true;
		}
		else
		{
			return false;
		}
	}
#endif

	void* _get_win32_handle() { return m_hdl; }

private:
	CryLock_WinMutex(const CryLock_WinMutex&);
	CryLock_WinMutex& operator=(const CryLock_WinMutex&);

private:
	void* m_hdl;
};

//! Critical section - don't use this, use CryCriticalSection instead.
class CryLock_CritSection
{
public:
	CryLock_CritSection();
	~CryLock_CritSection();

	void Lock();
	void Unlock();
	bool TryLock();

	//! Deprecated: do not use this function - its return value might already be wrong the moment it is returned.
	bool IsLocked()
	{
		return m_cs.RecursionCount > 0 && (UINT_PTR)m_cs.OwningThread == CryGetCurrentThreadId();
	}

private:
	CryLock_CritSection(const CryLock_CritSection&);
	CryLock_CritSection& operator=(const CryLock_CritSection&);

private:
	CRY_CRITICAL_SECTION m_cs;
};

template<> class CryLockT<CRYLOCK_RECURSIVE> : public CryLock_CritSection
{
};
template<> class CryLockT<CRYLOCK_FAST> : public CryLock_CritSection
{
};
class CryMutex : public CryLock_WinMutex
{
};
#define _CRYTHREAD_CONDLOCK_GLITCH 1

//////////////////////////////////////////////////////////////////////////
class CryConditionVariable
{
public:
	typedef CryMutex LockType;

	CryConditionVariable();
	~CryConditionVariable();
	void Wait(LockType& lock);
	bool TimedWait(LockType& lock, uint32 millis);
	void NotifySingle();
	void Notify();

private:
	CryConditionVariable(const CryConditionVariable&);
	CryConditionVariable& operator=(const CryConditionVariable&);

private:
	int                  m_waitersCount;
	CRY_CRITICAL_SECTION m_waitersCountLock;
	void*                m_sema;
	void*                m_waitersDone;
	size_t               m_wasBroadcast;
};

//////////////////////////////////////////////////////////////////////////
//! Platform independent wrapper for a counting semaphore.
class CrySemaphore
{
public:
	CrySemaphore(int nMaximumCount, int nInitialCount = 0);
	~CrySemaphore();
	void Acquire();
	void Release();

private:
	void* m_Semaphore;
};

//////////////////////////////////////////////////////////////////////////
//! Platform independent wrapper for a counting semaphore
//! except that this version uses C-A-S only until a blocking call is needed.
//! -> No kernel call if there are object in the semaphore.
class CryFastSemaphore
{
public:
	CryFastSemaphore(int nMaximumCount, int nInitialCount = 0);
	~CryFastSemaphore();
	void Acquire();
	void Release();

private:
	CrySemaphore   m_Semaphore;
	volatile int32 m_nCounter;
};

//////////////////////////////////////////////////////////////////////////
#if !defined(_CRYTHREAD_HAVE_RWLOCK)
class CryRWLock
{
	void* /*SRWLOCK*/ m_Lock;

	CryRWLock(const CryRWLock&);
	CryRWLock& operator=(const CryRWLock&);

public:
	CryRWLock();
	~CryRWLock();

	void RLock();
	void RUnlock();

	void WLock();
	void WUnlock();

	void Lock();
	void Unlock();

	#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
	//! Enabling TryXXX requires Windows 7 or newer.
	bool TryRLock();
	bool TryWLock();
	bool TryLock();
	#endif
};

//! Indicate that this implementation header provides an implementation for CryRWLock.
	#define _CRYTHREAD_HAVE_RWLOCK 1
#endif
