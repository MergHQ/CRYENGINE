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

#define OUTPUT_FILE_NAME       "%USER%/TestResultsUnitTest.xml"
#define OUTPUT_FILE_NAME_JUNIT "%USER%/TestResults/UnitTestJUnit.xml"

#if CRY_PLATFORM_WINDOWS
HWND hwndEdit = 0;
#endif

void CUnitTestExcelReporter::OnStartTesting(UnitTestRunContext& context)
{
	//gEnv->pConsole->SetScrollMax(600);
	//gEnv->pConsole->ShowConsole(true);

#if CRY_PLATFORM_WINDOWS
	ICVar* pVar = gEnv->pConsole->GetCVar("ats_window");
	if (pVar && pVar->GetIVal())
	{
		const char* szWindowClass = "UNIT_TEST_CLASS_WNDCLASS";
		// Register the window class
		WNDCLASS wndClass = { 0, ::DefWindowProc, 0, DLGWINDOWEXTRA, CryGetCurrentModule(), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)COLOR_BTNSHADOW, NULL, szWindowClass };
		RegisterClass(&wndClass);

		int cwX = CW_USEDEFAULT;
		int cwY = CW_USEDEFAULT;
		int cwW = 800;
		int cwH = 600;

		HWND hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT, szWindowClass, "CryENGINE Settings", WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
		                           cwX, cwY, cwW, cwH, 0, NULL, CryGetCurrentModule(), NULL);

		hwndEdit = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
		                        0, 0, 800, 600, // set size in WM_SIZE message
		                        hWnd,           // parent window
		                        (HMENU)1,       // edit control ID
		                        CryGetCurrentModule(),
		                        NULL); // pointer not needed

		/*
		   DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_CLIPSIBLINGS | DS_3DLOOK | DS_SETFONT | DS_MODALFRAME;
		   DWORD dwStyleEx = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;

		   HINSTANCE hInstance = CryGetCurrentModule();
		   HWND hWnd = CreateWindowEx( dwStyleEx, "CustomModelessDialog", "Unit Testing", dwStyle, 200,200,500,500, NULL, NULL, hInstance, NULL );
		 */
	}
#endif
}

void CUnitTestExcelReporter::OnFinishTesting(UnitTestRunContext& context)
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
	AddCell("Successed Tests", CELL_BOLD);
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

	for (uint32 i = 0; i < m_results.size(); i++)
	{
		TestResult& res = m_results[i];
		if (res.bSuccess)
			continue;

		AddRow();
		if (res.autoTestInfo.szTaskName != 0)
			name.Format("[%s] %s:%s.%s", res.testInfo.module, res.testInfo.suite, res.testInfo.name, res.autoTestInfo.szTaskName);
		else
			name.Format("[%s] %s:%s", res.testInfo.module, res.testInfo.suite, res.testInfo.name);
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

	for (uint32 i = 0; i < m_results.size(); i++)
	{
		TestResult& res = m_results[i];

		AddRow();
		if (res.autoTestInfo.szTaskName != 0)
			name.Format("[%s] %s:%s.%s", res.testInfo.module, res.testInfo.suite, res.testInfo.name, res.autoTestInfo.szTaskName);
		else
			name.Format("[%s] %s:%s", res.testInfo.module, res.testInfo.suite, res.testInfo.name);
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

	//const char *filename = OUTPUT_FILE_NAME;
	//SaveToFile( OUTPUT_FILE_NAME );
	char buf[128];
	time_t ltime;
	string filename = "%USER%/TestResults/UnitTest_";

	time(&ltime);
	tm* today = localtime(&ltime);
	strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", today);
	filename += buf;
	filename += ".xml";
	SaveToFile(filename.c_str());

	SaveJUnitCompatableXml();

#if CRY_PLATFORM_WINDOWS
	ICVar* pVar = gEnv->pConsole->GetCVar("ats_show_report");
	if (pVar && pVar->GetIVal())
	{
		::ShellExecute(NULL, "open", filename.c_str(), NULL, NULL, SW_SHOW);
	}
#endif
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
	int numTests = (int)m_results.size();
	for (int i = 0; i < numTests; i++)
	{
		TestResult& res = m_results[i];
		totalTime += res.fRunTimeInMs;
		failures += (res.bSuccess) ? 0 : 1;
	}
	XmlNodeRef suiteNode = root;
	suiteNode->setAttr("time", totalTime);
	suiteNode->setAttr("errors", errors);
	suiteNode->setAttr("failures", failures);
	suiteNode->setAttr("tests", numTests);
	suiteNode->setAttr("skipped", skipped);
	suiteNode->setAttr("name", "UnitTests");

	for (int i = 0; i < numTests; i++)
	{
		TestResult& res = m_results[i];

		XmlNodeRef testNode = suiteNode->newChild("testcase");
		testNode->setAttr("time", res.fRunTimeInMs);
		testNode->setAttr("name", res.testInfo.name);
		//<testcase time="0.146" name="TestPropertyValue"	classname="UnitTests.MainClassTest"/>

		if (!res.bSuccess)
		{
			XmlNodeRef failNode = testNode->newChild("failure");
			failNode->setAttr("type", res.testInfo.module);
			failNode->setAttr("message", res.failureDescription);
			string err;
			err.Format("%s at line %d", res.testInfo.filename, res.testInfo.lineNumber);
			failNode->setContent(err);
		}
	}

	root->saveToFile(OUTPUT_FILE_NAME_JUNIT);
}

void CUnitTestExcelReporter::OnTestStart(IUnitTest* pTest)
{
	CryUnitTest::UnitTestInfo testInfo;
	pTest->GetInfo(testInfo);

	string text;
	text.Format("Test Started: [%s] %s:%s", testInfo.module, testInfo.suite, testInfo.name);

#if CRY_PLATFORM_WINDOWS
	if (hwndEdit)
		SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM) text.c_str());
#endif
}

void CUnitTestExcelReporter::OnTestFinish(IUnitTest* pTest, float fRunTimeInMs, bool bSuccess, char const* failureDescription)
{
	TestResult res;

	pTest->GetInfo(res.testInfo);
	pTest->GetAutoTestInfo(res.autoTestInfo);
	res.fRunTimeInMs = fRunTimeInMs;
	res.bSuccess = bSuccess;
	res.failureDescription = failureDescription;

	m_results.push_back(res);

	string text;
	text.Format("Test Finished: [%s] %s:%s", res.testInfo.module, res.testInfo.suite, res.testInfo.name);

#if CRY_PLATFORM_WINDOWS
	if (hwndEdit)
		SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM) text.c_str());

	gEnv->pLog->UpdateLoadingScreen(text);
#endif
}
