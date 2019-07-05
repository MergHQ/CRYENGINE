// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

namespace CryVR
{
	namespace Emulator {

		class Device;

		class CPlugin_EmulatorVR : public Cry::IEnginePlugin, public ISystemEventListener
		{
			CRYINTERFACE_BEGIN()
			CRYINTERFACE_ADD(Cry::IEnginePlugin)
			CRYINTERFACE_END()
			CRYGENERATE_SINGLETONCLASS_GUID(CPlugin_EmulatorVR, "Plugin_EmulatorVR", "9105BDE2-B102-49F7-B4F5-FA902A343A34"_cry_guid)

			CPlugin_EmulatorVR();
			virtual ~CPlugin_EmulatorVR();

			//! This is called to initialize the new plugin.
			virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams);

		public:
			Device* CreateDevice();
			Device* GetDevice() const;

			// ISystemEventListener
			virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
			// ~ISystemEventListener

		public:
			Device* m_pDevice;
		};

	}      // namespace Oculus
}      // namespace CryVR