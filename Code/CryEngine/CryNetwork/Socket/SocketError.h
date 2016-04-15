// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SOCKETERROR_H__
#define __SOCKETERROR_H__

#pragma once

enum ESocketError
{
	eSE_Ok = 0,
	eSE_Cancelled,
	eSE_FragmentationOccured,
	eSE_BufferTooSmall,
	eSE_ZeroLengthPacket,
	eSE_ConnectInProgress,
	eSE_AlreadyConnected,
	eSE_MiscNonFatalError,

	eSE_Unrecoverable = 0x8000,
	eSE_UnreachableAddress,

	eSE_SocketClosed,
	eSE_ConnectionRefused,
	eSE_ConnectionReset,
	eSE_ConnectionTimedout,

	eSE_MiscFatalError
};

ESocketError OSErrorToSocketError(uint32 oserr);

#endif
