// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __VolumesManager_h__
#define __VolumesManager_h__

#pragma once

#include <CryAISystem/INavigationSystem.h>

/*
   This class is needed to keep track of the navigation shapes created in Sandbox and their
   respective IDs. It's necessary to keep consistency with the exported Navigation data
   since the AISystem has no knowledge of the Editor's shape
 */

class CVolumesManager
{
public:
	CVolumesManager() {}

	bool               RegisterArea(const char* volumeName);
	void               UnRegisterArea(const char* volumeName);

	bool               SetAreaID(const char* volumeName, NavigationVolumeID id);
	void               InvalidateID(NavigationVolumeID id);
	void               UpdateNameForAreaID(const NavigationVolumeID id, const char* newName);

	bool               IsAreaPresent(const char* volumeName) const;
	NavigationVolumeID GetAreaID(const char* volumeName) const;
	bool               GetAreaName(NavigationVolumeID id, string& name) const;

	void               GetVolumesNames(std::vector<string>& names) const;

private:
	typedef std::map<string, NavigationVolumeID> VolumesMap;
	VolumesMap m_volumeAreas;
};

#endif // __VolumesManager_h__
