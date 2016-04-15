// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "IDatagramSocket.h"

class CCompositeDatagramSocket : public IDatagramSocket
{
public:
	CCompositeDatagramSocket();
	~CCompositeDatagramSocket();

	void AddChild(IDatagramSocketPtr);

	// IDatagramSocketPtr
	virtual void         GetSocketAddresses(TNetAddressVec& addrs);
	virtual ESocketError Send(const uint8* pBuffer, size_t nLength, const TNetAddress& to);
	virtual ESocketError SendVoice(const uint8* pBuffer, size_t nLength, const TNetAddress& to);
	virtual void         RegisterListener(IDatagramListener* pListener);
	virtual void         UnregisterListener(IDatagramListener* pListener);
	virtual void         Die();
	virtual bool         IsDead();
	virtual CRYSOCKET    GetSysSocket();
	virtual void         RegisterBackoffAddress(TNetAddress addr);
	virtual void         UnregisterBackoffAddress(TNetAddress addr);
	// ~IDatagramSocketPtr

private:
	typedef std::vector<IDatagramSocketPtr> ChildVec;
	typedef ChildVec::iterator              ChildVecIter;
	ChildVec m_children;
};
