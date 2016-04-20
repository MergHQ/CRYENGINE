// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include <CrySystem/ISystem.h>
#include <CrySystem/ILog.h>

// PTHREAD_MUTEX_FAST_NP is only defined by Pthreads-w32 & Pthreads-w64
#if !defined(PTHREAD_MUTEX_FAST_NP)
	#define PTHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_NORMAL
#endif

#if !defined _CRYTHREAD_HAVE_LOCK
template<class LockClass> class _PthreadCond;
template<int PthreadMutexType> class _PthreadLockBase;

template<int PthreadMutexType> class _PthreadLockAttr
{
	friend class _PthreadLockBase<PthreadMutexType>;

protected:
	_PthreadLockAttr()
	{
		pthread_mutexattr_init(&m_Attr);
		pthread_mutexattr_settype(&m_Attr, PthreadMutexType);
	}
	~_PthreadLockAttr()
	{
		pthread_mutexattr_destroy(&m_Attr);
	}
	pthread_mutexattr_t m_Attr;
};

template<int PthreadMutexType> class _PthreadLockBase
{
protected:
	static pthread_mutexattr_t& GetAttr()
	{
		static _PthreadLockAttr<PthreadMutexType> m_Attr;
		return m_Attr.m_Attr;
	}
};

template<class LockClass, int PthreadMutexType> class _PthreadLock
	: public _PthreadLockBase<PthreadMutexType>
{
	friend class _PthreadCond<LockClass>;

public:
	_PthreadLock() : m_LockCount(0)
	{
		pthread_mutex_init(
		  &m_Lock,
		  &_PthreadLockBase<PthreadMutexType>::GetAttr());
	}
	~_PthreadLock() { pthread_mutex_destroy(&m_Lock); }

	void Lock() { pthread_mutex_lock(&m_Lock); ++m_LockCount; }

	bool TryLock()
	{
		const int rc = pthread_mutex_trylock(&m_Lock);
		if (0 == rc)
		{
			++m_LockCount;
			return true;
		}
		return false;
	}

	void Unlock() { --m_LockCount; pthread_mutex_unlock(&m_Lock); }

	//! Deprecated: do not use this function - its return value might already be wrong the moment it is returned.
	bool IsLocked()
	{
		return m_LockCount > 0;
	}

private:
	pthread_mutex_t m_Lock;
	volatile int    m_LockCount;
};

	#if defined CRYLOCK_HAVE_FASTLOCK
		#if defined(_DEBUG) && defined(PTHREAD_MUTEX_ERRORCHECK_NP)
template<> class CryLockT<CRYLOCK_FAST>
	: public _PthreadLock<CryLockT<CRYLOCK_FAST>, PTHREAD_MUTEX_ERRORCHECK_NP>
		#else
template<> class CryLockT<CRYLOCK_FAST>
	: public _PthreadLock<CryLockT<CRYLOCK_FAST>, PTHREAD_MUTEX_FAST_NP>
		#endif
{
	CryLockT(const CryLockT<CRYLOCK_FAST>&);
	void operator=(const CryLockT<CRYLOCK_FAST>&);

public:
	CryLockT() {}
};
	#endif // CRYLOCK_HAVE_FASTLOCK

template<> class CryLockT<CRYLOCK_RECURSIVE>
	: public _PthreadLock<CryLockT<CRYLOCK_RECURSIVE>, PTHREAD_MUTEX_RECURSIVE>
{
	CryLockT(const CryLockT<CRYLOCK_RECURSIVE>&);
	void operator=(const CryLockT<CRYLOCK_RECURSIVE>&);

public:
	CryLockT() {}
};

	#if !CRY_PLATFORM_LINUX && !CRY_PLATFORM_ANDROID && !CRY_PLATFORM_APPLE && !CRY_PLATFORM_ORBIS
		#if defined CRYLOCK_HAVE_FASTLOCK
class CryMutex : public CryLockT<CRYLOCK_FAST>
{
};
		#else
class CryMutex : public CryLockT<CRYLOCK_RECURSIVE>
{
};
		#endif
	#endif

template<class LockClass> class _PthreadCond
{
	pthread_cond_t m_Cond;

public:
	_PthreadCond() { pthread_cond_init(&m_Cond, NULL); }
	~_PthreadCond() { pthread_cond_destroy(&m_Cond); }
	void Notify()              { pthread_cond_broadcast(&m_Cond); }
	void NotifySingle()        { pthread_cond_signal(&m_Cond); }
	void Wait(LockClass& Lock) { pthread_cond_wait(&m_Cond, &Lock.m_Lock); }
	bool TimedWait(LockClass& Lock, uint32 milliseconds)
	{
		struct timeval now;
		struct timespec timeout;
		int err;

		gettimeofday(&now, NULL);
		while (true)
		{
			timeout.tv_sec = now.tv_sec + milliseconds / 1000;
			uint64 nsec = (uint64)now.tv_usec * 1000 + (uint64)milliseconds * 1000000;
			if (nsec >= 1000000000)
			{
				timeout.tv_sec += (long)(nsec / 1000000000);
				nsec %= 1000000000;
			}
			timeout.tv_nsec = (long)nsec;
	#ifdef ORIS_SCE_THREADS
			SceKernelUseconds to;
			to = milliseconds * 1000;
			err = pthread_cond_timedwait(&m_Cond, &Lock.m_Lock, to);
			if (err == SCE_KERNEL_ERROR_EINTR)
				continue;
			else if (err == SCE_KERNEL_ERROR_ETIMEDOUT)
				return false;
	#else
			err = pthread_cond_timedwait(&m_Cond, &Lock.m_Lock, &timeout);
			if (err == EINTR)
			{
				// Interrupted by a signal.
				continue;
			}
			else if (err == ETIMEDOUT)
			{
				return false;
			}
	#endif
			else
				assert(err == 0);
			break;
		}
		return true;
	}

	//! Get the POSIX pthread_cont_t.
	//! \note This method will not be available in the Win32 port of CryThread.
	pthread_cond_t& Get_pthread_cond_t() { return m_Cond; }
};

	#if CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE || CRY_PLATFORM_ORBIS
template<class LockClass> class CryConditionVariableT : public _PthreadCond<LockClass>
{
};

		#if defined CRYLOCK_HAVE_FASTTLOCK
template<>
class CryConditionVariableT<CryLockT<CRYLOCK_FAST>> : public _PthreadCond<CryLockT<CRYLOCK_FAST>>
{
	typedef CryLockT<CRYLOCK_FAST> LockClass;
	CryConditionVariableT(const CryConditionVariableT<LockClass>&);
	CryConditionVariableT<LockClass>& operator=(const CryConditionVariableT<LockClass>&);

public:
	CryConditionVariableT() {}
};
		#endif // CRYLOCK_HAVE_FASTLOCK

template<>
class CryConditionVariableT<CryLockT<CRYLOCK_RECURSIVE>> : public _PthreadCond<CryLockT<CRYLOCK_RECURSIVE>>
{
	typedef CryLockT<CRYLOCK_RECURSIVE> LockClass;
	CryConditionVariableT(const CryConditionVariableT<LockClass>&);
	CryConditionVariableT<LockClass>& operator=(const CryConditionVariableT<LockClass>&);

public:
	CryConditionVariableT() {}
};

		#if !defined(_CRYTHREAD_CONDLOCK_GLITCH)
typedef CryConditionVariableT<CryLockT<CRYLOCK_RECURSIVE>> CryConditionVariable;
		#else
typedef CryConditionVariableT<CryLockT<CRYLOCK_FAST>>      CryConditionVariable;
		#endif

		#define _CRYTHREAD_HAVE_LOCK 1

	#else

		#if defined CRYLOCK_HAVE_FASTLOCK
template<>
class CryConditionVariable : public _PthreadCond<CryLockT<CRYLOCK_FAST>>
{
	typedef CryLockT<CRYLOCK_FAST> LockClass;
	CryConditionVariable(const CryConditionVariable&);
	CryConditionVariable& operator=(const CryConditionVariable&);

public:
	CryConditionVariable() {}
};
		#endif // CRYLOCK_HAVE_FASTLOCK

template<>
class CryConditionVariable : public _PthreadCond<CryLockT<CRYLOCK_RECURSIVE>>
{
	typedef CryLockT<CRYLOCK_RECURSIVE> LockClass;
	CryConditionVariable(const CryConditionVariable&);
	CryConditionVariable& operator=(const CryConditionVariable&);

public:
	CryConditionVariable() {}
};

		#define _CRYTHREAD_HAVE_LOCK 1

	#endif
#endif // !defined _CRYTHREAD_HAVE_LOCK

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
	sem_t m_Semaphore;
};

//////////////////////////////////////////////////////////////////////////
inline CrySemaphore::CrySemaphore(int nMaximumCount, int nInitialCount)
{
#if defined(ORIS_SCE_THREADS)
	ORBIS_TO_IMPLEMENT;
#else
	sem_init(&m_Semaphore, 0, nInitialCount);
#endif
}

//////////////////////////////////////////////////////////////////////////
inline CrySemaphore::~CrySemaphore()
{
#if defined(ORIS_SCE_THREADS)
	ORBIS_TO_IMPLEMENT;
#else
	sem_destroy(&m_Semaphore);
#endif
}

//////////////////////////////////////////////////////////////////////////
inline void CrySemaphore::Acquire()
{
#if defined(ORIS_SCE_THREADS)
	ORBIS_TO_IMPLEMENT;
#else
	sem_wait(&m_Semaphore);
#endif
}

//////////////////////////////////////////////////////////////////////////
inline void CrySemaphore::Release()
{
#if defined(ORIS_SCE_THREADS)
	ORBIS_TO_IMPLEMENT;
#else
	sem_post(&m_Semaphore);
#endif
}

//////////////////////////////////////////////////////////////////////////
//! Platform independet wrapper for a counting semaphore,
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
inline CryFastSemaphore::CryFastSemaphore(int nMaximumCount, int nInitialCount) :
	m_Semaphore(nMaximumCount),
	m_nCounter(nInitialCount)
{
}

//////////////////////////////////////////////////////////////////////////
inline CryFastSemaphore::~CryFastSemaphore()
{
}

/////////////////////////////////////////////////////////////////////////
inline void CryFastSemaphore::Acquire()
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
inline void CryFastSemaphore::Release()
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
#if !defined _CRYTHREAD_HAVE_RWLOCK
class CryRWLock
{
	pthread_rwlock_t m_Lock;

	CryRWLock(const CryRWLock&);
	CryRWLock& operator=(const CryRWLock&);

public:
	CryRWLock() { pthread_rwlock_init(&m_Lock, NULL); }
	~CryRWLock() { pthread_rwlock_destroy(&m_Lock); }
	void RLock() { pthread_rwlock_rdlock(&m_Lock); }
	bool TryRLock()
	{
	#ifdef ORIS_SCE_THREADS
		return pthread_rwlock_tryrdlock(&m_Lock) != SCE_KERNEL_ERROR_EBUSY;
	#else
		return pthread_rwlock_tryrdlock(&m_Lock) != EBUSY;
	#endif
	}
	void RUnlock() { Unlock(); }
	void WLock()   { pthread_rwlock_wrlock(&m_Lock); }
	bool TryWLock()
	{
	#ifdef ORIS_SCE_THREADS
		return pthread_rwlock_trywrlock(&m_Lock) != SCE_KERNEL_ERROR_EBUSY;
	#else
		return pthread_rwlock_trywrlock(&m_Lock) != EBUSY;
	#endif
	}
	void WUnlock() { Unlock(); }
	void Lock()    { WLock(); }
	bool TryLock() { return TryWLock(); }
	void Unlock()  { pthread_rwlock_unlock(&m_Lock); }
};

//! Indicate that this implementation header provides an implementation for CryRWLock.
	#define _CRYTHREAD_HAVE_RWLOCK 1
#endif // !defined _CRYTHREAD_HAVE_RWLOCK

//! Provide TLS implementation using pthreads for those platforms without thread.
struct SCryPthreadTLSBase
{
	SCryPthreadTLSBase(void(*pDestructor)(void*))
	{
		pthread_key_create(&m_kKey, pDestructor);
	}

	~SCryPthreadTLSBase()
	{
		pthread_key_delete(m_kKey);
	}

	void* GetSpecific()
	{
		return pthread_getspecific(m_kKey);
	}

	void SetSpecific(const void* pValue)
	{
		pthread_setspecific(m_kKey, pValue);
	}

	pthread_key_t m_kKey;
};

template<typename T, bool bDirect> struct SCryPthreadTLSImpl {};

template<typename T> struct SCryPthreadTLSImpl<T, true> : private SCryPthreadTLSBase
{
	SCryPthreadTLSImpl()
		: SCryPthreadTLSBase(NULL)
	{
	}

	T Get()
	{
		void* pSpecific(GetSpecific());
		return *reinterpret_cast<const T*>(&pSpecific);
	}

	void Set(const T& kValue)
	{
		SetSpecific(*reinterpret_cast<const void* const*>(&kValue));
	}
};

template<typename T> struct SCryPthreadTLSImpl<T, false> : private SCryPthreadTLSBase
{
	SCryPthreadTLSImpl()
		: SCryPthreadTLSBase(&Destroy)
	{
	}

	T* GetPtr()
	{
		T* pPtr(static_cast<T*>(GetSpecific()));
		if (pPtr == NULL)
		{
			pPtr = new T();
			SetSpecific(pPtr);
		}
		return pPtr;
	}

	static void Destroy(void* pPointer)
	{
		delete static_cast<T*>(pPointer);
	}

	const T& Get()
	{
		return *GetPtr();
	}

	void Set(const T& kValue)
	{
		*GetPtr() = kValue;
	}
};

template<typename T>
struct SCryPthreadTLS : SCryPthreadTLSImpl<T, sizeof(T) <= sizeof(void*)>
{
};

//! CryEvent(Timed) represent a synchronization event.
class CryEventTimed
{
public:
	ILINE CryEventTimed(){ m_flag = false; }
	ILINE ~CryEventTimed(){}

	//! Reset the event to the unsignalled state.
	void Reset();

	//! Set the event to the signalled state.
	void Set();

	//! Access a HANDLE to wait on.
	void* GetHandle() const { return NULL; };

	//! Wait indefinitely for the object to become signalled.
	void Wait();

	//! Wait, with a time limit, for the object to become signalled.
	bool Wait(const uint32 timeoutMillis);

private:
	//! Lock for synchronization of notifications.
	CryCriticalSection m_lockNotify;
#if CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE
	CryConditionVariableT<CryLockT<CRYLOCK_RECURSIVE>> m_cond;
#else
	CryConditionVariable                               m_cond;
#endif
	volatile bool m_flag;
};

typedef CryEventTimed CryEvent;
#include <CryMemory/MemoryAccess.h>

///////////////////////////////////////////////////////////////////////////////
//! Base class for lock less Producer/Consumer queue, due platforms specific they are implemeted in CryThead_platform.h.
namespace CryMT {
namespace detail {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SingleProducerSingleConsumerQueueBase
{
public:
	SingleProducerSingleConsumerQueueBase()
	{}

	void Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize);
	void Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize);
};

///////////////////////////////////////////////////////////////////////////////
inline void SingleProducerSingleConsumerQueueBase::Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize)
{
	MemoryBarrier();
	// spin if queue is full
	int iter = 0;
	while (rProducerIndex - rComsumerIndex == nBufferSize)
	{
		Sleep(iter++ > 10 ? 1 : 0);
	}

	char* pBuffer = alias_cast<char*>(arrBuffer);
	uint32 nIndex = rProducerIndex % nBufferSize;
	memcpy(pBuffer + (nIndex * nObjectSize), pObj, nObjectSize);

	MemoryBarrier();
	rProducerIndex += 1;
	MemoryBarrier();
}

///////////////////////////////////////////////////////////////////////////////
inline void SingleProducerSingleConsumerQueueBase::Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize)
{
	MemoryBarrier();
	// busy-loop if queue is empty
	int iter = 0;
	while (rProducerIndex - rComsumerIndex == 0)
	{
		Sleep(iter++ > 10 ? 1 : 0);
	}

	char* pBuffer = alias_cast<char*>(arrBuffer);
	uint32 nIndex = rComsumerIndex % nBufferSize;

	memcpy(pObj, pBuffer + (nIndex * nObjectSize), nObjectSize);

	MemoryBarrier();
	rComsumerIndex += 1;
	MemoryBarrier();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class N_ProducerSingleConsumerQueueBase
{
public:
	N_ProducerSingleConsumerQueueBase()
	{
		CryInitializeSListHead(fallbackList);
	}

	void Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32* arrStates);
	bool Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32* arrStates);

	SLockFreeSingleLinkedListHeader fallbackList;
	struct SFallbackList
	{
		SLockFreeSingleLinkedListEntry nextEntry;
		char                           alignment_padding[128 - sizeof(SLockFreeSingleLinkedListEntry)];
		char                           object[1]; // struct will be overallocated with enough memory for the object
	};
};

///////////////////////////////////////////////////////////////////////////////
inline void N_ProducerSingleConsumerQueueBase::Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32* arrStates)
{
	MemoryBarrier();
	uint32 nProducerIndex;
	uint32 nComsumerIndex;

	int iter = 0;
	do
	{
		nProducerIndex = rProducerIndex;
		nComsumerIndex = rComsumerIndex;

		if (nProducerIndex - nComsumerIndex == nBufferSize)
		{
			Sleep(iter++ > 10 ? 1 : 0);
			if (iter > 20)   // 10 spins + 10 ms wait
			{
				uint32 nSizeToAlloc = sizeof(SFallbackList) + nObjectSize - 1;
				SFallbackList* pFallbackEntry = (SFallbackList*)CryModuleMemalign(nSizeToAlloc, 128);
				memcpy(pFallbackEntry->object, pObj, nObjectSize);
				CryInterlockedPushEntrySList(fallbackList, pFallbackEntry->nextEntry);
				return;
			}
			continue;
		}

		if (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&rProducerIndex), nProducerIndex + 1, nProducerIndex) == nProducerIndex)
			break;
	}
	while (true);

	char* pBuffer = alias_cast<char*>(arrBuffer);
	uint32 nIndex = nProducerIndex % nBufferSize;

	memcpy(pBuffer + (nIndex * nObjectSize), pObj, nObjectSize);

	MemoryBarrier();
	arrStates[nIndex] = 1;
	MemoryBarrier();
}

///////////////////////////////////////////////////////////////////////////////
inline bool N_ProducerSingleConsumerQueueBase::Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32* arrStates)
{
	MemoryBarrier();
	// busy-loop if queue is empty
	int iter = 0;
	do
	{
		SFallbackList* pFallback = (SFallbackList*)CryInterlockedPopEntrySList(fallbackList);
		IF (pFallback, 0)
		{
			memcpy(pObj, pFallback->object, nObjectSize);
			CryModuleMemalignFree(pFallback);
			return true;
		}

		if (iter > 10)
			Sleep(iter > 100 ? 1 : 0);
		iter++;
	}
	while (rRunning && rProducerIndex - rComsumerIndex == 0);

	if (rRunning == 0 && rProducerIndex - rComsumerIndex == 0)
	{
		// if the queue was empty, make sure we really are empty
		SFallbackList* pFallback = (SFallbackList*)CryInterlockedPopEntrySList(fallbackList);
		IF (pFallback, 0)
		{
			memcpy(pObj, pFallback->object, nObjectSize);
			CryModuleMemalignFree(pFallback);
			return true;
		}
		return false;
	}

	iter = 0;
	while (arrStates[rComsumerIndex % nBufferSize] == 0)
	{
		Sleep(iter++ > 10 ? 1 : 0);
	}

	char* pBuffer = alias_cast<char*>(arrBuffer);
	uint32 nIndex = rComsumerIndex % nBufferSize;

	memcpy(pObj, pBuffer + (nIndex * nObjectSize), nObjectSize);

	MemoryBarrier();
	arrStates[nIndex] = 0;
	MemoryBarrier();
	rComsumerIndex += 1;
	MemoryBarrier();

	return true;
}

} // namespace detail
} // namespace CryMT
