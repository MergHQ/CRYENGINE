// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>
#include "SoundAssetType.h"
#include "IEditorImpl.h"

#include "AssetSystem/AssetResourceSelector.h"
#include <Util/FileUtil.h>
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CryString/CryPath.h>

REGISTER_ASSET_TYPE(CSoundType)

// ---------------------------------------------------------------------------

SResourceSelectionResult SoundFileSelector(const SResourceSelectorContext& context, const char* previousValue)
{
	SResourceSelectionResult result{ false, previousValue };

	// start in Sounds folder if no sound is selected
	string startPath = PathUtil::GetPathWithoutFilename(previousValue);
	if (previousValue[0] == '\0')
		startPath = CRY_AUDIO_DATA_ROOT "/";

	string relativeFilename = previousValue;
	if (CFileUtil::SelectSingleFile(context.parentWidget, EFILE_TYPE_SOUND, relativeFilename, "", startPath))
	{
		result.selectedResource = relativeFilename.GetBuffer();
		result.selectionAccepted = true;
	}

	return result;

}
SResourceSelectionResult SoundAssetSelector(const SResourceSelectorContext& context, const char* previousValue)
{
	return SStaticAssetSelectorEntry::SelectFromAsset(context, { "Sound" }, previousValue);
}
SResourceSelectionResult SoundSelector(const SResourceSelectorContext& context, const char* previousValue)
{
	EAssetResourcePickerState state = (EAssetResourcePickerState)GetIEditorImpl()->GetSystem()->GetIConsole()->GetCVar("ed_enableAssetPickers")->GetIVal();
	if (state == EAssetResourcePickerState::EnableAll)
	{
		return SoundAssetSelector(context, previousValue);
	}
	else
	{
		return SoundFileSelector(context, previousValue);
	}
}
REGISTER_RESOURCE_SELECTOR("Sound", SoundSelector, "")
