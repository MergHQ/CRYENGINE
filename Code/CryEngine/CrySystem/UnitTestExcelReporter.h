// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   UnitTestExcelReporter.h
//  Created:     19/03/2008 by Timur.
//  Description: Implementation of the CryEngine Unit Testing framework
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <CrySystem/CryUnitTest.h>
#include "ExcelExport.h"

namespace CryUnitTest
{
//! Writes multiple excel documents for detailed results
class CUnitTestExcelReporter : public CExcelExportBase, public IUnitTestReporter
{
protected:
	virtual void OnStartTesting(const SUnitTestRunContext& context) override {}
	virtual void OnFinishTesting(const SUnitTestRunContext& context) override;
	virtual void OnSingleTestStart(const IUnitTest& test) override;
	virtual void OnSingleTestFinish(const IUnitTest& test, float fRunTimeInMs, bool bSuccess, char const* failureDescription) override;

	virtual void PostFinishTesting(const SUnitTestRunContext& context, bool bSavedReports) const {}

	bool         SaveJUnitCompatableXml();

	struct STestResult
	{
		SUnitTestInfo testInfo;
		SAutoTestInfo autoTestInfo;
		float         fRunTimeInMs;
		bool          bSuccess;
		string        failureDescription;
	};
	std::vector<STestResult> m_results;
};

//! Extends Excel reporter by opening failed report
class CUnitTestExcelNotificationReporter : public CUnitTestExcelReporter
{
	virtual void PostFinishTesting(const SUnitTestRunContext& context, bool bSavedReports) const override;
};
}
