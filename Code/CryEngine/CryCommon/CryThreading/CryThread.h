// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// Include basic multithread primitives.
#include "CryAtomics.h"
#include <CryCore/BitFiddling.h>

#define THREAD_NAME_LENGTH_MAX 64

enum CryLockType
{
	CRYLOCK_FAST      = 1,  //!< A fast potentially (non-recursive) mutex.
	CRYLOCK_RECURSIVE = 2,  //!< A recursive mutex.
};

#define CRYLOCK_HAVE_FASTLOCK 1

//! Primitive locks and conditions.
//! Primitive locks are represented by instance of class CryLockT<Type>.
template<CryLockType Type> class CryLockT
{
	/* Unsupported lock type. */
};

//////////////////////////////////////////////////////////////////////////
// Typedefs.
//////////////////////////////////////////////////////////////////////////
typedef CryLockT<CRYLOCK_RECURSIVE> CryCriticalSection;
typedef CryLockT<CRYLOCK_FAST>      CryCriticalSectionNonRecursive;
//////////////////////////////////////////////////////////////////////////

//! CryAutoCriticalSection implements a helper class to automatically.
//! lock critical section in constructor and release on destructor.
template<class LockClass> class CryAutoLock
{
private:
	LockClass* m_pLock;

	CryAutoLock();
	CryAutoLock(const CryAutoLock<LockClass>&);
	CryAutoLock<LockClass>& operator=(const CryAutoLock<LockClass>&);

public:
	CryAutoLock(LockClass& Lock) : m_pLock(&Lock) { m_pLock->Lock(); }
	CryAutoLock(const LockClass& Lock) : m_pLock(const_cast<LockClass*>(&Lock)) { m_pLock->Lock(); }
	~CryAutoLock() { m_pLock->Unlock(); }
};

//! CryOptionalAutoLock implements a helper class to automatically.
//! Lock critical section (if needed) in constructor and release on destructor.
template<class LockClass> class CryOptionalAutoLock
{
private:
	LockClass* m_Lock;
	bool       m_bLockAcquired;

	CryOptionalAutoLock();
	CryOptionalAutoLock(const CryOptionalAutoLock<LockClass>&);
	CryOptionalAutoLock<LockClass>& operator=(const CryOptionalAutoLock<LockClass>&);

public:
	CryOptionalAutoLock(LockClass& Lock, bool acquireLock) : m_Lock(&Lock), m_bLockAcquired(false)
	{
		if (acquireLock)
		{
			Acquire();
		}
	}
	~CryOptionalAutoLock()
	{
		Release();
	}
	void Release()
	{
		if (m_bLockAcquired)
		{
			m_Lock->Unlock();
			m_bLockAcquired = false;
		}
	}
	void Acquire()
	{
		if (!m_bLockAcquired)
		{
			m_Lock->Lock();
			m_bLockAcquired = true;
		}
	}
};

//! CryAutoSet implements a helper class to automatically.
//! set and reset value in constructor and release on destructor.
template<class ValueClass> class CryAutoSet
{
private:
	ValueClass* m_pValue;

	CryAutoSet();
	CryAutoSet(const CryAutoSet<ValueClass>&);
	CryAutoSet<ValueClass>& operator=(const CryAutoSet<ValueClass>&);

public:
	CryAutoSet(ValueClass& value) : m_pValue(&value) { *m_pValue = (ValueClass)1; }
	~CryAutoSet() { *m_pValue = (ValueClass)0; }
};

//! Auto critical section is the most commonly used type of auto lock.
typedef CryAutoLock<CryCriticalSection>             CryAutoCriticalSection;
typedef CryAutoLock<CryCriticalSectionNonRecursive> CryAutoCriticalSectionNoRecursive;

#define AUTO_LOCK_T(Type, lock) PREFAST_SUPPRESS_WARNING(6246); CryAutoLock<Type> __AutoLock(lock)
#define AUTO_LOCK(lock)         AUTO_LOCK_T(CryCriticalSection, lock)
#define AUTO_LOCK_CS(csLock)    CryAutoCriticalSection __AL__ ## csLock(csLock)

// Include architecture specific code.
#if CRY_PLATFORM_WINAPI
	#include <CryThreading/CryThread_win32.h>
#elif CRY_PLATFORM_POSIX
	#include <CryThreading/CryThread_posix.h>
#else
// Put other platform specific includes here!
	#include <CryThreading/CryThread_dummy.h>
#endif

#if !defined _CRYTHREAD_CONDLOCK_GLITCH
typedef CryLockT<CRYLOCK_RECURSIVE> CryMutex;
#endif // !_CRYTHREAD_CONDLOCK_GLITCH

#if !defined _CRYTHREAD_HAVE_RWLOCK && !defined _CRYTHREAD_CONDLOCK_GLITCH
//! If the architecture specific code does not define a class CryRWLock, then
//! a default implementation is provided here.
class CryRWLock
{

	CryCriticalSection   m_lockExclusiveAccess;
	CryCriticalSection   m_lockSharedAccessComplete;
	CryConditionVariable m_condSharedAccessComplete;

	int                  m_nSharedAccessCount;
	int                  m_nCompletedSharedAccessCount;
	bool                 m_bExclusiveAccess;

	CryRWLock(const CryRWLock&);
	CryRWLock& operator=(const CryRWLock&);

	void       AdjustSharedAccessCount()
	{
		m_nSharedAccessCount -= m_nCompletedSharedAccessCount;
		m_nCompletedSharedAccessCount = 0;
	}

public:
	CryRWLock()
		: m_nSharedAccessCount(0),
		m_nCompletedSharedAccessCount(0),
		m_bExclusiveAccess(false)
	{}

	void RLock()
	{
		m_lockExclusiveAccess.Lock();
		if (++m_nSharedAccessCount == INT_MAX)
		{
			m_lockSharedAccessComplete.Lock();
			AdjustSharedAccessCount();
			m_lockSharedAccessComplete.Unlock();
		}
		m_lockExclusiveAccess.Unlock();
	}

	bool TryRLock()
	{
		if (!m_lockExclusiveAccess.TryLock())
			return false;
		if (++m_nSharedAccessCount == INT_MAX)
		{
			m_lockSharedAccessComplete.Lock();
			AdjustSharedAccessCount();
			m_lockSharedAccessComplete.Unlock();
		}
		m_lockExclusiveAccess.Unlock();
		return true;
	}

	void RUnlock()
	{
		Unlock();
	}

	void WLock()
	{
		m_lockExclusiveAccess.Lock();
		m_lockSharedAccessComplete.Lock();
		assert(!m_bExclusiveAccess);
		AdjustSharedAccessCount();
		if (m_nSharedAccessCount > 0)
		{
			m_nCompletedSharedAccessCount -= m_nSharedAccessCount;
			do
			{
				m_condSharedAccessComplete.Wait(m_lockSharedAccessComplete);
			}
			while (m_nCompletedSharedAccessCount < 0);
			m_nSharedAccessCount = 0;
		}
		m_bExclusiveAccess = true;
	}

	bool TryWLock()
	{
		if (!m_lockExclusiveAccess.TryLock())
			return false;
		if (!m_lockSharedAccessComplete.TryLock())
		{
			m_lockExclusiveAccess.Unlock();
			return false;
		}
		assert(!m_bExclusiveAccess);
		AdjustSharedAccessCount();
		if (m_nSharedAccessCount > 0)
		{
			m_lockSharedAccessComplete.Unlock();
			m_lockExclusiveAccess.Unlock();
			return false;
		}
		else
			m_bExclusiveAccess = true;
		return true;
	}

	void WUnlock()
	{
		Unlock();
	}

	void Unlock()
	{
		if (!m_bExclusiveAccess)
		{
			m_lockSharedAccessComplete.Lock();
			if (++m_nCompletedSharedAccessCount == 0)
				m_condSharedAccessComplete.NotifySingle();
			m_lockSharedAccessComplete.Unlock();
		}
		else
		{
			m_bExclusiveAccess = false;
			m_lockSharedAccessComplete.Unlock();
			m_lockExclusiveAccess.Unlock();
		}
	}
};
#endif // !defined _CRYTHREAD_HAVE_RWLOCK

//! Sync primitive for multiple reads and exclusive locking change access.
//! Useful in case if you have rarely modified object that needs
//! to be read quite often from different threads but still
//! need to be exclusively modified sometimes.
//! Debug functionality:.
//! Can be used for debug-only lock calls, which verify that no
//! simultaneous access is attempted.
//! Use the bDebug argument of LockRead or LockModify,
//! or use the DEBUG_READLOCK or DEBUG_MODIFYLOCK macros.
//! There is no overhead in release builds, if you use the macros,
//! and the lock definition is inside #ifdef _DEBUG.
class CryReadModifyLock
{
public:
	CryReadModifyLock()
		: m_modifyCount(0), m_readCount(0)
	{
		SetDebugLocked(false);
	}

	bool LockRead(bool bTry = false, cstr strDebug = 0, bool bDebug = false) const
	{
		if (!WriteLock(bTry, bDebug, strDebug))     // wait until write unlocked
			return false;
		CryInterlockedIncrement(&m_readCount);      // increment read counter
		m_writeLock.Unlock();
		return true;
	}
	void UnlockRead() const
	{
		SetDebugLocked(false);
		const int counter = CryInterlockedDecrement(&m_readCount);     // release read
		assert(counter >= 0);
		if (m_writeLock.TryLock())
			m_writeLock.Unlock();
		else if (counter == 0 && m_modifyCount)
			m_ReadReleased.Set();                     // signal the final read released
	}
	bool LockModify(bool bTry = false, cstr strDebug = 0, bool bDebug = false) const
	{
		if (!WriteLock(bTry, bDebug, strDebug))
			return false;
		CryInterlockedIncrement(&m_modifyCount);    // increment write counter (counter is for nested cases)
		while (m_readCount)
			m_ReadReleased.Wait();                    // wait for all threads finish read operation
		return true;
	}
	void UnlockModify() const
	{
		SetDebugLocked(false);
		int counter = CryInterlockedDecrement(&m_modifyCount);    // decrement write counter
		assert(counter >= 0);
		m_writeLock.Unlock();                       // release exclusive lock
	}

protected:
	mutable volatile int       m_readCount;
	mutable volatile int       m_modifyCount;
	mutable CryEvent           m_ReadReleased;
	mutable CryCriticalSection m_writeLock;
	mutable bool               m_debugLocked;
	mutable const char*        m_debugLockStr;

	void SetDebugLocked(bool b, const char* str = 0) const
	{
#ifdef _DEBUG
		m_debugLocked = b;
		m_debugLockStr = str;
#endif
	}

	bool WriteLock(bool bTry, bool bDebug, const char* strDebug) const
	{
		if (!m_writeLock.TryLock())
		{
#ifdef _DEBUG
			assert(!m_debugLocked);
			assert(!bDebug);
#endif
			if (bTry)
				return false;
			m_writeLock.Lock();
		}
#ifdef _DEBUG
		if (!m_readCount && !m_modifyCount)         // not yet locked
			SetDebugLocked(bDebug, strDebug);
#endif
		return true;
	}
};

//! Auto-locking classes.
template<class T, bool bDEBUG = false>
class AutoLockRead
{
protected:
	const T& m_lock;
public:
	AutoLockRead(const T& lock, cstr strDebug = 0)
		: m_lock(lock) { m_lock.LockRead(bDEBUG, strDebug, bDEBUG); }
	~AutoLockRead()
	{ m_lock.UnlockRead(); }
};

template<class T, bool bDEBUG = false>
class AutoLockModify
{
protected:
	const T& m_lock;
public:
	AutoLockModify(const T& lock, cstr strDebug = 0)
		: m_lock(lock) { m_lock.LockModify(bDEBUG, strDebug, bDEBUG); }
	~AutoLockModify()
	{ m_lock.UnlockModify(); }
};

#define AUTO_READLOCK(p)      PREFAST_SUPPRESS_WARNING(6246) AutoLockRead<CryReadModifyLock> __readlock ## __LINE__(p, __FUNC__)
#define AUTO_READLOCK_PROT(p) PREFAST_SUPPRESS_WARNING(6246) AutoLockRead<CryReadModifyLock> __readlock_prot ## __LINE__(p, __FUNC__)
#define AUTO_MODIFYLOCK(p)    PREFAST_SUPPRESS_WARNING(6246) AutoLockModify<CryReadModifyLock> __modifylock ## __LINE__(p, __FUNC__)

#if defined(_DEBUG)
	#define DEBUG_READLOCK(p)   AutoLockRead<CryReadModifyLock> __readlock ## __LINE__(p, __FUNC__)
	#define DEBUG_MODIFYLOCK(p) AutoLockModify<CryReadModifyLock> __modifylock ## __LINE__(p, __FUNC__)
#else
	#define DEBUG_READLOCK(p)
	#define DEBUG_MODIFYLOCK(p)
#endif

// Include all multithreading containers.
#include "MultiThread_Containers.h"
