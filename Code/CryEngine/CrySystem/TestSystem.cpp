// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#if !defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
#include <setjmp.h>
#endif
#include <CrySystem/ISystem.h>                      // ISystem
#include <CrySystem/IConsole.h>                     // IConsole
#include <CryGame/IGameFramework.h>                 // IGameFramework
#include <CryCore/optional.h>
#include "UnitTestExcelReporter.h"
#include "TestSystem.h"

namespace CryTest
{

	static constexpr float FreezeTimeOut = 120.f;

	static void CreateTimeOutRecord(const string& testName)
	{
		const char* path = "%USER%/TestResults/UnitTestTimeOut.txt";
		FILE* timeoutFile = gEnv->pCryPak->FOpen(path, "w");
		if (timeoutFile)
		{
			gEnv->pCryPak->FPrintf(timeoutFile, "%s", testName.c_str());
			gEnv->pCryPak->FClose(timeoutFile);
		}
	}

	static stl::optional<string> DetectAndClearTimeOutRecord()
	{
		stl::optional<string> result;
		const char* path = "%USER%/TestResults/UnitTestTimeOut.txt";
		FILE* timeoutFile = gEnv->pCryPak->FOpen(path, "r");
		if (timeoutFile)
		{
			char testName[256] = {};
			gEnv->pCryPak->FGets(testName, 256, timeoutFile);
			if (*testName)
			{
				result = string(testName);
			}
			gEnv->pCryPak->FClose(timeoutFile);
			gEnv->pCryPak->RemoveFile(path);
		}
		return result;
	}

	CTestSystem::CTestSystem(ISystem* pSystem)
		: m_log(pSystem)
	{
		// Listen to asserts and fatal errors, turn them into unit test failures during the test
		if (!gEnv->pSystem->RegisterErrorObserver(this))
		{
			CRY_ASSERT_MESSAGE(false, "Test system failed to register error system callback");
		}

		// Spawn a thread to help figuring out time out or hang
		if (!gEnv->pThreadManager->SpawnThread(this, "Test System"))
		{
			CRY_ASSERT_MESSAGE(false, "Error spawning test system watch thread.");
		}

		// Register tests defined in this module
		for (CTestFactory* pFactory = CTestFactory::GetFirstInstance();
			pFactory != nullptr;
			pFactory = pFactory->GetNextInstance())
		{
			pFactory->SetModuleName("CrySystem");
			AddFactory(pFactory);
		}
	}

	CTestSystem::~CTestSystem()
	{
		SignalStopWork();
		gEnv->pThreadManager->JoinThread(this, eJM_Join);

		// Clean up listening to asserts and fatal errors
		if (!gEnv->pSystem->UnregisterErrorObserver(this))
		{
			CRY_ASSERT_MESSAGE(false, "Test system failed to unregister error system callback");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void RunUnitTests(IConsoleCmdArgs* pArgs)
	{
		// Check for "noquit" option.
		bool quitAfter = true;
		for (int i = 1; i < pArgs->GetArgCount(); i++)
		{
			if (strcmpi(pArgs->GetArg(i), "noquit") == 0)
			{
				quitAfter = false;
				break;
			}
		}

		ITestSystem* pTestSystem = gEnv->pSystem->GetITestSystem();
		pTestSystem->SetQuitAfterTests(quitAfter);
		pTestSystem->Run(EReporterType::Excel);
	}

	//////////////////////////////////////////////////////////////////////////
	/*static*/ void CTestSystem::InitCommands()
	{
		REGISTER_COMMAND("RunUnitTests", RunUnitTests, VF_INVISIBLE | VF_CHEAT, "Execute a set of unit tests");
	}

	//////////////////////////////////////////////////////////////////////////
	void CTestSystem::InitLog()
	{
		m_log.SetFileName("%USER%/TestResults/TestLog.log");
	}

	//////////////////////////////////////////////////////////////////////////
	ILog* CTestSystem::GetLog()
	{
		return &m_log;
	}

	//////////////////////////////////////////////////////////////////////////
	class CLogTestReporter : public IReporter
	{
	public:
		CLogTestReporter(ILog& log) : m_log(log) {}
	private:
		ILog & m_log;
	private:
		virtual void OnStartTesting(const SRunContext& context) override
		{
			m_log.Log("Testing Started");
		}
		virtual void OnFinishTesting(const SRunContext& context) override
		{
			m_log.Log("Testing Finished");
		}
		virtual void OnSingleTestStart(const STestInfo& testInfo) override
		{
			m_log.Log("Running test: [%s]%s:%s", testInfo.module.c_str(), testInfo.suite.c_str(), testInfo.name.c_str());
		}
		virtual void OnSingleTestFinish(const STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<SError>& failures) override
		{
			if (bSuccess)
				m_log.Log("Test result: [%s]%s:%s | OK (%3.2fms)", testInfo.module.c_str(), testInfo.suite.c_str(), testInfo.name.c_str(), fRunTimeInMs);
			else
			{
				m_log.Log("Test result: [%s]%s:%s | %d failures:", testInfo.module.c_str(), testInfo.suite.c_str(), testInfo.name.c_str(), failures.size());
				for (const SError& err : failures)
				{
					m_log.Log("at %s line %d:\t%s", err.fileName.c_str(), err.lineNumber, err.message.c_str());
				}
			}
		}
		virtual void OnBreakTesting(const SRunContext& context) override
		{
			//Nothing particular needs to be done
		}
		virtual void OnRecoverTesting(const SRunContext& context) override
		{
			//Nothing particular needs to be done
		}
	};

#if !defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
	jmp_buf* GetAssertJmpBuf()
	{
		static jmp_buf s_jmpBuf;
		return &s_jmpBuf;
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	std::unique_ptr<IReporter> CreateReporter(EReporterType reporterType, ILog& m_log)
	{
		std::unique_ptr<IReporter> pReporter;
		switch (reporterType)
		{
		case EReporterType::Excel:
			pReporter = stl::make_unique<CTestExcelReporter>(m_log);
			break;
		case EReporterType::ExcelWithNotification:
			pReporter = stl::make_unique<CTestExcelNotificationReporter>(m_log);
			break;
		case EReporterType::Regular:
			pReporter = stl::make_unique<CLogTestReporter>(m_log);
			break;
		default:
			CRY_ASSERT(false);
			break;
		}
		CRY_ASSERT(pReporter != nullptr);
		return pReporter;
	}

	template<typename F>
	stl::optional<SError> TryRun(F&& function)
	{
#if defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
		try
		{
			function();
			return {};
		}
		catch (assert_exception const& e)
		{
			return SError{
							 e.what(), e.m_filename, e.m_lineNumber
			};
		}
		catch (std::exception const& e)
		{
			string failureMessage;
			failureMessage.Format("Unhandled exception: %s", e.what());
			return SError{
							 failureMessage, "Unknown", 0
			};
		}
#else
		if (setjmp(*GetAssertJmpBuf()) == 0)
		{
			function();
			return {};
		}
		else
		{
			return SError{
							 "Crash", "Unknown", 0
			};
		}
#endif // defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
	}

	void CTestSystem::Update()
	{
		if (m_isRunningTest)
		{
			if (!m_remainingTestFactories.empty() && m_pCurrentTest == nullptr)
			{
				CTestFactory* pTestFactory = m_remainingTestFactories.front();
				CRY_ASSERT(pTestFactory);
				const bool wantSpawnTestForGame = pTestFactory->GetIsEnabledForGame() && !gEnv->IsEditor();
				const bool wantSpawnTestForEditor = pTestFactory->GetIsEnabledForEditor() && gEnv->IsEditor();
				if (wantSpawnTestForGame || wantSpawnTestForEditor)
				{
					m_pCurrentTestFactory = pTestFactory;
					m_currentTestInfo = m_pCurrentTestFactory->GetTestInfo();
					m_pCurrentTest = m_pCurrentTestFactory->CreateTest();
					m_remainingTestFactories.pop();
					if (!InitTest())//critical errors
					{
						FinishTest();
					}
				}
				else // test is skipped
				{
					m_remainingTestFactories.pop();
				}
			}

			if (m_pCurrentTest != nullptr)
			{
				bool isDone = false;
				TryRun([&]
				{
					isDone = m_pCurrentTest->UpdateCommands();
				});

				//check timeout
				std::chrono::duration<float> runTimeSeconds = std::chrono::system_clock::now() - m_currentTestStartTime;
				if (m_pCurrentTestFactory->GetTimeOut() > 0 && runTimeSeconds.count() > m_pCurrentTestFactory->GetTimeOut())
				{
					ReportNonCriticalError("Time out", "", 0);
					isDone = true;
				}

				if (isDone)
				{
					FinishTest();
				}
			}

			if (m_remainingTestFactories.empty() && m_pCurrentTest == nullptr)
			{
				m_pReporter->OnFinishTesting(m_testContext);
				CryLogAlways("Running all unit tests done.");
				m_isRunningTest = false;
				if (m_wantQuitAfterTestsDone)
				{
					gEnv->pSystem->Quit();
				}
			}
		}
	}

	void CTestSystem::Run(EReporterType reporterType)
	{
		CRY_ASSERT(!m_isRunningTest);
		m_isRunningTest = true; //before spawning tests because we'd like to catch errors during test spawns too
		CryLogAlways("Running all unit tests...");//this gets output to main log. details are output to test log.

		stl::optional<string> lastTimeOutTest = DetectAndClearTimeOutRecord();

		m_pReporter = CreateReporter(reporterType, m_log);
		m_testContext = {};

		m_pReporter->OnStartTesting(m_testContext);

		auto testStartIter = m_testFactories.begin();
		if (lastTimeOutTest)
		{
			string lastTimeOutTestName = lastTimeOutTest.value();
			testStartIter = std::find_if(m_testFactories.begin(), m_testFactories.end(), [&lastTimeOutTestName](CTestFactory* pFactory)
			{
				return pFactory->GetTestInfo().name == lastTimeOutTestName;
			});
			++testStartIter; //bypasses the time out test
			m_pReporter->OnRecoverTesting(m_testContext);
		}

		for (auto iter = testStartIter; iter != m_testFactories.end(); ++iter)
		{
			m_remainingTestFactories.push(*iter);
		}
	}

	bool CTestSystem::InitTest()
	{
		m_pReporter->OnSingleTestStart(m_currentTestInfo);
		m_testContext.testCount++;

		m_currentTestStartTime = std::chrono::system_clock::now();

		stl::optional<SError> err = TryRun([&]
		{
			m_pCurrentTest->Init();
			m_pCurrentTest->Run();
		});

		if (err)
		{
			m_testContext.failedTestCount++;
			m_currentTestFailures.push_back({ err.value().message, err.value().fileName, err.value().lineNumber });
			return false;
		}

		return true;
	}

	void CTestSystem::FinishTest()
	{
		bool failed = false;
		if (m_currentTestFailures.size())
		{
			m_testContext.failedTestCount++;
			failed = true;
		}
		else
		{
			m_testContext.succedTestCount++;
		}

		stl::optional<SError> err = TryRun([&]
		{
			m_pCurrentTest->Done();
		});

		auto timeAfterTest = std::chrono::system_clock::now();
		std::chrono::duration<float, std::milli> runTime = timeAfterTest - m_currentTestStartTime;
		m_pReporter->OnSingleTestFinish(m_currentTestInfo, runTime.count(), !failed, m_currentTestFailures);
		m_pCurrentTest.reset();
		m_pCurrentTestFactory = nullptr;

		m_currentTestInfo = {};
		m_currentTestFailures.clear();
	}

	void CTestSystem::SignalStopWork()
	{
		m_wantQuit = true;
	}

	void CTestSystem::ThreadEntry()
	{
		while (!m_wantQuit)
		{
			uint64 updateCounter = gEnv->pSystem->GetUpdateCounter();
			std::chrono::duration<float> timeSinceHeartBeat = std::chrono::system_clock::now() - m_heartBeatTime;
			if (m_pCurrentTest != nullptr && updateCounter == m_lastUpdateCounter && timeSinceHeartBeat.count() > FreezeTimeOut)
			{
				//record timeout and quit
				auto timeAfterTest = std::chrono::system_clock::now();
				std::chrono::duration<float, std::milli> runTime = timeAfterTest - m_currentTestStartTime;
				m_pReporter->OnSingleTestFinish(m_currentTestInfo, runTime.count(), false, m_currentTestFailures);

				CreateTimeOutRecord(m_currentTestInfo.name);
				SignalStopWork();
				m_pReporter->OnBreakTesting(m_testContext);
				gEnv->pSystem->Quit();
			}
			m_lastUpdateCounter = updateCounter;
			CrySleep(100);
		}
	}

	void CTestSystem::ReportCriticalError(const char* szExpression, const char* szFile, int line)
	{
		if (m_isRunningTest)
		{
#ifdef USE_CRY_ASSERT
			// Reset assert flag to not block other updates depending on the flag, e.g. editor update,
			// in case we do not immediately quit after the unit test.
			// The flag has to be set here, since the majority of engine code does not use exceptions
			// but we are using exception or longjmp here, so it no longer returns to the assert caller
			// which would otherwise reset the flag.
			gEnv->stoppedOnAssert = false;
#endif
			m_currentTestFailures.push_back({ szExpression, szFile, line });
#if defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
			throw assert_exception(szExpression, szFile, line);
#else
			longjmp(*GetAssertJmpBuf(), 1);
#endif
		}
	}

	void CTestSystem::ReportNonCriticalError(const char* szExpression, const char* szFile, int line)
	{
		CRY_ASSERT(m_pCurrentTest != nullptr);
		m_currentTestFailures.push_back({ szExpression, szFile, line });
	}

	void CTestSystem::AddFactory(CTestFactory * pFactory)
	{
		pFactory->SetupAttributes();
		m_testFactories.push_back(pFactory);
	}

	void CTestSystem::OnAssert(const char* szCondition, const char* szMessage, const char* szFileName, unsigned int fileLineNumber)
	{
		if (m_isRunningTest)
		{
			string cause;
			if (szMessage != nullptr && strlen(szMessage))
			{
				cause.Format("Assert: condition \"%s\" failed with \"%s\"", szCondition, szMessage);
			}
			else
			{
				cause.Format("Assert: \"%s\"", szCondition);
			}
			ReportNonCriticalError(cause.c_str(), szFileName, fileLineNumber);
		}
	}

	void CTestSystem::OnFatalError(const char* szMessage)
	{
		if (m_isRunningTest)
		{
			string cause;
			cause.Format("Fatal Error: %s", szMessage);
			ReportCriticalError(cause.c_str(), "", 0);
		}
	}

} // namespace CryTest


//void SimulateKey(unsigned vkey)
//{
//	INPUT ip;
//
//	// Set up a generic keyboard event.
//	ip.type = INPUT_KEYBOARD;
//	ip.ki.wScan = 0; // hardware scan code for key
//	ip.ki.time = 0;
//	ip.ki.dwExtraInfo = 0;
//
//	ip.ki.wVk = vkey;
//	ip.ki.dwFlags = 0; // 0 for key press
//	SendInput(1, &ip, sizeof(INPUT));
//
//	ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
//	SendInput(1, &ip, sizeof(INPUT));
//}
//
//class CCommandSimulateKey
//{
//public:
//	CCommandSimulateKey(unsigned vkey, float duration = 0.2f)
//		: m_vkey(vkey), m_duration(duration)
//	{
//	}
//
//	bool Update()
//	{
//		SimulateKey(m_vkey);
//		return true;
//	}
//
//	std::vector<CryTest::CCommand> GetSubCommands() const
//	{
//		return { CryTest::CCommandWait(m_duration) };
//	}
//
//private:
//
//	unsigned m_vkey = 0;
//	float    m_duration = 0;
//
//};
