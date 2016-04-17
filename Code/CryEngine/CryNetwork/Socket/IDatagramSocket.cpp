// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Config.h"
#include "IDatagramSocket.h"
#include "LocalDatagramSocket.h"
#include "UDPDatagramSocket.h"
#include "CompositeDatagramSocket.h"
#include "InternetSimulatorSocket.h"

class CNullSocket : public IDatagramSocket
{
public:
	virtual void         RegisterListener(IDatagramListener* pListener)                         {}
	virtual void         UnregisterListener(IDatagramListener* pListener)                       {}
	virtual void         GetSocketAddresses(TNetAddressVec& addrs)                              { addrs.push_back(TNetAddress(SNullAddr())); }
	virtual ESocketError Send(const uint8* pBuffer, size_t nLength, const TNetAddress& to)      { return eSE_MiscFatalError; }
	virtual ESocketError SendVoice(const uint8* pBuffer, size_t nLength, const TNetAddress& to) { return eSE_MiscFatalError; }
	virtual void         Die()                                                                  {}
	virtual bool         IsDead()                                                               { return true; }
	virtual void         RegisterBackoffAddress(TNetAddress addr)                               {}
	virtual void         UnregisterBackoffAddress(TNetAddress addr)                             {}
};

void CDatagramSocket::RegisterListener(IDatagramListener* pListener)
{
	m_listeners.Add(pListener, "");
}

void CDatagramSocket::UnregisterListener(IDatagramListener* pListener)
{
	m_listeners.Remove(pListener);
}

void CDatagramSocket::OnPacket(const TNetAddress& addr, const uint8* pData, uint32 length)
{
	for (TListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnPacket(addr, pData, length);
	}
}

void CDatagramSocket::OnError(const TNetAddress& addr, ESocketError error)
{
	for (TListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnError(addr, error);
	}
}

struct SOpenSocketVisitor : public boost::static_visitor<IDatagramSocketPtr>
{
private:
	uint32 flags;

public:
	SOpenSocketVisitor(uint32 _flags)
		: flags(_flags)
	{}

	template<class T>
	IDatagramSocketPtr CreateUDP(const T& addr)
	{
		CUDPDatagramSocket* pSocket = new CUDPDatagramSocket();
		if (!pSocket->Init(addr, flags))
		{
			delete pSocket;
			return NULL;
		}
		return pSocket;
	}

	IDatagramSocketPtr Create(TLocalNetAddress addr)
	{
		CLocalDatagramSocket* pSocket = new CLocalDatagramSocket();
		if (!pSocket->Init(addr))
		{
			delete pSocket;
			return NULL;
		}
		return pSocket;
	}

	IDatagramSocketPtr Create(const LobbyIdAddr& addr)
	{
		return new CNullSocket;
	}

#if USE_SOCKADDR_STORAGE_ADDR
	IDatagramSocketPtr Create(const SSockAddrStorageAddr& addr)
	{
		return new CNullSocket;
	}
#endif

	IDatagramSocketPtr Create(const SIPv4Addr& addr)
	{
		IDatagramSocketPtr pLocal = Create(addr.port);
		if (!pLocal)
			return NULL;
		IDatagramSocketPtr pNormal = CreateUDP(addr);
		if (!pNormal)
			return NULL;

		CCompositeDatagramSocket* pSocket = new CCompositeDatagramSocket();
		pSocket->AddChild(pLocal);
		pSocket->AddChild(pNormal);
		return pSocket;
	}

	IDatagramSocketPtr Create(SNullAddr)
	{
		return new CNullSocket;
	}

	template<class T>
	IDatagramSocketPtr operator()(T& type)
	{
		return Create(type);
	}
};

IDatagramSocketPtr OpenSocket(const TNetAddress& addr, uint32 flags)
{
	SOpenSocketVisitor v(flags);
	IDatagramSocketPtr pSocket = boost::apply_visitor(v, addr);
#if INTERNET_SIMULATOR
	if (pSocket && boost::get<const SIPv4Addr>(&addr))
		pSocket = new CInternetSimulatorSocket(pSocket);
#endif

	return pSocket;
}
