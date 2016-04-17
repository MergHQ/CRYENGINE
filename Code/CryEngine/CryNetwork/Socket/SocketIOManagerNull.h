// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "ISocketIOManager.h"

class CSocketIOManagerNull : public ISocketIOManager
{
public:
	CSocketIOManagerNull() : ISocketIOManager(0) {}

	virtual bool        PollWait(uint32 waitTime)                                                                     { return false; }
	virtual int         PollWork(bool& performedWork)                                                                 { performedWork = false; return eSM_COMPLETEDIO; }
	virtual const char* GetName()                                                                                     { return "null"; }

	virtual SSocketID   RegisterSocket(CRYSOCKET sock, int protocol)                                                  { return SSocketID(1, 2); }
	virtual void        SetRecvFromTarget(SSocketID sockid, IRecvFromTarget* pTarget)                                 {}
	virtual void        SetConnectTarget(SSocketID sockid, IConnectTarget* pTarget)                                   {}
	virtual void        SetSendToTarget(SSocketID sockid, ISendToTarget* pTarget)                                     {}
	virtual void        SetAcceptTarget(SSocketID sockid, IAcceptTarget* pTarget)                                     {}
	virtual void        SetRecvTarget(SSocketID sockid, IRecvTarget* pTarget)                                         {}
	virtual void        SetSendTarget(SSocketID sockid, ISendTarget* pTarget)                                         {}
	virtual void        RegisterBackoffAddressForSocket(TNetAddress addr, SSocketID sockid)                           {}
	virtual void        UnregisterBackoffAddressForSocket(TNetAddress addr, SSocketID sockid)                         {}
	virtual void        UnregisterSocket(SSocketID sockid)                                                            {}

	virtual bool        RequestRecvFrom(SSocketID sockid)                                                             { return false; }
	virtual bool        RequestSendTo(SSocketID sockid, const TNetAddress& addr, const uint8* pData, size_t len)      { return false; }
	virtual bool        RequestSendVoiceTo(SSocketID sockid, const TNetAddress& addr, const uint8* pData, size_t len) { return false; }

	virtual bool        RequestConnect(SSocketID sockid, const TNetAddress& addr)                                     { return false; }
	virtual bool        RequestAccept(SSocketID sock)                                                                 { return false; }
	virtual bool        RequestSend(SSocketID sockid, const uint8* pData, size_t len)                                 { return false; }
	virtual bool        RequestRecv(SSocketID sockid)                                                                 { return false; }

	virtual void        PushUserMessage(int msg)                                                                      {}

	virtual bool        HasPendingData()                                                                              { return false; }

#if LOCK_NETWORK_FREQUENCY
	virtual void ForceNetworkStart() {}
	virtual bool NetworkSleep()      { return true; }
#endif

	virtual IDatagramSocketPtr CreateDatagramSocket(const TNetAddress& addr, uint32 flags) { return NULL; }
	virtual void               FreeDatagramSocket(IDatagramSocketPtr pSocket)              {}
};
