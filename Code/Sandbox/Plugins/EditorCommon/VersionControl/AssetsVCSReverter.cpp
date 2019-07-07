// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "AssetsVCSReverter.h"

#include "AssetFilesProvider.h"
#include "DeletedWorkFilesStorage.h"
#include "AssetSystem/AssetManager.h"
#include "AssetSystem/FileOperationsExecutor.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "AssetsVCSStatusProvider.h"
#include "Controls/QuestionDialog.h"
#include "Objects/IObjectLayer.h"
#include "Objects/IObjectLayerManager.h"
#include "Objects/ObjectManager.h"
#include "PathUtils.h"
#include "VersionControl.h"

#include <QDialogButtonBox>
#include <algorithm>

namespace Private_AssetsVCSReverter
{

std::vector<CAsset*> FindAddedLocallyAssets(const std::vector<CAsset*>& assets, const std::vector<string>& folders = {})
{
	auto isAddedPred = [](CAsset* pAsset)
	{
		return CAssetsVCSStatusProvider::HasStatus(*pAsset, CVersionControlFileStatus::eState_AddedLocally);
	};

	std::vector<CAsset*> addedAssets;

	std::copy_if(assets.cbegin(), assets.cend(), std::back_inserter(addedAssets), isAddedPred);

	for (const string& folder : folders)
	{
		auto tmpAssets = CAssetManager::GetInstance()->GetAssetsFromDirectory(folder, isAddedPred);
		std::move(tmpAssets.begin(), tmpAssets.end(), std::back_inserter(addedAssets));
	}
	return addedAssets;
}

std::vector<IObjectLayer*> FindAddedLocallyLayers(const std::vector<IObjectLayer*>& layers)
{
	auto isAddedPred = [](IObjectLayer* pLayer)
	{
		return CAssetsVCSStatusProvider::HasStatus(PathUtil::MakeGamePath(pLayer->GetLayerFilepath())
			, CVersionControlFileStatus::eState_AddedLocally);
	};

	std::vector<IObjectLayer*> addedLayers;

	std::copy_if(layers.cbegin(), layers.cend(), std::back_inserter(addedLayers), isAddedPred);

	return addedLayers;
}

std::vector<IObjectLayer*> FilterNotAddedLayers(std::vector<IObjectLayer*>& addedLayers, const std::vector<IObjectLayer*>& layers)
{
	std::vector<IObjectLayer*> notAddedLayers;
	if (addedLayers.empty())
	{
		notAddedLayers = layers;
	}
	else
	{
		std::vector<IObjectLayer*> tmpLayers = layers;
		std::sort(tmpLayers.begin(), tmpLayers.end());
		std::sort(addedLayers.begin(), addedLayers.end());
		std::set_difference(tmpLayers.cbegin(), tmpLayers.cend(), addedLayers.cbegin(), addedLayers.cend(), std::back_inserter(notAddedLayers));
	}
	return notAddedLayers;
}

bool IsConfirmed(const QString& str, bool hasAdded, const QString& additionalInfo = "")
{
	auto question = !hasAdded ? QObject::tr("Are you sure you want to revert all changes to selected %1?").arg(str) :
		QObject::tr("Are you sure you want to revert all changes to selected %1? Some of them are added locally and will be deleted").arg(str);

	if (!additionalInfo.isEmpty())
	{
		question.append(QString("\n%1").arg(additionalInfo));
	}

	if (CQuestionDialog::SQuestion(QObject::tr("Revert %1").arg(str), question) != QDialogButtonBox::Yes)
	{
		return false;
	}
	return true;
}

void DoRevertLayers(const std::vector<IObjectLayer*>& layers, std::vector<IObjectLayer*>& addedLayers)
{
	CVersionControl::GetInstance().Revert(CAssetFilesProvider::GetForLayers(layers), {}, false
		, [layers = FilterNotAddedLayers(addedLayers, layers)](const auto& result)
	{
		IObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetIObjectLayerManager();
		for (IObjectLayer* pLayer : layers)
		{
			const auto& layersFullName = pLayer->GetFullName();
			pLayerManager->ReloadLayer(pLayer);
			pLayer = pLayerManager->FindLayerByFullName(layersFullName);
			pLayer->SetModified(false);
		}
	});

	if (addedLayers.empty())
	{
		return;
	}

	CFileOperationExecutor::GetDefaultExecutor()->AsyncDelete(CAssetFilesProvider::GetForLayers(addedLayers));

	for (IObjectLayer* pLayer : addedLayers)
	{
		GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->DeleteLayer(pLayer, true, false);
	}
}

void DoRevertAssets(const std::vector<CAsset*>& assets, const std::vector<string>& folders, std::vector<CAsset*> addedAssets)
{
	CVersionControl::GetInstance().Revert(CAssetFilesProvider::GetForAssets(assets), folders);

	if (!addedAssets.empty())
	{
		CAssetManager::GetInstance()->DeleteAssetsWithFiles(std::move(addedAssets));
	}
}

std::vector<CAsset*> ExtractAsset(std::vector<std::shared_ptr<IFilesGroupController>>& filesGroups)
{
	std::vector<CAsset*> assets;
	auto it = std::partition(filesGroups.begin(), filesGroups.end(), [](const auto& pGroup)
	{
		return !AssetLoader::IsMetadataFile(pGroup->GetMainFile());
	});
	std::transform(it, filesGroups.end(), std::back_inserter(assets), [](const auto& pGroup)
	{
		CAsset* pAsset = CAssetManager::GetInstance()->FindAssetForMetadata(pGroup->GetMainFile());
		CRY_ASSERT_MESSAGE(pAsset, "Couldn't find the asset for metadata file %s", pGroup->GetMainFile().c_str());
		return pAsset;
	});
	filesGroups.erase(it, filesGroups.end());
	return assets;
}

std::vector<IObjectLayer*> ExtractLayersOfOpenedLevel(std::vector<std::shared_ptr<IFilesGroupController>>& filesGroups)
{
	std::vector<IObjectLayer*> layers;
	IObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetIObjectLayerManager();
	auto it = std::partition(filesGroups.begin(), filesGroups.end(), [&layers, pLayerManager](const auto& pGroup)
	{
		auto pLayer = pLayerManager->GetLayerByFileIfOpened(pGroup->GetMainFile());
		if (pLayer)
		{
			layers.push_back(pLayer);
		}
		return pLayer == nullptr;
	});
	filesGroups.erase(it, filesGroups.end());
	return layers;
}

std::vector<std::shared_ptr<IFilesGroupController>> ExtractDeleted(std::vector<std::shared_ptr<IFilesGroupController>>& filesGroups)
{
	std::vector<std::shared_ptr<IFilesGroupController>> deletedFilesGroups;
	auto it = std::partition(filesGroups.begin(), filesGroups.end(), [](const auto& pGroup)
	{
		return !CAssetsVCSStatusProvider::HasStatus(pGroup->GetMainFile(), CVersionControlFileStatus::eState_DeletedLocally);
	});
	std::move(it, filesGroups.end(), std::back_inserter(deletedFilesGroups));
	filesGroups.erase(it, filesGroups.end());
	return deletedFilesGroups;
}

std::vector<string> FilterLayersFiles(const std::vector<std::shared_ptr<IFilesGroupController>>& filesGroups)
{
	std::vector<string> layersFiles;
	for (const auto& pGroup : filesGroups)
	{
		if (!AssetLoader::IsMetadataFile(pGroup->GetMainFile()))
		{
			layersFiles.push_back(pGroup->GetMainFile());
		}
	}
	return layersFiles;
}

void TransformToFiles(const std::vector<std::shared_ptr<IFilesGroupController>>& filesGroups, std::vector<string>& files, std::vector<string>& addedFiles)
{
	for (const auto& pGroup : filesGroups)
	{
		std::vector<string> groupFiles = pGroup->GetFiles(false);
		if (CAssetsVCSStatusProvider::HasStatus(pGroup->GetMainFile(), CVersionControlFileStatus::eState_AddedLocally))
		{
			std::copy(groupFiles.cbegin(), groupFiles.cend(), std::back_inserter(addedFiles));
		}
		std::move(groupFiles.begin(), groupFiles.end(), std::back_inserter(files));
	}
}

void RevertDeletedGroups(const std::vector<std::shared_ptr<IFilesGroupController>>& deletedFilesGroups)
{
	if (deletedFilesGroups.empty())
	{
		return;
	}

	std::vector<string> deletedFiles;
	deletedFiles.reserve(deletedFilesGroups.size() * 3);
	std::vector<string> deletedLayersFiles = FilterLayersFiles(deletedFilesGroups);
	for (const auto& pGroup : deletedFilesGroups)
	{
		std::vector<string> files = pGroup->GetFiles(false);
		std::move(files.begin(), files.end(), std::back_inserter(deletedFiles));
	}
	CVersionControl::GetInstance().Revert(std::move(deletedFiles), {}, false
		, [deletedLayersFiles = std::move(deletedLayersFiles)](const auto& result)
	{
		if (!result.IsSuccess())
		{
			return;
		}

		for (const auto& fs : result.GetStatusChanges())
		{
			CDeletedWorkFilesStorage::GetInstance().Remove(fs.GetFileName());
		}
		CDeletedWorkFilesStorage::GetInstance().Save();

		IObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetIObjectLayerManager();
		for (const string& deletedLayersFile : deletedLayersFiles)
		{
			if (pLayerManager->IsLayerFileOfOpenedLevel(deletedLayersFile))
			{
				if (IObjectLayer* pLayer = pLayerManager->GetLayerByFileIfOpened(deletedLayersFile))
				{
					pLayerManager->DeleteLayer(pLayer);
				}
				pLayerManager->ImportLayerFromFile(PathUtil::Make(PathUtil::GetGameProjectAssetsRelativePath(), deletedLayersFile));
			}
		}
	});
}

bool HasLayersWithSameNameInOpenedLevel(const std::vector<std::shared_ptr<IFilesGroupController>>& filesGroups)
{
	for (const auto& pGroup : filesGroups)
	{
		if (!AssetLoader::IsMetadataFile(pGroup->GetMainFile()))
		{
			if (GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->GetLayerByFileIfOpened(pGroup->GetMainFile()))
			{
				return true;
			}
		}
	}
	return false;
}

QString GetConfirmString(const std::vector<std::shared_ptr<IFilesGroupController>>& filesGroups)
{
	bool hasAssets = false;
	bool hasLayers = false;
	for (const auto& pGroup : filesGroups)
	{
		if (AssetLoader::IsMetadataFile(pGroup->GetMainFile()))
		{
			hasAssets = true;
		}
		else
		{
			hasLayers = true;
		}
		if (hasAssets && hasLayers)
		{
			break;
		}
	}
	if (!hasAssets)
	{
		return QObject::tr("layers");
	}
	if (!hasLayers)
	{
		return QObject::tr("assets");
	}
	return QObject::tr("assets and layers");
}

}

void CAssetsVCSReverter::RevertAssets(const std::vector<CAsset*>& assets, const std::vector<string>& folders)
{
	using namespace Private_AssetsVCSReverter;

	if (assets.empty() && folders.empty())
	{
		return;
	}

	std::vector<CAsset*> addedAssets = FindAddedLocallyAssets(assets, folders);

	if (!IsConfirmed(QObject::tr("assets"), !addedAssets.empty()))
	{
		return;
	}

	DoRevertAssets(assets, folders, std::move(addedAssets));
}

void CAssetsVCSReverter::RevertLayers(const std::vector<IObjectLayer*>& layers)
{
	using namespace Private_AssetsVCSReverter;

	if (layers.empty())
	{
		return;
	}

	std::vector<IObjectLayer*> addedLayers = FindAddedLocallyLayers(layers);

	if (!IsConfirmed(QObject::tr("layers"), !addedLayers.empty()))
	{
		return;
	}

	DoRevertLayers(layers, addedLayers);
}

void CAssetsVCSReverter::Revert(std::vector<std::shared_ptr<IFilesGroupController>> filesGroups)
{
	using namespace Private_AssetsVCSReverter;

	QString confirmString = GetConfirmString(filesGroups);
	std::vector<std::shared_ptr<IFilesGroupController>> deletedFilesGroups = ExtractDeleted(filesGroups);
	std::vector<CAsset*> assets = ExtractAsset(filesGroups);
	std::vector<IObjectLayer*> layers = ExtractLayersOfOpenedLevel(filesGroups);

	std::vector<string> layersFiles;
	std::vector<string> addedLayerFiles;
	// only not opened layers are left after filtering.
	TransformToFiles(filesGroups, layersFiles, addedLayerFiles);
	filesGroups.clear();

	std::vector<IObjectLayer*> addedLayers = FindAddedLocallyLayers(layers);
	std::vector<CAsset*> addedAssets = FindAddedLocallyAssets(assets);

	const bool hasAddedItem = !addedLayerFiles.empty() || !addedLayers.empty() || !addedAssets.empty();
	const bool willOverrideLayer = HasLayersWithSameNameInOpenedLevel(deletedFilesGroups);
	if (!IsConfirmed(confirmString, hasAddedItem, willOverrideLayer ? QObject::tr("Some layers with the same name exist in the current level and will be overridden.") : ""))
	{
		return;
	}

	if (!layers.empty())
	{
		DoRevertLayers(layers, addedLayers);
	}

	if (!assets.empty())
	{
		DoRevertAssets(assets, {}, std::move(addedAssets));
	}

	RevertDeletedGroups(deletedFilesGroups);

	if (layersFiles.empty())
	{
		return;
	}

	CVersionControl::GetInstance().Revert(std::move(layersFiles));
	
	if (!addedLayerFiles.empty())
	{
		CFileOperationExecutor::GetDefaultExecutor()->AsyncDelete(addedLayerFiles);
	}
}
