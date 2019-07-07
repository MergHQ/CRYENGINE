// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "VersionControlResult.h"
#include <atomic>
#include <deque>
#include <functional>
#include <memory>

namespace Private_VersionControlTaskManager
{
	class CTaskQueueExecutor;
	class CBlockingTask;
}

class CVersionControlCache;

//! The worker task that is being executed on a dedicated thread for executing version control operations.
class CVersionControlTask : public std::enable_shared_from_this<CVersionControlTask>
{
public:
	using TaskWrapper = std::function<void(std::shared_ptr<CVersionControlTask>)>;

	CVersionControlTask(TaskWrapper func)
		: m_func(std::move(func))
	{}

	virtual ~CVersionControlTask() {}

	//! Cancels execution of the current task if not started yet. Or prevents callback call is the task is not finished yet.
	void Cancel();

	//! Determines if the current task is canceled.
	bool IsCanceled() const { return m_isCanceled; }

	//! Returns the result of task execution.
	const CVersionControlResult& GetResult() const { return m_result; }

private:
	//! Starts execution of the task.
	virtual void Execute();

	//! Initializes the task when it's just created.
	virtual void Init() {}

	TaskWrapper m_func;
	CVersionControlResult m_result;
	std::atomic_bool m_isCanceled{ false };

	friend class Private_VersionControlTaskManager::CTaskQueueExecutor;
	friend class Private_VersionControlTaskManager::CBlockingTask;
	friend class CVersionControlTaskManager;
};

//! This class is responsible for executing VCS tasks in sequence on a dedicated thread.
//! It makes sure that no task is executed before the previous one is complete.
class EDITOR_COMMON_API CVersionControlTaskManager
{
public:
	using Callback = std::function<void(const CVersionControlResult&)>;

	explicit CVersionControlTaskManager(CVersionControlCache* pCache)
		: m_pCache(pCache) {}

	~CVersionControlTaskManager();

	//! Starts task manager by running a dedicated thread so that version control tasks can be executed on it.
	void Enable();
	//! Stops currently running dedicated thread and terminates the task that is currently being executed.
	void Disable();
	//! Terminates the currently running task and removing all other scheduled task.
	void Reset();

	//! Schedules a given task for execution on the dedicated thread.
	//! @taskFunc The actual task to be executed.
	//! @isBlocking Specifies if the thread this method is called from needs to be blocked and wait until the task is finished.
	//! This can't be done on main thread.
	//! @callback Callback to be called once the task is finished. Callback doesn't make much sense it the call is blocking.
	std::shared_ptr<CVersionControlTask> ScheduleTask(std::function<SVersionControlError()> taskFunc, bool isBlocking, Callback callback);

private:
	std::shared_ptr<CVersionControlTask> AddTask(CVersionControlTask::TaskWrapper func, bool isBlocking);

	CVersionControlCache* m_pCache{ nullptr };

	std::deque<std::shared_ptr<CVersionControlTask>> m_tasks;
};
