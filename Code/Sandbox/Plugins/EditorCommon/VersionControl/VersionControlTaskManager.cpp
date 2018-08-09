// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlTaskManager.h"
#include "CryThreading/CryThread.h"
#include "CryThreading/IThreadManager.h"
#include "VersionControlErrorHandler.h"

namespace Private_VersionControlTaskManager
{

CryMutex g_queueMutex;
CryConditionVariable g_cond;
volatile bool g_shouldTerminate = false;

using CTask = CVersionControlTaskManager::CTask;

class CBlockingTask : public CVersionControlTaskManager::CTask
{
public:
	CBlockingTask(CVersionControlTaskManager::TaskWrapper func)
		: CTask(std::move(func))
	{}

	virtual void Execute()
	{
		CTask::Execute();
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
	CTaskQueueExecutor(std::deque<std::shared_ptr<CTask>>& tasks)
		: m_tasks(tasks)
	{}

	virtual void ThreadEntry()
	{
		while (!g_shouldTerminate && !m_shouldTerminate)
		{
			while (!g_shouldTerminate && !m_shouldTerminate && !m_tasks.empty())
			{
				std::shared_ptr<CTask> pTask;
				{
					CryAutoLock<CryMutex> lock(g_queueMutex);
					pTask = m_tasks.front();
					m_tasks.pop_front();
				}
				pTask->Execute();
			}

			CryAutoLock<CryMutex> lock(g_queueMutex);
			g_cond.Wait(g_queueMutex);
		}
		gEnv->pThreadManager->JoinThread(this, eJM_Join);
		delete this;
	}

	void Terminate() { m_shouldTerminate = true; }

private:
	std::deque<std::shared_ptr<CTask>>& m_tasks;
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

std::shared_ptr<CVersionControlTaskManager::CTask> CVersionControlTaskManager::AddTask(CVersionControlTaskManager::TaskWrapper func, bool isBlocking)
{
	using namespace Private_VersionControlTaskManager;

	auto task = isBlocking ? std::make_shared<CBlockingTask>(std::move(func)) : std::make_shared<CTask>(std::move(func));

	{
		CryAutoLock<CryMutex> lock(g_queueMutex);
		m_tasks.push_back(task);
	}
	g_cond.Notify();
	task->Init();

	return task;
}

std::shared_ptr<const CVersionControlResult> CVersionControlTaskManager::ScheduleTask(std::function<void(CVersionControlResult&)> taskFunc, bool isBlocking, Callback callback)
{
	using namespace Private_VersionControlTaskManager;
	CRY_ASSERT_MESSAGE(g_pTaskQueueExecutor, "VCS Thread is not running");
	if (!g_pTaskQueueExecutor)
	{
		return nullptr;
	}
	auto pTask = AddTask([taskFunc, callback = std::move(callback)](std::shared_ptr<CVersionControlResult> result) mutable
	{
		taskFunc(*result);
		if (g_shouldTerminate)
		{
			result->SetError(EVersionControlError::Terminated);
		}
		GetIEditor()->PostOnMainThread([result, callback = std::move(callback)]()
		{
			VersionControlErrorHandler::Handle(result->GetError());
			if (callback && !result->GetError().isCritical)
			{
				callback(*result);
			}
		});
	}, isBlocking);
	return std::shared_ptr<const CVersionControlResult>(pTask, &pTask->GetResult());
}

void CVersionControlTaskManager::CTask::Execute()
{
	auto task = shared_from_this();
	auto result = std::shared_ptr<CVersionControlResult>(task, &m_result);
	m_func(result);
}
