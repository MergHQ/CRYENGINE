// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

#include "IHmdOpenVRDevice.h"

namespace CryVR
{
namespace OpenVR {
struct IOpenVRPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(IOpenVRPlugin, 0xCD1389A9B37547F9, 0xBC45D382D18B21B1);

public:
	virtual IOpenVRDevice* CreateDevice() = 0;
	virtual IOpenVRDevice* GetDevice() const = 0;
};

}      // namespace OpenVR
}      // namespace CryVR