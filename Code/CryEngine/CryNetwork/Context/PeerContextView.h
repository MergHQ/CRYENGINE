// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description:  Peer implementation of a context view
   -------------------------------------------------------------------------
   History:
   - 05/11/2009   : Created by Andrew Catlender
*************************************************************************/
#ifndef __PEERCONTEXTVIEW_H__
#define __PEERCONTEXTVIEW_H__

#pragma once

#include "ContextView.h"

// implements CContextView in a way that acts like a peer
class CPeerContextView :
	public CNetMessageSinkHelper<CPeerContextView, CContextView, 5>
{
public:
	CPeerContextView(CNetChannel* pNetChannel, CNetContext* pNetContext);

	virtual bool        IsPeer() const                                                                { return true; }

	virtual void        AddWaitForFileSyncComplete(IContextEstablisher* pEst, EContextViewState when) {}

	virtual void        BindObject(SNetObjectID nID)                                                  { NET_ASSERT(false); } // Shouldn't ever be called on a peer channel
	virtual void        UnbindObject(SNetObjectID nID)                                                { NET_ASSERT(false); } // Shouldn't ever be called on a peer channel
	virtual void        EstablishedContext()                                                          {}
	virtual const char* ValidateMessage(const SNetMessageDef* pMsg, bool bNetworkMsg)                 { return NULL; }
	virtual bool        HasRemoteDef(const SNetMessageDef* pDef)                                      { return ClassHasDef(pDef); } // Peer channels don't have a logical opposite...
	virtual void        GetMemoryStatistics(ICrySizer* pSizer)
	{
		SIZER_COMPONENT_NAME(pSizer, "CPeerContextView");

		pSizer->Add(*this);
		CContextView::GetMemoryStatistics(pSizer);
	}

	// INetMessageSink
	void DefineProtocol(IProtocolBuilder* pBuilder) {}

#if ENABLE_DEBUG_KIT
	virtual TNetChannelID GetLoggingChannelID(TNetChannelID local, TNetChannelID remote) { return -(int)remote; }
#endif

private:
	virtual bool        ShouldInitContext() { return false; }
	virtual const char* DebugString()       { return "PEER"; }
	virtual void        OnWitnessDeclared() {}
};

#endif // __PEERCONTEXTVIEW_H__
