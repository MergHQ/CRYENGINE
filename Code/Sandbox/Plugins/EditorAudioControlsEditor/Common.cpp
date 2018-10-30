// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Common.h"
#include "SystemControlsWidget.h"
#include "PropertiesWidget.h"
#include "MiddlewareDataWidget.h"
#include "Common/IImpl.h"

namespace ACE
{
Impl::IImpl* g_pIImpl = nullptr;
CSystemControlsWidget* g_pSystemControlsWidget = nullptr;
CPropertiesWidget* g_pPropertiesWidget = nullptr;
CMiddlewareDataWidget* g_pMiddlewareDataWidget = nullptr;

Platforms g_platforms;
SImplInfo g_implInfo;
} // namespace ACE