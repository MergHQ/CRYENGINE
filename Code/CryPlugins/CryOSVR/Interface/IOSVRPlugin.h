// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

#include "IHmdOSVRDevice.h"

namespace CryVR
{
namespace Osvr {
struct IOsvrPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(IOsvrPlugin, 0x47512E926A94404D, 0xA2C9A14F0633EEBE);

public:
	virtual IOsvrDevice* CreateDevice() = 0;
	virtual IOsvrDevice* GetDevice() const = 0;
	static IOsvrPlugin* GetPlugin() { return m_pThis; }
	
protected:
	static IOsvrPlugin* m_pThis;
};

IOsvrPlugin* IOsvrPlugin::m_pThis = 0;

}      // namespace Osvr
}      // namespace CryVR