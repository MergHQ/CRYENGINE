// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MaterialType.h"
#include "IEditorImpl.h"

#include "Material/MaterialManager.h"
#include "QT/Widgets/QPreviewWidget.h"
#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetEditor.h>
#include <Cry3DEngine/I3DEngine.h>
#include <PathUtils.h>
#include <ThreadingUtils.h>

REGISTER_ASSET_TYPE(CMaterialType)

// Detail attributes.
CItemModelAttribute CMaterialType::s_subMaterialCountAttribute("Sub Mtl count", &Attributes::s_intAttributeType, CItemModelAttribute::StartHidden);
CItemModelAttribute CMaterialType::s_textureCountAttribute("Texture count", &Attributes::s_intAttributeType, CItemModelAttribute::StartHidden);

CryIcon CMaterialType::GetIconInternal() const
{
	return CryIcon("icons:common/assets_material.ico");
}

void CMaterialType::GenerateThumbnail(const CAsset* pAsset) const
{
	ThreadingUtils::PostOnMainThread([=]()
	{
		CMaterialManager* pManager = GetIEditorImpl()->GetMaterialManager();
		if (!pManager)
			return;

		CRY_ASSERT(0 < pAsset->GetFilesCount());
		if (1 > pAsset->GetFilesCount())
			return;

		const char* materialFileName = pAsset->GetFile(0);
		if (!materialFileName || !*materialFileName)
			return;

		CMaterial* pMaterial = pManager->LoadMaterial(materialFileName);
		if (!pMaterial)
			return;

		if (!m_pPreviewWidget)
		{
			m_pPreviewWidget.reset(new QPreviewWidget());
			m_pPreviewWidget->SetGrid(false);
			m_pPreviewWidget->SetAxis(false);
			m_pPreviewWidget->EnableMaterialPrecaching(true);
			m_pPreviewWidget->LoadFile("%EDITOR%/Objects/mtlsphere.cgf");
			m_pPreviewWidget->SetCameraLookAt(1.6f, Vec3(0.1f, -1.0f, -0.1f));
			m_pPreviewWidget->setGeometry(0, 0, ASSET_THUMBNAIL_SIZE, ASSET_THUMBNAIL_SIZE);
		}
		m_pPreviewWidget->SetMaterial(pMaterial);

		const string thumbnailFileName = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), pAsset->GetThumbnailPath());

		m_pPreviewWidget->SavePreview(thumbnailFileName.c_str());
	}).get();
}

std::vector<CItemModelAttribute*> CMaterialType::GetDetails() const
{
	return
	{
		&s_subMaterialCountAttribute,
		&s_textureCountAttribute
	};
}

QVariant CMaterialType::GetDetailValue(const CAsset* pAsset, const CItemModelAttribute* pDetail) const
{
	if (pDetail == &s_subMaterialCountAttribute)
	{
		return GetVariantFromDetail(pAsset->GetDetailValue("subMaterialCount"), pDetail);
	}
	else if (pDetail == &s_textureCountAttribute)
	{
		return GetVariantFromDetail(pAsset->GetDetailValue("textureCount"), pDetail);
	}
	return QVariant();
}

CAssetEditor* CMaterialType::Edit(CAsset* pAsset) const
{
	if (!pAsset->GetFilesCount())
	{
		return nullptr;
	}

	return CAssetEditor::OpenAssetForEdit("Material Editor", pAsset);
}

bool CMaterialType::OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const
{
	string materialName;
	materialName.Format("%s%s", asset.GetFolder(), asset.GetName());
	CMaterial* newMaterial = GetIEditor()->GetMaterialManager()->CreateMaterial(materialName);
	if (!newMaterial->Save())
	{
		return false;
	}

	asset.AddFile( newMaterial->GetFilename(true).c_str() );
	return true;
}

bool CMaterialType::RenameAsset(CAsset* pAsset, const char* szNewName) const
{
	CMaterialManager* const pMaterialManager = GetIEditor()->GetMaterialManager();
	CRY_ASSERT(pMaterialManager);

	const string materialName = pMaterialManager->FilenameToMaterial(pAsset->GetFile(0));
	CMaterial* const pMaterial = static_cast<CMaterial*>(pMaterialManager->FindItemByName(materialName));

	if (!CAssetType::RenameAsset(pAsset, szNewName))
	{
		return false;
	}

	// Material is not loaded into the material manager.
	if (!pMaterial)
	{
		return true;
	}

	// The material manager is not case sensitive, so we need to manually handle cases when only the register has been changed.
	// Otherwise, the material editor will show the name of the old material until the editor is restarted.
	const string newMaterialName = pMaterialManager->FilenameToMaterial(pAsset->GetFile(0));
	if (materialName.CompareNoCase(newMaterialName) == 0)
	{
		const bool isModified = pAsset->IsModified();
		pMaterial->SetName(newMaterialName);
		pAsset->SetModified(isModified);
	}

	return true;
}

void CMaterialType::PreDeleteAssetFiles(const CAsset& asset) const
{
	CMaterialManager* const pMaterialManager = GetIEditor()->GetMaterialManager();
	CRY_ASSERT(pMaterialManager);

	const string materialName = pMaterialManager->FilenameToMaterial(asset.GetFile(0));
	IDataBaseItem* const pMaterial = pMaterialManager->FindItemByName(materialName);
	if (!pMaterial)
	{
		return;
	}
	pMaterialManager->DeleteItem(pMaterial);
}
