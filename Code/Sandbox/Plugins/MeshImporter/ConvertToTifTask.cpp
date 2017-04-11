// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "ConvertToTifTask.h"

#include <IEditor.h>

#include <CryCore/ToolsHelpers/SettingsManagerHelpers.h>
#include <CrySystem/File/LineStreamBuffer.h>

#include <CrySystem/IProjectManager.h>

void LogPrintf(const char* szFormat, ...);

typedef void (* Listener)(const char*);

void TifConversionListener(const char* szWhat)
{
	LogPrintf("Conversion to TIF: %s\n", szWhat);
}

const char* ConvertToTIF(const string& filename, Listener listener, const wchar_t* szWorkingDirectory)
{
	const string imageMagickFolder = PathUtil::Make(PathUtil::GetEnginePath(), "Tools/thirdparty/ImageMagick");

	HANDLE hChildStdOutRd, hChildStdOutWr;
	HANDLE hChildStdInRd, hChildStdInWr;
	PROCESS_INFORMATION pi;

	// Create a pipe to read the stdout of the RC.
	SECURITY_ATTRIBUTES saAttr;
	if (listener)
	{
		ZeroMemory(&saAttr, sizeof(saAttr));
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = 0;
		CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &saAttr, 0);
		SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN
		CreatePipe(&hChildStdInRd, &hChildStdInWr, &saAttr, 0);
		SetHandleInformation(hChildStdInWr, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN
	}

	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwX = 100;
	si.dwY = 100;
	if (listener)
	{
		si.hStdError = hChildStdOutWr;
		si.hStdOutput = hChildStdOutWr;
		si.hStdInput = hChildStdInRd;
		si.dwFlags = STARTF_USEPOSITION | STARTF_USESTDHANDLES;
	}
	else
	{
		si.dwFlags = STARTF_USEPOSITION;
	}

	ZeroMemory(&pi, sizeof(pi));

	SettingsManagerHelpers::CFixedString<wchar_t, MAX_PATH* 3> wRemoteCmdLine;
	wRemoteCmdLine.appendAscii("\"");
	wRemoteCmdLine.appendAscii(PathUtil::Make(imageMagickFolder, "convert.exe").replace("/", "\\"));
	wRemoteCmdLine.appendAscii("\" ");

	// Make sure to *not* write alpha channel.
	// "Normals" preset of RC does not work well with alpha channel.
	wRemoteCmdLine.appendAscii("-type TrueColor ");

	wRemoteCmdLine.appendAscii("\"");
	wRemoteCmdLine.appendAscii(filename.c_str());
	wRemoteCmdLine.appendAscii("\" ");

	wRemoteCmdLine.appendAscii("\"");
	wRemoteCmdLine.appendAscii(PathUtil::ReplaceExtension(filename, ".tif").c_str());
	wRemoteCmdLine.appendAscii("\"");

	SettingsManagerHelpers::CFixedString<wchar_t, MAX_PATH* 3> wEnv;
	wEnv.appendAscii("PATH=");
	wEnv.appendAscii(PathUtil::ToDosPath(imageMagickFolder));
	wEnv.appendAscii("\0", 1);
	wEnv.appendAscii("MAGICK_CODER_MODULE_PATH=");
	wEnv.appendAscii(PathUtil::ToDosPath(PathUtil::Make(imageMagickFolder, "modules\\coders")));
	wEnv.appendAscii("\0", 1);

	const wchar_t* const szStartingDirectory = szWorkingDirectory;

	if (!CreateProcessW(
	      NULL,                                          // No module name (use command line).
	      const_cast<wchar_t*>(wRemoteCmdLine.c_str()),  // Command line.
	      NULL,                                          // Process handle not inheritable.
	      NULL,                                          // Thread handle not inheritable.
	      TRUE,                                          // Set handle inheritance to TRUE.
	      CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, // creation flags.
	                                                     // NULL,                                         // Use parent's environment block.
	      (void*)wEnv.c_str(),
	      szStartingDirectory, // Set starting directory.
	                           // nullptr,                          // Set starting directory.
	      &si,                 // Pointer to STARTUPINFO structure.
	      &pi))                // Pointer to PROCESS_INFORMATION structure.
	{
		// The following  code block is commented out instead of being deleted
		// because it's good to have at hand for a debugging session.
#if 0
		const size_t charsInMessageBuffer = 32768;   // msdn about FormatMessage(): "The output buffer cannot be larger than 64K bytes."
		wchar_t szMessageBuffer[charsInMessageBuffer] = L"";
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, szMessageBuffer, charsInMessageBuffer, NULL);
		GetCurrentDirectoryW(charsInMessageBuffer, szMessageBuffer);
#endif

		// not found
		return "ImageMagick convert.exe not found";
	}

	bool bFailedToReadOutput = false;

	if (listener)
	{
		// Close the pipe that writes to the child process, since we don't actually have any input for it.
		CloseHandle(hChildStdInWr);

		// Read all the output from the child process.
		CloseHandle(hChildStdOutWr);

		struct SLineHandler
		{
			Listener listener;
			void     HandleLine(const char* szLine)
			{
				listener(szLine);
			}
		};
		SLineHandler lineHandler;
		lineHandler.listener = listener;

		LineStreamBuffer lineBuffer(&lineHandler, &SLineHandler::HandleLine);
		for (;; )
		{
			char buffer[2048];
			DWORD bytesRead;
			if (!ReadFile(hChildStdOutRd, buffer, sizeof(buffer), &bytesRead, NULL) || (bytesRead == 0))
			{
				break;
			}
			lineBuffer.HandleText(buffer, bytesRead);
		}

		bFailedToReadOutput = lineBuffer.IsTruncated();
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return nullptr;
}

void CConvertToTifTask::SetCallback(const Callback& callback)
{
	m_callback = callback;
}

void CConvertToTifTask::AddFile(const QString& filePath)
{
	if (!m_filePaths.contains(filePath))
	{
		m_filePaths.push_back(filePath);
	}
}

bool CConvertToTifTask::InitializeTask()
{
	return true;
}

bool CConvertToTifTask::PerformTask()
{
	for (QString& filePath : m_filePaths)
	{
		string workingDir = gEnv->pSystem->GetIProjectManager()->GetCurrentProjectDirectoryAbsolute();
		ConvertToTIF(QtUtil::ToString(filePath), TifConversionListener, QString(workingDir.c_str()).toStdWString().c_str());
	}

	return true;
}

void CConvertToTifTask::FinishTask(bool bTaskSucceeded)
{
	if (m_callback)
	{
		m_callback(this);
	}
	delete this;
}
