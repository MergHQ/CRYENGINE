// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IWindowMessageHandler;

struct IImeManager
{
	virtual ~IImeManager() {}
	// Check if IME is supported
	virtual bool IsImeSupported() = 0;

	// This is called by Scaleform in the case that IME support is compiled in
	// Returns false if IME should not be used
	virtual bool SetScaleformHandler(IWindowMessageHandler* pHandler) = 0;
};
