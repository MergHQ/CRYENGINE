// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct ILog;

namespace CryTest
{
class CTestFactory;

struct ITestSystem
{
	virtual ~ITestSystem() = default;

	//! Should be called every system update.
	virtual void  Update() = 0;

	virtual void  InitLog() = 0;

	virtual ILog* GetLog() = 0;

	//! Sets whether to shutdown the engine after tests are finished.
	virtual void SetQuitAfterTests(bool quitAfter) = 0;

	//! Sets whether to open the report locally after tests are finished.
	virtual void SetOpenReport(bool value) = 0;

	//! Runs all tests.
	virtual void Run() = 0;

	//! Runs one test of given name.
	virtual void RunSingle(const char* testName) = 0;

	//! Reports a non-recoverable error, breaking the current test.
	virtual void ReportCriticalError(const char* szExpression, const char* szFile, int line) = 0;

	//! Reports a recoverable test failure to be collected in the report
	virtual void ReportNonCriticalError(const char* szExpression, const char* szFile, int line) = 0;

	//! Helper called on module initialization. Do not use directly.
	virtual void AddFactory(CTestFactory* pFactory) = 0;
};

}