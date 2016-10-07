// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "resource.h"

// Insert your headers here
#include <CryCore/Platform/CryWindows.h>
#include <ShellAPI.h> // requires <windows.h>

// We need shell api for Current Root Extrection.
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")

#include <CryCore/Platform/CryLibrary.h>
#include <CryGame/IGameFramework.h>

#include <CrySystem/IConsole.h>
#include <CrySystem/File/ICryPak.h>

#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/Profilers/FrameProfiler/FrameProfiler_impl.h>
#include <CryString/StringUtils.h>

#include <CrySystem/ParseEngineConfig.h>

// Advise notebook graphics drivers to prefer discrete GPU when no explicit application profile exists
extern "C"
{
	// nVidia
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	// AMD
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#ifdef _LIB
extern "C" IGameFramework* CreateGameFramework();
#endif //_LIB

#ifdef _LIB
// Include common type defines for static linking
// Manually instantiate templates as needed here.
	#include <CryCore/Common_TypeInfo.h>
STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <float> )
STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <int> )
STRUCT_INFO_T_INSTANTIATE(Vec4_tpl, <short> )
STRUCT_INFO_T_INSTANTIATE(Vec3_tpl, <int> )
STRUCT_INFO_T_INSTANTIATE(Ang3_tpl, <float> )
STRUCT_INFO_T_INSTANTIATE(Quat_tpl, <float> )
STRUCT_INFO_T_INSTANTIATE(Plane_tpl, <float> )
STRUCT_INFO_T_INSTANTIATE(Matrix33_tpl, <float> )
STRUCT_INFO_T_INSTANTIATE(Color_tpl, <float> )
STRUCT_INFO_T_INSTANTIATE(Color_tpl, <uint8> )
#endif

#define CRY_LIVE_CREATE_MUTEX_ENABLE 0

#if CRY_LIVE_CREATE_MUTEX_ENABLE
class CCryLiveCreateMutex
{
private:
	HANDLE m_hMutex;

public:
	CCryLiveCreateMutex(bool bCreateMutex)
		: m_hMutex(NULL)
	{
		if (bCreateMutex)
		{
			// create PC only mutex that signals that the game is running
			// this is used by LiveCreate to make sure only one instance of game is running
			const char* szMutexName = "Global\\CryLiveCreatePCMutex";
			m_hMutex = ::CreateMutexA(NULL, FALSE, szMutexName);
			if (NULL == m_hMutex)
			{
				// mutex may already exist, then just open it
				m_hMutex = ::OpenMutexA(MUTEX_ALL_ACCESS, FALSE, szMutexName);
			}
		}
	}

	~CCryLiveCreateMutex()
	{
		if (NULL != m_hMutex)
		{
			::CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}
	}
};
#endif

int RunGame(const char* commandLine)
{
	SSystemInitParams startupParams;
	const char* frameworkDLLName = "CryAction.dll";

	CryFindRootFolderAndSetAsCurrentWorkingDirectory();

	//restart parameters
	static const size_t MAX_RESTART_LEVEL_NAME = 256;
	char fileName[MAX_RESTART_LEVEL_NAME];
	cry_strcpy(fileName, "");
	static const char logFileName[] = "Game.log";

#if CRY_LIVE_CREATE_MUTEX_ENABLE
	const bool bCreateMutex = (NULL == strstr(commandLine, "-nomutex"));
	CCryLiveCreateMutex liveCreateMutex(bCreateMutex);
#endif

	HMODULE frameworkDll = 0;

#if !defined(_LIB)
	// load the framework dll
	frameworkDll = CryLoadLibrary(frameworkDLLName);

	if (!frameworkDll)
	{
		string errorStr;
		errorStr.Format("Failed to load the Game DLL! %s", frameworkDLLName);

		MessageBox(0, errorStr.c_str(), "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);
		// failed to load the dll

		return 0;
	}

	// get address of startup function
	IGameFramework::TEntryFunction CreateGameFramework = (IGameFramework::TEntryFunction)CryGetProcAddress(frameworkDll, "CreateGameFramework");

	if (!CreateGameFramework)
	{
		// dll is not a compatible game dll
		CryFreeLibrary(frameworkDll);

		MessageBox(0, "Specified Game DLL is not valid! Please make sure you are running the correct executable", "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		return 0;
	}
#endif //!defined(_LIB)

	//SSystemInitParams startupParams;

	startupParams.sLogFileName = logFileName;
	cry_strcpy(startupParams.szSystemCmdLine, commandLine);
	//startupParams.pProtectedFunctions[0] = &TestProtectedFunction;

	// create the startup interface
	IGameFramework* pFramework = CreateGameFramework();
	if (!pFramework)
	{
		// failed to create the startup interface
		CryFreeLibrary(frameworkDll);

		const char* noPromptArg = strstr(commandLine, "-noprompt");
		if (noPromptArg == NULL)
			MessageBox(0, "Failed to create the GameStartup Interface!", "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		return 0;
	}

	bool oaRun = false;

	if (strstr(commandLine, "-norandom"))
		startupParams.bNoRandom = 1;

	// main game loop
	pFramework->StartEngine(startupParams);

	// The main engine loop has exited at this point, shut down
	pFramework->ShutdownEngine();

	// Unload the framework dll
	CryFreeLibrary(frameworkDll);

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Support relaunching for windows media center edition.
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_WINDOWS
bool ReLaunchMediaCenter()
{
	// Skip if not running on a Media Center
	if (GetSystemMetrics(SM_MEDIACENTER) == 0)
		return false;

	// Get the path to Media Center
	char szExpandedPath[MAX_PATH];
	if (!ExpandEnvironmentStrings("%SystemRoot%\\ehome\\ehshell.exe", szExpandedPath, MAX_PATH))
		return false;

	// Skip if ehshell.exe doesn't exist
	if (GetFileAttributes(szExpandedPath) == 0xFFFFFFFF)
		return false;

	// Launch ehshell.exe
	INT_PTR result = (INT_PTR)ShellExecute(NULL, TEXT("open"), szExpandedPath, NULL, NULL, SW_SHOWNORMAL);
	return (result > 32);
}
#endif // CRY_PLATFORM_WINDOWS
///////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// we need pass the full command line, including the filename
	// lpCmdLine does not contain the filename.
#if CAPTURE_REPLAY_LOG
	#ifndef _LIB
		CryLoadLibrary("CrySystem.dll");
	#endif
	CryGetIMemReplay()->StartOnCommandLine(lpCmdLine);
#endif

	//check for a restart
	const char* pos = strstr(lpCmdLine, "restart");
	if (pos != NULL)
	{
		Sleep(5000);               //wait for old instance to be deleted
		return RunGame(lpCmdLine); //pass the restart level if restarting
	}
	else
		pos = strstr(lpCmdLine, " -load ");// commandLine.find("load");

	if (pos != NULL)
		RunGame(lpCmdLine);

	int nRes = RunGame(GetCommandLineA());

	//////////////////////////////////////////////////////////////////////////
	// Support relaunching for windows media center edition.
	//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_WINDOWS
	if (strstr(lpCmdLine, "ReLaunchMediaCenter") != 0)
	{
		ReLaunchMediaCenter();
	}
#endif
	//////////////////////////////////////////////////////////////////////////

	return nRes;
}
