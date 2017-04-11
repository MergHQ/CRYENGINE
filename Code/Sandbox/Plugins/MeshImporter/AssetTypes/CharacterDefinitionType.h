// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

// Character Definition
// The character definition file is created in the Character Editor in Sandbox. It contains the reference to a .chr file and its attachments (can be .chr or .cgf).
class CCharacterDefinitionType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CCharacterDefinitionType);

	virtual char* GetTypeName() const { return "Character"; }
	virtual char* GetUiTypeName() const { return QT_TR_NOOP("Character"); }
	virtual bool IsImported() const { return true; }
	virtual bool CanBeEdited() const { return false; }
	virtual string GetTargetFilename(const string& sourceFilename) const override;
	virtual CryIcon GetIcon() const override;
	virtual CAsset* Import(const string&, CAssetImportContext& context) const override;
	virtual std::vector<string> GetImportExtensions() const override;
	virtual std::vector<string> GetImportDependencyTypeNames() const override;
};

