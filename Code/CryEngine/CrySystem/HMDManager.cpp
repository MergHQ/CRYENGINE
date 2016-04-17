// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "HMDManager.h"
#include "HMDCVars.h"

#include <CryRenderer/IStereoRenderer.h>

#if defined(INCLUDE_OCULUS_SDK)
	#include "Oculus/OculusResources.h"
	#include "Oculus/OculusDevice.h"
#endif

#if defined(INCLUDE_OSVR_SDK)
	#include "Osvr/OsvrDevice.h"
#endif
#if defined(INCLUDE_OPENVR_SDK)
	#include "OpenVR/OpenVRResources.h"
	#include "OpenVR/OpenVRDevice.h"
#endif
#include "Osvr/OsvrResources.h"

// Note:
//  We support a single HMD device at a time, with Oculus, OpenVR/Vive, PSVR/Morpheus as options.
// This manager will be updated accordingly as other HMDs come, based on their system's init/shutdown, etc requirements.
// That may imply changes in the interface.

#define VR_INIT(devNS)     if (!m_pHmdDevice)                      \
  {                                                                \
    CryVR::devNS::Resources::Init();                               \
    m_pHmdDevice = CryVR::devNS::Resources::GetAssociatedDevice(); \
    if (m_pHmdDevice)                                              \
      m_pHmdDevice->AddRef();                                      \
  }
#define VR_SHUTDOWN(devNS) if (m_pHmdDevice && m_pHmdDevice->GetClass() == eHmdClass_ ## devNS) \
  {                                                                                             \
    SAFE_RELEASE(m_pHmdDevice);                                                                 \
    CryVR::devNS::Resources::Shutdown();                                                        \
  }

CHmdManager::~CHmdManager()
{
	ShutDown();
}

// ------------------------------------------------------------------------
void CHmdManager::ShutDown()
{

#if defined(INCLUDE_OCULUS_SDK)
	VR_SHUTDOWN(Oculus)
#endif
#if defined(INCLUDE_OPENVR_SDK)
	VR_SHUTDOWN(OpenVR)
#endif
#ifdef INCLUDE_OSVR_SDK
	VR_SHUTDOWN(Osvr)
#endif

}

// ------------------------------------------------------------------------
void CHmdManager::SetupAction(EHmdSetupAction cmd)
{
	LOADING_TIME_PROFILE_SECTION;
	switch (cmd)
	{
	case EHmdSetupAction::eHmdSetupAction_CreateCvars:
		CryVR::CVars::Register();
		break;
	// ------------------------------------------------------------------------
	case EHmdSetupAction::eHmdSetupAction_PostInit: // Nothing to do for Oculus after SDK 0.6.0
		break;
	// ------------------------------------------------------------------------
	case EHmdSetupAction::eHmdSetupAction_Init:
		{
			if (gEnv->pConsole && gEnv->pConsole->GetCVar("sys_vr_support")->GetIVal() > 0)
			{
				m_pHmdDevice = nullptr;
#if defined(INCLUDE_OCULUS_SDK)
				if ((CryVR::CVars::hmd_driver == 0 || CryVR::CVars::hmd_driver == 1) && !m_pHmdDevice)
				{
					VR_INIT(Oculus)
				}
#endif
#if defined(INCLUDE_OPENVR_SDK)
				if ((CryVR::CVars::hmd_driver == 0 || CryVR::CVars::hmd_driver == 2) && !m_pHmdDevice)
				{
					VR_INIT(OpenVR)
				}
#endif
#if defined(INCLUDE_OSVR_SDK)
				if ((CryVR::CVars::hmd_driver == 0 || CryVR::CVars::hmd_driver == 3) && !m_pHmdDevice)
				{
					VR_INIT(Osvr)
				}
#endif
				gEnv->pRenderer->GetIStereoRenderer()->OnHmdDeviceChanged(m_pHmdDevice);
			}
		}
		break;

	default:
		assert(0);
	}

}

// ------------------------------------------------------------------------
void CHmdManager::Action(EHmdAction action)
{
	if (m_pHmdDevice)
	{
		switch (action)
		{
		case EHmdAction::eHmdAction_DrawInfo:
			m_pHmdDevice->UpdateInternal(IHmdDevice::eInternalUpdate_DebugInfo);
			break;
		default:
			assert(0);
		}
	}
}

// ------------------------------------------------------------------------
void CHmdManager::UpdateTracking(EVRComponent updateType)
{
	if (m_pHmdDevice)
	{
		m_pHmdDevice->UpdateTrackingState(updateType);
	}
}

// ------------------------------------------------------------------------
bool CHmdManager::IsStereoSetupOk() const
{
	if (m_pHmdDevice)
	{
		if (IStereoRenderer* pStereoRenderer = gEnv->pRenderer->GetIStereoRenderer())
		{
			EStereoDevice device = STEREO_DEVICE_NONE;
			EStereoMode mode = STEREO_MODE_NO_STEREO;
			EStereoOutput output = STEREO_OUTPUT_STANDARD;

			pStereoRenderer->GetInfo(&device, &mode, &output, 0);

			const bool stereo_output_hmd_selected = (output == STEREO_OUTPUT_HMD && (m_pHmdDevice->GetClass() == eHmdClass_Oculus || m_pHmdDevice->GetClass() == eHmdClass_OpenVR || m_pHmdDevice->GetClass() == eHmdClass_Osvr));

			return (
			  device == STEREO_DEVICE_FRAMECOMP &&
			  (mode == STEREO_MODE_POST_STEREO || mode == STEREO_MODE_DUAL_RENDERING) &&
			  (output == STEREO_OUTPUT_SIDE_BY_SIDE || stereo_output_hmd_selected)
			  );
		}
	}
	return false;
}

// ------------------------------------------------------------------------
bool CHmdManager::GetAsymmetricCameraSetupInfo(int nEye, SAsymmetricCameraSetupInfo& o_info) const
{

#if defined(INCLUDE_OCULUS_SDK) || defined(INCLUDE_OPENVR_SDK) || defined(INCLUDE_OSVR_SDK)
	if (m_pHmdDevice && (m_pHmdDevice->GetClass() == eHmdClass_Oculus || m_pHmdDevice->GetClass() == eHmdClass_OpenVR || m_pHmdDevice->GetClass() == eHmdClass_Osvr))
	{
		m_pHmdDevice->GetAsymmetricCameraSetupInfo(nEye, o_info.fov, o_info.aspectRatio, o_info.asymH, o_info.asymV, o_info.eyeDist);
		return true;
	}
#endif

	return false;
}
