// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ICryPlugin.h>

struct IUserAnalytics;

struct ICryUserAnalyticsPlugin : public ICryPlugin
{
	CRYINTERFACE_DECLARE(ICryUserAnalyticsPlugin, 0xC97AD475FD95416D, 0x9C3048B2D2C5B7F6);

public:
	virtual IUserAnalytics*         GetIUserAnalytics() const = 0;
	static ICryUserAnalyticsPlugin* GetCryUserAnalytics() { return m_pThis; }

protected:
	static ICryUserAnalyticsPlugin* m_pThis;
};

ICryUserAnalyticsPlugin* ICryUserAnalyticsPlugin::m_pThis = 0;
