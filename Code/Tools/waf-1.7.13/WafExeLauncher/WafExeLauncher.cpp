// WafExeLauncher.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <wchar.h>

int wmain(int argc, wchar_t* argv[])
{
	std::wstring cmdLineWithArgs;

	// Create path to cry_waf.exe
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	cmdLineWithArgs = std::wstring(buffer).substr(0, pos);
	cmdLineWithArgs.append(L"/Code/Tools/waf-1.7.13/bin/cry_waf.exe"); // hardcoded path to real cry_waf.exe

	// Get raw command line with original quotes
	wchar_t* arguments_raw = GetCommandLine();
	wchar_t* arguments_without_command = arguments_raw + wcslen(argv[0]) + ((arguments_raw[0] == L'\"') * 2); // handle command with quotes
	cmdLineWithArgs.append(arguments_without_command);

	// Create a job to ensure subprocess is killed when this app is killed forcefully
	HANDLE hJob;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
	hJob = CreateJobObject(NULL, NULL);
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

	// Create process
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL,  // the path
		&cmdLineWithArgs[0], // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB,
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi				// Pointer to PROCESS_INFORMATION structure
		))
	{
		std::wcerr << L"Create child process failed. Error code: " << GetLastError() << L"\nCmd:" << cmdLineWithArgs << std::endl;
		return 1;
	}

	// Assign process to job
	AssignProcessToJobObject(hJob, pi.hProcess);

	// Start process
	ResumeThread(pi.hThread);

	// Successfully created the process.  Wait for it to finish.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Get the exit code.
	DWORD exitCode;
	BOOL result = GetExitCodeProcess(pi.hProcess, &exitCode);

	// Close process and thread handles. 	
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(hJob);

	if (result == FALSE)
	{
		std::wcerr << L"GetExitCodeProcess() failure: " << GetLastError() << L"\n";
		return 1;
	}

	return exitCode;
}
