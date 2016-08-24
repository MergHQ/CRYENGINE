// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/IConsole.h>
#include <CrySystem/ISystem.h>

#include <CrySystem/VR/IHMDManager.h>

namespace CryVR
{
class CVars
{
private:
public:
	// Shared variables:
	static int hmd_info;
	static int hmd_social_screen;
	static int hmd_social_screen_keep_aspect;
	static ICVar* pSelectedHmdNameVar;

	static void OnHmdRecenter(IConsoleCmdArgs* pArgs) 
	{ 
		gEnv->pSystem->GetHmdManager()->RecenterPose();
	}

public:
	static void Register()
	{
		REGISTER_CVAR2("hmd_info", &hmd_info, hmd_info,
		               VF_NULL, "Shows hmd related profile information.");

		REGISTER_CVAR2("hmd_social_screen", &hmd_social_screen, hmd_social_screen,
		               VF_NULL, "Selects the social screen mode: \n"
		                        "-1- Off\n"
		                        "0 - Distorted dual image\n"
		                        "1 - Undistorted dual image\n"
		                        "2 - Undistorted left eye\n"
		                        "3 - Undistorted right eye\n"
		               );

		REGISTER_CVAR2("hmd_social_screen_keep_aspect", &hmd_social_screen_keep_aspect, hmd_social_screen_keep_aspect,
		               VF_NULL, "Keep aspect ratio when displaying images on social screen: \n"
		                        "0 - Off\n"
		                        "1 - On\n"
		               );

		pSelectedHmdNameVar = gEnv->pConsole->RegisterString("hmd_device", "", VF_NULL, 
						"Specifies the name of the VR device to use\nAvailable options depend on VR plugins registered with the engine");

		REGISTER_COMMAND("hmd_recenter_pose", &OnHmdRecenter,
		                 VF_NULL, "Recenters sensor orientation of the HMD.");

		REGISTER_COMMAND("hmd_recenter_pose", &OnHmdRecenter,
		                 VF_NULL, "Recenters sensor orientation of the HMD.");
	}
	static void Unregister()
	{
		if (IConsole* const pConsole = gEnv->pConsole)
		{
			pConsole->UnregisterVariable("hmd_info");
			pConsole->UnregisterVariable("hmd_social_screen");
			pConsole->UnregisterVariable("hmd_device");
			pConsole->UnregisterVariable("hmd_recenter_pose");
		}

	}
};
}
