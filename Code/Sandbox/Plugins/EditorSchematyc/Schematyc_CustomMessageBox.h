// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Schematyc
{
	class CCustomMessageBox
	{
	public:

		static int Execute(const char* szMessage, const char* szTitle, UINT uType);
		static int Execute(CPoint pos, const char* szMessage, const char* szTitle, UINT uType);

	private:

		static LRESULT CALLBACK MessageCallback(int nCode, WPARAM wParam, LPARAM lParam);

	private:

		static CPoint ms_pos;
		static HHOOK  ms_hMessageHook;
	};
}