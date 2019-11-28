// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class IPatchPakManagerListener
{
public:
	virtual ~IPatchPakManagerListener() {}

	virtual void UpdatedPermissionsNowAvailable() = 0;
};
