// Copyright 2001-2019 Crytek GmbH. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "IFileOperationsExecutor.h"
#include "CryString/CryString.h"
#include <vector>
#include <memory>

class EDITOR_COMMON_API CFileOperationExecutor 
{
public:
	static IFileOperationsExecutor* GetExecutor();

	static IFileOperationsExecutor* GetDefaultExecutor();

	static void SetExecutor(std::unique_ptr<IFileOperationsExecutor> pExecutor) { s_executor = std::move(pExecutor); }

	static void ResetToDefault() { s_executor = nullptr; }

private:
	static std::unique_ptr<IFileOperationsExecutor> s_executor;
};