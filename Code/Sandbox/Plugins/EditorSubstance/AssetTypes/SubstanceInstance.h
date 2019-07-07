// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

namespace EditorSubstance
{
namespace AssetTypes
{

class CSubstanceInstanceType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CSubstanceInstanceType);

	struct SSubstanceCreateParams : SCreateParams
	{
		string archiveName;
	};

	virtual const char*   GetTypeName() const override           { return "SubstanceInstance"; }
	virtual const char*   GetUiTypeName() const override         { return QT_TR_NOOP("Substance Instance"); }
	virtual bool          IsImported() const override            { return false; }
	virtual bool          CanBeEdited() const override           { return true; }
	virtual bool          CanBeCopied() const                    { return true; }
	virtual bool          CanAutoRepairMetadata() const override { return false; } //! The metadata file has built-in import options that can not be restored.
	virtual CryIcon       GetIcon() const override;
	virtual bool          HasThumbnail() const override          { return false; }
	virtual QColor        GetThumbnailColor() const override     { return QColor(79, 187, 185); }
	virtual const char*   GetFileExtension() const override      { return "crysub"; }

	virtual CAssetEditor* Edit(CAsset* asset) const override;
	virtual void          AppendContextMenuActions(const std::vector<CAsset*>& assets, CAbstractMenu* menu) const override;
	virtual bool          OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const override;
};
}
}
