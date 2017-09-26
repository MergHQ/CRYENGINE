// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioAssets.h"
#include <CryIcon.h>

namespace ACE
{
inline CryIcon GetItemTypeIcon(EItemType const type)
{
	switch (type)
	{
	case EItemType::Trigger:
		return CryIcon(":Icons/Trigger.ico");
	case EItemType::Parameter:
		return CryIcon(":Icons/Parameter.ico");
	case EItemType::Switch:
		return CryIcon(":Icons/Switch.ico");
	case EItemType::State:
		return CryIcon(":Icons/State.ico");
	case EItemType::Environment:
		return CryIcon(":Icons/Environment.ico");
	case EItemType::Preload:
		return CryIcon(":Icons/Preload.ico");
	case EItemType::Folder:
		return CryIcon("icons:General/Folder.ico");
	case EItemType::Library:
		return CryIcon("icons:General/File.ico");
	}
	return CryIcon("icons:Dialogs/dialog-error.ico");
}
} // namespace ACE
