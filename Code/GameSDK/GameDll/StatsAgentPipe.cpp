// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   StatsAgentPipe.cpp
//  Version:     v1.00
//  Created:     20/10/2011 by Sandy MacPherson
//  Description: Wrapper around platform-dependent pipe comms
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if defined(ENABLE_STATS_AGENT)

#include <CryCore/Project/ProjectDefines.h>
#include "StatsAgentPipe.h"
#include <CryCore/Platform/CryWindows.h>

namespace
{
	const int BUFFER_SIZE = 1024;

	CryFixedStringT<BUFFER_SIZE> s_pCommandBuffer;
	bool s_pipeOpen = false;

#if CRY_PLATFORM_DURANGO //FIXME ?
	const char *PIPE_BASE_NAME = "";
	volatile bool s_commandWaiting = false;
#elif CRY_PLATFORM_WINDOWS
	const char *PIPE_BASE_NAME = "\\\\.\\pipe\\CrysisTargetComms";
	HANDLE s_pipe = INVALID_HANDLE_VALUE;
#endif
};

static int statsagent_debug = 0;

bool CStatsAgentPipe::PipeOpen()
{
	return s_pipeOpen;
}

void CStatsAgentPipe::OpenPipe(const char *szPipeName)
{
	REGISTER_CVAR(statsagent_debug, 0, 0, "Enable/Disable StatsAgent debug messages");

	CryFixedStringT<255> buffer(PIPE_BASE_NAME);

	// Construct the pipe name
	buffer += szPipeName;
	buffer.TrimRight();
	buffer += ".pipe";

	CreatePipe(buffer.c_str());

	if (statsagent_debug && s_pipeOpen)
	{
		CryLogAlways("CStatsAgent: Pipe connection \"%s\" is open", buffer.c_str());
	}

	if (s_pipeOpen)
	{
		char pMsg[] = "connected";
		if (!Send(pMsg))
			ClosePipe();
	}
	else
	{
		if (statsagent_debug)
			CryLogAlways("CStatsAgent: Unable to connect pipe %s", buffer.c_str());
	}
}

bool CStatsAgentPipe::CreatePipe(const char *szName)
{
#if CRY_PLATFORM_WINDOWS
	s_pipe = ::CreateFile(szName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	DWORD dwMode = PIPE_NOWAIT;
	if (s_pipe != INVALID_HANDLE_VALUE)
	{
		s_pipeOpen = ::SetNamedPipeHandleState(s_pipe, &dwMode, NULL, NULL) == TRUE;
	}	
#endif

	return s_pipeOpen;
}

void CStatsAgentPipe::ClosePipe()
{
	if (s_pipeOpen)
	{
#if CRY_PLATFORM_WINDOWS 
		::CloseHandle(s_pipe);
		s_pipe = INVALID_HANDLE_VALUE;
#endif
		s_pipeOpen = false;
	}
}

bool CStatsAgentPipe::Send(const char *szMessage, const char *szPrefix, const char* szDebugTag)
{
	CryFixedStringT<BUFFER_SIZE> pBuffer;
	if (szPrefix)
	{
		pBuffer = szPrefix;
		pBuffer.Trim();
		pBuffer += " ";
	}
	pBuffer += szMessage;

	bool ok = true;
	uint32 nBytes = pBuffer.size() + 1;

	if (statsagent_debug)
	{
		if (szDebugTag)
		{
			CryLogAlways("CStatsAgent: Sending message \"%s\" [%s]", pBuffer.c_str(), szDebugTag);
		}
		else
		{
			CryLogAlways("CStatsAgent: Sending message \"%s\"", pBuffer.c_str());
		}
	}

#if CRY_PLATFORM_WINDOWS
	DWORD tx;
	ok = ::WriteFile(s_pipe, pBuffer.c_str(), nBytes, &tx, 0) == TRUE;
#endif

	if (statsagent_debug && !ok)
		CryLogAlways("CStatsAgent: Unable to write to pipe");

	return ok;
}

const char* CStatsAgentPipe::Receive()
{
	const char *szResult = NULL;

#if CRY_PLATFORM_WINDOWS
	DWORD size;
	if (::ReadFile(s_pipe, s_pCommandBuffer.m_strBuf, BUFFER_SIZE - 1, &size, 0) && size > 0)
	{
		s_pCommandBuffer.m_strBuf[size] = '\0';
		s_pCommandBuffer.TrimRight('\n');
		szResult = s_pCommandBuffer.c_str();
	}
#endif

	if (statsagent_debug && szResult)
		CryLogAlways("CStatsAgent: Received message \"%s\"", szResult);

	return szResult;
}

#endif	// defined(ENABLE_STATS_AGENT)
