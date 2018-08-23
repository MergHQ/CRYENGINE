// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlInitializer.h"
#include "AssetsVCSSynchronizer.h"
#include "VersionControl.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetsVCSReverter.h"
#include "AssetsVCSCheckerOut.h"
#include "AssetFilesProvider.h"
#include "AssetSystem/AssetManager.h"
#include "AssetSystem/Browser/AssetModel.h"
#include "AssetSystem/Browser/AssetBrowser.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "Notifications/NotificationCenter.h"
#include "UI/VersionControlMainWindow.h"

namespace Private_VersionControlInitializer
{

class CContextMenuStorage
{
	static std::vector<CAsset*> s_assets;
	static std::vector<string> s_folders;

public:
	static bool Update(const std::vector<CAsset*>& assets, const std::vector<string>& folders)
	{
		s_assets.clear();
		s_folders.clear();

		s_assets.reserve(assets.size());
		s_folders.reserve(folders.size());
		std::copy_if(assets.cbegin(), assets.cend(), std::back_inserter(s_assets), [](CAsset* pAsset)
		{
			return pAsset->GetMetadataFile()[0] != '%' && strcmp(pAsset->GetType()->GetTypeName(), "Level") != 0;
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

	void IncludeAssetsInFolders()
	{
		for (const string& folder : s_folders)
		{
			const auto& folderAssets = CAssetManager::GetInstance()->GetAssetsFromDirectory(folder, [](CAsset* pAsset)
			{
				return pAsset->GetMetadataFile()[0] != '%' && strcmp(pAsset->GetType()->GetTypeName(), "Level") != 0;
			});
			s_assets.reserve(s_assets.size() + folderAssets.size());
			for (const auto& pAsset : folderAssets)
			{
				s_assets.push_back(pAsset);
			}
		}
	}

};

std::vector<CAsset*> CContextMenuStorage::s_assets;
std::vector<string> CContextMenuStorage::s_folders;

void ExecuteDownloadLatest()
{
	//CVersionControl::GetInstance().DownloadLatest();
}

void OnMenu(CAbstractMenu& menu)
{
	auto vcsMenu = menu.CreateMenu(QObject::tr("VCS"));

	int section = vcsMenu->GetNextEmptySection();
	vcsMenu->SetSectionName(section, "Publish");

	auto action = vcsMenu->CreateAction(QObject::tr("Download Latest"), section);
	QObject::connect(action, &QAction::triggered, ExecuteDownloadLatest);
}

void OnContextMenu(CAbstractMenu& menu, const std::vector<CAsset*>& assets, const std::vector<string>& folders)
{
	if (!CContextMenuStorage::Update(assets, folders))
	{
		return;
	}

	CContextMenuStorage storage;

	int vcsSection = menu.GetNextEmptySection();
	menu.SetSectionName(vcsSection, "Version Control");

	auto action = menu.CreateAction(QObject::tr("Refresh"), vcsSection);
	QObject::connect(action, &QAction::triggered, [storage]()
	{
		CAssetsVCSStatusProvider::UpdateStatus(storage.GetAssets(), storage.GetFolders());
	});

	bool isVCSOnline = CVersionControl::GetInstance().IsOnline();

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
		storage.IncludeAssetsInFolders();
		CAssetsVCSCheckerOut::CheckOutAssets(storage.GetAssets());
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
		CVersionControl::GetInstance().RevertUnchanged(CAssetFilesProvider::GetForAssets(storage.GetAssets()),
			storage.GetFolders(), false);
	});

	action = menu.CreateAction(QObject::tr("Submit..."), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, [storage]() mutable
	{
		GetIEditor()->OpenView("Version Control System");
		auto pPane = GetIEditor()->FindDockable("Version Control System");
		if (pPane)
		{
			storage.IncludeAssetsInFolders();
			static_cast<CVersionControlMainWindow*>(pPane)->SelectAssets(storage.GetAssets());
		}
	});
}

void NotifyAboutAddedFiles()
{
	std::vector<std::shared_ptr<const CVersionControlFileStatus>> fileStatuses;
	CVersionControl::GetInstance().GetFileStatuses([](const CVersionControlFileStatus& fs)
	{
		if (!fs.HasState(CVersionControlFileStatus::eState_AddedLocally))
		{
			return false;
		}
		return AssetLoader::IsMetadataFile(fs.GetFileName());
	}, fileStatuses);
	if (!fileStatuses.empty())
	{
		GetIEditor()->GetNotificationCenter()->ShowInfo("Version Control System notification", QString("%1 new assets were added").arg(fileStatuses.size()));
	}
}

void AddAllAssets()
{
	std::vector<CAsset*> allAssets;
	allAssets.reserve(CAssetManager::GetInstance()->GetAssetsCount());
	CAssetManager::GetInstance()->ForeachAsset([&allAssets](CAsset* pAsset)
	{
		if (pAsset->GetMetadataFile()[0] != '%' && strcmp(pAsset->GetType()->GetTypeName(), "Level") != 0)
		{
			allAssets.push_back(pAsset);
		}
	});
	CVersionControl::GetInstance().AddFiles(CAssetFilesProvider::GetForAssets(allAssets), false, [](const auto& result)
	{
		NotifyAboutAddedFiles();
	});
}

void UpdateStatusIfOnline()
{
	CVersionControl& vcs = CVersionControl::GetInstance();
	if (vcs.IsOnline() && !GetIEditor()->GetAssetManager()->IsScanning())
	{
		vcs.UpdateStatus();
		AddAllAssets();
	}
}

void OnAssetEdit(CAsset& asset)
{
	if ((asset.GetType()->GetTypeName(), "Level") != 0)
	{
		CVersionControl::GetInstance().EditFiles(CAssetFilesProvider::GetForAssets({ &asset }));
	}
}

void OnAssetsAdded(const std::vector<CAsset*>& assets)
{
	std::vector<CAsset*> noLevelAssets;
	noLevelAssets.reserve(assets.size());
	std::copy_if(assets.cbegin(), assets.cend(), std::back_inserter(noLevelAssets), [](CAsset* pAsset)
	{
		return pAsset->GetMetadataFile()[0] != '%' && strcmp(pAsset->GetType()->GetTypeName(), "Level") != 0;
	});
	if (!noLevelAssets.empty())
	{
		CVersionControl::GetInstance().AddFiles(CAssetFilesProvider::GetForAssets(noLevelAssets));
	}
}

void OnAssetChanged(CAsset& asset, int flags)
{
	if (CVersionControl::GetInstance().IsOnline() && (flags & eAssetChangeFlags_Files))
	{
		// TODO: this method needs to be called only when there are new files, but ATM flag eACF_Files is not set by AssetManager.
		CVersionControl::GetInstance().AddFiles(CAssetFilesProvider::GetForAssetWithoutMetafile(asset));
		// TODO: there should be a check if any files from the asset has been deleted
		// for this the old version of the cryasset needs to be compared with the new one.
		// in this case we definitely need to know that the asset was changed due to underlying list of files change.
	}
}

void AddAssetsThumbnailIconProvider()
{
	CAssetModel::GetInstance()->AddThumbnailIconProvider("vcs", [](const CAsset* pAsset)
	{
		if (!CVersionControl::GetInstance().IsOnline() || strcmp(pAsset->GetType()->GetTypeName(), "Level") == 0)
		{
			return QIcon();
		}

		using FS = CVersionControlFileStatus;

		static const int notLatestStatus = FS::eState_UpdatedRemotely | FS::eState_DeletedRemotely;
		static const int notTrackedStatus = FS::eState_NotTracked | FS::eState_DeletedLocally;

		int status = CAssetsVCSStatusProvider::GetStatus(*pAsset);
		if (status & notLatestStatus)
		{
			return QIcon("icons:VersionControl/not_latest.ico");
		}
		else if (status & FS::eState_CheckedOutRemotely)
		{
			return QIcon("icons:VersionControl/locked.ico");
		}
		else if (status & FS::eState_CheckedOutLocally)
		{
			return QIcon(status & FS::eState_ModifiedLocally ? "icons:VersionControl/checked_out_modified.ico"
				: "icons:VersionControl/checked_out.ico");
		}
		else if (status & FS::eState_AddedLocally)
		{
			return QIcon("icons:VersionControl/new.ico");
		}
		else if (status & notTrackedStatus)
		{
			return QIcon();
		}
		return QIcon("icons:VersionControl/latest.ico");
	});
}

void RemoveAssetsThunbnailIconProvider()
{
	CAssetModel::GetInstance()->RemoveThumbnailIconProvider("vcs");
}

class CEventsListener
{
public:
	void Activate()
	{
		using namespace Private_VersionControlInitializer;

		auto id = reinterpret_cast<uintptr_t>(this);

		auto pAssetManager = GetIEditor()->GetAssetManager();
		pAssetManager->signalAssetChanged.Connect(&OnAssetChanged, id);
		if (pAssetManager->IsScanning())
		{
			pAssetManager->signalScanningCompleted.Connect([id]()
			{
				GetIEditor()->GetAssetManager()->signalAfterAssetsInserted.Connect(&OnAssetsAdded, id);
				GetIEditor()->GetAssetManager()->signalScanningCompleted.DisconnectById(id);
				UpdateStatusIfOnline();
			}, id);
		}
		else
		{
			GetIEditor()->GetAssetManager()->signalAfterAssetsInserted.Connect(&OnAssetsAdded, id);
		}

		AddAssetsThumbnailIconProvider();

		CAssetBrowser::s_signalMenuCreated.Connect(&OnMenu, id);
		CAssetBrowser::s_signalContextMenuRequested.Connect(&OnContextMenu, id);

		CVersionControl::GetInstance().signalOnlineChanged.Connect(&UpdateStatusIfOnline, id);
	}

	void Deactivate()
	{
		auto id = reinterpret_cast<uintptr_t>(this);

		auto pAssetManager = GetIEditor()->GetAssetManager();
		pAssetManager->signalAfterAssetsInserted.DisconnectById(id);
		pAssetManager->signalAssetChanged.DisconnectById(id);
		pAssetManager->signalScanningCompleted.DisconnectById(id);

		RemoveAssetsThunbnailIconProvider();

		CAssetBrowser::s_signalMenuCreated.DisconnectById(id);
		CAssetBrowser::s_signalContextMenuRequested.DisconnectById(id);

		CVersionControl::GetInstance().signalOnlineChanged.DisconnectById(id);
	}
};

class CVCSStatusOperator : public Attributes::IAttributeFilterOperator
{
public:
	virtual QString GetName() override { return QWidget::tr(""); }

	virtual bool    Match(const QVariant& value, const QVariant& filterValue) override
	{
		auto filterVal = filterValue.toInt();
		return filterVal == 0 || value.toInt() & filterVal;
	}

	virtual QWidget* CreateEditWidget(std::shared_ptr<CAttributeFilter> filter) override
	{
		QMenuComboBox* cb = new QMenuComboBox();
		cb->SetMultiSelect(true);
		cb->SetCanHaveEmptySelection(true);

		cb->AddItem("added locally", CVersionControlFileStatus::eState_AddedLocally);
		cb->AddItem("modified locally", CVersionControlFileStatus::eState_ModifiedLocally);
		cb->AddItem("checked out locally", CVersionControlFileStatus::eState_CheckedOutLocally);
		cb->AddItem("checked out remotely", CVersionControlFileStatus::eState_CheckedOutRemotely);
		cb->AddItem("updated remotely", CVersionControlFileStatus::eState_UpdatedRemotely);
		cb->AddItem("deleted remotely", CVersionControlFileStatus::eState_DeletedRemotely);

		QVariant& val = filter->GetFilterValue();
		if (val.isValid())
		{
			bool ok = false;
			int valToI = val.toInt(&ok);
			if (ok)
				cb->SetChecked(valToI);
			else
			{
				QStringList items = val.toString().split(", ");
				cb->SetChecked(items);
			}
		}

		cb->signalItemChecked.Connect([cb, filter]()
		{
			const auto& indices = cb->GetCheckedIndices();
			int sum = 0;
			for (int index : indices)
			{
				sum += cb->GetData(index).toInt();
			}
			filter->SetFilterValue(sum);
		});

		return cb;
	}

	virtual void UpdateWidget(QWidget* widget, const QVariant& value) override
	{
		QMenuComboBox* combo = qobject_cast<QMenuComboBox*>(widget);
		if (combo)
		{
			combo->SetChecked(value.toStringList());
		}
	}
};

static CAttributeType<int> g_vcsStatusMaskAttributeType({ new CVCSStatusOperator() });

static CEventsListener     g_eventsListener;
static CItemModelAttribute g_vcsStatusAttribute("VC status", &Attributes::s_stringAttributeType, CItemModelAttribute::Visible, false);
static CItemModelAttribute g_vcsStatusMaskAttribute("VCS status", &g_vcsStatusMaskAttributeType, CItemModelAttribute::AlwaysHidden);

QString AssetStatusToString(int status)
{
	if (status == 0)
	{
		return "up to date";
	}
	QString str = "";
	if (status & CVersionControlFileStatus::eState_ModifiedLocally)
	{
		str = "* ";
	}
	if (status & CVersionControlFileStatus::eState_AddedLocally)
	{
		return "added locally";
	}
	else if (status & CVersionControlFileStatus::eState_CheckedOutLocally)
	{
		return str + "checked out locally";
	}
	else if (status & CVersionControlFileStatus::eState_CheckedOutRemotely)
	{
		if (status & CVersionControlFileStatus::eState_UpdatedRemotely)
		{
			return str + "updated and checked out remotely";
		}
		else
		{
			return str + "checked out remotely";
		}
	}
	else if (status & CVersionControlFileStatus::eState_UpdatedRemotely)
	{
		return str + "updated remotely";
	}
	else if (status & CVersionControlFileStatus::eState_DeletedRemotely)
	{
		return str + "deleted remotely";
	}
	else if (status & CVersionControlFileStatus::eState_AddedRemotely)
	{
		return "added remotely";
	}
	return "";
}

void AddAssetsStatusColumn()
{
	CAssetModel::GetInstance()->AddColumn(&g_vcsStatusAttribute, [](const CAsset* pAsset, const CItemModelAttribute* pAttribute, int role)
	{
		if (!CVersionControl::IsAvailable() || role != Qt::DisplayRole)
		{
			return QVariant();
		}

		return QVariant(AssetStatusToString(CAssetsVCSStatusProvider::GetStatus(*pAsset)));
	});

	CAssetModel::GetInstance()->AddColumn(&g_vcsStatusMaskAttribute, [](const CAsset* pAsset, const CItemModelAttribute* pAttribute, int role)
	{
		if (!CVersionControl::IsAvailable() || role != Qt::DisplayRole)
		{
			return QVariant();
		}

		return QVariant(CAssetsVCSStatusProvider::GetStatus(*pAsset));
	});
}

}

void CVersionControlInitializer::ActivateListeners()
{
	using namespace Private_VersionControlInitializer;
	g_eventsListener.Activate();
}

void CVersionControlInitializer::DeactivateListeners()
{
	using namespace Private_VersionControlInitializer;
	g_eventsListener.Deactivate();
}

void CVersionControlInitializer::Initialize()
{
	using namespace Private_VersionControlInitializer;
	static bool isInitialized = false;
	if (!isInitialized)
	{
		AddAssetsStatusColumn();
		isInitialized = true;
	}
}
