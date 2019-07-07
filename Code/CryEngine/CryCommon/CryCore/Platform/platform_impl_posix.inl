// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <string.h>

const char* CryGetSystemErrorMessage(DWORD error)
{
	return strerror(error);
}

const char* CryGetLastSystemErrorMessage()
{
	return CryGetSystemErrorMessage(GetLastError());
}

void CryClearSytemError()
{
	SetLastError(0);
}