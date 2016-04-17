// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   UnitTestExcelReporter.h
//  Created:     19/03/2008 by Timur.
//  Description: Implementation of the CryEngine Unit Testing framework
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __UnitTestExcelReporter_h__
#define __UnitTestExcelReporter_h__
#pragma once

#include <CrySystem/CryUnitTest.h>
#include "ExcelExport.h"

namespace CryUnitTest
{
struct CUnitTestExcelReporter : public CExcelExportBase, public IUnitTestReporter
{
	virtual void OnStartTesting(UnitTestRunContext& context);
	virtual void OnFinishTesting(UnitTestRunContext& context);
	virtual void OnTestStart(IUnitTest* pTest);
	virtual void OnTestFinish(IUnitTest* pTest, float fRunTimeInMs, bool bSuccess, char const* failureDescription);

	void         SaveJUnitCompatableXml();

private:
	struct TestResult
	{
		UnitTestInfo testInfo;
		AutoTestInfo autoTestInfo;
		float        fRunTimeInMs;
		bool         bSuccess;
		string       failureDescription;
	};
	std::vector<TestResult> m_results;
};
};

#endif //__UnitTestExcelReporter_h__
