// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if !defined(_RELEASE) && CRY_PLATFORM_WINDOWS
#define DEBUG_MNM_ENABLED 1
#else
#define DEBUG_MNM_ENABLED 0
#endif

// If you want to debug the data consistency just set this define to 1
#define DEBUG_MNM_DATA_CONSISTENCY_ENABLED 0

// Log additional information about OffMesh link operation (add, remove, ...)
#define DEBUG_MNM_LOG_OFFMESH_LINK_OPERATIONS 0