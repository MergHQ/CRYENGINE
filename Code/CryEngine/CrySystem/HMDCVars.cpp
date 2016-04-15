// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "HMDCVars.h"
#include <CrySystem/VR/IHMDDevice.h>

namespace CryVR
{
std::vector<IVRCmdListener*> CVars::ms_listener;

int CVars::hmd_info = 0;
int CVars::hmd_social_screen = static_cast<int>(EHmdSocialScreen::eHmdSocialScreen_DistortedDualImage);
int CVars::hmd_social_screen_keep_aspect = 0;
int CVars::hmd_driver = 0;
int CVars::hmd_post_inject_camera = 0;
#if defined(INCLUDE_OCULUS_SDK)
int CVars::hmd_low_persistence = 0;
int CVars::hmd_dynamic_prediction = 0;
float CVars::hmd_ipd = -1.0f;
int CVars::hmd_queue_ahead = 1;
int CVars::hmd_projection = 0;
int CVars::hmd_perf_hud = 0;
float CVars::hmd_projection_screen_dist = 1.0f;
#endif // defined(INCLUDE_OCULUS_SDK)
#if defined(INCLUDE_OPENVR_SDK)
int CVars::hmd_reference_point = 0;
float CVars::hmd_quad_distance = 0.25f;
float CVars::hmd_quad_width = 1.0f;
#endif // defined(INCLUDE_OPENVR_SDK)
}
