// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlTaskManager.h"
#include "VersionControlCache.h"
#include "CryThreading/CryThread.h"
#include "CryThreading/IThreadManager.h"
#include "VersionControlErrorHandler.h"
#include "IEditor.h"

namespace Private_VersionControlTaskManager
{

CryMutex g_queueMutex;
CryConditionVariable g_cond;
volatile bool g_shouldTerminate = false;

class CBlockingTask : public CVersionControlTask
{
public:
	CBlockingTask(CVersionControlTask::TaskWrapper func)
		: CVersionControlTask(std::move(func))
	{}

	virtual void Execute()
	{
		CVersionControlTask::Execute();
		m_isExecuted = true;
		m_cond.Notify();
	}

	virtual void Init()
	{
		CRY_ASSERT_MESSAGE(CryGetCurrentThreadId() != gEnv->mMainThreadId, "Version Control task can't be set blocking on main thread");
		while (!m_isExecuted)
		{
			CryAutoLock<CryMutex> lock(m_mutex);
			m_cond.Wait(m_mutex);
		}
	}

private:
	CryMutex m_mutex;
	CryConditionVariable m_cond;
	volatile bool m_isExecuted{ false };
};

class CTaskQueueExecutor : public IThread
{
public:
	CTaskQueueExecutor(std::deque<std::shared_ptr<CVersionControlTask>>& tasks)
		: m_tasks(tasks)
	{}

	virtual void ThreadEntry()
	{
		while (!g_shouldTerminate && !m_shouldTerminate)
		{
			while (!g_shouldTerminate && !m_shouldTerminate && !m_tasks.empty())
			{
				std::shared_ptr<CVersionControlTask> pTask;
				{
					CryAutoLock<CryMutex> lock(g_queueMutex);
					pTask = m_tasks.front();
					m_tasks.pop_front();
				}
				if (!pTask->IsCanceled())
				{
					pTask->Execute();
				}
			}

			CryAutoLock<CryMutex> lock(g_queueMutex);
			g_cond.Wait(g_queueMutex);
		}
		gEnv->pThreadManager->JoinThread(this, eJM_Join);
		delete this;
	}

	void Terminate() { m_shouldTerminate = true; }

private:
	std::deque<std::shared_ptr<CVersionControlTask>>& m_tasks;
	// thread should have it's own terminate flag because it's possible to switch to another VCS while the first
	// one is still running (finishing last long operation).
	volatile bool m_shouldTerminate{ false };
};

static CTaskQueueExecutor* g_pTaskQueueExecutor = nullptr;

}

CVersionControlTaskManager::~CVersionControlTaskManager()
{
	Disable();
}

void CVersionControlTaskManager::Enable()
{
	using namespace Private_VersionControlTaskManager;
	if (g_pTaskQueueExecutor)
	{
		return;
	}

	g_shouldTerminate = false;
	g_pTaskQueueExecutor = new CTaskQueueExecutor(m_tasks);
	gEnv->pThreadManager->SpawnThread(g_pTaskQueueExecutor, "VersionControl");
}

void CVersionControlTaskManager::Disable()
{
	using namespace Private_VersionControlTaskManager;
	if (!g_pTaskQueueExecutor)
	{
		return;
	}

	using namespace Private_VersionControlTaskManager;
	g_shouldTerminate = true;
	g_pTaskQueueExecutor->Terminate();
	m_tasks.clear();
	g_cond.Notify();
	g_pTaskQueueExecutor = nullptr;
}

void CVersionControlTaskManager::Reset()
{
	Disable();
	Enable();
}

std::shared_ptr<CVersionControlTask> CVersionControlTaskManager::AddTask(CVersionControlTask::TaskWrapper func, bool isBlocking)
{
	using namespace Private_VersionControlTaskManager;

	auto task = isBlocking ? std::make_shared<CBlockingTask>(std::move(func)) : std::make_shared<CVersionControlTask>(std::move(func));

	{
		CryAutoLock<CryMutex> lock(g_queueMutex);
		m_tasks.push_back(task);
	}
	g_cond.Notify();
	task->Init();

	return task;
}

std::shared_ptr<CVersionControlTask> CVersionControlTaskManager::ScheduleTask(std::function<SVersionControlError()> taskFunc, bool isBlocking, Callback callback)
{
	using namespace Private_VersionControlTaskManager;
	CRY_ASSERT_MESSAGE(g_pTaskQueueExecutor, "VCS Thread is not running");
	if (!g_pTaskQueueExecutor)
	{
		return nullptr;
	}
	auto pTask = AddTask([this, taskFunc, callback = std::move(callback)](std::shared_ptr<CVersionControlTask> pTask) mutable
	{
		auto& result = pTask->m_result;
		m_pCache->m_lastUpdateList.clear();
		result.SetError(taskFunc());
		result.m_statusChanges = std::move(m_pCache->m_lastUpdateList);
		if (g_shouldTerminate)
		{
			result.SetError(EVersionControlError::Terminated);
		}
		GetIEditor()->PostOnMainThread([pTask, callback = std::move(callback)]()
		{
			if (pTask->IsCanceled())
			{
				return;
			}
			VersionControlErrorHandler::Handle(pTask->GetResult().GetError());
			if (callback && !pTask->GetResult().GetError().isCritical)
			{
				callback(pTask->GetResult());
			}
		});
	}, isBlocking);
	return pTask;
}

void CVersionControlTask::Cancel()
{
	m_isCanceled = true;
}

void CVersionControlTask::Execute()
{
	m_func(shared_from_this());
}
