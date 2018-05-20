// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct ILog;

namespace CryTest
{

struct IReporter;
class CTestFactory;

//! Describes in what way the test result is reported
enum class EReporterType
{
	Excel,                      //!< Writes multiple excel documents for detailed results
	ExcelWithNotification,      //!< In addition to excel document, also opens the document locally when any test fails
	Regular,                    //!< Writes detailed logs for every test
};

struct ITestSystem
{
	virtual ~ITestSystem() = default;

	//! Should be called every system update.
	virtual void  Update() = 0;

	virtual void  InitLog() = 0;

	virtual ILog* GetLog() = 0;

	//! Sets whether to shutdown the engine after tests are finished.
	virtual void SetQuitAfterTests(bool quitAfter) = 0;

	//! Runs all test instances and returns exit code.
	virtual void Run(EReporterType reporterType) = 0;

	//! Reports a non-recoverable error, breaking the current test.
	virtual void ReportCriticalError(const char* szExpression, const char* szFile, int line) = 0;

	//! Reports a recoverable test failure to be collected in the report
	virtual void ReportNonCriticalError(const char* szExpression, const char* szFile, int line) = 0;

	//! Helper called on module initialization. Do not use directly.
	virtual void AddFactory(CTestFactory* pFactory) = 0;
};

}