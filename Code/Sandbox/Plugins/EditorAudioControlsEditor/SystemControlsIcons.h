// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <SystemTypes.h>
#include <CryIcon.h>

namespace ACE
{
inline CryIcon GetItemTypeIcon(ESystemItemType const type)
{
	CryIcon typeIcon = CryIcon("icons:Dialogs/dialog-error.ico");

	switch (type)
	{
	case ESystemItemType::Trigger:
		typeIcon = CryIcon("icons:audio/system/trigger.ico");
		break;
	case ESystemItemType::Parameter:
		typeIcon = CryIcon("icons:audio/system/parameter.ico");
		break;
	case ESystemItemType::Switch:
		typeIcon = CryIcon("icons:audio/system/switch.ico");
		break;
	case ESystemItemType::State:
		typeIcon = CryIcon("icons:audio/system/state.ico");
		break;
	case ESystemItemType::Environment:
		typeIcon = CryIcon("icons:audio/system/environment.ico");
		break;
	case ESystemItemType::Preload:
		typeIcon = CryIcon("icons:audio/system/preload.ico");
		break;
	case ESystemItemType::Folder:
		typeIcon = CryIcon("icons:General/Folder.ico");
		break;
	case ESystemItemType::Library:
		typeIcon = CryIcon("icons:General/File.ico");
		break;
	default:
		typeIcon = CryIcon("icons:Dialogs/dialog-error.ico");
		break;
	}

	return typeIcon;
}
} // namespace ACE
