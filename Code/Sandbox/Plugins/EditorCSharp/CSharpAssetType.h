// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

class CSharpSourcefileAssetType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CSharpSourcefileAssetType);

	virtual const char*   GetTypeName() const           { return "CSharpScript"; }
	virtual const char*   GetUiTypeName() const         { return QT_TR_NOOP("C# script"); }
	virtual const char*   GetFileExtension() const      { return "cs"; }
	virtual bool          CanBeCreated() const override { return true; }
	virtual bool          IsImported() const            { return false; }
	virtual bool          CanBeCopied() const           { return true; }
	virtual bool          CanBeEdited() const           { return true; }
	virtual CAssetEditor* Edit(CAsset* asset) const override;

private:
	virtual bool    OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const override;
	virtual CryIcon GetIconInternal() const override;
	string          GetCleanName(const string& name) const;
};
