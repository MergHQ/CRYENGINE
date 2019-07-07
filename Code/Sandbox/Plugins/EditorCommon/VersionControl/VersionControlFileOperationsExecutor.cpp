// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "StdAfx.h"
#include "VersionControlFileOperationsExecutor.h"

#include "VersionControl.h"
#include "VersionControlPathUtils.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetsVCSSynchronizer.h"
#include "AssetFilesProvider.h"
#include "AssetSystem/FileOperationsExecutor.h"
#include "AssetSystem/IFilesGroupController.h"
#include "FileUtils.h"
#include "PathUtils.h"
#include "ThreadingUtils.h"

namespace Private_VersionControlFileOperationsExecutor
{

class CSingleFileController : public IFilesGroupController
{
public:
	CSingleFileController(const string& file)
		: m_file(file)
	{}

	virtual std::vector<string> GetFiles(bool includeGeneratedFile = true) const override { return { m_file }; }

	virtual const string& GetMainFile() const override { return m_file; }

	virtual const string& GetName() const override { return m_file; }

private:
	string m_file;
};

void RevertAndDeletePhysically(std::vector<string> files, std::function<void(void)> callback)
{
	if (files.empty())
	{
		callback();
		return;
	}

	CFileOperationExecutor::GetDefaultExecutor()->AsyncDelete(files, [files, callback = std::move(callback)]() mutable
	{
		CVersionControl::GetInstance().Revert(std::move(files), {}, false, [callback = std::move(callback)](const auto& result)
		{
			callback();
		});
	});
}

void DeleteOnlyFolders(const std::vector<string>& paths, std::function<void(void)> callback)
{
	std::vector<string> folders;
	for (const string& path : paths)
	{
		if (FileUtils::FolderExists(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), path)))
		{
			folders.push_back(path);
		}
	}
	if (!folders.empty())
	{
		CFileOperationExecutor::GetDefaultExecutor()->AsyncDelete(folders, callback);
	}
	else
	{
		callback();
	}
}

void DeleteFoldersAndFiles(const IFilesGroupController& pFileGroup, std::function<void(void)> callback)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	auto files = pFileGroup.GetFiles(false);

	if (!pFileGroup.GetGeneratedFile().empty())
	{
		RevertAndDeletePhysically({ pFileGroup.GetGeneratedFile() }, [] {});
	}

	VersionControlPathUtils::MatchCaseAndRemoveUnmatched(files);

	if (files.empty())
	{
		callback();
		return;
	}

	auto& vcs = CVersionControl::GetInstance();

	auto deleteCallback = [files, name = pFileGroup.GetName(), callback = std::move(callback)](const auto& result) mutable
	{
		if (result.IsSuccess())
		{
			DeleteOnlyFolders(files, std::move(callback));
			return;
		}

		if (result.GetError().type == EVersionControlError::AlreadyCheckedOutByOthers)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete %s because it's exclusively checked out", name);
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete %s", name);
		}
		callback();
	};

	vcs.DeleteFiles(std::move(files), false, std::move(deleteCallback));
}

void DeleteTrackedFiles(const std::shared_ptr<IFilesGroupController>& pFileGroup, std::function<void(void)> callback)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	if (CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_CheckedOutLocally))
	{
		CVersionControl::GetInstance().Revert(pFileGroup->GetFiles(false), {}, false
			, [pFileGroup, callback = std::move(callback)](const auto& result) mutable
		{
			pFileGroup->Update();
			DeleteFoldersAndFiles(*pFileGroup, std::move(callback));
		});
	}
	else
	{
		CAssetsVCSStatusProvider::UpdateStatus(*pFileGroup, [pFileGroup, callback = std::move(callback)]() mutable
		{
			if (CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_DeletedRemotely))
			{
				CAssetsVCSSynchronizer::Sync(pFileGroup, std::move(callback));
			}
			else if (CAssetsVCSStatusProvider::HasStatus(*pFileGroup, CVersionControlFileStatus::eState_UpdatedRemotely))
			{
				CAssetsVCSSynchronizer::Sync(pFileGroup, [pFileGroup, callback = std::move(callback)]() mutable
				{
					DeleteFoldersAndFiles(*pFileGroup, std::move(callback));
				});
			}
			else
			{
				DeleteFoldersAndFiles(*pFileGroup, std::move(callback));
			}
		});
	}
}

}

void CVersionControlFileOperationsExecutor::DoDelete(std::vector<std::unique_ptr<IFilesGroupController>> fileGroups,
	std::function<void(void)> callback)
{
	if (!callback)
	{
		callback = [] {};
	}

	using namespace Private_VersionControlFileOperationsExecutor;

	auto localFilesIt = std::partition(fileGroups.begin(), fileGroups.end(), [](const auto& pFileGroup)
	{
		return !CAssetsVCSStatusProvider::HasStatus(pFileGroup->GetMainFile(), CVersionControlFileStatus::eState_AddedLocally)
			&& !CAssetsVCSStatusProvider::HasStatus(pFileGroup->GetMainFile(), CVersionControlFileStatus::eState_NotTracked);
	});
	
	if (localFilesIt != fileGroups.end())
	{
		std::vector<string> locallyAddedFiles;
		for (auto it = localFilesIt; it != fileGroups.end(); ++it)
		{
			auto files = CAssetFilesProvider::GetForFileGroup(**it);
			locallyAddedFiles.reserve(locallyAddedFiles.size() + files.size());
			std::move(files.begin(), files.end(), std::back_inserter(locallyAddedFiles));
		}
		fileGroups.erase(localFilesIt, fileGroups.end());
		RevertAndDeletePhysically(std::move(locallyAddedFiles), fileGroups.empty() ? std::move(callback) : [] {});
	}

	for (std::unique_ptr<IFilesGroupController>& pFileGroup : fileGroups)
	{
		DeleteTrackedFiles(std::move(pFileGroup), pFileGroup == fileGroups.back() ? std::move(callback) : [] {});
	}
}

void CVersionControlFileOperationsExecutor::DoDelete(const std::vector<string>& files, 
	std::function<void(void)> callback)
{
	using namespace Private_VersionControlFileOperationsExecutor;

	std::vector<std::unique_ptr<IFilesGroupController>> fileProviders;
	fileProviders.reserve(files.size());
	for (const string& file : files)
	{
		fileProviders.push_back(std::make_unique<CSingleFileController>(file));
	}

	DoDelete(std::move(fileProviders), std::move(callback));
}
