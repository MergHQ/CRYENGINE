// Copyright 2001-2016 Crytek GmbH. All rights reserved.
#include "StdAfx.h"
#include "AssetType.h"

#include "Asset.h"
#include "AssetImportContext.h"

#include "FilePathUtil.h"
#include "QtUtil.h"
#include "ProxyModels/ItemModelAttribute.h"

#include <CryString/CryPath.h>

QVariant CAssetType::GetVariantFromDetail(const string& detailValue, const CItemModelAttribute* pAttrib)
{
	CRY_ASSERT(pAttrib);
	const QVariant v(QtUtil::ToQString(detailValue));
	switch(pAttrib->GetType())
	{
	case eAttributeType_String:
		return v.value<QString>();
	case eAttributeType_Int:
		return v.value<int>();
	case eAttributeType_Float:
		return v.value<float>();
	case eAttributeType_Boolean:
		return v.value<bool>();
	case eAttributeType_Enum:
		return v.value<int>();
	default:
		CRY_ASSERT(0 && "Unknown attribute type");
		return v;
	}
}

CryIcon CAssetType::GetIcon() const
{
	return CryIcon("icons:General/Placeholder.ico");
}

QWidget* CAssetType::CreatePreviewWidget(CAsset* pAsset, QWidget* pParent /*= nullptr*/) const
{
	//default implementation will not work for all asset types, using the existing preview widget
	CRY_ASSERT(pAsset);
	if (pAsset->GetFilesCount() > 0)
	{
		QString file(pAsset->GetFile(0));
		if (!file.isEmpty())
		{
			return GetIEditor()->CreatePreviewWidget(file, pParent);
		}
	}

	return nullptr;
}

CAsset* CAssetType::DoImport(const string& filePath, CAssetImportContext& context) const
{
	return CanImport(filePath, context) ? Import(filePath, context) : nullptr;
}

bool CAssetType::CanImport(const string& filePath, CAssetImportContext& context) const
{
	const string absOutputDirectoryPath = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), context.GetOutputDirectoryPath());
	const string absOutputFilePath = PathUtil::Make(absOutputDirectoryPath, GetTargetFilename(context.GetInputFilePath()));
	// absOutputFilePath = dir/name.source_ext
	
	const string absAssetPath = absOutputFilePath + ".cryasset";
	// absAssetPath = dir/name.target_ext.cryasset

	// Test whether name.target_ext.cryasset already exists.
	// If name.target_ext.cryasset is allowed to be overwritten, all other output files are assumed to be over-writable, too.
	if (!context.CanWrite(absAssetPath))
	{
		CryWarning(VALIDATOR_MODULE_ASSETS, VALIDATOR_ERROR, "File '%s' already exists.", absAssetPath.c_str());
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

// Fallback asset type if the actual type is not registered.
class CCryAssetType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CCryAssetType);

	virtual char* GetTypeName() const { return "cryasset"; }
	virtual char* GetUiTypeName() const { return "cryasset"; }
	virtual bool IsImported() const { return false; }
	virtual bool CanBeEdited() const { return false; }
};
REGISTER_ASSET_TYPE(CCryAssetType)


