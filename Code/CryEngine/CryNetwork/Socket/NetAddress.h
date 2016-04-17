// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __NETADDRESS_H__
#define __NETADDRESS_H__

#pragma once

#include <CryCore/BoostHelpers.h>
#include <CryMemory/STLGlobalAllocator.h>
#include <queue>
#include <CryLobby/ICryLobby.h>
#include <CryThreading/IThreadManager.h>
#include <CryNetwork/CrySocks.h>

#if !CRY_PLATFORM_APPLE && !CRY_PLATFORM_ORBIS && defined(__GNUC__)
	#define _SS_MAXSIZE 256
#endif

#if CRY_PLATFORM_DURANGO
	#define USE_SOCKADDR_STORAGE_ADDR 1
#else
	#define USE_SOCKADDR_STORAGE_ADDR 0
#endif

typedef uint16 TLocalNetAddress;
class CNetAddressResolver;

typedef CryFixedStringT<128> TAddressString;

#if USE_SOCKADDR_STORAGE_ADDR
struct SSockAddrStorageAddr
{
	ILINE SSockAddrStorageAddr() { memset(&addr, 0, sizeof(addr)); }
	ILINE SSockAddrStorageAddr(sockaddr_storage addr) { this->addr = addr; }
	sockaddr_storage addr;
	ILINE bool operator<(const SSockAddrStorageAddr& rhs) const
	{
		if (addr.ss_family == rhs.addr.ss_family)
		{
			switch (addr.ss_family)
			{
			case AF_INET6:
				{
					sockaddr_in6* pLHS = (sockaddr_in6*)&addr;
					sockaddr_in6* pRHS = (sockaddr_in6*)&rhs.addr;
					int addrcmp = memcmp(&pLHS->sin6_addr, &pRHS->sin6_addr, sizeof(pLHS->sin6_addr));

					return (addrcmp < 0) || ((addrcmp == 0) && (pLHS->sin6_port < pRHS->sin6_port));
				}

			default:
				CryFatalError("SSockAddrStorageAddr < Unknown address family %u", addr.ss_family);
				return false;
			}
		}
		else
		{
			CryFatalError("SSockAddrStorageAddr < Mismatched address families %u %u", addr.ss_family, rhs.addr.ss_family);
			return false;
		}
	}
	ILINE bool operator==(const SSockAddrStorageAddr& rhs) const
	{
		if (addr.ss_family == rhs.addr.ss_family)
		{
			switch (addr.ss_family)
			{
			case AF_INET6:
				{
					sockaddr_in6* pLHS = (sockaddr_in6*)&addr;
					sockaddr_in6* pRHS = (sockaddr_in6*)&rhs.addr;
					int addrcmp = memcmp(&pLHS->sin6_addr, &pRHS->sin6_addr, sizeof(pLHS->sin6_addr));
					return (addrcmp == 0) && (pLHS->sin6_port == pRHS->sin6_port);
				}

			default:
				CryFatalError("SSockAddrStorageAddr == Unknown address family %u", addr.ss_family);
				return false;
			}
		}
		else
		{
			CryFatalError("SSockAddrStorageAddr == Mismatched address families %u %u", addr.ss_family, rhs.addr.ss_family);
			return false;
		}
	}

	void GetMemoryUsage(ICrySizer* pSizer) const {}
};
#endif

struct LobbyIdAddr
{
	ILINE LobbyIdAddr(uint64 id) { this->id = id; }
	uint64 id;
	ILINE bool operator<(const LobbyIdAddr& rhs) const
	{
		return id < rhs.id;
	}

	ILINE bool operator==(const LobbyIdAddr& rhs) const
	{
		return id == rhs.id;
	}

	void GetMemoryUsage(ICrySizer* pSizer) const {}
};

struct SIPv4Addr
{
	ILINE SIPv4Addr() : addr(0), port(0), lobbyService(eCLS_NumServices) {}
	ILINE SIPv4Addr(uint32 addr, uint16 port) { this->addr = addr; this->port = port; this->lobbyService = eCLS_NumServices; }
	ILINE SIPv4Addr(uint32 addr, uint16 port, ECryLobbyService lobbyService) { this->addr = addr; this->port = port; this->lobbyService = lobbyService; }

	uint32 addr;
	uint16 port;

	// Can be used to specify which Lobby Socket Service to use to send lobby packets with if it differs from the current service.
	uint8 lobbyService;

	ILINE bool operator<(const SIPv4Addr& rhs) const
	{
		return addr < rhs.addr || (addr == rhs.addr && port < rhs.port);
	}

	ILINE bool operator==(const SIPv4Addr& rhs) const
	{
		return addr == rhs.addr && port == rhs.port;
	}

	void GetMemoryUsage(ICrySizer* pSizer) const {}
};

struct SNullAddr
{
	ILINE bool operator<(const SNullAddr& rhs) const
	{
		return false;
	}

	ILINE bool operator==(const SNullAddr& rhs) const
	{
		return true;
	}

	void GetMemoryUsage(ICrySizer* pSizer) const {}
};

typedef  boost::mpl::vector<
    SNullAddr, TLocalNetAddress, SIPv4Addr, LobbyIdAddr
#if USE_SOCKADDR_STORAGE_ADDR
    , SSockAddrStorageAddr
#endif
    > TNetAddressTypes;

typedef boost::make_variant_over<TNetAddressTypes>::type TNetAddress;

typedef DynArray<TNetAddress>                            TNetAddressVec;

struct IResolverRequest : public CMultiThreadRefCount
{
protected:
	IResolverRequest()
	{
		++g_objcnt.nameResolutionRequest;
	}
	~IResolverRequest()
	{
		--g_objcnt.nameResolutionRequest;
	}

private:
	friend class CNetAddressResolver;

	virtual void Execute(CNetAddressResolver* pR) = 0;
	virtual void Fail() = 0;
};

enum ENameRequestResult
{
	eNRR_Pending,
	eNRR_Succeeded,
	eNRR_Failed
};

typedef CryMutex TNameRequestLock;

class CNameRequest : public IResolverRequest
{
public:
	~CNameRequest();

	virtual ENameRequestResult GetResult(TNetAddressVec& res);
	virtual ENameRequestResult GetResult();
	virtual TNetAddressVec& GetAddrs();
	string                  GetAddrString();
	virtual void            Wait();
	bool                    TimedWait(float seconds);

private:
	friend class CNetAddressResolver;

	CNameRequest(const string& name, TNameRequestLock& parentLock, CryConditionVariable& parentCond);

	void Execute(CNetAddressResolver* pR);
	void Fail();

	typedef TNameRequestLock TLock;
	TLock&                      m_parentLock;
	CryConditionVariable&       m_parentCond;
	TNetAddressVec              m_addrs;
	TAddressString              m_str;
	volatile ENameRequestResult m_state;
};

class CToStringRequest : public IResolverRequest
{
public:
	~CToStringRequest();

	ENameRequestResult GetResult(TAddressString& res);
	ENameRequestResult GetResult();
	void               Wait();
	bool               TimedWait(float seconds);

private:
	friend class CNetAddressResolver;

	CToStringRequest(const TNetAddress& addr, TNameRequestLock& parentLock, CryConditionVariable& parentCond);

	void Execute(CNetAddressResolver* pR);
	void Fail();

	typedef TNameRequestLock TLock;
	TLock&                      m_parentLock;
	CryConditionVariable&       m_parentCond;
	TNetAddress                 m_addr;
	TAddressString              m_str;
	volatile ENameRequestResult m_state;
};

typedef _smart_ptr<CNameRequest>                                                                                                               CNameRequestPtr;

typedef std::multimap<TAddressString, TNetAddress, std::less<TAddressString>, stl::STLGlobalAllocator<std::pair<TAddressString, TNetAddress>>> TNameToAddressCache;
typedef std::map<TNetAddress, TAddressString, std::less<TNetAddress>, stl::STLGlobalAllocator<std::pair<TNetAddress, TAddressString>>>         TAddressToNameCache;

class CNetAddressResolver : public IThread
{
public:
	CNetAddressResolver();
	~CNetAddressResolver();

	// Start accepting work on thread
	virtual void ThreadEntry();

	// Signals the thread that it should not accept anymore work and exit
	void                    SignalStopWork();

	TAddressString          ToString(const TNetAddress& addr, float timeout = 0.01f);
	virtual TAddressString  ToNumericString(const TNetAddress& addr);
	virtual CNameRequestPtr RequestNameLookup(const string& name);

	bool                    IsPrivateAddr(const TNetAddress& addr);

private:
	friend class CNameRequest;
	friend class CToStringRequest;

	TNameToAddressCache m_nameToAddressCache;
	TAddressToNameCache m_addressToNameCache;

	bool PopulateCacheFor(const TAddressString& str, TNetAddressVec& addrs);
	bool PopulateCacheFor(const TNetAddress& addr, TAddressString& str);
	bool SlowLookupName(TAddressString str, TNetAddressVec& addrs);

	typedef TNameRequestLock             TLock;
	typedef _smart_ptr<IResolverRequest> TRequestPtr;
	TLock                m_lock;
	CryConditionVariable m_cond;
	CryConditionVariable m_condOut;
	volatile bool        m_die;
	typedef std::queue<TRequestPtr, std::deque<TRequestPtr, stl::STLGlobalAllocator<TRequestPtr>>> TRequestQueue;
	TRequestQueue        m_requests;
};

bool        ConvertAddr(const TNetAddress& addrIn, CRYSOCKADDR_IN* pSockAddr);
bool        ConvertAddr(const TNetAddress& addrIn, CRYSOCKADDR* pSockAddr, int* addrLen);
TNetAddress ConvertAddr(const CRYSOCKADDR* pSockAddr, int addrLength);

#endif
