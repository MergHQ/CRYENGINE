// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

//! \cond INTERNAL

//! Defines interfaces and macros for Cry Tests.
//! CRY_TEST_SUITE:           A suite name to group tests locally together.
//! CRY_TEST_FIXTURE:         A fixture (intermediate base class) as the container of variables and methods shared by tests.
//! CRY_TEST:                 A standard test block.
//! CRY_TEST_WITH_FIXTURE:    A test with a fixture as the base class.
//! CRY_TEST_ASSERT:          Fails and reports if the specified expression evaluates to false.
//! CRY_TEST_CHECK_CLOSE:     Fails and reports if the specified floating point values are not equal with respect to epsilon.
//! CRY_TEST_CHECK_EQUAL:     Fails and reports if the specified values are not equal.
//! CRY_TEST_CHECK_DIFFERENT: Fails and reports if the specified values are not different.
//!
//! Usage of comparison checks should be favored over the generic CRY_TEST_ASSERT because of detailed output in the report.

#pragma once

#include <CryString/StringUtils.h>
#include <CryCore/StaticInstanceList.h>
#include <type_traits>
#include <deque>
#if defined(CRY_TESTING_USE_EXCEPTIONS)
	#include <exception>
#endif
#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include "ITestSystem.h"
#include "Command.h"
#include "TestInfo.h"

namespace CryTest
{
#if defined(CRY_TESTING_USE_EXCEPTIONS)
//! Helper classes.
class assert_exception
	: public std::exception
{
public:
	virtual ~assert_exception() noexcept {}
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
	virtual char const* what() const noexcept override { return m_description; }
};
#endif

//! Global Suite for all tests that do not specify suite.
namespace CryTestSuite
{
inline const char* GetSuiteName() { return ""; }
}

//! Implementation, do not use directly
namespace impl
{
ILINE string FormatVar(float val)          { return string().Format("<float> %f", val); }
ILINE string FormatVar(double val)         { return string().Format("<double> %f", val); }
ILINE string FormatVar(int8 val)           { return string().Format("<int8> %d", val); }
ILINE string FormatVar(uint8 val)          { return string().Format("<uint8> %u (0x%x)", val, val); }
ILINE string FormatVar(int16 val)          { return string().Format("<int16> %d", val); }
ILINE string FormatVar(uint16 val)         { return string().Format("<uint16> %u (0x%x)", val, val); }
ILINE string FormatVar(int32 val)          { return string().Format("<int32> %d", val); }
ILINE string FormatVar(uint32 val)         { return string().Format("<uint32> %u (0x%x)", val, val); }
ILINE string FormatVar(int64 val)          { return string().Format("<int64> %" PRId64, val); }
ILINE string FormatVar(uint64 val)         { return string().Format("<uint64> %" PRIu64 " (0x" PRIx64 ")", val, val); }
ILINE string FormatVar(const char* val)    { return string().Format("\"%s\"", val); }
ILINE string FormatVar(const wchar_t* val) { return string().Format("\"%ls\"", val); }
ILINE string FormatVar(const string& val)  { return string().Format("\"%s\"", val.c_str()); }
ILINE string FormatVar(const wstring& val) { return string().Format("\"%ls\"", val.c_str()); }
ILINE string FormatVar(std::nullptr_t)     { return "nullptr"; }

template<typename T>
ILINE
typename std::enable_if<std::is_pointer<T>::value, string>::type
FormatVar(T val)
{
	return string().Format("<pointer> %p", val);
}

//! Fall-back overload for unsupported types: dump bytes
//! Taking const reference because not all types can be copied or copied without side effect.
template<typename T>
ILINE
typename std::enable_if<!std::is_pointer<T>::value, string>::type
FormatVar(const T& val)
{
	string result = "<unknown> ";
	const char* separator = "";
	for (const uint8* p = reinterpret_cast<const uint8*>(std::addressof(val)), * end = p + sizeof(T);
	     p != end; ++p)
	{
		result.Append(separator);
		result.AppendFormat("%02x", *p);
		separator = " ";
	}
	return result;
}

//! Equality comparison
template<typename T, typename U>
ILINE bool AreEqual(T&& t, U&& u) { return t == u; }

//! Inequality comparison, note this is necessary as it is more accurate to use operator!= for the types rather than the negation of '=='
template<typename T, typename U>
ILINE bool AreInequal(T&& t, U&& u) { return t != u; }

//! Equality comparison overloaded for types that do not compare equality with '==', such as raw string.
ILINE bool AreEqual(const char* str1, const char* str2) { return strcmp(str1, str2) == 0; }

template<std::size_t M, std::size_t N>
ILINE bool AreEqual(const char(&str1)[M], const char(&str2)[N]) { return strcmp(str1, str2) == 0; }

ILINE bool AreInequal(const char* str1, const char* str2)       { return strcmp(str1, str2) != 0; }

template<std::size_t M, std::size_t N>
ILINE bool  AreInequal(const char(&str1)[M], const char(&str2)[N]) { return strcmp(str1, str2) != 0; }

inline void ReportError(const char* file, int line, const char* conditionExpression)
{
#ifdef CRY_TESTING
	gEnv->pSystem->GetITestSystem()->ReportNonCriticalError(conditionExpression, file, line);
#endif // CRY_TESTING
}

inline void ReportError(const char* file, int line, string conditionExpression, const string& message)
{
#ifdef CRY_TESTING
	conditionExpression.append(" message:");
	conditionExpression.append(message);
	gEnv->pSystem->GetITestSystem()->ReportNonCriticalError(conditionExpression.c_str(), file, line);
#endif // CRY_TESTING
}

template<typename ... ArgsT>
void ReportError(const char* file, int line, string conditionExpression, const char* format, ArgsT&& ... args)
{
#ifdef CRY_TESTING
	conditionExpression.append(" message:");
	conditionExpression.AppendFormat(format, std::forward<ArgsT>(args) ...);
	gEnv->pSystem->GetITestSystem()->ReportNonCriticalError(conditionExpression.c_str(), file, line);
#endif // CRY_TESTING
}

}

//! Base class for all user tests expanded through macros
class CTest
{
public:
	virtual ~CTest() {}

	//! Must be implemented by the test creator.
	virtual void Run() = 0;

	//! Optional methods called by the system at the beginning of the testing and at the end of the testing.
	virtual void Init() {}
	virtual void Done() {}

protected:

	friend class CTestSystem;
	template<class TestClass> friend class CConcreteTestFactory;

	bool UpdateCommands()
	{
		while (commands.size() > 0)
		{
			CCommand& front = commands.front();
			bool bFinished = front.Update();
			std::vector<CCommand> subCommands = front.GetSubCommands();
			if (bFinished)
			{
				commands.pop_front();
			}

			//in case it returns subcommands, add them in front of the command
			if (!subCommands.empty())
			{
				commands.insert(commands.begin(), subCommands.begin(), subCommands.end());
			}

			if (!bFinished)
				break;  //do not repeat testing unfinished command
		}
		return commands.size() == 0;
	}

	//user may manipulate commands at runtime. The name is kept in sync with CTestFactory::commands
	std::deque<CCommand> commands;
};

//! Helper class whose sole purpose is to register static test instances defined in separate files to the system.
class CTestFactory : public CStaticInstanceList<CTestFactory>
{
public:

	CTestFactory(const char* szSuite, const char* szName, const char* szFileName, int line)
	{
		m_TestInfo.suite = szSuite;
		m_TestInfo.name = szName;
		m_TestInfo.fileName = szFileName;
		m_TestInfo.lineNumber = line;
	}
	virtual ~CTestFactory() {}
	CTestFactory(const CTestFactory&) = delete;

	virtual std::unique_ptr<CTest> CreateTest() const = 0;
	virtual void                   SetupAttributes() = 0;

	inline const STestInfo& GetTestInfo() const                     { return m_TestInfo; }
	void                    SetModuleName(const char* szModuleName) { m_TestInfo.module = szModuleName; }

	inline float            GetTimeOut() const                      { return timeout; }
	inline bool             IsEnabledForGame() const                { return game; }
	inline bool             IsEnabledForEditor() const              { return editor; }

protected:
	STestInfo m_TestInfo;

	//naming of following members needs to differ from coding convention due to macro statements
	std::vector<CCommand> commands;
	float                 timeout = 0;
	bool                  game = true;
	bool                  editor = true;
};

template<class TestClass>
class CConcreteTestFactory : public CTestFactory
{
public:
	using CTestFactory::CTestFactory;
private:
	virtual std::unique_ptr<CTest> CreateTest() const override;
};

template<class TestClass>
std::unique_ptr<CTest> CConcreteTestFactory<TestClass >::CreateTest() const
{
	auto test = stl::make_unique<TestClass>();
	test->commands = std::deque<CCommand>(this->commands.begin(), this->commands.end());
	return std::move(test);
}

} // namespace CryTest

//! Specifies a new test.
#define CRY_TEST(ClassName, ...)                                                                                                    \
  class ClassName : public ::CryTest::CTest                                                                                         \
  {                                                                                                                                 \
    virtual void Run() override;                                                                                                    \
  };                                                                                                                                \
  class ClassName ## TestFactory : public ::CryTest::CConcreteTestFactory<ClassName> {                                              \
    using ::CryTest::CConcreteTestFactory<ClassName>::CConcreteTestFactory;                                                         \
    virtual void SetupAttributes() override { __VA_ARGS__; }                                                                        \
  };                                                                                                                                \
  ClassName ## TestFactory autoreg_test_ ## ClassName { ::CryTest::CryTestSuite::GetSuiteName(), # ClassName, __FILE__, __LINE__ }; \
  void ClassName::Run()

//! Defines a fixture (intermediate base class) as the container of variables and methods shared by tests.
#define CRY_TEST_FIXTURE(FixureName) \
  struct FixureName : public ::CryTest::CTest

//! Specifies a new test with a fixture as the base class.
#define CRY_TEST_WITH_FIXTURE(ClassName, FixtureName, ...)                                                                          \
  class ClassName : public FixtureName                                                                                              \
  {                                                                                                                                 \
    virtual void Run();                                                                                                             \
  };                                                                                                                                \
  class ClassName ## TestFactory : public ::CryTest::CConcreteTestFactory<ClassName> {                                              \
    using ::CryTest::CConcreteTestFactory<ClassName>::CConcreteTestFactory;                                                         \
    virtual void SetupAttributes() override { __VA_ARGS__; }                                                                        \
  };                                                                                                                                \
  ClassName ## TestFactory autoreg_test_ ## ClassName { ::CryTest::CryTestSuite::GetSuiteName(), # ClassName, __FILE__, __LINE__ }; \
  void ClassName::Run()

//! Specifies a suite name to group tests locally together.
#define CRY_TEST_SUITE(SuiteName)                           \
  namespace SuiteName {                                     \
  namespace CryTestSuite {                                  \
  inline char const* GetSuiteName() { return # SuiteName; } \
  }                                                         \
  }                                                         \
  namespace SuiteName

//! Traps the debugger when attached.
//! Implemented as a Macro instead of function (see CryDebugBreak) to avoid additional layers in the call stack when debugging.
#if CRY_PLATFORM_WINDOWS
#define CRY_TEST_DEBUG_BREAK() if (IsDebuggerPresent()){__debugbreak();}
#else
#define CRY_TEST_DEBUG_BREAK() __debugbreak()
#endif

//! Fails and reports if the specified expression evaluates to false.
//! Usage: CRY_TEST_ASSERT(expression)
//! Usage: CRY_TEST_ASSERT(expression, errorMsg)              where errorMsg is string or string literal
//! Usage: CRY_TEST_ASSERT(expression, errorMsgFormat, ...)   where errorMsgFormat is string literal
#define CRY_TEST_ASSERT(expr, ...)                                            \
  do                                                                          \
  {                                                                           \
  if (!(expr))                                                                \
  {                                                                           \
    CRY_TEST_DEBUG_BREAK();                                                   \
    ::CryTest::impl::ReportError(__FILE__, __LINE__, # expr, ## __VA_ARGS__); \
  }                                                                           \
  } while (0)

//! Fails and reports if the specified floating point values are not equal with respect to epsilon.
//! Usage: CRY_TEST_CHECK_CLOSE(valueA, valueB, epsilon)
//! Usage: CRY_TEST_CHECK_CLOSE(valueA, valueB, epsilon, errorMsg)              where errorMsg is string or string literal
//! Usage: CRY_TEST_CHECK_CLOSE(valueA, valueB, epsilon, errorMsgFormat, ...)   where errorMsgFormat is string literal
#define CRY_TEST_CHECK_CLOSE(valueA, valueB, epsilon, ...)                                  \
  do                                                                                        \
  {                                                                                         \
    if (!(IsEquivalent(valueA, valueB, epsilon)))                                           \
    {                                                                                       \
      CRY_TEST_DEBUG_BREAK();                                                               \
      string message = # valueA " != " # valueB " with epsilon " # epsilon " [";            \
      message.append(::CryTest::impl::FormatVar(valueA)).append(" != ");                    \
      message.append(::CryTest::impl::FormatVar(valueB)).append("]");                       \
      ::CryTest::impl::ReportError(__FILE__, __LINE__, std::move(message), ## __VA_ARGS__); \
    }                                                                                       \
  } while (0)

//! Fails and reports if the specified values are not equal.
//! Usage: CRY_TEST_CHECK_EQUAL(valueA, valueB)
//! Usage: CRY_TEST_CHECK_EQUAL(valueA, valueB, errorMsg)              where errorMsg is string or string literal
//! Usage: CRY_TEST_CHECK_EQUAL(valueA, valueB, errorMsgFormat, ...)   where errorMsgFormat is string literal
#define CRY_TEST_CHECK_EQUAL(valueA, valueB, ...)                                           \
  do                                                                                        \
  {                                                                                         \
    if (!::CryTest::impl::AreEqual(valueA, valueB))                                         \
    {                                                                                       \
      CRY_TEST_DEBUG_BREAK();                                                               \
      string message = # valueA " != " # valueB " [";                                       \
      message.append(::CryTest::impl::FormatVar(valueA)).append(" != ");                    \
      message.append(::CryTest::impl::FormatVar(valueB)).append("]");                       \
      ::CryTest::impl::ReportError(__FILE__, __LINE__, std::move(message), ## __VA_ARGS__); \
    }                                                                                       \
  } while (0)

//! Fails and reports if the specified values are not different.
//! Usage: CRY_TEST_CHECK_DIFFERENT(valueA, valueB)
//! Usage: CRY_TEST_CHECK_DIFFERENT(valueA, valueB, errorMsg)              where errorMsg is string or string literal
//! Usage: CRY_TEST_CHECK_DIFFERENT(valueA, valueB, errorMsgFormat, ...)   where errorMsgFormat is string literal
#define CRY_TEST_CHECK_DIFFERENT(valueA, valueB, ...)                                       \
  do                                                                                        \
  {                                                                                         \
    if (!::CryTest::impl::AreInequal(valueA, valueB))                                       \
    {                                                                                       \
      CRY_TEST_DEBUG_BREAK();                                                               \
      string message = # valueA " == " # valueB " [";                                       \
      message.append(::CryTest::impl::FormatVar(valueA)).append(" != ");                    \
      message.append(::CryTest::impl::FormatVar(valueB)).append("]");                       \
      ::CryTest::impl::ReportError(__FILE__, __LINE__, std::move(message), ## __VA_ARGS__); \
    }                                                                                       \
  } while (0)

//The last line must exist to compile on gcc!

//! \endcond