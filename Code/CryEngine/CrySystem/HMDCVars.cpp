// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "HMDCVars.h"
#include <CrySystem/VR/IHMDDevice.h>

namespace CryVR
{
int CVars::hmd_info = 0;
int CVars::hmd_social_screen = static_cast<int>(EHmdSocialScreen::eHmdSocialScreen_DistortedDualImage);
int CVars::hmd_social_screen_keep_aspect = 0;
float CVars::hmd_resolution_scale = 1.f;
ICVar* CVars::pSelectedHmdNameVar = nullptr;
}
	