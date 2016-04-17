// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/VR/IHMDDevice.h>

struct IHmdManager
{
public:

	virtual ~IHmdManager() {}

	enum EHmdSetupAction
	{
		eHmdSetupAction_CreateCvars = 0,
		eHmdSetupAction_Init,
		eHmdSetupAction_PostInit
	};

	enum EHmdAction
	{
		eHmdAction_DrawInfo = 0,
	};

	struct SAsymmetricCameraSetupInfo
	{
		float fov, aspectRatio, asymH, asymV, eyeDist;
	};

	//! Basic functionality needed to setup and destroy an HMD during system init / system shutdown.
	virtual void SetupAction(EHmdSetupAction cmd) = 0;

	//! Trigger an action on the current HMD.
	virtual void Action(EHmdAction action) = 0;

	//! Update the tracking information.
	virtual void UpdateTracking(EVRComponent vrComponent) = 0;

	//! \return Active HMD (or NULL if none have been activated).
	virtual IHmdDevice* GetHmdDevice() const = 0;

	//! \return true if we have an HMD device recognized and r_stereodevice, r_stereooutput and r_stereomode are properly set for stereo rendering.
	virtual bool IsStereoSetupOk() const = 0;

	//! Populates o_info with the asymmetric camera information returned by the current HMD device.
	virtual bool GetAsymmetricCameraSetupInfo(int nEye, SAsymmetricCameraSetupInfo& outInfo) const = 0;
};
