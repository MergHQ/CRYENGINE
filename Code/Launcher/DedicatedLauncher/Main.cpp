// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryCore/Platform/platform_impl.inl>
#include "resource.h"
#include <CryCore/Platform/CryLibrary.h>
#include <CryGame/IGameStartup.h>
#include <CrySystem/IConsole.h>

#include <CryCore/Platform/CryWindows.h>
#include <ShellAPI.h> // requires <windows.h>

#include <CrySystem/ParseEngineConfig.h>

// We need shell api for Current Root Extruction.
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")

static unsigned key[4] = {1339822019,3471820962,4179589276,4119647811};
static unsigned text[4] = {4114048726,1217549643,1454516917,859556405};
static unsigned hash[4] = {324609294,3710280652,1292597317,513556273};

#ifdef _LIB
extern "C" IGameStartup* CreateGameStartup();
#endif

// src and trg can be the same pointer (in place encryption)
// len must be in bytes and must be multiple of 8 byts (64bits).
// key is 128bit:  int key[4] = {n1,n2,n3,n4};
// void encipher(unsigned int *const v,unsigned int *const w,const unsigned int *const k )
#define TEA_ENCODE( src,trg,len,key ) {\
	unsigned int *v = (src), *w = (trg), *k = (key), nlen = (len) >> 3; \
	unsigned int delta=0x9E3779B9,a=k[0],b=k[1],c=k[2],d=k[3]; \
	while (nlen--) {\
	unsigned int y=v[0],z=v[1],n=32,sum=0; \
	while(n-->0) { sum += delta; y += (z << 4)+a ^ z+sum ^ (z >> 5)+b; z += (y << 4)+c ^ y+sum ^ (y >> 5)+d; } \
	w[0]=y; w[1]=z; v+=2,w+=2; }}

// src and trg can be the same pointer (in place decryption)
// len must be in bytes and must be multiple of 8 byts (64bits).
// key is 128bit: int key[4] = {n1,n2,n3,n4};
// void decipher(unsigned int *const v,unsigned int *const w,const unsigned int *const k)
#define TEA_DECODE( src,trg,len,key ) {\
	unsigned int *v = (src), *w = (trg), *k = (key), nlen = (len) >> 3; \
	unsigned int delta=0x9E3779B9,a=k[0],b=k[1],c=k[2],d=k[3]; \
	while (nlen--) { \
	unsigned int y=v[0],z=v[1],sum=0xC6EF3720,n=32; \
	while(n-->0) { z -= (y << 4)+c ^ y+sum ^ (y >> 5)+d; y -= (z << 4)+a ^ z+sum ^ (z >> 5)+b; sum -= delta; } \
	w[0]=y; w[1]=z; v+=2,w+=2; }}

ILINE unsigned Hash( unsigned a )
{
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

// encode size ignore last 3 bits of size in bytes. (encode by 8bytes min)
#define TEA_GETSIZE( len ) ((len) & (~7))

static const LPWSTR GetProjectRootArgument(int argc, const LPWSTR argv[])
{
	for (int i = 1; i < argc - 1; i++)
	{
		if (wcscmp(argv[i], L"-projectroot") == 0)
			return argv[i + 1];
	}

	return nullptr;
}

static const LPWSTR GetProjectDllDirArgument(int argc, const LPWSTR argv[])
{
	for (int i = 1; i < argc - 1; i++)
	{
		if (wcscmp(argv[i], L"-projectdlldir") == 0)
			return argv[i + 1];
	}

	return nullptr;
}

ILINE int RunGame(const char *commandLine)
{
	SSystemInitParams startupParams;
	string gameDLLName;

	// set game project related system changes
	{
		int argc = 0;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		const LPWSTR sProjectRoot = GetProjectRootArgument(argc, argv);
		if (sProjectRoot)
		{
			SetCurrentDirectoryW(sProjectRoot);

			const LPWSTR sProjectDllDir = GetProjectDllDirArgument(argc, argv);
			if (sProjectDllDir && sProjectDllDir[0])
			{
				SetDllDirectoryW(sProjectDllDir);
				WideCharToMultiByte(CP_ACP, 0, sProjectDllDir, -1, startupParams.szProjectDllDir, CRY_ARRAY_COUNT(startupParams.szProjectDllDir), NULL, NULL);
			}

			CProjectConfig projectCfg;
			gameDLLName = projectCfg.m_gameDLL;
		}
		else
		{
			// default to old game folder behavior
			CryFindRootFolderAndSetAsCurrentWorkingDirectory();
			CEngineConfig engineCfg;
			gameDLLName = engineCfg.m_gameDLL;
		}
		LocalFree(argv);
	}
	
	//restart parameters
	static const char logFileName[] = "Server.log";

	unsigned buf[4];
	TEA_DECODE((unsigned*)text,buf,16,(unsigned*)key);

	HMODULE gameDll = 0;

#if !defined(_LIB) || defined(IS_EAAS)
	// load the game dll
	gameDll = CryLoadLibrary( gameDLLName.c_str() );

	if (!gameDll)
	{
		// failed to load the dll
		string errorStr;
		errorStr.Format("Failed to load the Game DLL! %s", gameDLLName.c_str());

		MessageBox(0, errorStr.c_str(), "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		return 0;
	}
	// get address of startup function
	IGameStartup::TEntryFunction CreateGameStartup = (IGameStartup::TEntryFunction)CryGetProcAddress(gameDll, "CreateGameStartup");

	if (!CreateGameStartup)
	{
		// dll is not a compatible game dll
		CryFreeLibrary(gameDll);

		MessageBox(0, "Specified Game DLL is not valid!", "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		return 0;
	}
#endif //!defined(_LIB) || defined(IS_EAAS)

	strcat((char*)commandLine, (char*)buf);	

	startupParams.hInstance = GetModuleHandle(0);
	startupParams.sLogFileName = logFileName;
	strcpy(startupParams.szSystemCmdLine, commandLine);

	for (int i=0; i<4; i++)
		if (Hash(buf[i])!=hash[i])
			return 1;

	// create the startup interface
	IGameStartup *pGameStartup = CreateGameStartup();

	if (!pGameStartup)
	{
		// failed to create the startup interface
		CryFreeLibrary(gameDll);

		MessageBox(0, "Failed to create the GameStartup Interface!", "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		return 0;
	}

	// run the game
	if (pGameStartup->Init(startupParams))
	{
		pGameStartup->Run(NULL);

		pGameStartup->Shutdown();
		pGameStartup = 0;

		CryFreeLibrary(gameDll);
	}
	else
	{
		MessageBox(0, "Failed to initialize the GameStartup Interface!", "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);

		// if initialization failed, we still need to call shutdown
		pGameStartup->Shutdown();
		pGameStartup = 0;

		CryFreeLibrary(gameDll);

		return 0;
	}

	return 0;
}


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

	char cmdLine[2048];
	cry_strcpy(cmdLine, GetCommandLineA());

/*
	unsigned buf[4];
                 //  0123456789abcdef
	char secret[16] = "  -dedicated   ";
	TEA_ENCODE((unsigned int*)secret, (unsigned int*)buf, 16, key);
	for (int i=0; i<4; i++)
		hash[i] = Hash(((unsigned*)secret)[i]);
*/

	return RunGame(cmdLine);
}

#ifdef _LIB
// Include common type defines for static linking
// Manually instantiate templates as needed here.
#include <CryCore/Common_TypeInfo.h>
STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Vec2_tpl, <int>)
STRUCT_INFO_T_INSTANTIATE(Vec4_tpl, <short>)
STRUCT_INFO_T_INSTANTIATE(Vec3_tpl, <int>)
STRUCT_INFO_T_INSTANTIATE(Ang3_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Quat_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Plane_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Matrix33_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Color_tpl, <float>)
STRUCT_INFO_T_INSTANTIATE(Color_tpl, <uint8>)
#endif
