// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlInitializer.h"
#include "VersionControl.h"
#include "AssetsVCSStatusProvider.h"
#include "AssetFilesProvider.h"
#include "UI/VersionControlMenuBuilder.h"
#include "UI/VersionControlUIHelper.h"
#include "AssetSystem/AssetManager.h"
#include "AssetSystem/Browser/AssetModel.h"
#include "AssetSystem/Loader/AssetLoaderHelpers.h"
#include "AssetSystem/AssetManagerHelpers.h"
#include "Notifications/NotificationCenter.h"
#include "IObjectManager.h"
#include "Objects/IObjectLayerManager.h"
#include "Objects/IObjectLayer.h"
#include "ThreadingUtils.h"
#include "Controls/QMenuComboBox.h"
#include <CryString/CryPath.h>

namespace Private_VersionControlInitializer
{

CVersionControlMenuBuilder menuBuilder;

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
	std::vector<CAsset*> assets;
	assets.reserve(CAssetManager::GetInstance()->GetAssetsCount());
	CAssetManager::GetInstance()->ForeachAsset([&assets](CAsset* pAsset)
	{
		if (pAsset->GetMetadataFile()[0] != '%')
		{
			assets.push_back(pAsset);
		}
	});
	
	ThreadingUtils::Async([assets = std::move(assets)]() mutable
	{
		std::vector<string> metadataFiles = CAssetFilesProvider::GetForAssets(assets, CAssetFilesProvider::EInclude::OnlyMainFile);

		// filter out all files that are tracked leaving only not tracked.
		metadataFiles.erase(std::remove_if(metadataFiles.begin(), metadataFiles.end(), [](const string& metadataFile)
		{
			if (metadataFile.empty())
				return true;
			const auto& fs = CVersionControl::GetInstance().GetFileStatus(metadataFile);
			return fs && !fs->HasState(CVersionControlFileStatus::eState_NotTracked);
		}), metadataFiles.end());

		assets.clear();
		for (const string& metadataFile : metadataFiles)
		{
			assets.push_back(CAssetManager::GetInstance()->FindAssetForMetadata(metadataFile));
		}

		if (assets.empty())
		{
			return;
		}

		CVersionControl::GetInstance().AddFiles(CAssetFilesProvider::GetForAssets(assets), false, [](const auto& result)
		{
			NotifyAboutAddedFiles();
		});
	});
}

void UpdateStatusIfOnline()
{
	CVersionControl& vcs = CVersionControl::GetInstance();
	if (vcs.IsOnline() && !GetIEditor()->GetAssetManager()->IsScanning())
	{
		vcs.UpdateStatus(false, [](const auto& result)
		{
			AddAllAssets();
		});
	}
}

void OnAssetsAdded(const std::vector<CAsset*>& assets)
{
	CVersionControl::GetInstance().AddFiles(CAssetFilesProvider::GetForAssets(assets));
}

void OnAssetChanged(CAsset& asset, int flags)
{
	if (CVersionControl::GetInstance().IsOnline() && (flags & eAssetChangeFlags_Files))
	{
		// TODO: this method needs to be called only when there are new files, but ATM flag eACF_Files is not set by AssetManager.
		CVersionControl::GetInstance().AddFiles(CAssetFilesProvider::GetForAssets({ &asset }, CAssetFilesProvider::EInclude::AllButMainFile));
		// TODO: there should be a check if any files from the asset has been deleted
		// for this the old version of the cryasset needs to be compared with the new one.
		// in this case we definitely need to know that the asset was changed due to underlying list of files change.
	}
}

void OnLayerSaved(IObjectLayer& layer)
{
	if (CVersionControl::GetInstance().IsOnline())
	{
		const auto& layerFile = PathUtil::MakeGamePath(layer.GetLayerFilepath());
		if (CAssetsVCSStatusProvider::HasStatus(layerFile, CVersionControlFileStatus::eState_NotTracked))
		{
			CVersionControl::GetInstance().AddFiles(CAssetFilesProvider::GetForLayers({ &layer }));
		}
		else if (CAssetsVCSStatusProvider::HasStatus(layerFile, CVersionControlFileStatus::eState_DeletedLocally))
		{
			auto files = CAssetFilesProvider::GetForLayers({ &layer });
			CVersionControl::GetInstance().ClearLocalState(files, {}, false);
			CVersionControl::GetInstance().EditFiles(std::move(files));
		}
	}
}

void AddAssetsThumbnailIconProvider()
{
	CAssetModel::GetInstance()->AddThumbnailIconProvider("vcs", [](const CAsset* pAsset)
	{
		if (!CVersionControl::GetInstance().IsOnline())
		{
			return QIcon();
		}

		return VersionControlUIHelper::GetIconFromStatus(CAssetsVCSStatusProvider::GetStatus(*pAsset));
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

		GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->signalLayerSaved.Connect(&OnLayerSaved, id);

		AddAssetsThumbnailIconProvider();

		menuBuilder.Activate();

		CVersionControl::GetInstance().signalOnlineChanged.Connect(&UpdateStatusIfOnline, id);
	}

	void Deactivate()
	{
		auto id = reinterpret_cast<uintptr_t>(this);

		auto pAssetManager = GetIEditor()->GetAssetManager();
		pAssetManager->signalAfterAssetsInserted.DisconnectById(id);
		pAssetManager->signalAssetChanged.DisconnectById(id);
		pAssetManager->signalScanningCompleted.DisconnectById(id);

		GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->signalLayerSaved.DisconnectById(id);

		RemoveAssetsThunbnailIconProvider();

		menuBuilder.Deactivate();

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
		return filterVal == 0 || value.toInt() & filterVal || (value.toInt() == 0 && filterVal & UP_TO_DATE_VAL);
	}

	virtual QWidget* CreateEditWidget(std::shared_ptr<CAttributeFilter> pFilter) override
	{
		QMenuComboBox* pComboBox = new QMenuComboBox();
		pComboBox->SetMultiSelect(true);
		pComboBox->SetCanHaveEmptySelection(true);

		pComboBox->AddItem(QObject::tr("Added"), CVersionControlFileStatus::eState_AddedLocally);
		pComboBox->AddItem(QObject::tr("Checked out"), CVersionControlFileStatus::eState_CheckedOutLocally);
		pComboBox->AddItem(QObject::tr("Checked out & Modified"), CVersionControlFileStatus::eState_ModifiedLocally);
		pComboBox->AddItem(QObject::tr("Checked out remotely"), CVersionControlFileStatus::eState_CheckedOutRemotely);
		pComboBox->AddItem(QObject::tr("Updated remotely"), CVersionControlFileStatus::eState_UpdatedRemotely);
		pComboBox->AddItem(QObject::tr("Deleted remotely"), CVersionControlFileStatus::eState_DeletedRemotely);
		pComboBox->AddItem(QObject::tr("Up-to-Date"), UP_TO_DATE_VAL);

		QVariant& val = pFilter->GetFilterValue();
		if (val.isValid())
		{
			bool ok = false;
			int valToI = val.toInt(&ok);
			if (ok)
				pComboBox->SetChecked(valToI);
			else
			{
				QStringList items = val.toString().split(", ");
				pComboBox->SetChecked(items);
			}
		}

		pComboBox->signalItemChecked.Connect([this, pComboBox, pFilter]()
		{
			const auto& indices = pComboBox->GetCheckedIndices();
			int sum = 0;
			for (int index : indices)
			{
				sum += pComboBox->GetData(index).toInt();
			}
			pFilter->SetFilterValue(sum);
		});

		return pComboBox;
	}

	virtual void UpdateWidget(QWidget* widget, const QVariant& value) override
	{
		QMenuComboBox* combo = qobject_cast<QMenuComboBox*>(widget);
		if (combo)
		{
			combo->SetChecked(value.toStringList());
		}
	}

private:
	// This is sort of a hack because up-to-date state is represented by 0 bitmask.
	static constexpr int UP_TO_DATE_VAL = 1 << 31;
};

static CAttributeType<int> g_vcsStatusMaskAttributeType({ new CVCSStatusOperator() });
static CAttributeType<QIcon> g_iconAttributeType(nullptr);

static CEventsListener     g_eventsListener;
static CItemModelAttribute g_vcsStatusAttribute("VC status", &g_iconAttributeType, CItemModelAttribute::Visible, false);
static CItemModelAttribute g_vcsStatusMaskAttribute("VCS status", &g_vcsStatusMaskAttributeType, CItemModelAttribute::AlwaysHidden);

void AddAssetsStatusColumn()
{
	CAssetModel::GetInstance()->AddColumn(&g_vcsStatusAttribute, [](const CAsset* pAsset, const CItemModelAttribute* pAttribute, int role) -> QVariant
	{
		if (!CVersionControl::GetInstance().IsOnline() || role != Qt::DecorationRole)
		{
			return QVariant();
		} 

		return VersionControlUIHelper::GetIconFromStatus(CAssetsVCSStatusProvider::GetStatus(*pAsset));
	});

	CAssetModel::GetInstance()->AddColumn(&g_vcsStatusMaskAttribute, [](const CAsset* pAsset, const CItemModelAttribute* pAttribute, int role)
	{
		if (!CVersionControl::GetInstance().IsOnline() || role != Qt::DisplayRole)
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
