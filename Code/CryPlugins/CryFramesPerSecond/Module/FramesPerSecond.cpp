// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FramesPerSecond.h"

#include <CrySystem/ISystem.h>
#include <Cry3DEngine/I3DEngine.h>

//------------------------------------------------------------------------
CFramesPerSecond::CFramesPerSecond()
	: m_currentFPS(0)
	, m_interval(0.5f)
	, m_previousTime(gEnv->pTimer->GetCurrTime())
{
}

//------------------------------------------------------------------------
void CFramesPerSecond::Update()
{
	uint32 width = gEnv->pRenderer->GetWidth();

	float currentTime = gEnv->pTimer->GetCurrTime();
	if (currentTime > m_previousTime + m_interval)
	{
		m_currentFPS = gEnv->pTimer->GetFrameRate();
		m_previousTime = currentTime;
	}

	gEnv->p3DEngine->DrawTextRightAligned((float)width, 0.0f, 3.0f, Col_Green, "%dfps", (int)m_currentFPS);
}
