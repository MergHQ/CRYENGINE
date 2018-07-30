// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "StdAfx.h"
#include "VersionControlFileOperationsExecutor.h"
#include "VersionControl.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetsVCSSynchronizer.h"
#include "AssetSystem/FileOperationsExecutor.h"
#include "AssetSystem/IFilesGroupProvider.h"
#include "FilePathUtil.h"

namespace Private_VersionControlFileOperationsExecutor
{

void RevertAndDeletePhysically(std::vector<string> files)
{
	if (files.empty())
	{
		return;
	}
	CFileOperationExecutor::GetDefaultExecutor()->Delete(files);
	CVersionControl::GetInstance().Revert(std::move(files));
}

void DeleteOnlyFolders(const std::vector<string>& files)
{
	for (const string& file : files)
	{
		QFileInfo info(QtUtil::ToQString(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), file)));
		if (info.isDir())
		{
			CFileOperationExecutor::GetDefaultExecutor()->Delete({ file });
		}
	}
}

void DeleteFiles(std::vector<string> files, string name)
{
	auto& vcs = CVersionControl::GetInstance();

	auto deleteCallback = [files, name](const auto& result) mutable
	{
		if (result.IsSuccess())
		{
			CVersionControl::GetInstance().SubmitFiles(std::move(files), "Deleted " + name);
		}
		else if (result.GetError().type == EVersionControlError::AlreadyCheckedOutByOthers)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete asset %s because it's exclusively checked out", name);
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete asset %s", name);
		}
	};

	vcs.DeleteFiles(std::move(files), false, std::move(deleteCallback));
}

void DeleteFoldersAndFiles(const IFilesGroupProvider& pFileGroup)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	auto files = pFileGroup.GetFiles();
	DeleteOnlyFolders(files);
	DeleteFiles(std::move(files), pFileGroup.GetName());
}

void DeleteTrackedFiles(const std::shared_ptr<IFilesGroupProvider>& pFileGroup, const std::shared_ptr<const CVersionControlFileStatus>& pFileStatus)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	if (pFileStatus->HasState(CVersionControlFileStatus::eState_CheckedOutLocally))
	{
		CVersionControl::GetInstance().Revert(pFileGroup->GetFiles(), {}, false, [pFileGroup](const auto& result) mutable
		{
			pFileGroup->Update();
			DeleteFoldersAndFiles(*pFileGroup);
		});
	}
	else
	{
		CAssetsVCSStatusProvider::UpdateStatus(*pFileGroup, [pFileGroup]()
		{
			if (CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_DeletedRemotely))
			{
				CAssetsVCSSynchronizer::Sync(pFileGroup);
			}
			else if (CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_UpdatedRemotely))
			{
				CAssetsVCSSynchronizer::Sync(pFileGroup, [pFileGroup]()
				{
					DeleteFoldersAndFiles(*pFileGroup);
				});
			}
			else
			{
				DeleteFoldersAndFiles(*pFileGroup);
			}
		});
	}
}

}

void CVersionControlFileOperationsExecutor::DoDelete(std::vector<std::unique_ptr<IFilesGroupProvider>> fileGroups)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	auto& vcs = CVersionControl::GetInstance();

	std::vector<string> locallyAddedFiles;
	for (std::unique_ptr<IFilesGroupProvider>& pFileGroup : fileGroups)
	{
		auto pFileStatus = vcs.GetFileStatus(pFileGroup->GetMainFile());
		if (!pFileStatus || pFileStatus->HasState(CVersionControlFileStatus::eState_AddedLocally))
		{
			auto files = pFileGroup->GetFiles();
			locallyAddedFiles.reserve(locallyAddedFiles.size() + files.size());
			std::move(files.begin(), files.end(), std::back_inserter(locallyAddedFiles));
		}
		else
		{
			DeleteTrackedFiles(std::move(pFileGroup), pFileStatus);
		}
	}

	RevertAndDeletePhysically(std::move(locallyAddedFiles));
}

void CVersionControlFileOperationsExecutor::DoDelete(const std::vector<string>& files)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	auto& vcs = CVersionControl::GetInstance();

	std::vector<string> locallyAddedFiles;
	for (const auto& file : files)
	{
		auto pFileStatus = vcs.GetFileStatus(file);
		if (!pFileStatus || pFileStatus->HasState(CVersionControlFileStatus::eState_AddedLocally))
		{
			locallyAddedFiles.push_back(file);
		}
		else
		{
			vcs.DeleteFiles({ file }, false, [file](const auto& result) mutable
			{
				if (result.IsSuccess())
				{
					string message = "Deleted file " + file;
					CVersionControl::GetInstance().SubmitFiles({ std::move(file) }, std::move(message));
				}
				else if (result.GetError().type == EVersionControlError::AlreadyCheckedOutByOthers)
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete file %s because it's exclusively checked out", file);
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete file %s", file);
				}
			});
		}
	}

	DeleteOnlyFolders(files);
	RevertAndDeletePhysically(std::move(locallyAddedFiles));
}
