// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

namespace CrySchematycEditor {

class CEntityAssetType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CEntityAssetType);

	static const char* TypeName() { return "SchematycEntity"; }

	// CAssetType
	virtual const char*   GetTypeName() const override      { return TypeName(); }
	virtual const char*   GetUiTypeName() const override    { return QT_TR_NOOP("Schematyc Entity"); }

	virtual const char*   GetFileExtension() const override { return "schematyc_ent"; }

	virtual bool          CanBeCreated() const override     { return true; }
	virtual bool          IsImported() const override       { return false; }
	virtual bool          CanBeEdited() const override      { return true; }

	virtual CAssetEditor* Edit(CAsset* pAsset) const override;
	virtual bool          DeleteAssetFiles(const CAsset& asset, bool bDeleteSourceFile, size_t& numberOfFilesDeleted) const;

	virtual const char*   GetObjectClassName() const { return TypeName(); }

protected:
	virtual bool OnCreate(CEditableAsset& editAsset) const override;

private:
	virtual CryIcon GetIconInternal() const override;
	// ~CAssetType
};

}
