// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "TestInfo.h"
#include <vector>

namespace CryTest
{
struct SRunContext
{
	int testCount = 0;
	int failedTestCount = 0;
	int succedTestCount = 0;
};

struct SError
{
	string message;
	string fileName;
	int    lineNumber = 0;

	SError() = default;

	SError(string inMessage, string inFileName, int inLineNumber = 0)
		: message(std::move(inMessage))
		, fileName(std::move(inFileName))
		, lineNumber(inLineNumber)
	{
	}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(message, "message");
		ar(fileName, "fileName");
		ar(lineNumber, "lineNumber");
	}
};

struct ITestReporter
{
	virtual ~ITestReporter() = default;

	//! Notify reporter the test system started
	virtual void OnStartTesting(const SRunContext& context) = 0;

	//! Notify reporter the test system finished
	virtual void OnFinishTesting(const SRunContext& context, bool openReport) = 0;

	//! Notify reporter one test started
	virtual void OnSingleTestStart(const STestInfo& testInfo) = 0;

	//! Notify reporter one test finished, along with necessary results
	virtual void OnSingleTestFinish(const STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<SError>& failures) = 0;

	//! Save the test instance to prepare for possible time out or other unrecoverable errors
	virtual void SaveTemporaryReport() = 0;

	//! Recover from last save
	virtual void RecoverTemporaryReport() = 0;

	//! Returns true if the report contains a previous entry that matches certain test
	virtual bool HasTest(const STestInfo& testInfo) const = 0;
};
}
