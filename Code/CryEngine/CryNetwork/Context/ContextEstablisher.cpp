// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ContextEstablisher.h"
#include <CryRenderer/IRenderAuxGeom.h>

class CCET_ExecConsole : public CCET_Base
{
public:
	CCET_ExecConsole(string cmd, string name) : m_cmd(cmd), m_name(name) {}

	const char*                 GetName() { return m_name.c_str(); }

	EContextEstablishTaskResult OnStep(SContextEstablishState&)
	{
		gEnv->pConsole->ExecuteString(m_cmd.c_str());
		return eCETR_Ok;
	}

private:
	string m_cmd;
	string m_name;
};

CContextEstablisher::CContextEstablisher()
{
	m_offset = -1;
	m_done = false;
	m_debuggedFrame = -1;
	++g_objcnt.contextEstablisher;

	m_begin = gEnv->pTimer->GetAsyncTime();
}

CContextEstablisher::~CContextEstablisher()
{
	ASSERT_PRIMARY_THREAD;
	--g_objcnt.contextEstablisher;
	for (size_t i = 0; i < m_tasks.size(); ++i)
	{
		m_tasks[i].pTask->Release();
	}
}

void CContextEstablisher::Done()
{
	m_done = true;
}

void CContextEstablisher::AddTask(EContextViewState state, IContextEstablishTask* pTask)
{
	NET_ASSERT(m_offset < 0);
	m_tasks.push_back(STask(state, pTask));
#if ENABLE_DEBUG_KIT
	// If this is ever needed in release, change net_dump_object_state registration to non dev only (NetCVars.cpp)
	//	m_tasks.push_back(STask(state, new CCET_ExecConsole("net_dump_object_state", "DOS: " + string(pTask->GetName()))));
#endif
}

void CContextEstablisher::Start()
{
	NET_ASSERT(m_offset < 0);
	std::stable_sort(m_tasks.begin(), m_tasks.end());
	m_offset = 0;
}

bool CContextEstablisher::StepTo(SContextEstablishState& state)
{
	SLICE_SCOPE_DEFINE();

	if (m_offset < 0)
	{
		return false;
	}

	while (m_offset < (int)m_tasks.size() && m_tasks[m_offset].state <= state.contextState)
	{
#if LOG_CONTEXT_ESTABLISHMENT
		m_tasks[m_offset].numRuns++;
#endif
		switch (m_tasks[m_offset].pTask->OnStep(state))
		{
		case eCETR_Ok:
#if LOG_CONTEXT_ESTABLISHMENT
			m_tasks[m_offset].done = gEnv->pTimer->GetAsyncTime();
#endif
#if LOG_CONTEXT_ESTABLISHMENT
			m_tasks[m_offset].done = gEnv->pTimer->GetAsyncTime();
			if (CVARS.net_log_context_establishment > 0)
				NetLogEstablishment(1, "Establishment: Did %s on %s in %.2fms, state %d:%s", m_tasks[m_offset].pTask->GetName(), state.pSender ? state.pSender->GetName() : "Context",
					(m_tasks[m_offset].done - (m_offset ? m_tasks[m_offset - 1].done : m_begin)).GetMilliSeconds(), 
					m_tasks[m_offset].state, CContextView::GetStateName(m_tasks[m_offset].state));
#endif
			m_offset++;
#if LOG_CONTEXT_ESTABLISHMENT
			if (m_offset < m_tasks.size())
				if (CVARS.net_log_context_establishment > 0)
					NetLogEstablishment(1, "Establishment: Beginning %s on %s, state %d:%s", m_tasks[m_offset].pTask->GetName(), state.pSender ? state.pSender->GetName() : "Context", 
						m_tasks[m_offset].state, CContextView::GetStateName(m_tasks[m_offset].state));
#endif
			break;
		case eCETR_Failed:
			m_offset++;
			Fail(eDC_GameError, string().Format("Couldn't establish context: %s failed", m_tasks[m_offset - 1].pTask->GetName()));
			return false;
		case eCETR_Wait:
#if LOG_CONTEXT_ESTABLISHMENT
			NetLogEstablishment(2, "Establishment: Waiting on task [%s], state %d:%s", m_tasks[m_offset].pTask->GetName(), 
				m_tasks[m_offset].state, CContextView::GetStateName(m_tasks[m_offset].state));
#endif // LOG_CONTEXT_ESTABLISHMENT
			return true;
		}
	}
	return true;
}

void CContextEstablisher::Fail(EDisconnectionCause cause, const string& reason)
{
	// for mp, this can legitimately happen in non-fatal cases, a client losing connection during loading, for example
	if (!gEnv->bMultiplayer)
		CryWarning(VALIDATOR_MODULE_NETWORK, VALIDATOR_WARNING, "!Failed to establish context! cause %d, %s", (int)cause, reason.c_str());
	else
	{
#if LOG_CONTEXT_ESTABLISHMENT
		NetWarnEstablishment("Establishment: Failed to establish context, cause %d, reason: %s", (int)cause, reason.c_str());
#endif
	}

	m_failureReason = reason;
	for (size_t i = 0; i < m_tasks.size(); i++)
	{
		m_tasks[i].pTask->OnFailLoading(i < (size_t)m_offset);
	}
	m_offset = -1;

	if (m_disconnectOnFail)
	{
		SCOPED_GLOBAL_LOCK;
		m_disconnectOnFail->Disconnect(cause, reason.c_str());
	}
}

#if ENABLE_DEBUG_KIT
#include <CryRenderer/IRenderAuxGeom.h>

void CContextEstablisher::DebugDraw()
{
	ASSERT_PRIMARY_THREAD;
	float y = 400;
	static float x;
	static int frameid = -1;
	if (frameid != gEnv->nMainFrameID)
	{
		x = 10;
		frameid = gEnv->nMainFrameID;
	}
	if (frameid != m_debuggedFrame)
	{
		CTimeValue beg = m_begin;
		for (size_t i = 0; i < m_tasks.size(); i++)
		{
			if (!m_tasks[i].pTask->GetName()[0])
				continue;
			float tm = -1.0f;
			if (m_tasks[i].done > 0.0f)
			{
				tm = (m_tasks[i].done - beg).GetMilliSeconds();
				beg = m_tasks[i].done;
			}
			float clr[4] = { 1, 1, 1, (i >= m_offset) * 0.5f + 0.5f };
			gEnv->pAuxGeomRenderer->Draw2dLabel(x, y, 1, clr, false, "%d: %s [%d @ %.2f]", m_tasks[i].state, m_tasks[i].pTask->GetName(), m_tasks[i].numRuns, tm);
			y += 10;
		}
		x += 130;
	}
}
#endif

#if LOG_CONTEXT_ESTABLISHMENT
void CContextEstablisher::OutputTiming(void)
{
	CryComment("-- OutputTiming");
	{
		CTimeValue beg = m_begin;
		for (size_t i = 0; i < m_tasks.size(); i++)
		{
			if (!m_tasks[i].pTask->GetName()[0])
				continue;
			float tm = -1.0f;
			if (m_tasks[i].done > 0.0f)
			{
				tm = (m_tasks[i].done - beg).GetMilliSeconds();
				beg = m_tasks[i].done;
			}
			CryComment("%d: %s [%d @ %.2f] started at %." PRId64, m_tasks[i].state, m_tasks[i].pTask->GetName(), m_tasks[i].numRuns, tm, beg.GetValue());
		}
	}
}
#endif

#if LOG_CONTEXT_ESTABLISHMENT

void NetWarnEstablishment(const char* szFormat, ...)
{
	if (CVARS.net_log_context_establishment > 0)
	{
		va_list args;
		va_start(args, szFormat);
		gEnv->pLog->LogV(ILog::eWarningAlways, szFormat, args);
		va_end(args);
	}
}


void NetLogEstablishment(int level, const char* szFormat, ...)
{
	if (level <= CVARS.net_log_context_establishment)
	{
		va_list args;
		va_start(args, szFormat);
		gEnv->pLog->LogV(ILog::eAlways, szFormat, args);
		va_end(args);
	}
}
#endif
