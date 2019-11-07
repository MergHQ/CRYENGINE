// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include "CrySpatial.h"
#include "CrySpatialPlugin.h"
#include "CrySpatialAttachmentPlugin.h"
#include "../SoundEnginePlugin/CrySpatialConfig.h"

#include <AK/Wwise/Utilities.h>
#include <AK/Tools/Common/AkAssert.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
namespace Plugins
{
BEGIN_MESSAGE_MAP(CrySpatialApp, CWinApp)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CrySpatialApp theApp;

//////////////////////////////////////////////////////////////////////////
BOOL CrySpatialApp::InitInstance()
{
	CWinApp::InitInstance();
	AK::Wwise::RegisterWwisePlugin();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
AK::Wwise::IPluginBase* __stdcall AkCreatePlugin(unsigned short companyID, unsigned short pluginID)
{
	if (companyID == CrySpatialConfig::g_companyID && pluginID == CrySpatialConfig::g_pluginID)
	{
		return new CrySpatialPlugin;
	}

	if (companyID == CrySpatialAttachmentConfig::g_companyID && pluginID == CrySpatialAttachmentConfig::g_pluginID)
	{
		return new CrySpatialAttachmentPlugin;
	}

	return nullptr;
}
} // namespace Plugins
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
DEFINEDUMMYASSERTHOOK;
