// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   StatsAgent.cpp
//  Version:     v1.00
//  Created:     06/10/2009 by Steve Barnett.
//  Description: This the declaration for CStatsAgent
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <CryCore/Project/ProjectDefines.h>
#include <CrySystem/ICmdLine.h>

#if defined(ENABLE_STATS_AGENT)

	#include <cstdlib>
	#include <cstring>

	#include "StatsAgent.h"
	#include "StatsAgentPipe.h"
	#include <CryThreading/MultiThread_Containers.h>

	#include <CryThreading/IThreadManager.h>
	#include "ITelemetryCollector.h"

namespace
{
class CStatsAgentThread;

/* 10mins - assuming 60fps which the game can run at when it is in the precache phase */
const int DELAY_MESSAGES_TIMEOUT = 60 * 60 * 10;

typedef CryMT::vector<CryStringLocal> TStatsAgentMessageQueue;
static TStatsAgentMessageQueue* s_pMessageQueue;

static CStatsAgentThread* s_pStatsThread = 0;
}

namespace
{
class CStatsAgentThread : public IThread
{
public:
	CStatsAgentThread()
		: m_bQuit(false)
	{
		if (!gEnv->pThreadManager->SpawnThread(this, "StatsAgent"))
		{
			CryFatalError((string().Format("Error spawning \"%s\" thread.", "StatsAgent").c_str()));
		}
	}

	virtual ~CStatsAgentThread()
	{
		SignalStopWork();
		gEnv->pThreadManager->JoinThread(this, eJM_Join);
	}

	// Signals the thread that it should not accept anymore work and exit
	void SignalStopWork()
	{
		m_bQuit = true;
	}

protected:
	// Start accepting work on thread
	virtual void ThreadEntry()
	{
		while (!m_bQuit)
		{
			if (CStatsAgentPipe::PipeOpen())
			{
				if (const char* szMessage = CStatsAgentPipe::Receive())
				{
					if (*szMessage)
						s_pMessageQueue->push_back(szMessage);
				}
			}
			CrySleep(1000);
		}
	}

	volatile bool m_bQuit;
};
}
struct CSystemEventListener_StatsAgent : public ISystemEventListener
{
public:
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
	{
		switch (event)
		{
		case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
		case ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN:
		case ESYSTEM_EVENT_LEVEL_LOAD_START:
		case ESYSTEM_EVENT_LEVEL_UNLOAD:
			{
				CryLogAlways("CStatsAgent: starting delay of messages");
				CStatsAgent::SetDelayMessages(true);
				break;
			}
		case ESYSTEM_EVENT_LEVEL_LOAD_END:
		case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
			{
				CryLogAlways("CStatsAgent: starting long timeout of messages");
				CStatsAgent::SetDelayTimeout(DELAY_MESSAGES_TIMEOUT);
				break;
			}
		case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
		case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
		case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
			{
				CryLogAlways("CStatsAgent: stopping delay of messages");
				CStatsAgent::SetDelayMessages(false);
				break;
			}
		}
	}
};
static CSystemEventListener_StatsAgent g_system_event_listener_statsagent;

const int DELAY_MESSAGES_EXIT_DELAY = 10;

bool CStatsAgent::s_delayMessages = false;
int CStatsAgent::s_iDelayMessagesCounter = -1;

void CStatsAgent::CreatePipe(const ICmdLineArg* pPipeName)
{
	CryLogAlways("CStatsAgent::CreatePipe: pPipeName = %s", pPipeName ? pPipeName->GetValue() : "null");
	if (pPipeName)
	{
		s_delayMessages = false;
		gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_statsagent, "CStatsAgent");

		CStatsAgentPipe::OpenPipe(pPipeName->GetValue());

		if (CStatsAgentPipe::PipeOpen())
		{
			// Kick off a thread to listen on the pipe
			s_pMessageQueue = new TStatsAgentMessageQueue();
			s_pStatsThread = new CStatsAgentThread();
		}
	}
}

void CStatsAgent::ClosePipe()
{
	if (s_pStatsThread)
	{
		s_pStatsThread->SignalStopWork();
		gEnv->pThreadManager->JoinThread(s_pStatsThread, eJM_Join);
		SAFE_DELETE(s_pStatsThread);
		SAFE_DELETE(s_pMessageQueue);
		gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(&g_system_event_listener_statsagent);
	}
	CStatsAgentPipe::ClosePipe();
}

namespace StatsAgentHelper
{
//Whether string starts with certain prefix literal.
template<int Length>
static bool StrStartsWith(const CryStringLocal& str, const char(&szSubstr)[Length])
{
	static_assert(Length >= 1, "invalid string literal");   //at least '\0'
	if (Length - 1 > str.GetLength())
 		return false;
	return strncmp(str.c_str(), szSubstr, Length - 1) == 0;
}

//Returns remaining 0-terminated substring if matching prefix, or nullptr otherwise.
template<int Length>
static const char* TryParse(const CryStringLocal& str, const char(&szSubstr)[Length])
{
	if (StrStartsWith(str, szSubstr))
	{
		return str.c_str() + Length - 1;
	}
	return nullptr;
}
}

void CStatsAgent::Update()
{
	if (CStatsAgentPipe::PipeOpen())
	{
		if (s_delayMessages)
		{
			const size_t numMessages = s_pMessageQueue->size();
			CryLogAlways("CStatsAgent: Delaying processing of %d messages delayCounter:%d", numMessages, s_iDelayMessagesCounter);
			for (size_t i = 0; i < numMessages; i++)
			{
				CryLogAlways("CStatsAgent: Message[%d] '%s'", i, (*s_pMessageQueue)[i].c_str());
			}
			if (s_iDelayMessagesCounter >= 0)
			{
				--s_iDelayMessagesCounter;
				if (s_iDelayMessagesCounter <= 0)
				{
					s_iDelayMessagesCounter = 0;
					s_delayMessages = false;
				}
			}
		}
		CryStringLocal message;
		std::vector<CryStringLocal> unprocessedMessages;
		bool bSaveMessages = false;
		while (s_pMessageQueue->try_pop_front(message))
		{
			// Can't process the message - have to save it and put it back after the queue has been drained
			if (bSaveMessages)
			{
				unprocessedMessages.push_back(message);
				continue;
			}
			// Just return the dump filename
			if (StatsAgentHelper::StrStartsWith(message, "getdumpfilename"))
			{
				CryReplayInfo info {};
				CryGetIMemReplay()->GetInfo(info);
				const char* pFilename = info.filename;
				if (!pFilename) { pFilename = "<unknown>"; }

				CStatsAgentPipe::Send(pFilename, "dumpfile", message.c_str());
			}
			// Get the value of a CVAR
			else if (const char* pCVARName = StatsAgentHelper::TryParse(message, "getcvarvalue "))
			{
				if (ICVar* pValue = gEnv->pConsole->GetCVar(pCVARName))
				{
					CStatsAgentPipe::Send(pValue->GetString(), "cvarvalue", message.c_str());
				}
				else
				{
					CryFixedStringT<64> response = "Unknown CVAR '";
					response += pCVARName;
					response += "'";
					CStatsAgentPipe::Send(response.c_str(), "error", message.c_str());
				}
			}
			// Create a new CVAR
			else if (const char* pCVARName = StatsAgentHelper::TryParse(message, "createcvar "))
			{
				REGISTER_STRING(pCVARName, "", 0, "Amble CVAR");

				CStatsAgentPipe::Send("finished", nullptr, message.c_str());
			}
			// Get telemetry session ID
			else if (StatsAgentHelper::StrStartsWith(message, "getsessionid"))
			{
				ITelemetryCollector* tc = nullptr;
				if (g_pGame)
				{
					tc = g_pGame->GetITelemetryCollector();
				}
				if (tc)
				{
					CStatsAgentPipe::Send(tc->GetSessionId().c_str(), "sessionid", message.c_str());
				}
				else
				{
					CStatsAgentPipe::Send("", "sessionid", message.c_str());
				}
			}
			// Execute the command
			else if (const char* pCommand = StatsAgentHelper::TryParse(message, "exec "))
			{
				if (!s_delayMessages)
				{
					// Execute the rest of the string
					if (gEnv && gEnv->pConsole)
					{
						gEnv->pConsole->ExecuteString(pCommand);

						CStatsAgentPipe::Send("finished", nullptr, message.c_str());
					}
				}
				else
				{
					// Stop processing the message queue - store the queue and restore it
					bSaveMessages = true;
					unprocessedMessages.push_back(message);
				}
			}
			// Unknown command
			else
			{
				CryFixedStringT<64> response = "Unknown command '";
				response += message.c_str();
				response += "'";
				CStatsAgentPipe::Send(response.c_str(), "error", message.c_str());
			}
		}
		s_pMessageQueue->append(unprocessedMessages.begin(), unprocessedMessages.end());
	}
}

void CStatsAgent::SetDelayTimeout(const int timeout)
{
	CryLogAlways("CStatsAgent: SetDelayTimeout %d", timeout);
	s_iDelayMessagesCounter = timeout;
}

void CStatsAgent::SetDelayMessages(bool enable)
{
	if (enable)
	{
		s_delayMessages = true;
		s_iDelayMessagesCounter = -1;
	}
	else if (s_delayMessages)
	{
		SetDelayTimeout(DELAY_MESSAGES_EXIT_DELAY);
	}
}

#endif  // defined(ENABLE_STATS_AGENT)
