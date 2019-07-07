// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef CRY_PLATFORM_WINAPI
#error Including CryWinStringUtils.h on a non winapi platform
#endif

#include "StringUtils.h"
#include "UnicodeFunctions.h"

#ifndef NOT_USE_CRY_STRING
#include <CryCore/Platform/CryWindows.h>

namespace CryStringUtils
{
//! Converts a string from the local Windows codepage to UTF-8.
inline string ANSIToUTF8(const char* str)
{
	int wideLen = MultiByteToWideChar(CP_ACP, 0, str, -1, 0, 0);
	wchar_t* unicode = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, -1, unicode, wideLen);
	string utf = CryStringUtils::WStrToUTF8(unicode);
	free(unicode);
	return utf;
}
} // namespace CryStringUtils

#endif