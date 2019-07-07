// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Platform/CryWindows.h>

typedef void* CRY_HWND;

#if CRY_PLATFORM_WINDOWS
//! Window message handler for Windows OS
struct IWindowMessageHandler
{
	virtual ~IWindowMessageHandler() {}
	//! The low-level pre-process message handler for Windows
	//! This is called before TranslateMessage/DispatchMessage (which will eventually end up in the HandleMessage handler)
	//! Typically, do not implement this function, unless you need to see messages before the IME can touch it
	virtual void PreprocessMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {}

	//! The low-level window message handler for Windows
	//! The return value specifies if the implementation wants to modify the message handling result
	//! When returning true, the desired result value should be written through the pResult pointer
	//! When returning false, the value stored through pResult (if any) is ignored
	//! If more than one implementation write different results, the behavior is undefined
	//! If none of the implementations write any result, the default OS result will be used instead
	//! In general, return false if the handler doesn't care about the message, or only uses it for informational purposes
	virtual bool HandleMessage(CRY_HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) { return false; }
};
#else
//! Dummy window message handler
//! This is used for platforms that don't use window message handlers
struct IWindowMessageHandler
{
};
#endif
