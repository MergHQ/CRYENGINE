// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/VR/IHMDManager.h>

class CHmdManager : public IHmdManager
{
public:
	CHmdManager() : m_pHmdDevice(nullptr) {}
	virtual ~CHmdManager();

	// basic functionality needed to setup and destroy an Hmd during system init / system shutdown
	virtual void SetupAction(EHmdSetupAction cmd) override;

	// trigger an action on the current Hmd
	virtual void Action(EHmdAction action) override;

	// update the tracking information
	virtual void UpdateTracking(EVRComponent vrComponent) override;

	// returns the active Hmd (or NULL if none has been activated)
	virtual struct IHmdDevice* GetHmdDevice() const override { return m_pHmdDevice; }

	// returns true if we have an Hmd device recognized and r_stereodevice, r_stereooutput and r_stereomode are properly set for stereo rendering
	virtual bool IsStereoSetupOk() const override;

	// populates o_info with the asymmetric camera information returned by the current Hmd device
	virtual bool GetAsymmetricCameraSetupInfo(int nEye, SAsymmetricCameraSetupInfo& outInfo) const override;

private:

	void ShutDown();

private:
	struct IHmdDevice* m_pHmdDevice;
};
