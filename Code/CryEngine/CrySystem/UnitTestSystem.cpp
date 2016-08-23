// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "UnitTestSystem.h"
#include <CryMemory/HeapAllocator.h>
#include <CryString/StringUtils.h>  // cry_strXXX()
#include <CryCore/CryCustomTypes.h> // CRY_ARRAY_COUNT

#if !defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
	#include <setjmp.h>
#endif

#include <CrySystem/ISystem.h>
#include <CrySystem/ITimer.h>
#include <CrySystem/IConsole.h>
#include "UnitTestExcelReporter.h"

using namespace CryUnitTest;

#if !defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
jmp_buf* GetAssertJmpBuf()
{
	static jmp_buf s_jmpBuf;
	return &s_jmpBuf;
}
#endif

struct SAutoTestsContext
{
	float                           fStartTime;
	float                           fCurrTime;
	string                          sSuiteName;
	string                          sTestName;
	int                             waitAfterMSec;
	int                             iter;
	int                             runCount;
	CryUnitTest::UnitTestRunContext context;

	SAutoTestsContext() : fStartTime(0), fCurrTime(0), waitAfterMSec(0), iter(-1), runCount(1) { memset(&context, 0, sizeof(context)); }
};

CUnitTestManager::CUnitTestManager()
	: m_pCurrentTest(nullptr)
{
	m_failureMsg[0] = '\0';
	m_pAutoTestsContext = new SAutoTestsContext;
	assert(m_pAutoTestsContext);

	// Register tests defined in this module
	CreateTests(CryUnitTest::Test::m_pFirst, "CrySystem");
}

CUnitTestManager::~CUnitTestManager()
{
	delete m_pAutoTestsContext;
	m_pAutoTestsContext = 0;

	RemoveTests();
}

void CUnitTestManager::RemoveTests()
{
	for (size_t i = 0; i < m_tests.size(); ++i)
		delete m_tests[i];

	m_tests.clear();
}

IUnitTest* CUnitTestManager::CreateTest(const UnitTestInfo& info)
{
	for (size_t i = 0; i < m_tests.size(); ++i)
	{
		CUnitTest* pt = m_tests[i];

		if (pt->m_info.pTestImpl == info.pTestImpl)
			return pt;
	}

	CUnitTest* pTest = new CUnitTest(info);
	m_tests.push_back(pTest);
	return pTest;
}

int CUnitTestManager::RunAllTests(CryUnitTest::TReporterToUse reporterToUse)
{
	CryUnitTest::UnitTestRunContext context;
	StartTesting(context, reporterToUse);

	for (uint32 i = 0; i < m_tests.size(); i++)
	{
		m_tests[i]->Init();
		RunTest(m_tests[i], context);
		m_tests[i]->Done();
	}

	EndTesting(context);
	return (context.failedTestCount == 0) ? 0 : 1;
}

void CUnitTestManager::RunMatchingTests(const char* sName, UnitTestRunContext& context)
{
	StartTesting(context, CryUnitTest::ExcelReporter);

	for (uint32 i = 0; i < m_tests.size(); i++)
	{
		if (strstr(m_tests[i]->m_info.name, sName) != 0 || strcmp(m_tests[i]->m_info.suite, sName) == 0 || strcmp(m_tests[i]->m_info.module, sName) == 0)
		{
			m_tests[i]->Init();
			RunTest(m_tests[i], context);
			m_tests[i]->Done();
		}
	}

	EndTesting(context);
}

void CUnitTestManager::RunAutoTests(const char* sSuiteName, const char* sTestName)
{
	// prepare auto tests context
	// tests actually will be called during Update call
	m_pAutoTestsContext->fStartTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
	m_pAutoTestsContext->fCurrTime = m_pAutoTestsContext->fStartTime;
	m_pAutoTestsContext->sSuiteName = sSuiteName;
	m_pAutoTestsContext->sTestName = sTestName;
	m_pAutoTestsContext->waitAfterMSec = 0;
	m_pAutoTestsContext->iter = 0;
	m_pAutoTestsContext->runCount = max(gEnv->pConsole->GetCVar("ats_loop")->GetIVal(), 1);

	StartTesting(m_pAutoTestsContext->context, CryUnitTest::ExcelReporter);
}

void CUnitTestManager::Update()
{
	if (m_pAutoTestsContext->iter != -1)
	{
		m_pAutoTestsContext->fCurrTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();

		if ((m_pAutoTestsContext->fCurrTime - m_pAutoTestsContext->fStartTime) > m_pAutoTestsContext->waitAfterMSec)
		{
			bool wasFound = false;

			for (size_t i = m_pAutoTestsContext->iter; i < m_tests.size(); i++)
			{
				if (IsTestMatch(m_tests[i], m_pAutoTestsContext->sSuiteName, m_pAutoTestsContext->sTestName))
				{
					m_tests[i]->Init();
					RunTest(m_tests[i], m_pAutoTestsContext->context);
					m_tests[i]->Done();

					AutoTestInfo info;
					m_tests[i]->GetAutoTestInfo(info);
					m_pAutoTestsContext->waitAfterMSec = info.waitMSec;
					m_pAutoTestsContext->iter = i;

					if (info.runNextTest)
						m_pAutoTestsContext->iter++;

					wasFound = true;
					break;
				}
			}
			if (!wasFound)
			{
				// no tests were found so stop testing
				m_pAutoTestsContext->runCount--;

				if (0 == m_pAutoTestsContext->runCount)
				{
					EndTesting(m_pAutoTestsContext->context);
					m_pAutoTestsContext->iter = -1;

					if (gEnv->pConsole->GetCVar("ats_exit")->GetIVal())
						exit(0);
				}
				else
				{
					m_pAutoTestsContext->waitAfterMSec = 0;
					m_pAutoTestsContext->iter = 0;
				}
			}
			m_pAutoTestsContext->fStartTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
			m_pAutoTestsContext->fCurrTime = m_pAutoTestsContext->fStartTime;
		}
	}
}

void CUnitTestManager::StartTesting(UnitTestRunContext& context, CryUnitTest::TReporterToUse reporterToUse)
{
	context.testCount = 0;
	context.failedTestCount = 0;
	context.succedTestCount = 0;

	switch (reporterToUse)
	{
	case ExcelReporter:
		context.pReporter = new CUnitTestExcelReporter;
		break;
	case MinimalReporter:
		context.pReporter = new CMinimalLogUnitTestReporter;
		break;
	case RegularReporter:
		context.pReporter = new CLogUnitTestReporter;
		break;
	default:
		assert(0);
	}

	if (context.pReporter)
		context.pReporter->OnStartTesting(context);
}

void CUnitTestManager::EndTesting(UnitTestRunContext& context)
{
	if (context.pReporter)
		context.pReporter->OnFinishTesting(context);
}

void CUnitTestManager::RunTest(IUnitTest* pTest, UnitTestRunContext& context)
{

	m_failureMsg[0] = 0;

	bool bFail = false;

	if (context.pReporter)
		context.pReporter->OnTestStart(pTest);

	context.testCount++;

	CTimeValue t0 = gEnv->pTimer->GetAsyncTime();

#if defined(CRY_UNIT_TESTING)
	m_pCurrentTest = pTest; // store test to be able to handle exception cause callbacks

	#if !defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
	if (setjmp(*GetAssertJmpBuf()) == 0)
	#else
	try
	#endif
	{
		pTest->Run(context);
		context.succedTestCount++;
	}
	#if !defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
	else
	{
		context.failedTestCount++;
		bFail = true;
	}
	#else
	catch (assert_exception const& e)
	{
		context.failedTestCount++;
		bFail = true;
		cry_strcpy(m_failureMsg, e.what());

		// copy filename and line number of unit test assert
		// will be used later for test reporting
		CUnitTest* pT = static_cast<CUnitTest*>(pTest);
		pT->m_info.sFilename = e.m_filename;
		pT->m_info.filename = pT->m_info.sFilename.c_str();
		pT->m_info.lineNumber = e.m_lineNumber;
	}
	catch (std::exception const& e)
	{
		context.failedTestCount++;

		bFail = true;
		cry_strcpy(m_failureMsg, "Unhandled exception: ");
		cry_strcpy(m_failureMsg, e.what());
	}
	catch (...)
	{
		context.failedTestCount++;

		bFail = true;
		cry_strcpy(m_failureMsg, "Crash");
	}
	#endif

	m_pCurrentTest = NULL;
#endif
	CTimeValue t1 = gEnv->pTimer->GetAsyncTime();

	float fRunTimeInMs = (t1 - t0).GetMilliSeconds();

	if (context.pReporter)
		context.pReporter->OnTestFinish(pTest, fRunTimeInMs, !bFail, m_failureMsg);
}

void CUnitTestManager::SetExceptionCause(const char* expression, const char* file, int line)
{
	CUnitTest* pT = static_cast<CUnitTest*>(m_pCurrentTest);
	if (pT)
	{
		pT->m_info.sFilename = file;
		pT->m_info.filename = file;
		pT->m_info.lineNumber = line;
		cry_strcpy(m_failureMsg, expression);
	}
#if defined(CRY_UNIT_TESTING_USE_EXCEPTIONS)
	throw CryUnitTest::assert_exception(expression, file, line);
#else
	longjmp(*GetAssertJmpBuf(), 1);
#endif
}

bool CUnitTestManager::IsTestMatch(CUnitTest* pTest, const string& sSuiteName, const string& sTestName)
{
	assert(pTest);

	bool isMatch = true; // by default test is match

	if (sSuiteName != "" && sSuiteName != "*")
	{
		isMatch &= (strcmp(sSuiteName.c_str(), pTest->m_info.suite) == 0);
	}
	if (sTestName != "" && sTestName != "*")
	{
		isMatch &= (strcmp(sTestName.c_str(), pTest->m_info.name) == 0);
	}
	return isMatch;
}

void CLogUnitTestReporter::OnStartTesting(UnitTestRunContext& context)
{
	CryLog("UnitTesting Started");
}

void CLogUnitTestReporter::OnFinishTesting(UnitTestRunContext& context)
{
	CryLog("UnitTesting Finished");
}

void CLogUnitTestReporter::OnTestStart(IUnitTest* pTest)
{
	UnitTestInfo info;
	pTest->GetInfo(info);
	CryLog("UnitTestStart:  [%s]%s:%s", info.module, info.suite, info.name);
}

void CLogUnitTestReporter::OnTestFinish(IUnitTest* pTest, float fRunTimeInMs, bool bSuccess, char const* failureDescription)
{
	UnitTestInfo info;
	pTest->GetInfo(info);
	if (bSuccess)
		CryLog("UnitTestFinish: [%s]%s:%s | OK (%3.2fms)", info.module, info.suite, info.name, fRunTimeInMs);
	else
		CryLog("UnitTestFinish: [%s]%s:%s | FAIL (%s)", info.module, info.suite, info.name, failureDescription);
}

void CMinimalLogUnitTestReporter::OnStartTesting(UnitTestRunContext& context)
{
	m_nRunTests = 0;
	m_nSucceededTests = 0;
	m_nFailedTests = 0;
	m_fTimeTaken = 0.0f;
}

void CMinimalLogUnitTestReporter::OnFinishTesting(UnitTestRunContext& context)
{
	CryLog("UnitTesting Finished Tests: %d Succeeded: %d, Failed: %d, Time: %5.2f ms", m_nRunTests, m_nSucceededTests, m_nFailedTests, m_fTimeTaken);
}

void CMinimalLogUnitTestReporter::OnTestStart(IUnitTest* pTest)
{
}

void CMinimalLogUnitTestReporter::OnTestFinish(IUnitTest* pTest, float fRunTimeInMs, bool bSuccess, char const* failureDescription)
{
	m_nRunTests += 1;
	m_nSucceededTests += (bSuccess == true);
	m_nFailedTests += (bSuccess == false);
	m_fTimeTaken += fRunTimeInMs;

	if (!bSuccess)
	{
		UnitTestInfo info;
		pTest->GetInfo(info);
		CryLog("-- FAIL (%s): [%s]%s:%s (%s:%d)", failureDescription, info.module, info.suite, info.name, info.filename, info.lineNumber);
	}

}

#if defined(CRY_UNIT_TESTING)

CRY_UNIT_TEST_SUITE(BITFIDDLING)
{

	CRY_UNIT_TEST(CUT_BITFIDDLING)
	{
		for (uint32 i = 0, j = ~0U; i < 32; ++i, j >>= 1)
			CRY_UNIT_TEST_ASSERT(countLeadingZeros32(j) == i);
		for (uint32 i = 0, j = ~0U; i < 32; ++i, j <<= 1)
			CRY_UNIT_TEST_ASSERT(countTrailingZeros32(j) == i);

		for (uint64 i = 0, j = ~0ULL; i < 64; ++i, j >>= 1)
			CRY_UNIT_TEST_ASSERT(countLeadingZeros64(j) == i);
		for (uint64 i = 0, j = ~0ULL; i < 64; ++i, j <<= 1)
			CRY_UNIT_TEST_ASSERT(countTrailingZeros64(j) == i);

		for (uint8 i = 7, j = 0x80; i > 0; --i, j >>= 1)
		{
			CRY_UNIT_TEST_ASSERT(IntegerLog2        (uint8(j           )) ==  i     );
			CRY_UNIT_TEST_ASSERT(IntegerLog2        (uint8(j + (j >> 1))) ==  i     );
			CRY_UNIT_TEST_ASSERT(IntegerLog2_RoundUp(uint8(j           )) ==  i     );
			CRY_UNIT_TEST_ASSERT(IntegerLog2_RoundUp(uint8(j + (j >> 1))) == (i + 1));
		}
	}
}
CRY_UNIT_TEST_SUITE(Math)
{
	using namespace crymath;
	const float HALF_EPSILON = 0.00025f;

	template<typename Real, typename S>
	void TestFunction(Real (* func)(Real), S in, S res)
	{
		Real out = func(convert<Real>(in));
		CRY_UNIT_TEST_ASSERT(All(out == convert<Real>(res)));
	}

	template<typename Real, typename S>
	void TestFunction(Real (* func)(Real, Real), S a, S b, S res)
	{
		Real out = func(convert<Real>(a), convert<Real>(b));
		CRY_UNIT_TEST_ASSERT(All(out == convert<Real>(res)));
	}

	template<typename Real, typename S>
	void TestFunction(Real (* func)(Real, Real, Real), S a, S b, S c, S res)
	{
		Real out = func(convert<Real>(a), convert<Real>(b), convert<Real>(c));
		CRY_UNIT_TEST_ASSERT(All(out == convert<Real>(res)));
	}

	template<typename Real, typename S>
	bool IsEquiv(Real a, Real b, S epsilon)
	{
		return NumberValid(a) && All(abs(a - b) <= abs(b) * convert<Real>(epsilon));
	}

	template<typename Real, typename S>
	void TestFunction(Real (* func)(Real), S in, S res, S epsilon)
	{
		Real out = func(convert<Real>(in));
		CRY_UNIT_TEST_ASSERT(IsEquiv(out, convert<Real>(res), epsilon));
	}

	template<typename Real, typename S>
	void TestFunction(Real (* func)(Real, Real), S a, S b, S res, S epsilon)
	{
		Real out = func(convert<Real>(a), convert<Real>(b));
		CRY_UNIT_TEST_ASSERT(IsEquiv(out, convert<Real>(res), epsilon));
	}

	template<typename Real, typename S>
	void TestFunction(Real (* func)(Real, Real, Real), S a, S b, S c, S res, S epsilon)
	{
		Real out = func(convert<Real>(a), convert<Real>(b), convert<Real>(c));
		CRY_UNIT_TEST_ASSERT(IsEquiv(out, convert<Real>(res), epsilon));
	}

	template<typename Real, typename S>
	void FunctionApproxTest(Real (* func)(Real), Real (* func_fast)(Real), S in, S res)
	{
		TestFunction(func, in, res, FLT_EPSILON * 2.0f);
		TestFunction(func_fast, in, res, HALF_EPSILON * 2.0f);
	}

	template<typename Real, typename Int, typename Uint>
	void FunctionTest()
	{
		// Check trunc, floor, ceil
		float fdata[] = { 0.0f, 257.85f, 124.35f, 73.0f, 0.806228399f, 0.2537399f, -0.123432f, -0.986552f, -5.0f, -63.4f, -102.7f, 1e-18f, -4.567e8f };

		for (float f : fdata)
		{
			float res;

			res = float(int(f));
			TestFunction<Real>(trunc, f, res);

			res = std::floor(f);
			TestFunction<Real>(floor, f, res);

			res = std::ceil(f);
			TestFunction<Real>(ceil, f, res);

			res = f<0.0f ? 0.0f : f> 1.0f ? 1.0f : f;
			TestFunction<Real>(saturate, f, res);

			res = f < 0.0f ? -f : f;
			TestFunction<Real>(abs, f, res);

			res = f < 0.0f ? -1.0f : 1.0f;
			TestFunction<Real>(signnz, f, res);

			res = f<0.0f ? -1.0f : f> 0.0f ? 1.0f : 0.0f;
			TestFunction<Real>(sign, f, res);

			FunctionApproxTest<Real>(sqrt, sqrt_fast, f * f, abs(f));

			if (f != 0.0f)
			{
				FunctionApproxTest<Real>(rcp, rcp_fast, f, 1.0f / f);
				FunctionApproxTest<Real>(rsqrt, rsqrt_fast, 1.0f / (f * f), abs(f));
			}
		}

		TestFunction<Real>(max, 257.35f, 510.0f, 510.0f);
		TestFunction<Real>(max, -102.0f, -201.4f, -102.0f);

		TestFunction<Real>(mod, 3.0f, 1.0f, 0.0f, FLT_EPSILON * 4);
		TestFunction<Real>(mod, 0.75f, 0.5f, 0.25f, FLT_EPSILON * 4);
		TestFunction<Real>(mod, -0.6f, 0.5f, -0.1f, FLT_EPSILON * 4);
		TestFunction<Real>(mod, -0.2f, -0.3f, -0.2f, FLT_EPSILON * 4);

		TestFunction<Real>(wrap, -6.0f, -1.0f, 3.0f, 2.0f, FLT_EPSILON * 4);
		TestFunction<Real>(wrap, -6.3f, 0.0f, 1.0f, 0.7f, FLT_EPSILON * 4);
		TestFunction<Real>(wrap, 4.8f, -1.0f, 1.0f, 0.8f, FLT_EPSILON * 4);

		TestFunction<Int>(min, -3, 1, -3);
		TestFunction<Int>(max, -3, 1, 1);

		TestFunction<Uint>(min, 8u, 3u, 3u);
		TestFunction<Uint>(min, 8u, 0x9FFFFFFFu, 8u);
		TestFunction<Uint>(max, 8u, 0x9FFFFFFFu, 0x9FFFFFFFu);
		TestFunction<Uint>(max, 0x90000000u, 0xA0000000u, 0xA0000000u);
	}

	#if CRY_PLATFORM_SSE2

	template<typename V, typename T>
	void ConvertTest()
	{
		V v = convert<V>(T(0), T(1), T(2), T(3));
		CRY_UNIT_TEST_ASSERT(NumberValid(v));
		CRY_UNIT_TEST_ASSERT(get_element<0>(v) == T(0));
		CRY_UNIT_TEST_ASSERT(get_element<3>(v) == T(3));
		v = convert<V>(T(1), 0);
		CRY_UNIT_TEST_ASSERT(All(v == convert<V>(T(1), T(0), T(0), T(0))));
	}

		#define ELEMENT_OPERATE(V, a, op, b) convert<V>( \
		  get_element<0>(a) op get_element<0>(b),        \
		  get_element<1>(a) op get_element<1>(b),        \
		  get_element<2>(a) op get_element<2>(b),        \
		  get_element<3>(a) op get_element<3>(b))        \

	template<typename V>
	void TestIntOperators(V a, V b)
	{
		CRY_UNIT_TEST_ASSERT(All((a + b) == ELEMENT_OPERATE(V, a, +, b)));
		CRY_UNIT_TEST_ASSERT(All((a - b) == ELEMENT_OPERATE(V, a, -, b)));
		CRY_UNIT_TEST_ASSERT(All((a * b) == ELEMENT_OPERATE(V, a, *, b)));
		CRY_UNIT_TEST_ASSERT(All((a / b) == ELEMENT_OPERATE(V, a, /, b)));
		CRY_UNIT_TEST_ASSERT(All((a % b) == ELEMENT_OPERATE(V, a, %, b)));
		CRY_UNIT_TEST_ASSERT(All((a & b) == ELEMENT_OPERATE(V, a, &, b)));
		CRY_UNIT_TEST_ASSERT(All((a | b) == ELEMENT_OPERATE(V, a, |, b)));
		CRY_UNIT_TEST_ASSERT(All((a ^ b) == ELEMENT_OPERATE(V, a, ^, b)));
		CRY_UNIT_TEST_ASSERT(All((a << Scalar(3)) == ELEMENT_OPERATE(V, a, <<, convert<V>(3))));
		CRY_UNIT_TEST_ASSERT(All((a >> Scalar(2)) == ELEMENT_OPERATE(V, a, >>, convert<V>(2))));
	}

	template<typename V>
	void TestFloatOperators(V a, V b)
	{
		CRY_UNIT_TEST_ASSERT(All((a + b) == ELEMENT_OPERATE(V, a, +, b)));
		CRY_UNIT_TEST_ASSERT(All((a - b) == ELEMENT_OPERATE(V, a, -, b)));
		CRY_UNIT_TEST_ASSERT(All((a * b) == ELEMENT_OPERATE(V, a, *, b)));
		CRY_UNIT_TEST_ASSERT(IsEquiv(a / b, ELEMENT_OPERATE(V, a, /, b), FLT_EPSILON));
	}

	void OperatorTest()
	{
		i32v4 idata[6] = {
			convert<i32v4>(1,    -2,    3,   -4),     convert<i32v4>(-2,    -1,     5,    6),
			convert<i32v4>(7,    -12,   -13, 24),     convert<i32v4>(-2,    -5,     5,    6),
			convert<i32v4>(7654, -1234, -13, 249999), convert<i32v4>(-2373, -59999, 5432, 62)
		};

		for (int i = 0; i < 6; i += 2)
		{
			i32v4 a = idata[i];
			i32v4 b = idata[i + 1];

			TestIntOperators(a, b);
			TestIntOperators(convert<u32v4>(abs(a)), convert<u32v4>(abs(b)));
			TestIntOperators(convert<u32v4>(a), convert<u32v4>(b));
			TestFloatOperators(convert<f32v4>(a), convert<f32v4>(b));
		}
	}

	template<typename V>
	void CompareTest()
	{
		CRY_UNIT_TEST_ASSERT(All(convert<V>(-3, -5, 6, 9) == convert<V>(-3, -5, 6, 9)));
		CRY_UNIT_TEST_ASSERT(Any(convert<V>(-3, -5, 6, 9) != convert<V>(-3, 5, 6, 9)));
		CRY_UNIT_TEST_ASSERT(All(convert<V>(-3, -5, 6, 9) < convert<V>(4, -2, 7, 10)));
		CRY_UNIT_TEST_ASSERT(All(convert<V>(-3, -5, 6, 9) <= convert<V>(4, -2, 6, 10)));
		CRY_UNIT_TEST_ASSERT(All(convert<V>(-3, -5, 6, 9) > convert<V>(-4, -7, -8, 0)));
		CRY_UNIT_TEST_ASSERT(All(convert<V>(-3, -5, 6, 9) >= convert<V>(-3, -6, 6, 8)));
	}
	void UCompareTest()
	{
		CRY_UNIT_TEST_ASSERT(All(convert<u32v4>(1, 2, 3, 4) == convert<u32v4>(1, 2, 3, 4)));
		CRY_UNIT_TEST_ASSERT(!All(convert<u32v4>(1, 2, 3, 4) == convert<u32v4>(0, 2, 3, 4)));
		CRY_UNIT_TEST_ASSERT(All(convert<u32v4>(1, 2, 3, 4) != convert<u32v4>(4, 3, 2, 1)));
		CRY_UNIT_TEST_ASSERT(All(convert<u32v4>(0, 2, 3, 4) < convert<u32v4>(2u, 4u, 8u, 0x99999999)));
		CRY_UNIT_TEST_ASSERT(All(convert<u32v4>(0, 2, 3, 4) <= convert<u32v4>(2u, 2u, 3u, 0x99999999)));
		CRY_UNIT_TEST_ASSERT(All(convert<u32v4>(1u, 2u, 3u, 0x80000000) > convert<u32v4>(0, 1, 2, 3)));
		CRY_UNIT_TEST_ASSERT(All(convert<u32v4>(1u, 2u, 3u, 0x80000000) >= convert<u32v4>(1, 2, 3, 4)));
	}

	void SelectTest()
	{
		CRY_UNIT_TEST_ASSERT(if_else(3 == 4, 1.0f, 2.0f) == 2.0f);
		CRY_UNIT_TEST_ASSERT(if_else_zero(7.0f != 5.0f, 9) == 9);
		CRY_UNIT_TEST_ASSERT(__fsel(-7.0f, -5.0f, -3.0f) == -3.0f);
		CRY_UNIT_TEST_ASSERT(All(if_else(
		                           convert<f32v4>(0.5f, 0.0f, -7.0f, -0.0f) >= convert<f32v4>(2.0f, -4.0f, -5.0f, 0.0f),
		                           convert<i32v4>(-3, 3, -5, 0),
		                           convert<i32v4>(4, 40, -1, -7)
		                           ) == convert<i32v4>(4, 3, -1, 0)));
		CRY_UNIT_TEST_ASSERT(All(if_else_zero(
		                           convert<i32v4>(3, -7, 0, 4) >= convert<i32v4>(2, 7, -1, -8),
		                           convert<f32v4>(-3.0f, 3.0f, -3.0f, 0.9f)
		                           ) == convert<f32v4>(-3.0f, 0.0f, -3.0f, 0.9f)));
		CRY_UNIT_TEST_ASSERT(All(__fsel(
		                           convert<f32v4>(0.5f, 0.0f, -7.0f, -0.0f),
		                           convert<f32v4>(2.0f, -4.0f, -5.0f, 2.5f),
		                           convert<f32v4>(-3.0f, 3.0f, -3.0f, 0.9f)
		                           ) == convert<f32v4>(2.0f, -4.0f, -3.0f, 2.5f)));
	}

	#endif // CRY_PLATFORM_SSE2

	void QuadraticTest()
	{
		float r[2];
		for (int i = 0; i < 20; ++i)
		{
			float c0 = cry_random(-999.f, 999.f) * (i % 5);
			float c1 = cry_random(-999.f, 999.f) * (i % 4);
			float c2 = cry_random(-999.f, 999.f) * (i % 3);
			int n = solve_quadratic(c2, c1, c0, r);
			while (--n >= 0)
			{
				float f = c0 + c1 * r[n] + c2 * sqr(r[n]);
				CRY_UNIT_TEST_ASSERT(abs(f) <= 0.01f);
			}
		}
	}

	CRY_UNIT_TEST(CUT_Math)
	{
		QuadraticTest();
		FunctionTest<float, int, uint>();
	#if CRY_PLATFORM_SSE2
		FunctionTest<f32v4, i32v4, u32v4>();
		ConvertTest<f32v4, float>();
		ConvertTest<i32v4, int>();
		OperatorTest();
		CompareTest<f32v4>();
		CompareTest<i32v4>();
		UCompareTest();
		SelectTest();
	#endif
	}
}

CRY_UNIT_TEST_SUITE(Strings)
{
	CRY_UNIT_TEST(CUT_Strings)
	{
		bool bOk;
		char bf[4];

		// cry_strcpy()

		bOk = cry_strcpy(0, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcpy(0, 0, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcpy(0, 1, 0);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcpy(0, 1, 0, 1);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcpy(0, 1, "");
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcpy(0, 1, "", 1);
		CRY_UNIT_TEST_ASSERT(!bOk);

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 0, "");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 0, "", 1);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 1, 0);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 1, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 0);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 3, "qwerty");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "qw\000d", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 3, "qwerty", 4);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "qw\000d", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 3, "qwerty", 3);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "qw\000d", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 3, "qwerty", 2);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "qw\000d", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 3, "qwerty", 1);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "q\000cd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, 3, "qwerty", 0);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, "qwerty");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "qwe\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, "qwerty", 4);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "qwe\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, "qwerty", 3);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "qwe\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, "qwerty", 2);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "qw\000d", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, "qwe");
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "qwe\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, "qwe", 4);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "qwe\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, "qw", 3);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "qw\000d", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, sizeof(bf), "q");
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "q\000cd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcpy(bf, sizeof(bf), "q", 2);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "q\000cd", 4));

		// cry_strcat()

		bOk = cry_strcat(0, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcat(0, 0, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcat(0, 1, 0);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcat(0, 1, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcat(0, 1, "");
		CRY_UNIT_TEST_ASSERT(!bOk);

		bOk = cry_strcat(0, 1, "", 1);
		CRY_UNIT_TEST_ASSERT(!bOk);

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 0, "xy");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 0, "xy", 3);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 0, "xy", 0);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 1, "xyz");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 1, "xyz", 4);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 1, "xyz", 1);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 1, "xyz", 0);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, 1, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "\000bcd", 4));

		memcpy(bf, "a\000cd", 4);
		bOk = cry_strcat(bf, 3, "xyz");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "ax\000d", 4));

		memcpy(bf, "a\000cd", 4);
		bOk = cry_strcat(bf, 3, "xyz", 4);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "ax\000d", 4));

		memcpy(bf, "a\000cd", 4);
		bOk = cry_strcat(bf, 3, "xyz", 2);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "ax\000d", 4));

		memcpy(bf, "a\000cd", 4);
		bOk = cry_strcat(bf, 3, "xyz", 1);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "ax\000d", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, "xyz");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abc\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, "xyz", 4);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abc\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, "xyz", 1);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abc\000", 4));

		memcpy(bf, "abcd", 4);
		bOk = cry_strcat(bf, "xyz", 0);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "abc\000", 4));

		memcpy(bf, "ab\000d", 4);
		bOk = cry_strcat(bf, "xyz");
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abx\000", 4));

		memcpy(bf, "ab\000d", 4);
		bOk = cry_strcat(bf, "xyz", 4);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "abx\000", 4));

		memcpy(bf, "ab\000d", 4);
		bOk = cry_strcat(bf, "xyz", 1);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "abx\000", 4));

		memcpy(bf, "ab\000d", 4);
		bOk = cry_strcat(bf, "xyz", 0);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "ab\000d", 4));

		memcpy(bf, "ab\000d", 4);
		bOk = cry_strcat(bf, 0, 0);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "ab\000d", 4));

		memcpy(bf, "ab\000d", 4);
		bOk = cry_strcat(bf, 0, 1);
		CRY_UNIT_TEST_ASSERT(!bOk && !memcmp(bf, "ab\000d", 4));

		memcpy(bf, "a\000cd", 4);
		bOk = cry_strcat(bf, sizeof(bf), "xy");
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "axy\000", 4));

		memcpy(bf, "a\000cd", 4);
		bOk = cry_strcat(bf, sizeof(bf), "xy", 3);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "axy\000", 4));

		memcpy(bf, "a\000cd", 4);
		bOk = cry_strcat(bf, sizeof(bf), "xy", 1);
		CRY_UNIT_TEST_ASSERT(bOk && !memcmp(bf, "ax\000d", 4));

		// cry_sprintf()
		{
			const auto test = [](const bool bNull, int charCount, const char* src, const char* initBuffer, const char* resultBuffer, bool resultBool)
			{
				char buffer[3] = { initBuffer[0], initBuffer[1], initBuffer[2] };
				assert(charCount >= 0 && charCount <= CRY_ARRAY_COUNT(buffer));
				const bool b = cry_sprintf(bNull ? nullptr : buffer, charCount * sizeof(buffer[0]), src);
				CRY_UNIT_TEST_ASSERT(b == resultBool && !memcmp(buffer, resultBuffer, sizeof(buffer)));
			};

			test(true, 0, 0, "abc", "abc", false);
			test(true, 1, 0, "abc", "abc", false);
			test(false, 0, 0, "abc", "abc", false);
			test(false, 1, 0, "abc", "\000bc", false);

			test(true, 0, "", "abc", "abc", false);
			test(true, 1, "", "abc", "abc", false);
			test(true, 0, "x", "abc", "abc", false);
			test(true, 1, "x", "abc", "abc", false);
			test(true, 2, "x", "abc", "abc", false);

			test(false, 0, "", "abc", "abc", false);
			test(false, 1, "", "abc", "\000bc", true);
			test(false, 0, "x", "abc", "abc", false);
			test(false, 1, "x", "abc", "\000bc", false);
			test(false, 2, "x", "abc", "x\000c", true);
			test(false, 3, "x", "abc", "x\000c", true);
			test(false, 2, "xy", "abc", "x\000c", false);
			test(false, 3, "xy", "abc", "xy\000", true);
		}

		// CryStringUtils_Internal::compute_length_formatted()
		{
			const auto test = [](int length, std::vector<char>& c)
			{
				c.resize(length + 1);
				const int count = CryStringUtils_Internal::compute_length_formatted("%*s", length, "");
				CRY_UNIT_TEST_ASSERT(count == length);
				const bool b = cry_sprintf(c.data(), (length + 1) * sizeof(c[0]), "%*s", length, "");
				CRY_UNIT_TEST_ASSERT(b && strlen(c.data()) == length);
			};

			std::vector<char> c;
			c.reserve(4096);

			test(0, c);
			test(1, c);
			test(2, c);
			test(64, c);
			test(127, c);
			test(128, c);
			test(512, c);
			test(4095, c);
		}
	}
}

CRY_UNIT_TEST_SUITE(CryString)
{
	CRY_UNIT_TEST(CUT_CryString)
	{
		//////////////////////////////////////////////////////////////////////////
		// Based on MS documentation of find_last_of
		string strTestFindLastOfOverload1("abcd-1234-abcd-1234");
		string strTestFindLastOfOverload2("ABCD-1234-ABCD-1234");
		string strTestFindLastOfOverload3("456-EFG-456-EFG");
		string strTestFindLastOfOverload4("12-ab-12-ab");

		const char* cstr2 = "B1";
		const char* cstr2b = "D2";
		const char* cstr3a = "5EZZZZZZ";
		string str4a("ba3");
		string str4b("a2");

		size_t nPosition(string::npos);

		nPosition = strTestFindLastOfOverload1.find_last_of('d', 14);
		CRY_UNIT_TEST_ASSERT(nPosition == 13);

		nPosition = strTestFindLastOfOverload2.find_last_of(cstr2, 12);
		CRY_UNIT_TEST_ASSERT(nPosition == 11);

		nPosition = strTestFindLastOfOverload2.find_last_of(cstr2b);
		CRY_UNIT_TEST_ASSERT(nPosition == 16);

		nPosition = strTestFindLastOfOverload3.find_last_of(cstr3a, 8, 8);
		CRY_UNIT_TEST_ASSERT(nPosition == 4);

		nPosition = strTestFindLastOfOverload4.find_last_of(str4a, 8);
		CRY_UNIT_TEST_ASSERT(nPosition == 4);

		nPosition = strTestFindLastOfOverload4.find_last_of(str4b);
		CRY_UNIT_TEST_ASSERT(nPosition == 9);

		//////////////////////////////////////////////////////////////////////////
		// Based on MS documentation of find_last_not_of
		string strTestFindLastNotOfOverload1("dddd-1dd4-abdd");
		string strTestFindLastNotOfOverload2("BBB-1111");
		string strTestFindLastNotOfOverload3("444-555-GGG");
		string strTestFindLastNotOfOverload4("12-ab-12-ab");

		const char* cstr2NF = "B1";
		const char* cstr3aNF = "45G";
		const char* cstr3bNF = "45G";

		string str4aNF("b-a");
		string str4bNF("12");

		size_t nPosition3A(string::npos);

		nPosition = strTestFindLastNotOfOverload1.find_last_not_of('d', 7);
		CRY_UNIT_TEST_ASSERT(nPosition == 5);

		nPosition = strTestFindLastNotOfOverload1.find_last_not_of("d");
		CRY_UNIT_TEST_ASSERT(nPosition == 11);

		nPosition = strTestFindLastNotOfOverload2.find_last_not_of(cstr2NF, 6);
		CRY_UNIT_TEST_ASSERT(nPosition == 3);

		nPosition = strTestFindLastNotOfOverload3.find_last_not_of(cstr3aNF);
		CRY_UNIT_TEST_ASSERT(nPosition == 7);

		nPosition = strTestFindLastNotOfOverload3.find_last_not_of(cstr3bNF, 6, 3);    //nPosition - 1 );
		CRY_UNIT_TEST_ASSERT(nPosition == 3);

		nPosition = strTestFindLastNotOfOverload4.find_last_not_of(str4aNF, 5);
		CRY_UNIT_TEST_ASSERT(nPosition == 1);

		nPosition = strTestFindLastNotOfOverload4.find_last_not_of(str4bNF);
		CRY_UNIT_TEST_ASSERT(nPosition == 10);
	}
}

CRY_UNIT_TEST_SUITE(FixedString)
{
	CRY_UNIT_TEST(CUT_FixedString)
	{
		CryStackStringT<char, 10> str1;
		CryStackStringT<char, 10> str2;
		CryStackStringT<char, 4> str3;
		CryStackStringT<char, 10> str4;
		CryStackStringT<wchar_t, 16> wstr1;
		CryStackStringT<wchar_t, 255> wstr2;
		CryFixedStringT<100> fixedString100;
		CryFixedStringT<200> fixedString200;

		typedef CryStackStringT<char, 10> T;
		T* pStr = new T;
		*pStr = "adads";
		delete pStr;

		str1 = "abcd";
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "abcd");

		str2 = "efg";
		CRY_UNIT_TEST_CHECK_EQUAL(str2, "efg");

		str2 = str1;
		CRY_UNIT_TEST_CHECK_EQUAL(str2, "abcd");

		str1 += "XY";
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "abcdXY");

		str2 += "efghijk";
		CRY_UNIT_TEST_CHECK_EQUAL(str2, "abcdefghijk");

		str1.replace("bc", "");
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "adXY");

		str1.replace("XY", "1234");
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "ad1234");

		str1.replace("1234", "1234567890");
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "ad1234567890");

		str1.reserve(200);
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "ad1234567890");
		CRY_UNIT_TEST_ASSERT(str1.capacity() == 200);
		str1.reserve(0);
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "ad1234567890");
		CRY_UNIT_TEST_ASSERT(str1.capacity() == str1.length());

		str1.erase(7); // doesn't change capacity
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "ad12345");

		str4.assign("abc");
		CRY_UNIT_TEST_CHECK_EQUAL(str4, "abc");
		str4.reserve(9);
		CRY_UNIT_TEST_ASSERT(str4.capacity() >= 9);  // capacity is always >= MAX_SIZE-1
		str4.reserve(0);
		CRY_UNIT_TEST_ASSERT(str4.capacity() >= 9);  // capacity is always >= MAX_SIZE-1

		size_t idx = str1.find("123");
		CRY_UNIT_TEST_ASSERT(idx == 2);

		idx = str1.find("123", 3);
		CRY_UNIT_TEST_ASSERT(idx == str1.npos);

		wstr1 = L"abc";
		CRY_UNIT_TEST_CHECK_EQUAL(wstr1, L"abc");
		CRY_UNIT_TEST_ASSERT(wstr1.compare(L"aBc") > 0);
		CRY_UNIT_TEST_ASSERT(wstr1.compare(L"babc") < 0);
		CRY_UNIT_TEST_ASSERT(wstr1.compareNoCase(L"aBc") == 0);
		str1.Format("This is a %s %ls with %d params", "mixed", L"string", 3);
		str2.Format("This is a %ls %s with %d params", L"mixed", "string", 3);
		CRY_UNIT_TEST_CHECK_EQUAL(str1, "This is a mixed string with 3 params");
		CRY_UNIT_TEST_CHECK_EQUAL(str1, str2);
	}
}

//////////////////////////////////////////////////////////////////////////
// Unit Testing of aligned_vector
//////////////////////////////////////////////////////////////////////////
CRY_UNIT_TEST(CUT_AlignedVector)
{
	stl::aligned_vector<int, 16> vec;

	vec.push_back(1);
	vec.push_back(2);
	vec.push_back(3);

	CRY_UNIT_TEST_ASSERT(vec.size() == 3);
	CRY_UNIT_TEST_ASSERT(((INT_PTR)(&vec[0]) % 16) == 0);
}

CRY_UNIT_TEST(CUT_DynArray)
{
	DynArray<int> a;
	a.push_back(3);
	a.insert(&a[0], 1, 1);
	a.insert(&a[1], 1, 2);
	a.insert(&a[0], 1, 0);

	for (int i = 0; i < 4; i++)
		CRY_UNIT_TEST_ASSERT(a[i] == i);

	const int nStrs = 11;
	string Strs[nStrs] = { "nought", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten" };
	DynArray<string> s;
	for (int i = 0; i < nStrs; i += 2)
		s.push_back(Strs[i]);
	for (int i = 1; i < nStrs; i += 2)
		s.insert(i, Strs[i]);
	for (int i = 0; i < nStrs; i++)
		CRY_UNIT_TEST_ASSERT(s[i] == Strs[i]);

	DynArray<string> s2 = s;
	s.erase(5, 2);
	CRY_UNIT_TEST_ASSERT(s.size() == nStrs - 2);

	s.insert(&s[3], &Strs[5], &Strs[8]);

	s2 = s2(3, 4);
	CRY_UNIT_TEST_ASSERT(s2.size() == 4);
}

CRY_UNIT_TEST(CUT_HeapAlloc)
{
	typedef stl::HeapAllocator<stl::PSyncNone> THeap;
	THeap beep;

	for (int i = 0; i < 8; ++i)
	{
		THeap::Array<float> af(beep, 77);
		af.push_back(3.14);
		THeap::Array<int, uint, 16> ai(beep);
		ai.reserve(9111);
		ai.push_back(-7);
		ai.push_back(-6);
	}
	CRY_UNIT_TEST_ASSERT(beep.GetTotalMemory().nUsed == 0);
	THeap::Array<string> as(beep, 8);
	as[0] = "nil";
	as[7] = "sevven";
}

CRY_UNIT_TEST(CUT_SimplifyFilePath)
{
	// Check a number of invalid outputs and too-short-buffer scenarios.
	// They should do the same for Windows and Posix.
	{
		for (int i = 0; i < 2; ++i)
		{
			PathUtil::EPathStyle style = (i == 0 ? PathUtil::ePathStyle_Windows : PathUtil::ePathStyle_Posix);
			char buf[1];
			bool result = PathUtil::SimplifyFilePath("foo", buf, 1, style);
			CRY_UNIT_TEST_ASSERT(!result && !*buf);

			result = PathUtil::SimplifyFilePath(nullptr, buf, 1, style);
			CRY_UNIT_TEST_ASSERT(!result && !*buf);

			result = PathUtil::SimplifyFilePath("foo", nullptr, 9001, style);
			CRY_UNIT_TEST_ASSERT(!result);

			result = PathUtil::SimplifyFilePath("foo", buf, 0, style);
			CRY_UNIT_TEST_ASSERT(!result);

			result = PathUtil::SimplifyFilePath("", buf, 1, style);
			CRY_UNIT_TEST_ASSERT(!result && !*buf);
		}
	}

	// tests with sufficient buffer
	{
		struct
		{
			const char* szInput;
			bool        bWinExpected;
			const char* szWinExpected;
			bool        bUnixExpected;
			const char* szUnixExpected;
		}
		items[] =
		{
			//                              WINAPI                                   POSIX
			// input                        success   output                         success   output
			{ "",                            false, "",                          false, ""                       },
			{ "c:",                          true,  "c:",                        true,  "c:"                     },// c: and c:. counts as a regular folder name in posix
			{ "c:.",                         true,  "c:",                        true,  "c:."                    },
			{ "c:..",                        true,  "c:..",                      true,  "c:.."                   },
			{ "c:abc/.",                     true,  "c:abc",                     true,  "c:abc"                  },
			{ "c:abc/..",                    true,  "c:",                        true,  ""                       },
			{ "c:abc/../..",                 true,  "c:..",                      true,  ".."                     },
			{ "c:\\",                        true,  "c:\\",                      true,  "c:"                     },
			{ ":\\.",                        false, "",                          true,  ":"                      },
			{ "abc:\\.",                     false, "",                          true,  "abc:"                   },
			{ "c:\\.",                       true,  "c:\\",                      true,  "c:"                     },
			{ "c:\\..",                      false, "",                          true,  ""                       },
			{ "c:/abc/.",                    true,  "c:\\abc",                   true,  "c:/abc"                 },
			{ "c:/abc/..",                   true,  "c:\\",                      true,  "c:"                     },
			{ "c:/abc\\..\\..",              false, "",                          true,  ""                       },
			{ "\\\\storage\\.",              true,  "\\\\storage",               false, ""                       },
			{ "\\\\storage\\..",             false, "",                          false, ""                       },
			{ "\\\\storage\\abc\\..\\",      true,  "\\\\storage",               false, ""                       },
			{ "c:/should/be/unchanged",      true,  "c:\\should\\be\\unchanged", true,  "c:/should/be/unchanged" },
			{ "c:\\disappear/..\\remain",    true,  "c:\\remain",                true,  "c:/remain"              },
			{ "c:\\disappear\\..\\remain",   true,  "c:\\remain",                true,  "c:/remain"              },
			{ "/disappear/../remain",        true,  "\\remain",                  true,  "/remain"                },
			{ "c:/trailing/slash/gone/",     true,  "c:\\trailing\\slash\\gone", true,  "c:/trailing/slash/gone" },
			{ "c:\\trailing\\slash\\gone\\", true,  "c:\\trailing\\slash\\gone", true,  "c:/trailing/slash/gone" },
			{ "/trailing/slash/gone/",       true,  "\\trailing\\slash\\gone",   true,  "/trailing/slash/gone"   },
			{ ".",                           true,  ".",                         true,  "."                      },
			{ "..",                          true,  "..",                        true,  ".."                     },
			{ "..\\..",                      true,  "..\\..",                    true,  "../.."                  },
			{ "\\..\\..\\",                  false, "",                          false, ""                       },
			{ "../.\\..",                    true,  "..\\..",                    true,  "../.."                  },
			{ "\\\\.\\myfile",               false, "",                          false, ""                       },
			{ "/some/rooted/path",           true,  "\\some\\rooted\\path",      true,  "/some/rooted/path"      },
			{ "\\\\s2\\..\\s1",              false, "",                          false, ""                       },
			{ "/",                           true,  "\\",                        true,  "/"                      },
			{ "/.",                          true,  "\\",                        true,  "/"                      },
			{ "/..",                         false, "",                          false, ""                       },
			{ "/abc/.",                      true,  "\\abc",                     true,  "/abc"                   },
			{ "/abc/..",                     true,  "\\",                        true,  "/"                      },
			{ "/abc/../..",                  false, "",                          false, ""                       },
			{ "../abc",                      true,  "..\\abc",                   true,  "../abc"                 },
			{ "../../abc",                   true,  "..\\..\\abc",               true,  "../../abc"              },
			{ "bar/../../../abc",            true,  "..\\..\\abc",               true,  "../../abc"              },
			{ "/abc/foo/bar",                true,  "\\abc\\foo\\bar",           true,  "/abc/foo/bar"           },
			{ "abc",                         true,  "abc",                       true,  "abc"                    },{ "abc/.", true, "abc", true, "abc" },
			{ "abc/..",                      true,  "",                          true,  ""                       },
			{ "abc/../..",                   true,  "..",                        true,  ".."                     },
			{ "abc/foo/bar",                 true,  "abc\\foo\\bar",             true,  "abc/foo/bar"            },
			{ "/foo//bar",                   true,  "\\foo\\bar",                true,  "/foo/bar"               },
			{ "/foo/bar//",                  true,  "\\foo\\bar",                true,  "/foo/bar"               },
			{ "/foo/////bar////",            true,  "\\foo\\bar",                true,  "/foo/bar"               },
			{ "\\foo\\\\bar",                true,  "\\foo\\bar",                true,  "/foo/bar"               },
			{ "\\foo\\bar\\\\",              true,  "\\foo\\bar",                true,  "/foo/bar"               },
			{ "\\foo\\\\\\\\\\bar\\\\\\",    true,  "\\foo\\bar",                true,  "/foo/bar"               },
			{ ".\\/system.cfg",              true,  "system.cfg",                true,  "system.cfg"             },
			{ ".\\/./system.cfg",            true,  "system.cfg",                true,  "system.cfg"             },
			{ ".\\/./%USER%/game.cfg",       true,  "%USER%\\game.cfg",          true,  "%USER%/game.cfg"        },
		};
		const size_t numItems = CRY_ARRAY_COUNT(items);

		const size_t bufLength = 100;
		char buf[bufLength];

		for (size_t i = 0; i < numItems; ++i)
		{
			bool bWinResult = PathUtil::SimplifyFilePath(items[i].szInput, buf, bufLength, PathUtil::ePathStyle_Windows);
			bool bWinMatches = strcmp(buf, items[i].szWinExpected) == 0;
			bool bUnixResult = PathUtil::SimplifyFilePath(items[i].szInput, buf, bufLength, PathUtil::ePathStyle_Posix);
			bool bUnixMatches = strcmp(buf, items[i].szUnixExpected) == 0;
			CRY_UNIT_TEST_CHECK_EQUAL(bWinResult, items[i].bWinExpected);
			CRY_UNIT_TEST_CHECK_EQUAL(bUnixResult, items[i].bUnixExpected);
			CRY_UNIT_TEST_CHECK_EQUAL(bWinMatches, true);
			CRY_UNIT_TEST_CHECK_EQUAL(bUnixMatches, true);
		}
	}

	// test complex case
	{
		const size_t bufLength = 100;
		char buf[bufLength];

		const char* const szComplex1 = "foo/bar/../baz/./../../../../././hi/.";
		const char* const szResult1 = "../../hi";
		bool result = PathUtil::SimplifyFilePath(szComplex1, buf, bufLength, PathUtil::ePathStyle_Posix);
		CRY_UNIT_TEST_ASSERT(result && strcmp(buf, szResult1) == 0);

		const char* const szComplex2 = "c:/foo/bar/./disappear/disappear/disappear/../.\\../../../baz\\";
		const char* const szResult2 = "c:\\foo\\baz";
		result = PathUtil::SimplifyFilePath(szComplex2, buf, bufLength, PathUtil::ePathStyle_Windows);
		CRY_UNIT_TEST_ASSERT(result && strcmp(buf, szResult2) == 0);
	}
}

	#include <CryCore/RingBuffer.h>

CRY_UNIT_TEST(CUT_RingBuffer)
{
	{
		CRingBuffer<int, 256> buffer;
		CRY_UNIT_TEST_ASSERT(buffer.empty() && buffer.size() == 0 && buffer.full() == false);

		// 0, 1, 2, 3
		buffer.push_back(2);
		buffer.push_front(1);
		buffer.push_front_overwrite(0);
		buffer.push_back_overwrite(3);

		CRY_UNIT_TEST_ASSERT(buffer.front() == 0);
		CRY_UNIT_TEST_ASSERT(buffer.back() == 3);

		buffer.pop_front();
		buffer.pop_back();

		CRY_UNIT_TEST_ASSERT(buffer.size() == 2);
		CRY_UNIT_TEST_ASSERT(buffer.front() == 1);
		CRY_UNIT_TEST_ASSERT(buffer.back() == 2);
	}

	{
		CRingBuffer<float, 3> buffer;

		// 0, 1, 2
		buffer.push_back(0.0f);
		buffer.push_back(1.0f);
		buffer.push_back(2.0f);
		CRY_UNIT_TEST_ASSERT(buffer.full());

		// 3, 1, 2
		buffer.pop_front();
		buffer.push_front(3.0f);
		bool bPushed = buffer.push_front(4.0f);
		CRY_UNIT_TEST_ASSERT(!bPushed && buffer.front() != 4.0f);

		// 4, 3, 1
		buffer.push_front_overwrite(4.0f);
		CRY_UNIT_TEST_ASSERT(buffer.front() == 4.0f);
		CRY_UNIT_TEST_ASSERT(buffer.back() == 1.0f);

		// nop
		bPushed = buffer.push_back(5.0f);
		CRY_UNIT_TEST_ASSERT(!bPushed && buffer.back() != 5.0f);

		// 3, 1, 5
		buffer.push_back_overwrite(5.0f);
		CRY_UNIT_TEST_ASSERT(buffer.front() == 3.0f);
		CRY_UNIT_TEST_ASSERT(buffer.back() == 5.0f);
	}
}

#endif //CRY_UNIT_TESTING
