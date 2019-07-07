// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

enum EPakPriority
{
	ePakPriorityFileFirst         = 0,
	ePakPriorityPakFirst          = 1,
	ePakPriorityPakOnly           = 2,
	ePakPriorityFileFirstModsOnly = 3,
};

// Variables that control behavior of CryPak/StreamEngine subsystems
struct PakVars
{
	int nSaveLevelResourceList      = 0;
	int nSaveFastloadResourceList   = 0;
	int nSaveTotalResourceList      = 0;
	int nSaveMenuCommonResourceList = 0;
	int nLogMissingFiles            = 0;
	int nLoadCache                  = 0; // Load in memory paks from _FastLoad folder
	int nLoadModePaks               = 0; // Load menucommon/gamemodeswitch paks
	int nReadSlice                  = 0;
	int nPriority                   =
#if defined(_RELEASE)
		ePakPriorityPakOnly;  // Only read from pak files by default
#else
		ePakPriorityFileFirst;
#endif
	int nMessageInvalidFileAccess   = 0;
	int nValidateFileHashes         =
#if defined(_DEBUG)
		1;
#else
		0;
#endif
	int nTotalInMemoryPakSizeLimit  = 30; // Megabytes
	int nStreamCache                = 0;
	int nInMemoryPerPakSizeLimit    = 6;  // 6 Megabytes limit
	int nLogInvalidFileAccess       = 0;
	int nLoadFrontendShaderCache    = 0;
	int nUncachedStreamReads        = 1;
#ifndef _RELEASE
	int nLogAllFileAccess           = 0;
#endif
	int nDisableNonLevelRelatedPaks = 1;
};

