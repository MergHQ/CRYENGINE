// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TimeUtil.h"
#include <chrono>

namespace TimeUtil
{

long GetCurrentTimeStamp()
{
	using namespace std::chrono;
	auto nowTimePoint = system_clock::now();
	auto nowTimePointSeconds = time_point_cast<seconds>(nowTimePoint);
	return nowTimePointSeconds.time_since_epoch().count();
}

}