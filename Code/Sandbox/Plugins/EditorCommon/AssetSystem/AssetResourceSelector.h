// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "IResourceSelectorHost.h"
#include "AssetSystem/Browser/AssetBrowserDialog.h"

class CAssetType;

//! Asset based resource picker
//! A picker is registered automatically for every asset type unless otherwise specified, i.e. CAssetType::IsUsingGenericPropertyTreePicker returns false.
struct EDITOR_COMMON_API SStaticAssetSelectorEntry : public SStaticResourceSelectorEntry
{
	SStaticAssetSelectorEntry(const char* typeName);
	SStaticAssetSelectorEntry(const char* typeName, const std::vector<const char*>& typeNames);
	SStaticAssetSelectorEntry(const char* typeName, const std::vector<string>& typeNames);

	const std::vector<const CAssetType*>& GetAssetTypes() const;
	const std::vector<string>&            GetAssetTypeNames() const { return assetTypeNames; }

	virtual bool                          ShowTooltip(const SResourceSelectorContext& context, const char* value) const override;
	virtual void                          HideTooltip(const SResourceSelectorContext& context, const char* value) const override;

	//This helper method is here for special selectors that cannot use SStaticAssetSelectorEntry directly
	static SResourceSelectionResult SelectFromAsset(const SResourceSelectorContext& context, const std::vector<string>& types, const char* previousValue);

private:
	std::vector<string>            assetTypeNames;
	std::vector<const CAssetType*> assetTypes;
};

//! Spawn an asset selection dialog and stores the results of the operation
class EDITOR_COMMON_API CAssetSelector
{
public:
	//! The asset types this context will show in the dialog
	CAssetSelector(const std::vector<string>& assetTypeNames);

	//! Spawns an asset selection dialog and returns if the selection was selected or not
	//! \param onValueChangedCallback a lambda called every time the asset selection is changed
	//! \param types used in the lookup process for previousValue
	//! \param previousValue the asset that will be selected when the selection dialog opens
	bool Execute(std::function<void(const char* newValue)> onValueChangedCallback, const std::vector<string>& types, const char* previousValue);

	//! Spawns an asset selection dialog and returns if the selection was selected or not
	//! \param context contains selection changed callbacks and other contextual info
	//! \param types used in the lookup process for previousValue
	//! \param previousValue the asset that will be selected when the selection dialog opens
	bool Execute(const SResourceSelectorContext& context, const std::vector<string>& types, const char* previousValue);

	//! Returns the currently selected asset if we have one
	CAsset* GetSelectedAsset() { return m_dialog.GetSelectedAsset(); }

private:
	CAssetBrowserDialog m_dialog;
};

//Register a picker type that accept several asset types
//Usage REGISTER_RESOURCE_SELECTOR_ASSET_MULTIPLETYPES("Picker", std::vector<const char*>({ "type1", "type2", ...}))
#define REGISTER_RESOURCE_SELECTOR_ASSET_MULTIPLETYPES(name, asset_types)                                                                      \
	namespace Private_ResourceSelector { SStaticAssetSelectorEntry INTERNAL_RSH_COMBINE(selector_ ## function, __LINE__)((name), (asset_types)); \
	                                     INTERNAL_REGISTER_RESOURCE_SELECTOR(INTERNAL_RSH_COMBINE(selector_ ## function, __LINE__)) }