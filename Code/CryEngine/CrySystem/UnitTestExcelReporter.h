// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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
#include <CrySystem/Testing/ITestReporter.h>

namespace CryTest
{

//! Writes multiple excel documents for detailed results
class CTestExcelReporter : public ITestReporter, public CExcelExportBase
{
public:
	explicit CTestExcelReporter(ILog& log) : m_log(log) {}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(m_results, "m_results");
	}

	//! Notify reporter the test system started
	virtual void OnStartTesting(const SRunContext& context) override {}

	//! Notify reporter the test system finished
	virtual void OnFinishTesting(const SRunContext& context, bool openReport) override;

	//! Notify reporter one test started
	virtual void OnSingleTestStart(const STestInfo& testInfo) override;

	//! Notify reporter one test finished, along with necessary results
	virtual void OnSingleTestFinish(const STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<SError>& failures) override;

	//! Save the test instance to prepare for possible time out or other unrecoverable errors
	virtual void SaveTemporaryReport() override;

	//! Recover from last save
	virtual void RecoverTemporaryReport() override;

	//! Returns whether the report contains certain test
	virtual bool HasTest(const STestInfo& testInfo) const override;

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
