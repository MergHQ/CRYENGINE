// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/IConsole.h>
#include <CrySystem/ISystem.h>

namespace CryVR
{
enum EVRCmd
{
	eVRC_Recenter,
};
enum EVRCallbackVars
{
	eVRCV_Gamma,
};

struct IVRCmdListener
{
public:
	virtual void OnCommand(EVRCmd cmd, IConsoleCmdArgs* pArgs) = 0;
protected:
	virtual ~IVRCmdListener() {}
};

class CVars
{
private:
	typedef std::vector<IVRCmdListener*> Listeners;
	static Listeners ms_listener;

public:
	// Shared variables:
	static int hmd_info;
	static int hmd_social_screen;
	static int hmd_social_screen_keep_aspect;
	static int hmd_driver;
	static int hmd_post_inject_camera;

#if defined(INCLUDE_OCULUS_SDK) // Oculus-specific variables:
	static int   hmd_low_persistence;
	static int   hmd_dynamic_prediction;
	static float hmd_ipd;
	static int   hmd_queue_ahead;
	static int   hmd_projection;
	static int   hmd_perf_hud;
	static float hmd_projection_screen_dist;
#endif                           // defined(INCLUDE_OCULUS_SDK)

#if defined(INCLUDE_OPENVR_SDK) // OpenVR-specific variables:
	static int   hmd_reference_point;
	static float hmd_quad_distance;
	static float hmd_quad_width;
#endif                           // defined(INCLUDE_OPENVR_SDK)

private:
	static void Raise(EVRCmd cmd, IConsoleCmdArgs* pArgs)
	{
		for (Listeners::iterator it = ms_listener.begin(); it != ms_listener.end(); ++it)
			(*it)->OnCommand(cmd, pArgs);
	}
	static void OnHmdRecenter(IConsoleCmdArgs* pArgs) { Raise(eVRC_Recenter, pArgs); }

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

		REGISTER_CVAR2("hmd_driver", &hmd_driver, hmd_driver,
		               VF_NULL, "(Only) use specific VR driver:\n"
		                        "0 - Any\n"
		                        "1 - Oculus\n"
		                        "2 - OpenVR\n"
		                        "3 - Osvr\n"
		               );

		REGISTER_CVAR2("hmd_post_inject_camera", &hmd_post_inject_camera, hmd_post_inject_camera,
		               VF_NULL, "Reduce latancy by injecting updated updated camera at render thread as late as possible before submitting draw calls to GPU\n"
		                        "0 - Disable\n"
		                        "1 - Enable\n"
		               );

		REGISTER_COMMAND("hmd_recenter_pose", &OnHmdRecenter,
		                 VF_NULL, "Recenters sensor orientation of the HMD.");

#if defined(INCLUDE_OCULUS_SDK)
		REGISTER_CVAR2("hmd_low_persistence", &hmd_low_persistence, hmd_low_persistence,
		               VF_NULL, "Enables low persistence mode.");

		REGISTER_CVAR2("hmd_dynamic_prediction", &hmd_dynamic_prediction, hmd_dynamic_prediction,
		               VF_NULL, "Enables dynamic prediction based on internally measured latency.");

		REGISTER_CVAR2("hmd_ipd", &hmd_ipd, hmd_ipd,
		               VF_NULL, "HMD Interpupillary distance. If set to a value lower than zero it reads the IPD from the HMD device");

		REGISTER_CVAR2("hmd_queue_ahead", &hmd_queue_ahead, hmd_queue_ahead,
		               VF_NULL, "Enable/Disable Queue Ahead for Oculus");

		REGISTER_CVAR2("hmd_projection", &hmd_projection, eHmdProjection_Stereo,
		               VF_NULL, "Selects the way the image is projected into the hmd: \n"
		                        "0 - normal stereoscopic mode\n"
		                        "1 - monoscopic (cinema-like)\n"
		                        "2 - monoscopic (head-locked)\n"
		               );

		REGISTER_CVAR2("hmd_projection_screen_dist", &hmd_projection_screen_dist, 0.f,
		               VF_NULL, "If >0 it forces the 'cinema screen' distance to the HMD when using 'monoscopic (cinema-like)' projection");

		REGISTER_CVAR2("hmd_perf_hud", &hmd_perf_hud, 0,
		               VF_NULL, "Performance HUD Display for HMDs \n"
		                        "0 - off\n"
		                        "1 - Summary\n"
		                        "2 - Latency timing\n"
		                        "3 - App Render timing\n"
		                        "4 - Compositor Render timing\n"
		                        "5 - Version Info\n"
		               );
#endif // defined(INCLUDE_OCULUS_SDK)

#if defined(INCLUDE_OPENVR_SDK)
		REGISTER_CVAR2("hmd_reference_point", &hmd_reference_point, hmd_reference_point,
		               VF_NULL, "HMD center reference point.\n"
		                        "0 - Camera (/Actor's head)\n"
		                        "1 - Actor's feet\n");

		REGISTER_CVAR2("hmd_quad_distance", &hmd_quad_distance, hmd_quad_distance, VF_NULL, "Distance between eyes and UI quad");

		REGISTER_CVAR2("hmd_quad_width", &hmd_quad_width, hmd_quad_width, VF_NULL, "Width of the UI quad in meters");
#endif // defined(INCLUDE_OPENVR_SDK)

		REGISTER_COMMAND("hmd_recenter_pose", &OnHmdRecenter,
		                 VF_NULL, "Recenters sensor orientation of the HMD.");
	}
	static void Unregister()
	{
		if (IConsole* const pConsole = gEnv->pConsole)
		{
			pConsole->UnregisterVariable("hmd_info");
			pConsole->UnregisterVariable("hmd_social_screen");
			pConsole->UnregisterVariable("hmd_driver");
			pConsole->UnregisterVariable("hmd_recenter_pose");
#if defined(INCLUDE_OCULUS_SDK)
			pConsole->UnregisterVariable("hmd_low_persistence");
			pConsole->UnregisterVariable("hmd_dynamic_prediction");
			pConsole->UnregisterVariable("hmd_ipd");
			pConsole->UnregisterVariable("hmd_queue_ahead");
			pConsole->UnregisterVariable("hmd_projection");
			pConsole->UnregisterVariable("hmd_projection_screen_dist");
			pConsole->UnregisterVariable("hmd_perf_hud");
#endif  // defined(INCLUDE_OCULUS_SDK)
#if defined(INCLUDE_OPENVR_SDK)
			pConsole->UnregisterVariable("hmd_reference_point");
			pConsole->UnregisterVariable("hmd_quad_distance");
			pConsole->UnregisterVariable("hmd_quad_width");
#endif  // defined(INCLUDE_OPENVR_SDK)
		}

	}
	static void RegisterListener(IVRCmdListener* listener)
	{
		ms_listener.push_back(listener);
	}
	static void UnregisterListener(IVRCmdListener* listener)
	{
		ms_listener.erase(std::remove(ms_listener.begin(), ms_listener.end(), listener), ms_listener.end());
	}
};
}
