#pragma once

#include <atomic>
#include <type_traits>

#if !defined(_RELEASE)

namespace DebugTrackThreadAccess
{

struct SThreadAccessInfo
{
	std::atomic<threadID> m_currentThread;
	std::atomic<int>      m_currentThreadLockCount;

	SThreadAccessInfo() : m_currentThread(0), m_currentThreadLockCount(0) {}

	void LockToCurrentThread()
	{
		threadID curThread = CryGetCurrentThreadId();
		threadID prevThread = m_currentThread.exchange(curThread);

		CRY_ASSERT((prevThread == 0 || prevThread == curThread) && m_currentThreadLockCount >= 0);
		++m_currentThreadLockCount;
	}

	void UnlockFromCurrentThread()
	{
		threadID curThread = CryGetCurrentThreadId();
		threadID prevThread = m_currentThread.load();

		CRY_ASSERT(prevThread == curThread && m_currentThreadLockCount > 0);
		if (--m_currentThreadLockCount == 0)
			m_currentThread = 0;
	}
};

template <bool b>
struct SScopedLockToThread
{
	SScopedLockToThread(const SScopedLockToThread&) = delete;
	SScopedLockToThread &operator=(const SScopedLockToThread&) = delete;

	SThreadAccessInfo* m_pAccessInfo = nullptr;

	template <typename T>
	SScopedLockToThread(const T* pObj)
		: m_pAccessInfo(pObj ? const_cast<DebugTrackThreadAccess::SThreadAccessInfo*>(&pObj->dbgThreadAccessInfo) : nullptr)
	{
		if (m_pAccessInfo)
		{
			m_pAccessInfo->LockToCurrentThread();
		}
	}

	~SScopedLockToThread()
	{
		if (m_pAccessInfo)
		{
			m_pAccessInfo->UnlockFromCurrentThread();
		}
	}
};

template <>
struct SScopedLockToThread<false>
{
	template <typename T>
	SScopedLockToThread(const T&) {}
};

	
// Until c++17 and std::void_t
template <typename... Ts> struct make_void { typedef void type; };
template <typename... Ts> using void_t = typename make_void<Ts...>::type;

template <class, class = void_t<>>
struct has_dbg_thread_info_member : std::false_type {};
template <class T>
struct has_dbg_thread_info_member<T, void_t<decltype(std::declval<T>().dbgThreadAccessInfo)>> : std::true_type {};

}

#define DBG_THREAD_ACCESS_INFO   DebugTrackThreadAccess::SThreadAccessInfo   dbgThreadAccessInfo;
#define DBG_LOCK_TO_THREAD(pObj) \
	 DebugTrackThreadAccess::SScopedLockToThread<DebugTrackThreadAccess::has_dbg_thread_info_member<std::decay<decltype(*pObj)>::type>::value> dbgThreadLock(pObj);

#else

#define DBG_THREAD_ACCESS_INFO
#define DBG_LOCK_TO_THREAD(pObj)

#endif
