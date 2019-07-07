// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include "CryTestNode.h"
#include <EditorStyleHelper.h>
#include <CrySandbox/CrySignal.h>
#include <CrySystem/Testing/TestInfo.h>
#include <CrySystem/Testing/ITestReporter.h>
#include <CrySystem/Testing/CryTest.h>
#include <QString>

class CCryTestRunnerSystem;

enum class ECryTestState
{
	NotRun,
	Running,
	Success,
	Failure
};

//! A helper struct to build comparable index for CryTest in the editor
struct SCryTestIndex
{
	QString moduleName;
	QString testName;

	bool operator<(const SCryTestIndex& other) const
	{
		return std::tie(moduleName, testName) < std::tie(other.moduleName, other.testName);
	}
};

//! Listens to test system events and redirects them to the plugin
class CCryTestRunnerReporter : public CryTest::ITestReporter
{

public:
	explicit CCryTestRunnerReporter(CCryTestRunnerSystem& system);

	virtual void OnStartTesting(const CryTest::SRunContext& context) override;

	virtual void OnFinishTesting(const CryTest::SRunContext& context, bool openReport) override;

	virtual void OnSingleTestStart(const CryTest::STestInfo& testInfo) override;

	virtual void OnSingleTestFinish(const CryTest::STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<CryTest::SError>& failures) override;

	virtual void SaveTemporaryReport() override
	{
	}

	virtual void RecoverTemporaryReport() override
	{
	}

	virtual bool HasTest(const CryTest::STestInfo& testInfo) const override
	{
		return false;
	}

private:

	CCryTestRunnerSystem& m_system;
};

//! Represents a single CryTest
class CCryTestEntry : public ICryTestNode
{
public:
	CCryTestEntry(ICryTestNode* pParent, CCryTestRunnerSystem& system, CryTest::CTestFactory* pFactory)
		: m_pParent(pParent)
		, m_system(system)
		, m_pFactory(pFactory)
	{}

	ECryTestState GetResult() const { return m_result; }

	QString       GetName() const
	{
		return QtUtil::ToQString(m_pFactory->GetTestInfo().name);
	}

	const CryTest::STestInfo GetTestInfo() const
	{
		return m_pFactory->GetTestInfo();
	}

	virtual ICryTestNode* GetParent() const override
	{
		return m_pParent;
	}

	virtual size_t GetChildrenCount() const override
	{
		return 0;
	}

	virtual QString GetDisplayName() const override;

	virtual QString GetTestSummary() const override;

	virtual void          SetIndex(int index) override
	{
		m_index = index;
	}

	virtual int           GetIndex() const override
	{
		return m_index;
	}

	virtual void RequireRefresh() override { }

	virtual void Run() override;

	virtual QString GetOutput() const override
	{
		return m_message;
	}

	void OnStart();

	void OnFinish(bool success, float durationMs, const std::vector<CryTest::SError>& failures);

private:

	ICryTestNode * m_pParent;
	CCryTestRunnerSystem&  m_system;
	CryTest::CTestFactory* m_pFactory;
	ECryTestState          m_result = ECryTestState::NotRun;
	float                  m_duration = 0;
	QString                m_message;
	int                    m_index = -1;
};

//! Represents a CRYENGINE module containing tests
class CCryTestModule : public ICryTestNode
{
public:
	explicit CCryTestModule(CCryTestRunnerSystem& system, const QString& moduleName)
		: m_system(system)
		, m_moduleName(moduleName)
	{}

	virtual size_t GetChildrenCount() const override;

	virtual QString GetDisplayName() const override;

	virtual QString GetTestSummary() const override;

	virtual QString GetOutput() const override;

	virtual void          RequireRefresh() override    { m_needRefresh = true; }
	virtual void          SetIndex(int index) override { m_index = index; }
	virtual int           GetIndex() const override    { return m_index; }
	virtual ICryTestNode* GetParent() const override   { return nullptr; }

	virtual void Run() override;

	void AddTestEntry(CryTest::CTestFactory* pFactory);

	DynArray<CCryTestEntry>& GetTestEntries()
	{
		return m_testEntries;
	}

private:

	void LazyRefresh() const;

	CCryTestRunnerSystem&   m_system;
	QString                 m_moduleName;
	DynArray<CCryTestEntry> m_testEntries;
	int                     m_index = -1;
	mutable QString         m_summary;
	mutable QString         m_output;
	mutable bool            m_needRefresh = false;
};

class CCryTestRunnerSystem
{
public:
	CCrySignal<void()>              SignalTestsStarted;
	CCrySignal<void()>              SignalTestsFinished;
	CCrySignal<void(ICryTestNode*)> SignalNodeSelected;

	~CCryTestRunnerSystem();

	void Init();

	const DynArray<CCryTestModule*>& GetTestModules() const { return m_modules; }

	void RunAll();

	void Run(const string& name);

	void Run(const DynArray<string>& names);

	void OnSingleTestStart(const CryTest::STestInfo& testInfo);

	void OnSingleTestFinish(const CryTest::STestInfo& testInfo, float fRunTimeInMs, bool bSuccess, const std::vector<CryTest::SError>& failures);

	CCryTestEntry* GetTestFromTestInfo(const CryTest::STestInfo& info);
private:

	//Note: creates new objects that need to be deleted
	DynArray<CCryTestModule*> PopulateTestModules();

	std::shared_ptr<CCryTestRunnerReporter> m_pReporter;
	DynArray<CCryTestModule*>               m_modules;

	// Cache test entries for quick lookup
	// If test entries change, this needs to be refreshed
	mutable std::map<SCryTestIndex, CCryTestEntry*> m_testsByIndex;
};

extern CCryTestRunnerSystem g_cryTestRunnerSystem;