// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IFramesPerSecond
{
	virtual ~IFramesPerSecond() {}
	virtual void  Update() = 0;

	virtual float GetInterval() const = 0;
	virtual void  SetInterval(float interval) = 0;
	virtual int   GetFPS() const = 0;
};
