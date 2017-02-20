// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

//! Implementation of the CryEngine Unit Testing framework

#pragma once

#include <CrySystem/CryUnitTest.h>

struct SAutoTestsContext;

namespace CryUnitTest
{

class CUnitTest final : public IUnitTest
{
public:
	CUnitTest(SUnitTestInfo info) : m_info(std::move(info)) {}
	virtual const SUnitTestInfo& GetInfo() const override { return m_info; };
	virtual const SAutoTestInfo& GetAutoTestInfo() const override
	{
		return m_info.test.m_autoTestInfo;
	}
	virtual void Init() override
	{
		m_info.test.Init();
	};
	virtual void Run() override
	{
		m_info.test.Run();
	};
	virtual void Done() override
	{
		m_info.test.Done();
	};

	SUnitTestInfo m_info;
};

class CLogUnitTestReporter : public IUnitTestReporter
{
public:
	virtual void OnStartTesting(const SUnitTestRunContext& context) override;
	virtual void OnFinishTesting(const SUnitTestRunContext& context) override;
	virtual void OnSingleTestStart(const IUnitTest& test) override;
	virtual void OnSingleTestFinish(const IUnitTest& test, float fRunTimeInMs, bool bSuccess, char const* szFailureDescription) override;
};

class CMinimalLogUnitTestReporter : public IUnitTestReporter
{
public:
	virtual void OnStartTesting(const SUnitTestRunContext& context) override;
	virtual void OnFinishTesting(const SUnitTestRunContext& context) override;
	virtual void OnSingleTestStart(const IUnitTest& test) override {}
	virtual void OnSingleTestFinish(const IUnitTest& test, float fRunTimeInMs, bool bSuccess, char const* szFailureDescription) override;
private:
	int   m_nRunTests = 0;
	int   m_nSucceededTests = 0;
	int   m_nFailedTests = 0;
	float m_fTimeTaken = 0.f;
};

class CUnitTestManager : public IUnitTestManager
{
public:
	CUnitTestManager();
	virtual ~CUnitTestManager();

public:
	virtual IUnitTest* GetTestInstance(const SUnitTestInfo& info) override;
	virtual int        RunAllTests(EReporterType reporterType) override;
	virtual void       RunAutoTests(const char* szSuiteName, const char* szTestName) override; //!< Currently not in use
	virtual void       Update() override; //!< Currently not in use

	virtual void       SetExceptionCause(const char* szExpression, const char* szFile, int line) override;
private:
	void               StartTesting(SUnitTestRunContext& context, EReporterType reporterToUse);
	void               EndTesting(SUnitTestRunContext& context);
	void               RunTest(CUnitTest& test, SUnitTestRunContext& context);
	bool               IsTestMatch(const CUnitTest& Test, const string& sSuiteName, const string& sTestName) const;

private:
	std::vector<std::unique_ptr<CUnitTest>> m_tests;
	char                                    m_failureMsg[256] = {};
	std::unique_ptr<SAutoTestsContext>      m_pAutoTestsContext;
	bool                                    m_bRunningTest = false;
};
} // namespace CryUnitTest
