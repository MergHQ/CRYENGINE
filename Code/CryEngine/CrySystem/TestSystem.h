// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <chrono>
#include <CrySystem/Testing/ITestSystem.h>// ITestSystem
#include <CrySystem/Testing/CryTest.h>    // CTest
#include <CryThreading/IThreadManager.h>  // IThread
#include "Log.h"                          // CLog
#include "UnitTestExcelReporter.h"

struct ISystem;
struct IConsoleCmdArgs;

namespace CryTest
{
class CTestRecordFile;

class CTestSystem
	: public ITestSystem
	  , public IThread
	  , private IErrorObserver
{
public:

	//! constructs test system that outputs to dedicated log
	CTestSystem(ISystem* pSystem);

	virtual ~CTestSystem();

	//! initializes supported console commands. Must be called after the console is set up.
	static void InitCommands();

	//! Runs all tests.
	virtual void Run() override;
	//! Runs one test of given name.
	virtual void RunSingle(const char* testName) override;
	virtual void ReportCriticalError(const char* szExpression, const char* szFile, int line) override;
	virtual void ReportNonCriticalError(const char* szExpression, const char* file, int line) override;

	virtual void AddFactory(CTestFactory* pFactory) override;

private:
	//! Implement ITestSystem
	virtual void  Update() override;
	virtual void  InitLog() override;
	virtual ILog* GetLog() override;
	virtual void  SetQuitAfterTests(bool quitAfter) override { m_wantQuitAfterTestsDone = quitAfter; }
	virtual void  SetOpenReport(bool value) override         { m_openReport = value; }

	//! Implement IThread
	virtual void ThreadEntry() override;

	//! Implement IErrorObserver
	virtual void OnAssert(const char* szCondition, const char* szMessage, const char* szFileName, unsigned int fileLineNumber) override;
	virtual void OnFatalError(const char* szMessage) override;

private:   // --------------------------------------------------------------

	bool InitTest();
	void FinishTest();
	template<typename F> void RunTestsFilter(F&& predicate);
	void SignalStopWork();

private:   // --------------------------------------------------------------

	CLog                       m_log;
	std::vector<CTestFactory*> m_testFactories;

	/* runtime variables */
	std::unique_ptr<CTestRecordFile> m_pTestRecordFile;
	std::queue<CTestFactory*>        m_remainingTestFactories;

	class CTestInstance;

	std::unique_ptr<CTestInstance>        m_currentTestInstance;

	std::unique_ptr<CTestExcelReporter>   m_pReporter;
	SRunContext                           m_testContext;
	volatile bool                         m_isRunningTest = false;
	bool                                  m_wantQuitAfterTestsDone = false;
	bool                                  m_openReport = false;

	volatile bool                         m_wantQuit = false;

	uint64                                m_lastUpdateCounter = 0;
	std::chrono::system_clock::time_point m_heartBeatTime = std::chrono::system_clock::now();
};
}
