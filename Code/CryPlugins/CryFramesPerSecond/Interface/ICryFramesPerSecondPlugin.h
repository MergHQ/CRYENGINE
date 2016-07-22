// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

struct IFramesPerSecond;

struct ICryFramesPerSecondPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(ICryFramesPerSecondPlugin, 0x4D38C8412F254A0A, 0x7A68ECB7AE4444C0);

public:
	virtual IFramesPerSecond*         GetIFramesPerSecond() const = 0;
	static ICryFramesPerSecondPlugin* GetCryFramesPerSecond() { return m_pThis; }

protected:
	static ICryFramesPerSecondPlugin* m_pThis;
};

ICryFramesPerSecondPlugin* ICryFramesPerSecondPlugin::m_pThis = 0;
