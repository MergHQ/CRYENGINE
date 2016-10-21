// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_CustomMessageBox.h"

namespace Schematyc
{
	CPoint CCustomMessageBox::ms_pos         = CPoint();
	HHOOK CCustomMessageBox::ms_hMessageHook = NULL;

	int CCustomMessageBox::Execute(const char* szMessage, const char* szTitle, UINT uType)
	{
		CPoint pos;
		GetCursorPos(&pos);

		const float defaultOffset = 20;
		pos.x -= static_cast<LONG>(defaultOffset);
		pos.y -= static_cast<LONG>(defaultOffset);
		
		return Execute(pos, szMessage, szTitle, uType);
	}

	int CCustomMessageBox::Execute(CPoint pos, const char* szMessage, const char* szTitle, UINT uType)
	{
		ms_pos          = pos;
		ms_hMessageHook = ::SetWindowsHookEx(WH_CALLWNDPROCRET, MessageCallback, 0, GetCurrentThreadId());

		const int result = ::MessageBox(NULL, szMessage, szTitle, uType);

		UnhookWindowsHookEx(ms_hMessageHook);
		ms_hMessageHook = NULL;

		return result;
	}

	LRESULT CALLBACK CCustomMessageBox::MessageCallback(int nCode, WPARAM wParam, LPARAM lParam)
	{
		const LRESULT result = ::CallNextHookEx(ms_hMessageHook, nCode, wParam, lParam);
		if(nCode == HC_ACTION)
		{
			CWPRETSTRUCT *pMsg = (CWPRETSTRUCT*)lParam;
			if(pMsg->message == WM_INITDIALOG)
			{
				RECT rect;
				::GetWindowRect(pMsg->hwnd, &rect);
				::MoveWindow(pMsg->hwnd, ms_pos.x, ms_pos.y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
			}
		}
		return result;
	}
}