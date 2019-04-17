// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControl/VersionControlEventHandler.h"
#include "VersionControl/AssetsVCSStatusProvider.h"
#include "VersionControl/AssetsVCSSynchronizer.h"
#include "VersionControl/AssetsVCSCheckerOut.h"
#include "VersionControl/AssetsVCSReverter.h"
#include "VersionControl/VersionControl.h"
#include "VersionControl/AssetFilesProvider.h"
#include "VersionControl/VersionControlPathUtils.h"
#include "VersionControl/UI/VersionControlSubmissionPopup.h"

#include "AssetSystem/Browser/AssetBrowser.h"
#include "AssetSystem/AssetManager.h"

#include "Objects/IObjectLayer.h"
#include "Objects/IObjectLayerManager.h"
#include "PathUtils.h"

#include "Notifications/NotificationCenter.h"

#include "CrySystem/ISystem.h"
#include "Controls/QuestionDialog.h"

namespace Private_VersionControlEvenHandler
{

void InsertAssetsFromFolders(const std::vector<string>& folders, std::vector<CAsset*>& assets)
{
	for (const string& folder : folders)
	{
		const auto& folderAssets = CAssetManager::GetInstance()->GetAssetsFromDirectory(folder, [](CAsset* pAsset)
		{
			return pAsset->GetMetadataFile()[0] != '%';
		});
		assets.reserve(assets.size() + folderAssets.size());
		for (const auto& pAsset : folderAssets)
		{
			assets.push_back(pAsset);
		}
	}
}

std::vector<IObjectLayer*> GetTrackedIObjectLayers(const std::vector<IObjectLayer*>& layers)
{
	std::vector<IObjectLayer*> result;
	std::copy_if(layers.cbegin(), layers.cend(), std::back_inserter(result), [](IObjectLayer* pLayer)
	{
		return !CAssetsVCSStatusProvider::HasStatus(*pLayer, CVersionControlFileStatus::eState_NotTracked);
	});
	return result;
}

std::vector<string> ToSelectedFolders(const std::vector<IObjectLayer*>& layerFolders, bool hasSelectedLayer)
{
	std::vector<string> result;
	result.reserve(layerFolders.size());
	for (IObjectLayer* pFolderLayer : layerFolders)
	{
		result.push_back(PathUtil::MakeGamePath(pFolderLayer->GetLayerFilepath()));
	}

	if (result.empty() && !hasSelectedLayer)
	{
		result = { PathUtil::MakeGamePath(GetIEditor()->GetLevelPath()) };
	}

	return result;
}

}

namespace VersionControlEventHandler
{

void HandleOnAssetBrowser(const QString& event, std::vector<CAsset*> assets, std::vector<string> folders)
{
	assets.erase(std::remove_if(assets.begin(), assets.end(), [](CAsset* pAsset)
	{
		return pAsset->GetMetadataFile()[0] == '%';
	}), assets.end());

	folders.erase(std::remove_if(folders.begin(), folders.end(), [](const string& folder)
	{
		return folder[0] == '%';
	}), folders.end());

	if (assets.empty() && folders.empty())
	{
		GetIEditor()->GetNotificationCenter()->ShowInfo("Can't execute command",
			QString("Command %1.%2 can't be executed because no asset is selected").arg("version_control_system", event));
		return;
	}

	if (event == "refresh")
	{
		CAssetsVCSStatusProvider::UpdateStatus(assets, folders);
	}
	else if (event == "get_latest")
	{
		CAssetsVCSSynchronizer::Sync(assets, folders);
	}
	else if (event == "check_out")
	{
		Private_VersionControlEvenHandler::InsertAssetsFromFolders(folders, assets);
		CAssetsVCSCheckerOut::CheckOut(assets);
	}
	else if (event == "revert")
	{
		CAssetsVCSReverter::RevertAssets(assets, folders);
	}
	else if (event == "revert_unchanged")
	{
		CVersionControl::GetInstance().ClearLocalState(CAssetFilesProvider::GetForAssets(assets), folders, true);
	}
	else if (event == "submit")
	{
		CVersionControlSubmissionPopup::ShowPopup(assets, folders);
	}
	else if (assets.size() == 1 && folders.empty() && !assets[0]->GetWorkFiles().empty())
	{
		std::vector<string> workFiles = assets[0]->GetWorkFiles();
		VersionControlPathUtils::MatchCaseAndRemoveUnmatched(workFiles);
		if (workFiles.empty())
		{
			return;
		}
		if (event == "refresh_work_files")
		{
			CVersionControl::GetInstance().UpdateFileStatus(workFiles);
		}
		else if (event == "get_latest_work_files")
		{
			CVersionControl::GetInstance().GetLatest(workFiles, {});
		}
		else if (event == "check_out_work_files")
		{
			CVersionControl::GetInstance().EditFiles(workFiles);
		}
		else if (event == "revert_work_files")
		{
			CVersionControl::GetInstance().Revert(workFiles);
		}
		else if (event == "revert_unchanged_work_files")
		{
			CVersionControl::GetInstance().ClearLocalState(workFiles, {}, true);
		}
		else if (event == "submit_work_files")
		{
			CVersionControlSubmissionPopup::ShowPopup(workFiles);
		}
		else if (event == "remove_local_work_files")
		{
			if (CQuestionDialog::SQuestion(QObject::tr("Warning"),
				QObject::tr("Are you sure you want to remove local work files?")) == QDialogButtonBox::Yes)
			{
				CVersionControl::GetInstance().RemoveFilesLocally(workFiles);
			}
		}
	}
}

void HandleOnLevelExplorer(const QString& event, std::vector<IObjectLayer*> layers, std::vector<IObjectLayer*> layerFolders)
{
	using namespace Private_VersionControlEvenHandler;
	if (event == "refresh")
	{
		CAssetsVCSStatusProvider::UpdateStatus(GetTrackedIObjectLayers(layers));
	}
	else if (event == "get_latest")
	{
		CAssetsVCSSynchronizer::Sync(GetTrackedIObjectLayers(layers), ToSelectedFolders(
			layerFolders, !layers.empty()));
	}
	else if (event == "check_out")
	{
		CAssetsVCSCheckerOut::CheckOut(GetTrackedIObjectLayers(layers));
	}
	else if (event == "revert")
	{
		CAssetsVCSReverter::RevertLayers(GetTrackedIObjectLayers(layers));
	}
	else if (event == "revert_unchanged")
	{
		CVersionControl::GetInstance().ClearLocalState(CAssetFilesProvider::GetForLayers(
			GetTrackedIObjectLayers(layers)), {}, true);
	}
	else if (event == "submit")
	{
		CVersionControlSubmissionPopup::ShowPopup(CAssetFilesProvider::GetForLayers(
			GetTrackedIObjectLayers(layers)), ToSelectedFolders(layerFolders, !layers.empty()));
	}
}

}
