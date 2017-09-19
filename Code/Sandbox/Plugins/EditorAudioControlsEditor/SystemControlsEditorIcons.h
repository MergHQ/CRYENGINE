// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryIcon.h>
#include "AudioAssets.h"

namespace ACE
{
inline CryIcon GetItemTypeIcon(EItemType const type)
{
	switch (type)
	{
	case EItemType::eItemType_Trigger:
		return CryIcon(":Icons/Trigger.ico");
	case EItemType::eItemType_Parameter:
		return CryIcon(":Icons/Parameter.ico");
	case EItemType::eItemType_Switch:
		return CryIcon(":Icons/Switch.ico");
	case EItemType::eItemType_State:
		return CryIcon(":Icons/Property.ico");
	case EItemType::eItemType_Environment:
		return CryIcon(":Icons/Environment.ico");
	case EItemType::eItemType_Preload:
		return CryIcon(":Icons/Bank.ico");
	case EItemType::eItemType_Folder:
		return CryIcon(":Icons/Folder.ico");
	case EItemType::eItemType_Library:
		return CryIcon("icons:common/assets_geomcache.ico");
	}
	return CryIcon("icons:Dialogs/dialog-error.ico");
}
} // namespace ACE
