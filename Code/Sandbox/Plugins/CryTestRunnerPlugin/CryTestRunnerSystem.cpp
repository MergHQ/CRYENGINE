// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "CryTestRunnerSystem.h"

void CCryTestRunnerReporter::OnSingleTestStart(const CryTest::STestInfo& testInfo)
{
	m_system.OnSingleTestStart(testInfo);
}

void CCryTestRunnerReporter::OnSingleTestFinish(const CryTest::STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<CryTest::SError>& failures)
{
	m_system.OnSingleTestFinish(testInfo, fRunTimeInMs, bSuccess, failures);
}

CCryTestRunnerReporter::CCryTestRunnerReporter(CCryTestRunnerSystem& system) : m_system(system)
{

}

void CCryTestRunnerReporter::OnStartTesting(const CryTest::SRunContext& context)
{
	m_system.SignalTestsStarted();
}

void CCryTestRunnerReporter::OnFinishTesting(const CryTest::SRunContext& context, bool openReport)
{
	m_system.SignalTestsFinished();
}

CCryTestRunnerSystem g_cryTestRunnerSystem;

CCryTestRunnerSystem::~CCryTestRunnerSystem()
{
	for (CCryTestModule* pModule : m_modules)
	{
		delete pModule;
	}
}

void CCryTestRunnerSystem::Init()
{
	m_pReporter = std::make_shared<CCryTestRunnerReporter>(*this);
	m_modules = PopulateTestModules();
}

void CCryTestRunnerSystem::RunAll()
{
	CryTest::ITestSystem* pTestSystem = gEnv->pSystem->GetITestSystem();
	pTestSystem->SetReporter(m_pReporter);
	pTestSystem->Run();
}

void CCryTestRunnerSystem::Run(const string& name)
{
	CryTest::ITestSystem* pTestSystem = gEnv->pSystem->GetITestSystem();
	pTestSystem->SetReporter(m_pReporter);
	pTestSystem->RunSingle(name);
}

void CCryTestRunnerSystem::Run(const DynArray<string>& names)
{
	CryTest::ITestSystem* pTestSystem = gEnv->pSystem->GetITestSystem();
	pTestSystem->SetReporter(m_pReporter);
	pTestSystem->RunTestsByName(names);
}

void CCryTestRunnerSystem::OnSingleTestStart(const CryTest::STestInfo& testInfo)
{
	CCryTestEntry* pTestEntry = GetTestFromTestInfo(testInfo);
	CRY_ASSERT(pTestEntry);
	pTestEntry->OnStart();
}

void CCryTestRunnerSystem::OnSingleTestFinish(const CryTest::STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<CryTest::SError>& failures)
{
	CCryTestEntry* pTestEntry = GetTestFromTestInfo(testInfo);
	CRY_ASSERT(pTestEntry);
	pTestEntry->OnFinish(bSuccess, fRunTimeInMs, failures);
}

CCryTestEntry* CCryTestRunnerSystem::GetTestFromTestInfo(const CryTest::STestInfo& info)
{
	auto CryTestIndexFromTestInfo = [](const CryTest::STestInfo& info)
	{
		return SCryTestIndex{ QtUtil::ToQString(info.module), QtUtil::ToQString(info.name) };
	};

	//use cache to speed up the look up
	if (m_testsByIndex.empty())
	{
		for (CCryTestModule* pTestModule : m_modules)
		{
			for (CCryTestEntry& entry : pTestModule->GetTestEntries())
			{
				m_testsByIndex[CryTestIndexFromTestInfo(entry.GetTestInfo())] = &entry;
			}
		}
	}
	return m_testsByIndex.at(CryTestIndexFromTestInfo(info));
}

DynArray<CCryTestModule*> CCryTestRunnerSystem::PopulateTestModules()
{
	std::map<QString, std::set<CryTest::CTestFactory*>> testsByModuleName;
	DynArray<CryTest::CTestFactory*> testFactories = gEnv->pSystem->GetITestSystem()->GetFactories();
	for (CryTest::CTestFactory* pFactory : testFactories)
	{
		QString module = QtUtil::ToQString(pFactory->GetTestInfo().module);
		testsByModuleName[module].insert(pFactory);
	}

	DynArray<CCryTestModule*> result;
	int index = 0;
	for (auto& pair : testsByModuleName)
	{
		CCryTestModule* pModule = new CCryTestModule(*this, pair.first);
		for (CryTest::CTestFactory* pFactory : pair.second)
		{
			pModule->AddTestEntry(pFactory);
		}
		result.push_back(pModule);
		pModule->SetIndex(index);
		++index;
	}
	return result;
}

QString CCryTestEntry::GetDisplayName() const
{
	QString displayName = QtUtil::ToQString(m_pFactory->GetTestInfo().name);
	// by default, the editor plugin runs all editor tests.
	// Game-only tests are marked as such and not run
	if (!m_pFactory->IsEnabledForEditor())
	{
		displayName += " (Game only)";
	}
	return displayName;
}

QString CCryTestEntry::GetTestSummary() const
{
	switch (m_result)
	{
	case ECryTestState::NotRun:
		return QString(""); //blank on purpose
	case ECryTestState::Running:
		return QString("Running ...");
	case ECryTestState::Failure:
		return QString("Fail");
	case ECryTestState::Success:
		return QString("Success");
	default:
		break;
	}
	return QString("");
}

void CCryTestEntry::Run()
{
	m_system.Run(m_pFactory->GetTestInfo().name);
}

void CCryTestEntry::OnStart()
{
	m_result = ECryTestState::Running;
	m_duration = 0;
	m_message = "";
	m_pParent->RequireRefresh();
}

void CCryTestEntry::OnFinish(bool success, float durationMs, const std::vector<CryTest::SError>& failures)
{
	m_result = success ? ECryTestState::Success : ECryTestState::Failure;
	m_duration = durationMs;
	m_message = QString("Duration: %1ms").arg(m_duration);
	if (!failures.empty())
	{
		m_message += "\n";
		for (const CryTest::SError& failure : failures)
		{
			m_message += QString("%1 at %2 line %3\n").arg(QtUtil::ToQString(failure.message), QtUtil::ToQString(failure.fileName), QString::number(failure.lineNumber));
		}
	}
	m_pParent->RequireRefresh();
}

size_t CCryTestModule::GetChildrenCount() const
{
	return m_testEntries.size();
}

QString CCryTestModule::GetDisplayName() const
{
	return m_moduleName + "(" + QString::number(m_testEntries.size()) + ")";
}

QString CCryTestModule::GetTestSummary() const
{
	LazyRefresh();
	return m_summary;
}

QString CCryTestModule::GetOutput() const
{
	LazyRefresh();
	return m_output;
}

void CCryTestModule::Run()
{
	DynArray<string> names;
	for (CCryTestEntry& entry : m_testEntries)
	{
		names.push_back(entry.GetTestInfo().name);
	}
	m_system.Run(names);
}

void CCryTestModule::AddTestEntry(CryTest::CTestFactory* pFactory)
{
	m_testEntries.emplace_back(this, m_system, pFactory);
	m_testEntries.back().SetIndex(m_testEntries.size() - 1);
}

void CCryTestModule::LazyRefresh() const
{
	auto ConvertResultColoredText = [](ECryTestState state)
	{
		QColor color;
		const char* msg;
		switch (state)
		{
		case ECryTestState::NotRun:
			color = GetStyleHelper()->textColor();
			msg = "Not run";
			break;
		case ECryTestState::Running:
			color = GetStyleHelper()->textColor();
			msg = "Running";
			break;
		case ECryTestState::Success:
			color = GetStyleHelper()->successColor();
			msg = "Success";
			break;
		case ECryTestState::Failure:
			color = GetStyleHelper()->errorColor();
			msg = "Fail";
			break;
		default:
			break;
		}
		return QString("<a style=\"color:") + color.name() + "\">" + msg + "</a>";
	};

	auto RefreshSummary = [this]
	{
		int successCount = 0;
		int failureCount = 0;
		int notRunCount = 0;
		for (const CCryTestEntry& entry : m_testEntries)
		{
			switch (entry.GetResult())
			{
			case ECryTestState::Success:
				++successCount;
				break;
			case ECryTestState::Failure:
				++failureCount;
				break;
			default:
				++notRunCount;
				break;
			}
		}
		m_summary = QString("%1 passed, %2 failed, %3 not run").arg(successCount).arg(failureCount).arg(notRunCount);
	};

	auto RefreshOutput = [this, ConvertResultColoredText]
	{
		m_output.clear();
		for (const CCryTestEntry& entry : m_testEntries)
		{
			// Only tests that already have an outcome are shown in the category view
			if (entry.GetResult() == ECryTestState::Success || entry.GetResult() == ECryTestState::Failure)
			{
				m_output +=
					"Result of [" + entry.GetName() + "]: " +
					ConvertResultColoredText(entry.GetResult()) +
					"<br>";
				m_output += entry.GetOutput() + "<br><br>";
			}
		}
	};

	if (m_needRefresh)
	{
		RefreshSummary();
		RefreshOutput();
		m_needRefresh = false;
	}
}
