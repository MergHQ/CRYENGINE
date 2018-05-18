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

#include <CrySystem/Testing/IReporter.h>
#include <CryString/CryString.h>
#include <CrySystem/ILog.h>
#include <CrySerialization/IArchive.h>
#include <CrySerialization/STL.h>
#include "ExcelExport.h"
#include <CrySystem/Testing/TestInfo.h>

namespace CryTest
{
//! Writes multiple excel documents for detailed results
class CTestExcelReporter : public CExcelExportBase, public IReporter
{
public:
	explicit CTestExcelReporter(ILog& log) : m_log(log) {}

	void Serialize(Serialization::IArchive& ar)
	{
		ar(m_results, "m_results");
	}

protected:
	virtual void OnStartTesting(const SRunContext& context) override {}
	virtual void OnFinishTesting(const SRunContext& context) override;
	virtual void OnSingleTestStart(const STestInfo& testInfo) override;
	virtual void OnSingleTestFinish(const STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<SError>& failures) override;
	virtual void OnBreakTesting(const SRunContext& context) override;
	virtual void OnRecoverTesting(const SRunContext& context) override;

	virtual void PostFinishTesting(const SRunContext& context, bool bSavedReports) const {}

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

//! Extends Excel reporter by opening failed report
class CTestExcelNotificationReporter : public CTestExcelReporter
{
public:
	using CTestExcelReporter::CTestExcelReporter;
protected:
	virtual void PostFinishTesting(const SRunContext& context, bool bSavedReports) const override;
};
}
