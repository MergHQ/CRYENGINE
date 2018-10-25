// Copyright 2001-2018 Crytek GmbH. All rights reserved.

#include "StdAfx.h"
#include "IFileOperationsExecutor.h"
#include <CrySystem/ISystem.h>

namespace Private_IFileOperationsExecutor
{

void AssertNotMainThread()
{
	CRY_ASSERT_MESSAGE(CryGetCurrentThreadId() != gEnv->mMainThreadId, "File operations shouldn't be run on main thread");
}

}

void IFileOperationsExecutor::Delete(std::vector<std::unique_ptr<IFilesGroupProvider>> fileGroups)
{
	Private_IFileOperationsExecutor::AssertNotMainThread();
	DoDelete(std::move(fileGroups));
}

void IFileOperationsExecutor::Delete(const std::vector<string>& files)
{
	Private_IFileOperationsExecutor::AssertNotMainThread();
	DoDelete(files);
}
