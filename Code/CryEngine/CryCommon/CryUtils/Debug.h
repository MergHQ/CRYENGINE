// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <CryString/CryFixedString.h>

namespace Cry {
namespace Utils {

// TODO: We should move this into a VisualStudio namespace
// Opens specified file in current Visual Studio instance (szFilename is expected to be absolute).
inline void OpenSourceFile(const char* szFilename)
{
	// TODO: Does this even make sense on other systems?
#if CRY_PLATFORM_WINDOWS
	if (szFilename)
	{
		// Opening directories somehow stalls VS2017, therefore make sure we have a file.
		size_t strLen = strlen(szFilename);
		if (strLen > 4 && szFilename[strLen - 4] == '.')
		{
			// TODO: Make this dynamic for different VS versions.
			// Check which devenv.exe is available (prefer newer).
			const char* szVS2015 = "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\Common7\\IDE\\devenv.exe";
			const char* szVS2017 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\Common7\\IDE\\devenv.exe";

			const char* szDevenvPath = szVS2017;
			struct stat desc;
			if (stat(szDevenvPath, &desc) != 0)
			{
				szDevenvPath = szVS2015;
				if (stat(szDevenvPath, &desc) != 0)
				{
					CRY_ASSERT(false, "Couldn't find devenv.exe");
					return;
				}
			}
			// ~TODO

			stack_string command(szDevenvPath);
			command.append(" /Edit \"");
			// TODO: Make sure that this is a valid (and sane) path/filename.
			command.append(szFilename);
			// ~TODO
			command.append("\"");

			STARTUPINFO startupInfo = {};
			startupInfo.dwFlags = STARTF_USESHOWWINDOW;
			startupInfo.wShowWindow = SW_HIDE;

			PROCESS_INFORMATION processInformation = {};
			if (CreateProcess(NULL, command.m_str, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation))
			{
				CloseHandle(processInformation.hThread);
				CloseHandle(processInformation.hProcess);
			}
		}
		return;
	}
	CRY_ASSERT(szFilename, "Can't view source file.");
#else
	CRY_ASSERT(false, "View source is only supported on Windows right now.");
#endif
}
// ~TODO

template<bool HASH_IN_LOWERCASE>
struct SStringHash
{
	size_t operator()(const char* szData) const
	{
		return (HASH_IN_LOWERCASE ? CCrc32::ComputeLowercase(szData) : CCrc32::Compute(szData));
	}

	size_t operator()(const string& data) const
	{
		return (HASH_IN_LOWERCASE ? CCrc32::ComputeLowercase(data.c_str()) : CCrc32::Compute(data.c_str()));
	}
};

} // ~Utils namespace
} // ~Cry namespace
