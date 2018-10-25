// Copyright 2001-2018 Crytek GmbH. All rights reserved.
#include "StdAfx.h"
#include "FileOperationsExecutor.h"

#include "IFilesGroupProvider.h"
#include "PathUtils.h"
#include "QtUtil.h"
#include <QFileInfo>
#include <QDir>

namespace Private_FileOperationsExecutor
{

class CDefaultFileOperationsExecutor : public IFileOperationsExecutor
{
protected:
	virtual void DoDelete(std::vector<std::unique_ptr<IFilesGroupProvider>> fileGroups) override
	{
		for (const auto& pFileGroup : fileGroups)
		{
			DoDelete(pFileGroup->GetFiles());
		}
	}

	virtual void DoDelete(const std::vector<string>& files) override
	{
		for (const auto& file : files)
		{
			auto const path = QtUtil::ToQString(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), file));
			QFileInfo info(path);
			if (info.isDir())
			{
				if (!QDir(path).removeRecursively())
				{
					GetISystem()->GetILog()->LogWarning("Can not remove folder %s", file.c_str());
				}
			} else if (!QFile::remove(path))
			{
				GetISystem()->GetILog()->LogWarning("Can not delete asset file %s", file.c_str());
			}
		}
	}
};

CDefaultFileOperationsExecutor g_defaultExecutor;

}

std::unique_ptr<IFileOperationsExecutor> CFileOperationExecutor::s_executor;

IFileOperationsExecutor* CFileOperationExecutor::GetExecutor()
{
	using namespace Private_FileOperationsExecutor;
	return s_executor ? s_executor.get() : &g_defaultExecutor;
}

IFileOperationsExecutor* CFileOperationExecutor::GetDefaultExecutor()
{
	using namespace Private_FileOperationsExecutor;
	return &g_defaultExecutor;
}
