// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <Python.h>
#include <shlwapi.h>

int main(int argc, wchar_t** pArgv)
{
	WCHAR filename[FILENAME_MAX];
	
	int filnameLength = GetModuleFileNameW(NULL, filename, FILENAME_MAX);
	if (!filnameLength)
	{
		return -1;
	}

	PathRemoveFileSpecW(filename);
	wcscat(filename, L"\\..\\..\\Editor\\Python\\windows");
	Py_SetPythonHome(filename);
	return Py_Main(argc, pArgv);
}
