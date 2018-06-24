// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   UnitTestExcelReporter.h
//  Created:     19/03/2008 by Timur.
//  Description: Implementation of the CryEngine Unit Testing framework
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <CryString/CryString.h>
#include <CrySystem/ILog.h>
#include <CrySerialization/IArchive.h>
#include <CrySerialization/STL.h>
#include "ExcelExport.h"
#include <CrySystem/Testing/TestInfo.h>

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
	int    lineNumber;

	void   Serialize(Serialization::IArchive& ar)
	{
		ar(message, "message");
		ar(fileName, "fileName");
		ar(lineNumber, "lineNumber");
	}
};

//! Writes multiple excel documents for detailed results
class CTestExcelReporter : public CExcelExportBase
{
public:
	explicit CTestExcelReporter(ILog& log) : m_log(log) {}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(m_results, "m_results");
	}

	//! Notify reporter the test system started
	void OnStartTesting(const SRunContext& context) {}

	//! Notify reporter the test system finished
	void OnFinishTesting(const SRunContext& context, bool openReport);

	//! Notify reporter one test started
	void OnSingleTestStart(const STestInfo& testInfo);

	//! Notify reporter one test finished, along with necessary results
	void OnSingleTestFinish(const STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<SError>& failures);

	//! Save the test instance to prepare for possible time out or other unrecoverable errors
	void SaveTemporaryReport();

	//! Recover from last save
	void RecoverTemporaryReport();

	//! Returns whether the report contains certain test
	bool HasTest(const STestInfo& testInfo) const;

private:
	ILog& m_log;

	struct STestResult
	{
		STestInfo           testInfo;
		float               runTimeInMs;
		bool                isSuccessful;
		std::vector<SError> failures;

		void                Serialize(Serialization::IArchive& ar)
		{
			ar(testInfo, "testInfo");
			ar(runTimeInMs, "runTimeInMs");
			ar(isSuccessful, "isSuccessful");
			ar(failures, "failures");
		}
	};
	std::vector<STestResult> m_results;
};
}
