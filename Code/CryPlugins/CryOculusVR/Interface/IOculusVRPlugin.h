// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

#include "IHmdOculusRiftDevice.h"

namespace CryVR
{
namespace Oculus {
struct IOculusVRPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(IOculusVRPlugin, 0xE4B6593F30CE4F80, 0x9348285F842E11C1);

public:
	virtual IOculusDevice* CreateDevice() = 0;
	virtual IOculusDevice* GetDevice() const = 0;
	static IOculusVRPlugin* GetPlugin() { return m_pThis; }
	
protected:
	static IOculusVRPlugin* m_pThis;
};

IOculusVRPlugin* IOculusVRPlugin::m_pThis = 0;

}      // namespace Oculus
}      // namespace CryVR