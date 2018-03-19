// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "MonoDevelop_Manager.h"

#include <IEditor.h>
#include <ICommandManager.h>
#include <CryGame/IGame.h>
#include <CryGame/IGameFramework.h>
#include <CryMono/IMonoRuntime.h>

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>

#define BUFSIZE 65536

CMonoDevelopManager* CMonoDevelopManager::g_pInstance = NULL;

CMonoDevelopManager::CMonoDevelopManager(void) :
	m_pMonoDevelopWait(NULL),
	m_pMonoDevelopProcess(NULL)
{
}

CMonoDevelopManager::~CMonoDevelopManager(void)
{
	if (m_pMonoDevelopWait)
	{
		::UnregisterWaitEx(m_pMonoDevelopWait, INVALID_HANDLE_VALUE);
		CloseHandle(m_pMonoDevelopProcess);
	}
}

void CMonoDevelopManager::Register()
{
	CommandManagerHelper::RegisterCommand(GetIEditor()->GetICommandManager(), "plugin", "mono_reload_game", "Reload game assembly", "-", functor(COMMAND_ReloadGame));
	CommandManagerHelper::RegisterCommand(GetIEditor()->GetICommandManager(), "plugin", "mono_open_monodevelop", "Open mono develop", "-", functor(COMMAND_OpenMonoDevelop));
}

void CMonoDevelopManager::Unregister()
{
	ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
	pCommandManager->UnregisterCommand("plugin", "mono_reload_game");
	pCommandManager->UnregisterCommand("plugin", "mono_open_monodevelop");
}

CMonoDevelopManager* CMonoDevelopManager::GetInstance()
{
	if (!g_pInstance)
		g_pInstance = new CMonoDevelopManager();
	return g_pInstance;
}

void CMonoDevelopManager::COMMAND_ReloadGame()
{
	if (gEnv->pMonoRuntime)
		gEnv->pMonoRuntime->ReloadGame();
}

void CMonoDevelopManager::COMMAND_OpenMonoDevelop()
{
	CMonoDevelopManager* instance = GetInstance();
	if (instance->m_pMonoDevelopWait)
	{
		gEnv->pLog->LogWarning("[MonoDevelop] Already running!");
		/*UnregisterWaitEx(instance->m_pMonoDevelop, INVALID_HANDLE_VALUE);
		   instance->m_pMonoDevelop = NULL;*/
	}
	else
	{
		gEnv->pLog->Log("[MonoDevelop] Start . . .");

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		/*instance->m_pProcessInfo = new PROCESS_INFORMATION();
		   ZeroMemory(instance->m_pProcessInfo, sizeof(*instance->m_pProcessInfo));*/

		/*char pathMono[ICryPak::g_nMaxPath] = "";
		   gEnv->pCryPak->AdjustFileName(string("BinMono//bin//mono.exe"), pathMono, ICryPak::FLAGS_PATH_REAL);*/
		char pathMonoDevelop[ICryPak::g_nMaxPath] = "";
		gEnv->pCryPak->AdjustFileName(string("Tools//MonoDevelop//bin//monodevelop.exe"), pathMonoDevelop, ICryPak::FLAGS_PATH_REAL);

		TCHAR lpvEnvNew[BUFSIZE];
		LPTSTR lpszVariable;
		LPTSTR lpszVariableNew;
		LPTCH lpvEnv;
		// Get Current env vars
		lpvEnv = GetEnvironmentStrings();
		if (lpvEnv == NULL)
		{
			gEnv->pLog->LogError("GetEnvironmentStrings failed (%d)\n", GetLastError());
			return;
		}
		// Copy env vars to new env var block
		lpszVariable = (LPTSTR)lpvEnv;
		lpszVariableNew = (LPTSTR)lpvEnvNew;
		int bufSize = BUFSIZE;
		while (*lpszVariable)
		{
			int segmentSize = lstrlen(lpszVariable) + 1;
			StringCchCopy(lpszVariableNew, bufSize, lpszVariable);
			lpszVariable += segmentSize;
			lpszVariableNew += segmentSize;
			bufSize -= segmentSize;
		}
		FreeEnvironmentStrings(lpvEnv);
		// Add CryPort env var
		StringCchPrintf(lpszVariableNew, bufSize, TEXT("CryPort=%d"), 4600);//gEnv->pSystem->GetIRemoteConsole()->GetListenPort());
		int segmentSize = lstrlen(lpszVariableNew) + 1;
		lpszVariableNew += segmentSize;
		bufSize -= segmentSize;
		// Add CryEditor env var
		StringCchPrintf(lpszVariableNew, bufSize, TEXT("CryEditor=%d"), GetCurrentProcessId());
		segmentSize = lstrlen(lpszVariableNew) + 1;
		lpszVariableNew += segmentSize;
		bufSize -= segmentSize;
		// Add tailing 0
		*lpszVariableNew = 0;

		if (!CreateProcess(NULL,
		                   pathMonoDevelop,
		                   NULL,
		                   NULL,
		                   TRUE,
		                   0,
		                   lpvEnvNew, // ENV VAR
		                   NULL,
		                   &si,
		                   &pi))
		{
			gEnv->pLog->LogError("[MonoDevelop]   failed!");
			return;
		}

		instance->m_pMonoDevelopProcess = pi.hProcess;
		RegisterWaitForSingleObject(&instance->m_pMonoDevelopWait, pi.hProcess, OnMonoDevelopExit, instance, INFINITE, WT_EXECUTEONLYONCE);

		gEnv->pLog->Log("[MonoDevelop]   done.");
	}
}

void CALLBACK CMonoDevelopManager::OnMonoDevelopExit(void* context, BOOLEAN isTimeOut)
{
	gEnv->pLog->Log("[MonoDevelop] closed");
	GetInstance()->m_pMonoDevelopWait = NULL;
	GetInstance()->m_pMonoDevelopProcess = NULL;
}

