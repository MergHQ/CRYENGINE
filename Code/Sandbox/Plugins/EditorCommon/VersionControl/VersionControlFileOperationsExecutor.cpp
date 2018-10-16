// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "StdAfx.h"
#include "VersionControlFileOperationsExecutor.h"
#include "VersionControl.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetsVCSSynchronizer.h"
#include "AssetFilesProvider.h"
#include "AssetSystem/FileOperationsExecutor.h"
#include "AssetSystem/IFilesGroupProvider.h"
#include "FilePathUtil.h"
#include "ThreadingUtils.h"

namespace Private_VersionControlFileOperationsExecutor
{

class CSingleFileProvider : public IFilesGroupProvider
{
public:
	CSingleFileProvider(const string& file)
		: m_file(file)
	{}

	virtual std::vector<string> GetFiles(bool includeGeneratedFile = true) const override { return { m_file }; }

	virtual const string& GetMainFile() const override { return m_file; }

	virtual const string& GetName() const override { return m_file; }

private:
	string m_file;
};

void RevertAndDeletePhysically(std::vector<string> files)
{
	if (files.empty())
	{
		return;
	}
	ThreadingUtils::AsyncQueue([files = std::move(files)]() mutable
	{
		CFileOperationExecutor::GetDefaultExecutor()->Delete(files);
		CVersionControl::GetInstance().Revert(std::move(files));
	});
}

void DeleteOnlyFolders(const std::vector<string>& paths)
{
	std::vector<string> folders;
	for (const string& path : paths)
	{
		if (PathUtil::FolderExists(PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), path)))
		{
			folders.push_back(path);
		}
	}
	if (!folders.empty())
	{
		ThreadingUtils::AsyncQueue([folders = std::move(folders)]()
		{
			CFileOperationExecutor::GetDefaultExecutor()->Delete(folders);
		});
	}
}

void DeleteFoldersAndFiles(const IFilesGroupProvider& pFileGroup)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	auto files = pFileGroup.GetFiles(false);

	if (!pFileGroup.GetGeneratedFile().empty())
	{
		RevertAndDeletePhysically({ pFileGroup.GetGeneratedFile() });
	}

	for (string& file : files)
	{
		file = PathUtil::MatchGamePathToCaseOnFileSystem(file);
	}

	auto& vcs = CVersionControl::GetInstance();

	auto deleteCallback = [files, name = pFileGroup.GetName()](const auto& result) mutable
	{
		if (result.IsSuccess())
		{
			DeleteOnlyFolders(files);
		}
		else if (result.GetError().type == EVersionControlError::AlreadyCheckedOutByOthers)
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete %s because it's exclusively checked out", name);
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Can't delete %s", name);
		}
	};

	vcs.DeleteFiles(std::move(files), false, std::move(deleteCallback));
}

void DeleteTrackedFiles(const std::shared_ptr<IFilesGroupProvider>& pFileGroup, const std::shared_ptr<const CVersionControlFileStatus>& pFileStatus)
{
	using namespace Private_VersionControlFileOperationsExecutor;
	if (pFileStatus->HasState(CVersionControlFileStatus::eState_CheckedOutLocally))
	{
		CVersionControl::GetInstance().Revert(pFileGroup->GetFiles(false), {}, false, [pFileGroup](const auto& result) mutable
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
			auto files = CAssetFilesProvider::GetForFileGroup(*pFileGroup);
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

	std::vector<std::unique_ptr<IFilesGroupProvider>> fileProviders;
	fileProviders.reserve(files.size());
	for (const string& file : files)
	{
		fileProviders.push_back(std::make_unique<CSingleFileProvider>(file));
	}

	DoDelete(std::move(fileProviders));
}
