// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Common.h"
#include "AssetsManager.h"
#include "ImplManager.h"

namespace ACE
{
CAssetsManager g_assetsManager;
CImplManager g_implManager;
Impl::IImpl* g_pIImpl = nullptr;
CMainWindow* g_pMainWindow = nullptr;
CSystemControlsWidget* g_pSystemControlsWidget = nullptr;
CPropertiesWidget* g_pPropertiesWidget = nullptr;
CMiddlewareDataWidget* g_pMiddlewareDataWidget = nullptr;
CFileMonitorMiddleware* g_pFileMonitorMiddleware = nullptr;

Platforms g_platforms;
SImplInfo g_implInfo;

ControlIds g_importedItemIds;
} // namespace ACE