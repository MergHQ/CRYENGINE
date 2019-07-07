// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SubstanceInstance.h"

#include "SandboxPlugin.h"
#include "EditorSubstanceManager.h"

#include <AssetSystem/AssetEditor.h>
#include <AssetSystem/AssetImportContext.h>
#include <AssetSystem/EditableAsset.h>
#include <PathUtils.h>
#include <ThreadingUtils.h>

namespace EditorSubstance
{
	namespace AssetTypes
	{
		REGISTER_ASSET_TYPE(CSubstanceInstanceType)
		CAssetEditor* CSubstanceInstanceType::Edit(CAsset* asset) const
		{
			return CAssetEditor::OpenAssetForEdit("Substance Instance Editor", asset);
		}

		CryIcon CSubstanceInstanceType::GetIcon() const
		{
			return CryIcon("icons:substance/instance.ico");
		}

		void CSubstanceInstanceType::AppendContextMenuActions(const std::vector<CAsset*>& assets, CAbstractMenu* menu) const
		{
			// todo support more instances selected
			if (assets.size() == 1)
			{
				EditorSubstance::CManager::Instance()->AddSubstanceInstanceContextMenu(assets[0], menu);
			}
		}

		bool CSubstanceInstanceType::OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const
		{
			const string dataFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());
			asset.AddFile(dataFilePath);

			CRY_ASSERT_MESSAGE(pCreateParams, __FUNCTION__" expects a valid pCreateParams");
			if (pCreateParams)
			{
				const string& archiveName = static_cast<const SSubstanceCreateParams*>(pCreateParams)->archiveName;
				asset.SetDependencies({ {archiveName, 1} });
			}
			return true;
		}

	}
}
