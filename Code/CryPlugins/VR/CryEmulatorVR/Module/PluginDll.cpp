// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "PluginDll.h"
#include "HMDDeviceEmulator.h"
#include <CrySystem/VR/IHMDManager.h>
#include <CrySystem/ConsoleRegistration.h>

namespace CryVR
{
	namespace Emulator {

		CPlugin_EmulatorVR::CPlugin_EmulatorVR()
		{
			m_pDevice = Device::CreateInstance();
			if (m_pDevice != nullptr)
			{
				CryLogAlways("[HMD][Oculus] Oculus HMD was created.");
				return;
			}
			else
			{
				CryLogAlways("[HMD][Oculus] Oculus HMD device creation failed");
			}
		}

		CPlugin_EmulatorVR::~CPlugin_EmulatorVR()
		{
			GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
			SAFE_DELETE(m_pDevice);
		}

		bool CPlugin_EmulatorVR::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
		{
			GetISystem()->GetISystemEventDispatcher()->RegisterListener(this, "CPlugin_EmulatorVR");

			return true;
		}

		void CPlugin_EmulatorVR::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
		{
			switch (event)
			{
			case ESYSTEM_EVENT_PRE_RENDERER_INIT:
			{
				if (auto *pDevice = GetDevice())
				{
					gEnv->pSystem->GetHmdManager()->RegisterDevice(GetName(), *pDevice);
				}
			}
			break;
			case ESYSTEM_EVENT_FAST_SHUTDOWN:
			case ESYSTEM_EVENT_FULL_SHUTDOWN:
			{
				gEnv->pSystem->GetHmdManager()->UnregisterDevice(GetName());
			}
			break;
			}
		}

		Device* CPlugin_EmulatorVR::CreateDevice()
		{
			return GetDevice();
		}

		Device* CPlugin_EmulatorVR::GetDevice() const
		{
			return m_pDevice;
		}

		CRYREGISTER_SINGLETON_CLASS(CPlugin_EmulatorVR)

	}      // namespace Emulator
}      // namespace CryVR

#include <CryCore/CrtDebugStats.h>
