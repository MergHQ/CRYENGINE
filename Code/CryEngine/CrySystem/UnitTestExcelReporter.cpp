// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   UnitTestExcelReporter.cpp
//  Created:     19/03/2008 by Timur.
//  Description: Implementation of the CryEngine Unit Testing framework
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UnitTestExcelReporter.h"

#include <CrySystem/ISystem.h>
#include <CrySystem/IConsole.h>

#include <CryCore/Platform/CryLibrary.h>

#if CRY_PLATFORM_WINDOWS
	#include <shellapi.h> // requires <windows.h>
#endif

using namespace CryUnitTest;

constexpr char kOutputFileName[] = "%USER%/TestResults/UnitTest.xml";
constexpr char kOutputFileNameJUnit[] = "%USER%/TestResults/UnitTestJUnit.xml";

void CUnitTestExcelReporter::OnFinishTesting(const SUnitTestRunContext& context)
{
	// Generate report.
	XmlNodeRef Workbook = GetISystem()->CreateXmlNode("Workbook");
	InitExcelWorkbook(Workbook);
	NewWorksheet("UnitTests");

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 200);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 50);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 80);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 200);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 200);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 60);

	AddRow();
	AddCell("Run Tests", CELL_BOLD);
	AddCell(context.testCount);
	AddRow();
	AddCell("Succeeded Tests", CELL_BOLD);
	AddCell(context.succedTestCount);
	AddRow();
	AddCell("Failed Tests", CELL_BOLD);
	AddCell(context.failedTestCount);
	if (context.failedTestCount != 0)
		SetCellFlags(m_CurrCell, CELL_BOLD | CELL_CENTERED);
	AddRow();
	AddCell("Success Ratio %", CELL_BOLD);
	if (context.testCount > 0)
		AddCell(100 * context.succedTestCount / context.testCount);
	else
		AddCell(0);

	AddRow();
	AddRow();

	AddRow();
	m_CurrRow->setAttr("ss:StyleID", "s25");
	AddCell("Test Name");
	AddCell("Status");
	AddCell("Run Time(ms)");
	AddCell("Failure Description");
	AddCell("File");
	AddCell("Line");

	string name;

	for (const STestResult& res : m_results)
	{
		if (res.bSuccess)
			continue;

		AddRow();
		if (res.autoTestInfo.szTaskName != 0)
			name.Format("[%s] %s:%s.%s", res.testInfo.GetModule(), res.testInfo.suite, res.testInfo.name, res.autoTestInfo.szTaskName);
		else
			name.Format("[%s] %s:%s", res.testInfo.GetModule(), res.testInfo.suite, res.testInfo.name);
		AddCell(name);
		AddCell("FAIL", CELL_CENTERED);
		AddCell((int)res.fRunTimeInMs);
		AddCell(res.failureDescription, CELL_CENTERED);
		AddCell(res.testInfo.filename);
		AddCell(res.testInfo.lineNumber);
	}

	/////////////////////////////////////////////////////////////////////////////
	NewWorksheet("All Tests");

	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 200);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 50);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 80);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 200);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 200);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 60);

	AddRow();
	m_CurrRow->setAttr("ss:StyleID", "s25");
	AddCell("Test Name");
	AddCell("Status");
	AddCell("Run Time(ms)");
	AddCell("Failure Description");
	AddCell("File");
	AddCell("Line");

	for (const STestResult& res : m_results)
	{
		AddRow();
		if (res.autoTestInfo.szTaskName != 0)
			name.Format("[%s] %s:%s.%s", res.testInfo.GetModule(), res.testInfo.suite, res.testInfo.name, res.autoTestInfo.szTaskName);
		else
			name.Format("[%s] %s:%s", res.testInfo.GetModule(), res.testInfo.suite, res.testInfo.name);
		AddCell(name);
		if (res.bSuccess)
			AddCell("OK", CELL_CENTERED);
		else
			AddCell("FAIL", CELL_CENTERED);
		AddCell((int)res.fRunTimeInMs);
		AddCell(res.failureDescription, CELL_CENTERED);
		AddCell(res.testInfo.filename);
		AddCell(res.testInfo.lineNumber);
	}

	SaveToFile(kOutputFileName);
	SaveJUnitCompatableXml();
}

//////////////////////////////////////////////////////////////////////////
void CUnitTestExcelReporter::SaveJUnitCompatableXml()
{
	XmlNodeRef root = GetISystem()->CreateXmlNode("testsuite");

	//<testsuite failures="0" time="0.289" errors="0" tests="3" skipped="0"	name="UnitTests.MainClassTest">

	int errors = 0;
	int skipped = 0;
	int failures = 0;
	float totalTime = 0;
	for (const STestResult& res : m_results)
	{
		totalTime += res.fRunTimeInMs;
		failures += (res.bSuccess) ? 0 : 1;
	}
	XmlNodeRef suiteNode = root;
	suiteNode->setAttr("time", totalTime);
	suiteNode->setAttr("errors", errors);
	suiteNode->setAttr("failures", failures);
	suiteNode->setAttr("tests", (int)m_results.size());
	suiteNode->setAttr("skipped", skipped);
	suiteNode->setAttr("name", "UnitTests");

	for (const STestResult& res : m_results)
	{
		XmlNodeRef testNode = suiteNode->newChild("testcase");
		testNode->setAttr("time", res.fRunTimeInMs);
		testNode->setAttr("name", res.testInfo.name);
		//<testcase time="0.146" name="TestPropertyValue"	classname="UnitTests.MainClassTest"/>

		if (!res.bSuccess)
		{
			XmlNodeRef failNode = testNode->newChild("failure");
			failNode->setAttr("type", res.testInfo.GetModule());
			failNode->setAttr("message", res.failureDescription);
			string err;
			err.Format("%s at line %d", res.testInfo.filename, res.testInfo.lineNumber);
			failNode->setContent(err);
		}
	}

	root->saveToFile(kOutputFileNameJUnit);
}

void CUnitTestExcelReporter::OnSingleTestStart(const IUnitTest& test)
{
	const SUnitTestInfo& testInfo = test.GetInfo();
	string text;
	text.Format("Test Started: [%s] %s:%s", testInfo.GetModule(), testInfo.suite, testInfo.name);
	gEnv->pLog->UpdateLoadingScreen(text);
}

void CUnitTestExcelReporter::OnSingleTestFinish(const IUnitTest& test, float fRunTimeInMs, bool bSuccess, char const* failureDescription)
{
	const SUnitTestInfo& testInfo = test.GetInfo();
	string text;
	text.Format("Test Finished: [%s] %s:%s", testInfo.GetModule(), testInfo.suite, testInfo.name);
	gEnv->pLog->UpdateLoadingScreen(text);

	STestResult testResult
	{
		testInfo,
		test.GetAutoTestInfo(),
		fRunTimeInMs,
		bSuccess,
		failureDescription
	};
	m_results.push_back(testResult);
}

void CUnitTestExcelNotificationReporter::OnFinishTesting(const SUnitTestRunContext& context)
{
	CUnitTestExcelReporter::OnFinishTesting(context);

	//Open report file if any test failed. Since the notification is used for local testing only, we only need Windows
#if CRY_PLATFORM_WINDOWS
	if (context.failedTestCount > 0)
	{
		CryLogAlways("%d Tests failed, opening report...", context.failedTestCount);
		int nAdjustFlags = 0;
		char path[_MAX_PATH];
		const char* szAdjustedPath = gEnv->pCryPak->AdjustFileName(kOutputFileName, path, nAdjustFlags);
		if (szAdjustedPath != nullptr)
		{
			//should open it with Excel
			int err = (int)::ShellExecute(NULL, "open", szAdjustedPath, NULL, NULL, SW_SHOW);
			if (err <= 32)//returns a value greater than 32 if succeeds.
			{
				CryLogAlways("Failed to open report %s, error code: %d", szAdjustedPath, err);
			}
		}
	}
#endif
}
