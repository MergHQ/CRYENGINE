// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

//! Defines interfaces and macros for Cry Unit Tests.
//! CRY_UNIT_TEST_SUITE: Specify a suit name to group tests locally together.
//! CRY_UNIT_TEST: Specify a new test which is automatically registered.
//! CRY_UNIT_TEST_ASSERT: Fails and reports if the specified expression evaluates to false.
//! CRY_UNIT_TEST_CHECK_CLOSE: Fails and reports if the specified floating pointer values are not equal with respect to epsilon.
//! CRY_UNIT_TEST_CHECK_EQUAL: Fails and reports if the specified values are not equal.

#pragma once

#include <CrySystem/ITestSystem.h>
#include <CryString/StringUtils.h>

#if defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
	#include <exception>
#endif

namespace CryUnitTest
{
struct STest;
struct IUnitTestReporter;

//! Describes in what way the test result is reported
enum class EReporterType
{
	Excel,   //!< Writes multiple excel documents for detailed results
	Minimal, //!< Writes minimal logs
	Regular, //!< Writes detailed logs for every test
};

struct SUnitTestInfo
{
	STest&      test;
	char const* module = "";
	char const* suite = "";
	char const* name = "";
	char const* filename = "";
	int         lineNumber = 0;
	string      sFilename;           //!< Storage to keep Lua test file for test reporting. Currently not in use.

	//! Instantiates test info bound to a test instance
	SUnitTestInfo(STest& test)
		: test(test)
	{
	}

	//! Equality comparison. Comparing related test instances is sufficient.
	friend bool operator==(const SUnitTestInfo& lhs, const SUnitTestInfo& rhs)
	{
		return &lhs.test == &rhs.test;
	}
};

//! Defines info needed for script bound auto testing.
//! Currently not in use
struct SAutoTestInfo
{
	bool        runNextTest = true;  //!< To organize auto tests cycle.
	int         waitMSec = 0;        //!< Identify waiting period after each auto test.
	const char* szTaskName = 0;      //!< Current test task.
};

//! Base class for all user tests expanded through macros
struct STest
{
	virtual ~STest(){}

	//! Must be implemented by the test creator.
	virtual void Run() = 0;

	//! Optional methods called by the system at the beginning of the testing and at the end of the testing.
	virtual void Init() {};
	virtual void Done() {};

	SUnitTestInfo m_unitTestInfo { *this };
	SAutoTestInfo m_autoTestInfo;//!< Currently not in use
	STest*        m_pNext = nullptr;
	static STest* m_pFirst;
	static STest* m_pLast;
};

struct SUnitTestRunContext
{
	int testCount = 0;
	int failedTestCount = 0;
	int succedTestCount = 0;
	std::unique_ptr<IUnitTestReporter> pReporter;
};

struct IUnitTest
{
	virtual ~IUnitTest(){}
	virtual const SUnitTestInfo& GetInfo() const = 0;
	virtual const SAutoTestInfo& GetAutoTestInfo() const = 0;
	virtual void                 Init() = 0;
	virtual void                 Run() = 0;
	virtual void                 Done() = 0;
};

struct IUnitTestReporter
{
	virtual ~IUnitTestReporter(){}

	//! Notify reporter the test system started
	virtual void OnStartTesting(const SUnitTestRunContext& context) = 0;

	//! Notify reporter the test system finished
	virtual void OnFinishTesting(const SUnitTestRunContext& context) = 0;

	//! Notify reporter one test started
	virtual void OnSingleTestStart(const IUnitTest& test) = 0;

	//! Notify reporter one test finished, along with necessary results
	virtual void OnSingleTestFinish(const IUnitTest& test, float fRunTimeInMs, bool bSuccess, char const* szFailureDescription) = 0;
};

struct IUnitTestManager
{
	virtual ~IUnitTestManager(){}

	virtual IUnitTest* GetTestInstance(const SUnitTestInfo& info) = 0;

	void CreateTests(const char* moduleName = "")
	{
		for (STest* pTest = STest::m_pFirst; pTest != nullptr; pTest = pTest->m_pNext)
		{
			pTest->m_unitTestInfo.module = moduleName;
			GetTestInstance(pTest->m_unitTestInfo);
		}
	}

	//! Runs all test instances and returns exit code.
	//! \return 0 if all succeeded, non-zero when at least one test failed.
	virtual int  RunAllTests(EReporterType reporterType) = 0;

	virtual void RunAutoTests(const char* sSuiteName, const char* sTestName) = 0; //!< Currently not in use
	virtual void Update() = 0;//! Currently not in use

	//! 'callback' for failed tests, to prevent storing information in the exception, allowing use of setjmp/longjmp.
	virtual void SetExceptionCause(const char* expression, const char* file, int line) = 0;
};

#if defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
//! Helper classes.
class assert_exception
	: public std::exception
{
public:
	virtual ~assert_exception() throw() {}
	char m_description[256];
	char m_filename[256];
	int  m_lineNumber;
public:
	assert_exception(char const* szDescription, char const* szFilename, int lineNumber)
	{
		cry_strcpy(m_description, szDescription);
		cry_strcpy(m_filename, szFilename);
		m_lineNumber = lineNumber;
	}
	virtual char const* what() const throw()override { return m_description; };
};
#endif

//////////////////////////////////////////////////////////////////////////
class CAutoRegisterUnitTest
{
public:
	CAutoRegisterUnitTest(STest* pTest, const char* suite, const char* name, const char* filename, int line)
	{
		CRY_ASSERT(pTest != nullptr);

		if (!STest::m_pLast)
			STest::m_pFirst = pTest;
		else
			STest::m_pLast->m_pNext = pTest;
		STest::m_pLast = pTest;

		pTest->m_unitTestInfo.module = "";
		pTest->m_unitTestInfo.suite = suite;
		pTest->m_unitTestInfo.name = name;
		pTest->m_unitTestInfo.filename = filename;
		pTest->m_unitTestInfo.lineNumber = line;
	}
};
} // namespace CryUnitTest

//! Global Suite for all tests that do not specify suite.
namespace CryUnitTestSuite
{
inline const char* GetSuiteName() { return ""; }
}

#define CRY_UNIT_TEST_FIXTURE(FixureName) \
  struct FixureName : public CryUnitTest::STest

#define CRY_UNIT_TEST_NAME_WITH_FIXTURE(ClassName, TestName, Fixture)                                                                                                       \
  class ClassName : public Fixture                                                                                                                                          \
  {                                                                                                                                                                         \
    virtual void Run();                                                                                                                                                     \
  };                                                                                                                                                                        \
  ClassName auto_unittest_instance_ ## ClassName;                                                                                                                           \
  CryUnitTest::CAutoRegisterUnitTest autoreg_unittest_ ## ClassName(&auto_unittest_instance_ ## ClassName, CryUnitTestSuite::GetSuiteName(), TestName, __FILE__, __LINE__); \
  void ClassName::Run()

#define CRY_UNIT_TEST_NAME(ClassName, TestName)            CRY_UNIT_TEST_NAME_WITH_FIXTURE(ClassName, TestName, CryUnitTest::STest)
#define CRY_UNIT_TEST(ClassName)                           CRY_UNIT_TEST_NAME(ClassName, # ClassName)
#define CRY_UNIT_TEST_WITH_FIXTURE(ClassName, FixtureName) CRY_UNIT_TEST_NAME_WITH_FIXTURE(ClassName, # ClassName, FixtureName)

#define CRY_UNIT_TEST_REGISTER(ClassName)         \
  ClassName auto_unittest_instance_ ## ClassName; \
  CryUnitTest::CAutoRegisterUnitTest autoreg_unittest_ ## ClassName(&auto_unittest_instance_ ## ClassName, CryUnitTestSuite::GetSuiteName(), # ClassName, __FILE__, __LINE__);

#define CRY_UNIT_TEST_REGISTER_NAME(ClassName, TestName) \
  ClassName auto_unittest_instance_ ## ClassName;        \
  CryUnitTest::CAutoRegisterUnitTest autoreg_unittest_ ## ClassName(&auto_unittest_instance_ ## ClassName, CryUnitTestSuite::GetSuiteName(), TestName, __FILE__, __LINE__);

#define CRY_UNIT_TEST_SUITE(SuiteName)                      \
  namespace SuiteName {                                     \
  namespace CryUnitTestSuite {                              \
  inline char const* GetSuiteName() { return # SuiteName; } \
  }                                                         \
  }                                                         \
  namespace SuiteName

#define CRY_UNIT_TEST_ASSERT(condition)                                                                          \
  do                                                                                                             \
  {                                                                                                              \
  if (!(condition))                                                                                              \
  {                                                                                                              \
    gEnv->pSystem->GetITestSystem()->GetIUnitTestManager()->SetExceptionCause( # condition, __FILE__, __LINE__); \
  }                                                                                                              \
  } while (0)

#define CRY_UNIT_TEST_CHECK_CLOSE(valueA, valueB, epsilon)                                                                      \
  do                                                                                                                            \
  {                                                                                                                             \
    if (!(IsEquivalent(valueA, valueB, epsilon)))                                                                               \
    {                                                                                                                           \
      gEnv->pSystem->GetITestSystem()->GetIUnitTestManager()->SetExceptionCause( # valueA " != " # valueB, __FILE__, __LINE__); \
    }                                                                                                                           \
  } while (0)

ILINE string CryUnitTestFormatVar(float val)          { return string().Format("<float> %f", val);       }
ILINE string CryUnitTestFormatVar(double val)         { return string().Format("<double> %f", val);      }
ILINE string CryUnitTestFormatVar(int8 val)           { return string().Format("<int8> %d", val);        }
ILINE string CryUnitTestFormatVar(uint8 val)          { return string().Format("<uint8> %u", val);       }
ILINE string CryUnitTestFormatVar(int16 val)          { return string().Format("<int16> %d", val);       }
ILINE string CryUnitTestFormatVar(uint16 val)         { return string().Format("<uint16> %u", val);      }
ILINE string CryUnitTestFormatVar(int32 val)          { return string().Format("<int32> %d", val);       }
ILINE string CryUnitTestFormatVar(uint32 val)         { return string().Format("<uint32> %u", val);      }
ILINE string CryUnitTestFormatVar(int64 val)          { return string().Format("<int64> %" PRId64, val); }
ILINE string CryUnitTestFormatVar(uint64 val)         { return string().Format("<uint64> %" PRIu64, val); }
ILINE string CryUnitTestFormatVar(void* val)          { return string().Format("<pointer> %p", val);     }
ILINE string CryUnitTestFormatVar(const string& val)  { return string().Format("\"%s\"", val.c_str());   }
ILINE string CryUnitTestFormatVar(const wstring& val) { return string().Format("\"%ls\"", val.c_str());  }

#define CRY_UNIT_TEST_CHECK_EQUAL(valueA, valueB)                                                                     \
  do                                                                                                                  \
  {                                                                                                                   \
    if (!(valueA == valueB))                                                                                          \
    {                                                                                                                 \
      stack_string message = # valueA " != " # valueB " [";                                                           \
      message.append(CryUnitTestFormatVar(valueA).c_str()).append(" != ");                                            \
      message.append(CryUnitTestFormatVar(valueB).c_str()).append("]");                                               \
      gEnv->pSystem->GetITestSystem()->GetIUnitTestManager()->SetExceptionCause(message.c_str(), __FILE__, __LINE__); \
    }                                                                                                                 \
  } while (0)

#define CRY_UNIT_TEST_CHECK_DIFFERENT(valueA, valueB)                                                                 \
  do                                                                                                                  \
  {                                                                                                                   \
    if (!(valueA != valueB))                                                                                          \
    {                                                                                                                 \
      stack_string message = # valueA " == " # valueB " [";                                                           \
      message.append(CryUnitTestFormatVar(valueA).c_str()).append(" != ");                                            \
      message.append(CryUnitTestFormatVar(valueB).c_str()).append("]");                                               \
      gEnv->pSystem->GetITestSystem()->GetIUnitTestManager()->SetExceptionCause(message.c_str(), __FILE__, __LINE__); \
    }                                                                                                                 \
  } while (0)
//The last line must exist to compile on gcc!
