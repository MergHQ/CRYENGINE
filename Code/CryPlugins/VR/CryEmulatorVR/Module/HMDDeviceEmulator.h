// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once


#include <CrySystem/ISystem.h>
#include <CrySystem/VR/IHMDManager.h>

#include <atomic>

namespace CryVR
{
namespace Emulator
{
//! Represents a head-mounted device (Virtual Reality) connected to the system
class Device : public IHmdDevice, public IHmdEventListener, public ISystemEventListener
{
public:
	Device();
	void      AddRef() override;
	void      Release() override;

	EHmdClass GetClass() const override { return eHmdClass_Emulator; }
	void      GetDeviceInfo(HmdDeviceInfo& info) const override;

	void      GetCameraSetupInfo(float& fov, float& aspectRatioFactor) const override;
	HMDCameraSetup GetHMDCameraSetup(int nEye, float projRatio, float fnear) const override;

	void      UpdateInternal(EInternalUpdate) override;
	void      RecenterPose() override;
	void      UpdateTrackingState(EVRComponent, uint64_t frameId);

	//! \return Tracking state in Hmd's internal coordinates system.
	const HmdTrackingState& GetNativeTrackingState() const override;

	//! \return Tracking state in the Hmd's local tracking space using CRYENGINE's coordinate system.
	const HmdTrackingState& GetLocalTrackingState() const override;

	//! \return Quad (four corners) of the Play Area in which the player can move around safely. A value of zero indicates that this HMD does not handle room-scale functionality.
	Quad GetPlayArea() const override { return Quad(); }
	//! \return 2D width and height of the play area in which the player can move around safely. A value of zero indicates that this HMD does not handle room-scale functionality.
	Vec2 GetPlayAreaSize() const override { return Vec2(0, 0); }

	const IHmdController*   GetController()      const override { return nullptr;	}
	int                     GetControllerCount() const override { return 0; }

	//useful for querying preferred rendering resolution for devices where the screen resolution is not the resolution the SDK wants for rendering (OSVR)
	 void GetPreferredRenderResolution(unsigned int& width, unsigned int& height) override;
	//Disables & Resets the tracking state to identity. Useful for benchmarking where we want the HMD to behave normally except that we want to force the direction of the camera.
	void DisableHMDTracking(bool disable) override;

	//! Can be called from any thread to retrieve most up to date camera transformation
	stl::optional<Matrix34> RequestAsyncCameraUpdate(uint64_t frameId, const Quat& q, const Vec3 &p) override;

	static Device* CreateInstance();

	// IVRCmdListener
	virtual void OnRecentered() override;
	// ~IVRCmdListener

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
	// ~ISystemEventListener

	float GetCurrentIPD() const;
	void  DebugDraw(float& xPosLabel, float& yPosLabel) const;
	
	~Device();

protected:

	const char* s_deviceName = "EmulatedDevice";

	volatile int     m_refCount;
	uint64_t         m_lastFrameID_UpdateTrackingState; // we could remove this. at some point we may want to sample more than once the tracking state per frame.
	HmdDeviceInfo    m_devInfo;
	bool                                   m_disableHeadTracking;
	HmdTrackingState                       m_localTrackingState;
	HmdTrackingState                       m_nativeTrackingState;
	HmdTrackingState                       m_disabledTrackingState;
};
}
}
