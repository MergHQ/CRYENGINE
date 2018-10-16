// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlMenuBuilder.h"
#include "VersionControl/AssetsVCSSynchronizer.h"
#include "VersionControl/VersionControl.h"
#include "VersionControl/AssetsVCSStatusProvider.h"
#include "VersionControl/AssetsVCSReverter.h"
#include "VersionControl/AssetsVCSCheckerOut.h"
#include "VersionControl/AssetFilesProvider.h"
#include "AssetSystem/AssetManager.h"
#include "AssetSystem/Browser/AssetBrowser.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "VersionControlMainWindow.h"
#include "VersionControlSubmissionPopup.h"
#include "IObjectManager.h"
#include "Objects/IObjectLayerManager.h"
#include "Objects/IObjectLayer.h"

namespace Private_VersionControlMenuBuilder
{

bool IsLayerTracked(const IObjectLayer& layer)
{
	return !CAssetsVCSStatusProvider::HasStatus(layer, CVersionControlFileStatus::eState_NotTracked);
}

class CContextMenuAssetsStorage
{
	static std::vector<CAsset*> s_assets;
	static std::vector<string>  s_folders;

public:
	static bool Update(const std::vector<CAsset*>& assets, const std::vector<string>& folders)
	{
		s_assets.clear();
		s_folders.clear();

		std::copy_if(assets.cbegin(), assets.cend(), std::back_inserter(s_assets), [](CAsset* pAsset)
		{
			return pAsset->GetMetadataFile()[0] != '%';
		});

		for (const string& folder : folders)
		{
			if (folder[0] == '%')
			{
				continue;
			}
			s_folders.push_back(folder);
		}
		return !s_assets.empty() || !s_folders.empty();
	}

	const std::vector<CAsset*>& GetAssets() const { return s_assets; }
	const std::vector<string>& GetFolders() const { return s_folders; }

	bool IncludeAssetsInFolders()
	{
		for (const string& folder : s_folders)
		{
			const auto& folderAssets = CAssetManager::GetInstance()->GetAssetsFromDirectory(folder, [](CAsset* pAsset)
			{
				return pAsset->GetMetadataFile()[0] != '%';
			});
			s_assets.reserve(s_assets.size() + folderAssets.size());
			for (const auto& pAsset : folderAssets)
			{
				s_assets.push_back(pAsset);
			}
		}
		return !s_assets.empty();
	}

};

class CContextMenuLayersStorage
{
	static std::vector<IObjectLayer*> s_layers;
	static std::vector<string>        s_folders;

public:
	static bool Update(const std::vector<IObjectLayer*>& layers, const std::vector<IObjectLayer*>& layerFolders)
	{
		s_layers.clear();
		s_folders.clear();
		s_layers.reserve(layers.size());
		std::copy_if(layers.cbegin(), layers.cend(), std::back_inserter(s_layers), [](IObjectLayer* pLayer)
		{
			return IsLayerTracked(*pLayer);
		});

		for (IObjectLayer* pFolderLayer : layerFolders)
		{
			AddChildLayers(pFolderLayer);
		}

		if (s_layers.empty() && layers.empty())
		{
			IObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetIObjectLayerManager();
			s_layers = pLayerManager->GetLayers();
			s_layers.erase(std::remove_if(s_layers.begin(), s_layers.end(), [](IObjectLayer* pLayer)
			{
				return pLayer->IsFolder() || !IsLayerTracked(*pLayer);
			}), s_layers.end());
			s_folders = { PathUtil::MakeGamePath(GetIEditor()->GetLevelPath()) };
		}

		return !s_layers.empty();
	}

	const std::vector<IObjectLayer*>& GetLayers() const { return s_layers; }

	const std::vector<string>&        GetFolders() const { return s_folders; }

	std::vector<string> GetLayersFiles() const
	{
		return CAssetFilesProvider::GetForLayers(s_layers);
	}

private:
	static void AddChildLayers(IObjectLayer* pLayer)
	{
		const int numChildren = pLayer->GetChildCount();
		if (numChildren == 0 && IsLayerTracked(*pLayer))
		{
			s_layers.push_back(pLayer);
			return;
		}
		for (int i = 0 ; i < numChildren; ++i)
		{
			AddChildLayers(pLayer->GetChildIObjectLayer(i));
		}
	}
};

std::vector<CAsset*> CContextMenuAssetsStorage::s_assets;
std::vector<string> CContextMenuAssetsStorage::s_folders;
std::vector<string> CContextMenuLayersStorage::s_folders;
std::vector<IObjectLayer*> CContextMenuLayersStorage::s_layers;

void OnLevelExplorerContextMenu(CAbstractMenu& menu, const std::vector<CBaseObject*>&, const std::vector<IObjectLayer*>& layers, const std::vector<IObjectLayer*>& folders)
{
	if (!CContextMenuLayersStorage::Update(layers, folders))
	{
		return;
	}

	CContextMenuLayersStorage storage;

	const int vcsSection = menu.GetNextEmptySection();
	menu.SetSectionName(vcsSection, "Version Control");

	auto action = menu.CreateAction(QObject::tr("Refresh"), vcsSection);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CAssetsVCSStatusProvider::UpdateStatus(storage.GetLayers());
	});

	const bool isVCSOnline = CVersionControl::GetInstance().IsOnline();

	action = menu.CreateAction(QObject::tr("Get Latest"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CAssetsVCSSynchronizer::Sync(storage.GetLayers(), storage.GetFolders());
	});

	action = menu.CreateAction(QObject::tr("Check Out"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]() mutable
	{
		CAssetsVCSCheckerOut::CheckOut(storage.GetLayers());
	});

	action = menu.CreateAction(QObject::tr("Revert"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CAssetsVCSReverter::RevertLayers(storage.GetLayers());
	});

	action = menu.CreateAction(QObject::tr("Revert if Unchanged"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CVersionControl::GetInstance().ClearLocalState(CAssetFilesProvider::GetForLayers(storage.GetLayers()), {}, true);
	});

	action = menu.CreateAction(QObject::tr("Submit..."), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]() mutable
	{
		CVersionControlSubmissionPopup::ShowPopup({}, storage.GetLayersFiles());
	});
}

void OnAssetBrowserContextMenu(CAbstractMenu& menu, const std::vector<CAsset*>& assets, const std::vector<string>& folders)
{
	if (!CContextMenuAssetsStorage::Update(assets, folders))
	{
		return;
	}

	CContextMenuAssetsStorage storage;

	const int vcsSection = menu.GetNextEmptySection();
	menu.SetSectionName(vcsSection, "Version Control");

	auto action = menu.CreateAction(QObject::tr("Refresh"), vcsSection);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CAssetsVCSStatusProvider::UpdateStatus(storage.GetAssets(), storage.GetFolders());
	});

	const bool isVCSOnline = CVersionControl::GetInstance().IsOnline();

	action = menu.CreateAction(QObject::tr("Get Latest"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CAssetsVCSSynchronizer::Sync(storage.GetAssets(), storage.GetFolders());
	});

	action = menu.CreateAction(QObject::tr("Check Out"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]() mutable
	{
		if (storage.IncludeAssetsInFolders())
		{
			CAssetsVCSCheckerOut::CheckOut(storage.GetAssets());
		}
	});

	action = menu.CreateAction(QObject::tr("Revert"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CAssetsVCSReverter::RevertAssets(storage.GetAssets(), storage.GetFolders());
	});

	action = menu.CreateAction(QObject::tr("Revert if Unchanged"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CVersionControl::GetInstance().ClearLocalState(CAssetFilesProvider::GetForAssets(storage.GetAssets()),
			storage.GetFolders(), true);
	});

	action = menu.CreateAction(QObject::tr("Submit..."), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]() mutable
	{
		CVersionControlSubmissionPopup::ShowPopup(storage.GetAssets(), {}, storage.GetFolders());
	});
}

}

void CVersionControlMenuBuilder::Activate()
{
	using namespace Private_VersionControlMenuBuilder;

	auto id = reinterpret_cast<uintptr_t>(this);

	CAssetBrowser::s_signalContextMenuRequested.Connect(&OnAssetBrowserContextMenu, id);

	GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->signalContextMenuRequested.Connect(&OnLevelExplorerContextMenu, id);

}

void CVersionControlMenuBuilder::Deactivate()
{
	auto id = reinterpret_cast<uintptr_t>(this);

	CAssetBrowser::s_signalContextMenuRequested.DisconnectById(id);

	GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->signalContextMenuRequested.DisconnectById(id);
}
