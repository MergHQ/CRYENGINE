// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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

namespace CryTest
{
constexpr char kOutputFileName[] = "%USER%/TestResults/UnitTest.xml";

void CTestExcelReporter::OnFinishTesting(const SRunContext& context)
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
	Column->setAttr("ss:Width", 300);
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
	{
		SetCellFlags(m_CurrCell, CELL_BOLD | CELL_CENTERED);
	}
	AddRow();
	AddCell("Success Ratio %", CELL_BOLD);
	if (context.testCount > 0)
	{
		AddCell(100 * context.succedTestCount / context.testCount);
	}
	else
	{
		AddCell(0);
	}

	AddRow();
	AddRow();

	AddRow();
	m_CurrRow->setAttr("ss:StyleID", "s25");
	AddCell("Test Name");
	AddCell("Status");
	AddCell("Run Time(ms)");
	AddCell("Failures");
	AddCell("File");
	AddCell("Line");

	string name;

	for (const STestResult& res : m_results)
	{
		if (res.isSuccessful)
		{
			continue;
		}

		AddRow();
		name.Format("[%s] %s:%s", res.testInfo.module.c_str(), res.testInfo.suite.c_str(), res.testInfo.name.c_str());
		AddCell(name);
		AddCell("FAIL", CELL_CENTERED);
		AddCell((int)res.runTimeInMs);
		AddCell((uint64)res.failures.size());
		AddCell(res.testInfo.fileName.c_str());
		AddCell(res.testInfo.lineNumber);

		for (auto& failure : res.failures)
		{
			AddRow();
			AddCellAtIndex(4, failure.message);
			AddCell(failure.fileName);
			AddCell(failure.lineNumber);
		}
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
	Column->setAttr("ss:Width", 60);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 200);
	Column = m_CurrTable->newChild("Column");
	Column->setAttr("ss:Width", 60);

	AddRow();
	m_CurrRow->setAttr("ss:StyleID", "s25");
	AddCell("Test Name");
	AddCell("Status");
	AddCell("Run Time(ms)");
	AddCell("Failures");
	AddCell("File");
	AddCell("Line");

	for (const STestResult& res : m_results)
	{
		AddRow();
		name.Format("[%s] %s:%s", res.testInfo.module.c_str(), res.testInfo.suite.c_str(), res.testInfo.name.c_str());
		AddCell(name);
		if (res.isSuccessful)
		{
			AddCell("OK", CELL_CENTERED);
		}
		else
		{
			AddCell("FAIL", CELL_CENTERED);
		}
		AddCell(static_cast<int>(res.runTimeInMs));
		AddCell(static_cast<uint32>(res.failures.size()));
		AddCell(res.testInfo.fileName.c_str());
		AddCell(res.testInfo.lineNumber);
	}

	bool bSaveSucceed = SaveToFile(kOutputFileName);
	PostFinishTesting(context, bSaveSucceed);
}

void CTestExcelReporter::OnSingleTestStart(const STestInfo& testInfo)
{
	string text;
	text.Format("Test Started: [%s] %s:%s", testInfo.module.c_str(), testInfo.suite.c_str(), testInfo.name.c_str());
	m_log.Log(text);
}

void CTestExcelReporter::OnSingleTestFinish(const STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<SError>& failures)
{
	if (bSuccess)
	{
		m_log.Log("Test result: [%s]%s:%s | OK (%3.2fms)", testInfo.module.c_str(), testInfo.suite.c_str(), testInfo.name.c_str(), fRunTimeInMs);
	}
	else
	{
		m_log.Log("Test result: [%s]%s:%s | %d failures:", testInfo.module.c_str(), testInfo.suite.c_str(), testInfo.name.c_str(), failures.size());
		for (const SError& err : failures)
		{
			m_log.Log("at %s line %d:\t%s", err.fileName.c_str(), err.lineNumber, err.message.c_str());
		}
	}

	m_results.push_back(STestResult { testInfo, fRunTimeInMs, bSuccess, failures });
}

static constexpr const char* szExcelReporterSave = "%USER%/TestResults/UnitTestTemp.json";

void CTestExcelReporter::OnBreakTesting(const SRunContext& context)
{
	Serialization::SaveJsonFile(szExcelReporterSave, *this);
}

void CTestExcelReporter::OnRecoverTesting(const SRunContext& context)
{
	Serialization::LoadJsonFile(*this, szExcelReporterSave);
	gEnv->pCryPak->RemoveFile(szExcelReporterSave);
}

void CTestExcelNotificationReporter::PostFinishTesting(const SRunContext& context, bool bSavedReports) const
{
#if CRY_PLATFORM_WINDOWS
	if (!bSavedReports)
	{
		// For local unit testing notify user to close previously opened report.
		// Use primitive windows msgbox because we are supposed to hide all pop-ups during auto testing.
		CryMessageBox("Unit test failed to save one or more report documents, make sure the file is writable!", "Unit Test", eMB_Error);
	}
	else
	{
		//Open report file if any test failed. Since the notification is used for local testing only, we only need Windows
		if (context.failedTestCount > 0)
		{
			m_log.Log("%d Tests failed, opening report...", context.failedTestCount);
			int nAdjustFlags = 0;
			char path[_MAX_PATH];
			const char* szAdjustedPath = gEnv->pCryPak->AdjustFileName(kOutputFileName, path, nAdjustFlags);
			if (szAdjustedPath != nullptr)
			{
				//should open it with Excel
				int err = (int)::ShellExecute(NULL, "open", szAdjustedPath, NULL, NULL, SW_SHOW);
				if (err <= 32)  //returns a value greater than 32 if succeeds.
				{
					m_log.Log("Failed to open report %s, error code: %d", szAdjustedPath, err);
				}
			}
		}
	}
#endif
}

}