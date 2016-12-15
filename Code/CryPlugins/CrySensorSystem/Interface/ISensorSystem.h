// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct ISensorMap;
struct ISensorTagLibrary;

struct ISensorSystem
{
	virtual ~ISensorSystem() {}

	virtual ISensorTagLibrary& GetTagLibrary() = 0;
	virtual ISensorMap&        GetMap() = 0;
};
