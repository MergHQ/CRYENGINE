// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "VersionControlResult.h"
#include <deque>
#include <memory>
#include <functional>

//! This class is responsible for executing VCS tasks in sequence on a dedicated thread.
//! It makes sure that no task is executed before the previous one is complete.
class EDITOR_COMMON_API CVersionControlTaskManager
{
public:
	using TaskWrapper = std::function<void(std::shared_ptr<CVersionControlResult>)>;
	using Callback = std::function<void(const CVersionControlResult&)>;

	class CTask : public std::enable_shared_from_this<CTask>
	{
	public:
		CTask(TaskWrapper func)
			: m_func(std::move(func))
		{}

		virtual ~CTask() {}

		virtual void Execute();

		virtual void Init() {}

		const CVersionControlResult& GetResult() const { return m_result; }

	private:
		TaskWrapper m_func;
		CVersionControlResult m_result;
	};

	~CVersionControlTaskManager();

	void Enable();
	void Disable();
	void Reset();

	std::shared_ptr<const CVersionControlResult> ScheduleTask(std::function<void(CVersionControlResult&)> taskFunc, bool isBlocking, Callback callback);

private:
	std::shared_ptr<CTask> AddTask(TaskWrapper func, bool isBlocking);

	std::deque<std::shared_ptr<CTask>> m_tasks;
};
