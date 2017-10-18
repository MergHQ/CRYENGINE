// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <SystemTypes.h>
#include <CryIcon.h>

namespace ACE
{
inline CryIcon GetItemTypeIcon(ESystemItemType const type)
{
	switch (type)
	{
	case ESystemItemType::Trigger:
		return CryIcon(":Icons/system/trigger.ico");
	case ESystemItemType::Parameter:
		return CryIcon(":Icons/system/parameter.ico");
	case ESystemItemType::Switch:
		return CryIcon(":Icons/system/switch.ico");
	case ESystemItemType::State:
		return CryIcon(":Icons/system/state.ico");
	case ESystemItemType::Environment:
		return CryIcon(":Icons/system/environment.ico");
	case ESystemItemType::Preload:
		return CryIcon(":Icons/system/preload.ico");
	case ESystemItemType::Folder:
		return CryIcon("icons:General/Folder.ico");
	case ESystemItemType::Library:
		return CryIcon("icons:General/File.ico");
	}
	return CryIcon("icons:Dialogs/dialog-error.ico");
}
} // namespace ACE
