// Copyright 2001-2019 Crytek GmbH. All rights reserved.

#include "StdAfx.h"
#include "IFileOperationsExecutor.h"
#include "ThreadingUtils.h"
#include "CrySystem/ISystem.h"
#include "CryCore/smartptr.h"

namespace Private_FileOperationsExecutor
{
	void AssertNotMainThread()
	{
		CRY_ASSERT_MESSAGE(CryGetCurrentThreadId() != gEnv->mMainThreadId, "Method Delete of IFileOperationExecutor can not be called on the main thread");
	}
}

void IFileOperationsExecutor::AsyncDelete(std::vector<std::unique_ptr<IFilesGroupController>> fileGroups
	, std::function<void(void)> callback , bool forceAsync)
{
	if (!callback)
	{
		callback = [] {};
	}
	if (forceAsync || CryGetCurrentThreadId() == gEnv->mMainThreadId)
	{
		ThreadingUtils::AsyncQueue([this, fileGroups = std::move(fileGroups), callback = std::move(callback)]() mutable
		{
			DoDelete(std::move(fileGroups), std::move(callback));
		});
	}
	else
	{
		DoDelete(std::move(fileGroups), std::move(callback));
	}
}

void IFileOperationsExecutor::AsyncDelete(const std::vector<string>& files, std::function<void(void)> callback
	, bool forceAsync)
{
	if (!callback)
	{
		callback = [] {};
	}
	if (forceAsync || CryGetCurrentThreadId() == gEnv->mMainThreadId)
	{
		ThreadingUtils::AsyncQueue([this, files, callback = std::move(callback)]()
		{
			DoDelete(files, std::move(callback));
		});
	}
	else
	{
		DoDelete(files, std::move(callback));
	}
}

void IFileOperationsExecutor::Delete(std::vector<std::unique_ptr<IFilesGroupController>> fileGroups, std::function<void(void)> callback /*= nullptr*/, bool forceAsync /*= false*/)
{
	Private_FileOperationsExecutor::AssertNotMainThread();
	DoDelete(std::move(fileGroups), std::move(callback));
}

void IFileOperationsExecutor::Delete(const std::vector<string>& files, std::function<void(void)> callback /*= nullptr*/, bool forceAsync /*= false*/)
{
	Private_FileOperationsExecutor::AssertNotMainThread();
	DoDelete(files, std::move(callback));
}
