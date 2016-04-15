// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Network.h"
#include "TCPStreamSocket.h"

struct SCreateStreamSocketVisitor : public boost::static_visitor<void>
{
	IStreamSocketPtr pStreamSocket;

	template<class T>
	void operator()(T& type)
	{
		Visit(type);
	}

	template<typename T>
	ILINE void Visit(const T&)
	{
		NetWarning("unsupported address type for CreateStreamSocket");
	}

	void Visit(const SIPv4Addr&)
	{
		CTCPStreamSocket* pSocket = new CTCPStreamSocket();
		if (!pSocket->Init())
		{
			delete pSocket;
			pSocket = NULL;
		}
		pStreamSocket = pSocket;
	}
};

IStreamSocketPtr CreateStreamSocket(const TNetAddress& addr)
{
	SCreateStreamSocketVisitor visitor;
	boost::apply_visitor(visitor, addr);
	return visitor.pStreamSocket;
}
