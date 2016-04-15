// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   UnitTestSystem.h
//  Created:     19/03/2008 by Timur.
//  Description: Implementation of the CryEngine Unit Testing framework
// -------------------------------------------------------------------------
//  History:
//		22-1-2009 Fran	Made legacy. CryUnit is the new unit testing
//						framework
//
////////////////////////////////////////////////////////////////////////////

#ifndef __UnitTestSystem_h__
#define __UnitTestSystem_h__
#pragma once

#include <CrySystem/CryUnitTest.h>

struct SAutoTestsContext;

namespace CryUnitTest
{

class CUnitTest : public IUnitTest
{
public:
	CUnitTest(const UnitTestInfo& info) : m_info(info) {}
	virtual void GetInfo(UnitTestInfo& info) { info = m_info; };
	virtual void GetAutoTestInfo(AutoTestInfo& info)
	{
		if (m_info.pTestImpl)
			info = m_info.pTestImpl->m_autoTestInfo;
	}
	virtual void Run(UnitTestRunContext& context)
	{
		if (m_info.pTestImpl)
			m_info.pTestImpl->Run();
	};
	virtual void Init()
	{
		if (m_info.pTestImpl)
			m_info.pTestImpl->Init();
	};
	virtual void Done()
	{
		if (m_info.pTestImpl)
			m_info.pTestImpl->Done();
	};

	UnitTestInfo m_info;
};

struct CLogUnitTestReporter : public IUnitTestReporter
{
	virtual void OnStartTesting(UnitTestRunContext& context);
	virtual void OnFinishTesting(UnitTestRunContext& context);
	virtual void OnTestStart(IUnitTest* pTest);
	virtual void OnTestFinish(IUnitTest* pTest, float fRunTimeInMs, bool bSuccess, char const* failureDescription);
};

struct CMinimalLogUnitTestReporter : public IUnitTestReporter
{
	virtual void OnStartTesting(UnitTestRunContext& context);
	virtual void OnFinishTesting(UnitTestRunContext& context);
	virtual void OnTestStart(IUnitTest* pTest);
	virtual void OnTestFinish(IUnitTest* pTest, float fRunTimeInMs, bool bSuccess, char const* failureDescription);
private:
	int   m_nRunTests;
	int   m_nSucceededTests;
	int   m_nFailedTests;
	float m_fTimeTaken;
};

class CUnitTestManager : public IUnitTestManager
{
public:
	CUnitTestManager();
	virtual ~CUnitTestManager();

public:
	virtual IUnitTest* CreateTest(const UnitTestInfo& info);
	virtual int        RunAllTests(CryUnitTest::TReporterToUse);
	virtual void       RunMatchingTests(const char* sName, UnitTestRunContext& context);
	virtual void       RunAutoTests(const char* sSuiteName, const char* sTestName);
	virtual void       Update();
	virtual void       RemoveTests();

	virtual void       SetExceptionCause(const char* expression, const char* file, int line);
private:
	void               StartTesting(UnitTestRunContext& context, CryUnitTest::TReporterToUse reporterToUse);
	void               EndTesting(UnitTestRunContext& context);
	void               RunTest(IUnitTest* pTest, UnitTestRunContext& context);
	bool               IsTestMatch(CUnitTest* pTest, const string& sSuiteName, const string& sTestName);

private:
	std::vector<CUnitTest*> m_tests;
	char                    m_failureMsg[256];
	SAutoTestsContext*      m_pAutoTestsContext;
	IUnitTest*              m_pCurrentTest;
};
};

#endif //__UnitTestSystem_h__
