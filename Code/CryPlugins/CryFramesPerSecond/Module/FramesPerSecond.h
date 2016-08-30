// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Interface/IFramesPerSecond.h"

class CFramesPerSecond final : public IFramesPerSecond
{
public:
	CFramesPerSecond();
	virtual ~CFramesPerSecond() {}
	virtual void  Update() override;
	virtual float GetInterval() const override         { return m_interval;  }
	virtual void  SetInterval(float interval) override { m_interval = interval; }
	virtual int   GetFPS() const override              { return (int)m_currentFPS; }

private:
	float m_currentFPS;
	float m_interval;
	float m_previousTime;
};
