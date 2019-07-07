// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include <CrySystem/Testing/CryTest.h>
#include <CrySystem/Testing/CryTestCommands.h>

CRY_TEST(JobSystemSwitchOffRobustnessTest, timeout = 60, editor = false)
{
	commands = {
		CryTest::CCommandConsoleCmd("sys_job_system_enable 0"),
		CryTest::CCommandLoadLevel("woodland"),
		CryTest::CCommandConsoleCmd("sys_job_system_enable 1"),
	};
}
