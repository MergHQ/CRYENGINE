// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <vector>

namespace CryTest
{
struct STestInfo;
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
	int    lineNumber;

	void   Serialize(Serialization::IArchive& ar)
	{
		ar(message, "message");
		ar(fileName, "fileName");
		ar(lineNumber, "lineNumber");
	}
};
struct IReporter
{
	virtual ~IReporter() {}

	//! Notify reporter the test system started
	virtual void OnStartTesting(const SRunContext& context) = 0;

	//! Notify reporter the test system finished
	virtual void OnFinishTesting(const SRunContext& context) = 0;

	//! Notify reporter one test started
	virtual void OnSingleTestStart(const STestInfo& testInfo) = 0;

	//! Notify reporter one test finished, along with necessary results
	virtual void OnSingleTestFinish(const STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<SError>& failures) = 0;

	//! Notify reporter we have to break the test instance (due to time out or other unrecoverable errors)
	virtual void OnBreakTesting(const SRunContext& context) = 0;

	//! Notify reporter to recover from last breakage
	virtual void OnRecoverTesting(const SRunContext& context) = 0;
};

}