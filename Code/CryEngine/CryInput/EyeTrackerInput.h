// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
//
// Description: EyeTracker for Windows (covering EyeX)

#pragma once

#include <CryInput/IInput.h>

#ifdef CRY_PLATFORM_WINDOWS
#ifdef USE_EYETRACKER

class CEyeTrackerInput : public IEyeTrackerInput
{
public:
	CEyeTrackerInput();
	~CEyeTrackerInput();

	virtual bool        Init();
	virtual void        Update();

	void                SetEyeProjection(int iX, int iY);

private:
	bool    m_bProjectionChanged;
	float   m_fEyeX;
	float   m_fEyeY;
};

#endif //USE_EYETRACKER
#endif //CRY_PLATFORM_WINDOWS
