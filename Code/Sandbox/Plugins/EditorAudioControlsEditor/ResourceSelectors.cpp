// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "AssetsManager.h"
#include "ResourceSelectorDialog.h"

#include <IResourceSelectorHost.h>
#include <ListSelectionDialog.h>
#include <IEditor.h>

namespace ACE
{
SResourceSelectionResult ShowSelectDialog(SResourceSelectorContext const& context, char const* szPreviousValue, EAssetType controlType)
{
	CResourceSelectorDialog dialog(controlType, context.parentWidget);
	CResourceSelectorDialog::SResourceSelectionDialogResult dialogResult = dialog.ChooseItem(szPreviousValue);
	SResourceSelectionResult result{ dialogResult.selectionAccepted, dialogResult.selectedItem.c_str() };
	return result;
}

SResourceSelectionResult AudioTriggerSelector(SResourceSelectorContext const& context, char const* szPreviousValue)
{
	return ShowSelectDialog(context, szPreviousValue, EAssetType::Trigger);
}

SResourceSelectionResult AudioSwitchSelector(SResourceSelectorContext const& context, char const* szPreviousValue)
{
	return ShowSelectDialog(context, szPreviousValue, EAssetType::Switch);
}

SResourceSelectionResult AudioSwitchStateSelector(SResourceSelectorContext const& context, char const* szPreviousValue)
{
	return ShowSelectDialog(context, szPreviousValue, EAssetType::State);
}

SResourceSelectionResult AudioParameterSelector(SResourceSelectorContext const& context, char const* szPreviousValue)
{
	return ShowSelectDialog(context, szPreviousValue, EAssetType::Parameter);
}

SResourceSelectionResult AudioEnvironmentSelector(SResourceSelectorContext const& context, char const* szPreviousValue)
{
	return ShowSelectDialog(context, szPreviousValue, EAssetType::Environment);
}

SResourceSelectionResult AudioPreloadRequestSelector(SResourceSelectorContext const& context, char const* szPreviousValue)
{
	return ShowSelectDialog(context, szPreviousValue, EAssetType::Preload);
}

SResourceSelectionResult AudioSettingSelector(SResourceSelectorContext const& context, char const* szPreviousValue)
{
	return ShowSelectDialog(context, szPreviousValue, EAssetType::Setting);
}

REGISTER_RESOURCE_SELECTOR("AudioTrigger", AudioTriggerSelector, "")
REGISTER_RESOURCE_SELECTOR("AudioSwitch", AudioSwitchSelector, "")
REGISTER_RESOURCE_SELECTOR("AudioSwitchState", AudioSwitchStateSelector, "")
REGISTER_RESOURCE_SELECTOR("AudioRTPC", AudioParameterSelector, "")
REGISTER_RESOURCE_SELECTOR("AudioEnvironment", AudioEnvironmentSelector, "")
REGISTER_RESOURCE_SELECTOR("AudioPreloadRequest", AudioPreloadRequestSelector, "")
REGISTER_RESOURCE_SELECTOR("AudioSetting", AudioSettingSelector, "")
} // namespace ACE
