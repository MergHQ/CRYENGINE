// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

// Note: Can't use #pragma once here, since (like assert.h) this file CAN be included more than once.
// Each time it's included, it will re-define assert to match CRY_ASSERT.
// This behavior can be used to "fix" 3rdParty headers that #include <assert.h> (by including CryAssert.h after the 3rdParty header).
// In the case where assert's definition gets trampled, just including CryAssert.h should fix that problem for the remainder of the file.
#ifndef CRY_ASSERT_H_INCLUDED
	#define CRY_ASSERT_H_INCLUDED

	#if !defined(_RELEASE)
		#define USE_CRY_ASSERT
		#define CRY_WAS_USE_ASSERT_SET 1

//-----------------------------------------------------------------------------------------------------
// Use like this:
// CRY_ASSERT(expression);
// CRY_ASSERT_MESSAGE(expression,"Useful message");
// CRY_ASSERT_TRACE(expression,("This should never happen because parameter %d named %s is %f",iParameter,szParam,fValue));
//-----------------------------------------------------------------------------------------------------

// FORCE_STANDARD_ASSERT is set by some tools (like RC)
		#if !defined(FORCE_STANDARD_ASSERT)

// defined in CryAssert_impl.h
const char* GetCryModuleName(uint cryModuleId);

enum class ECryAssertLevel
{
	Disabled,
	Enabled,
	FatalErrorOnAssert,
	DebugBreakOnAssert
};

bool CryAssertIsEnabled();
void CryAssertTrace(const char*, ...);
void CryLogAssert(const char*, const char*, unsigned int, bool*);
bool CryAssert(const char*, const char*, unsigned int, bool*);
void CryDebugBreak();

namespace Detail
{
struct SAssertData
{
	char const* const szExpression;
	char const* const szFunction;
	char const* const szFile;
	long const        line;
};

struct SAssertCond
{
	bool bIgnoreAssert;
	bool bLogAssert;
};

bool CryAssertHandler(SAssertData const&, SAssertCond&);
bool CryAssertHandler(SAssertData const& data, SAssertCond& cond, char const* const szMessage);

template<typename ... TraceArgs>
bool CryAssertHandler(SAssertData const& data, SAssertCond& cond, char const* const szFormattedMessage, TraceArgs ... traceArgs)
{
	CryAssertTrace(szFormattedMessage, traceArgs ...);
	return CryAssertHandler(data, cond);
}
} // namespace Detail

//! The code to insert when assert is used.
			#define CRY_AUX_VA_ARGS(...)    __VA_ARGS__
			#define CRY_AUX_STRIP_PARENS(X) X

			#define CRY_ASSERT_MESSAGE(condition, ...)                           \
	do                                                                       \
	{                                                                        \
		IF_UNLIKELY (!(condition))                                             \
		{                                                                      \
			::Detail::SAssertData const assertData =                             \
			{                                                                    \
				# condition,                                                       \
				__func__,                                                          \
				__FILE__,                                                          \
				__LINE__ };                                                        \
			static ::Detail::SAssertCond assertCond =                            \
			{                                                                    \
				false, true };                                                     \
			if (::Detail::CryAssertHandler(assertData, assertCond, __VA_ARGS__)) \
				CryDebugBreak();                                                   \
		}                                                                      \
		PREFAST_ASSUME(condition);                                             \
	} while (false)

			#define CRY_ASSERT_TRACE(condition, parenthese_message) \
	CRY_ASSERT_MESSAGE(condition, CRY_AUX_STRIP_PARENS(CRY_AUX_VA_ARGS parenthese_message))

			#define CRY_ASSERT(condition) CRY_ASSERT_MESSAGE(condition, nullptr)

		#else

//! Use the platform's default assert.
			#include <assert.h>
			#undef CRY_WAS_USE_ASSERT_SET
			#define CRY_WAS_USE_ASSERT_SET 0
			#undef USE_CRY_ASSERT
			#undef CRY_ASSERT_TRACE
			#undef CRY_ASSERT_MESSAGE
			#undef CRY_ASSERT
			#define CRY_ASSERT_TRACE(condition, parenthese_message) assert(condition)
			#define CRY_ASSERT_MESSAGE(condition, ...)              assert(condition)
			#define CRY_ASSERT(condition)                           assert(condition)

		#endif

		#ifdef IS_EDITOR_BUILD
			#undef  Q_ASSERT
			#undef  Q_ASSERT_X
// Q_ASSERT handler
// Qt uses expression with chained asserts separated by ',' like: { return Q_ASSERT(n >= 0), Q_ASSERT(n < size()), QChar(m_data[n]); }
			#define  CRY_ASSERT_HANDLER_QT(cond, ...)                                        \
	([&] {                                                                               \
		const ::Detail::SAssertData assertData = { # cond, __func__, __FILE__, __LINE__ }; \
		static ::Detail::SAssertCond assertCond = { false, true };                         \
		if (::Detail::CryAssertHandler(assertData, assertCond, __VA_ARGS__))               \
		{                                                                                  \
		  CryDebugBreak();                                                                 \
		}                                                                                  \
		return static_cast<void>(0);                                                       \
	} ())

			#define Q_ASSERT(cond, ...)           ((cond) ? static_cast<void>(0) : CRY_ASSERT_HANDLER_QT(cond, "Q_ASSERT"))
			#define Q_ASSERT_X(cond, where, what) ((cond) ? static_cast<void>(0) : CRY_ASSERT_HANDLER_QT(cond, "Q_ASSERT_X" where what))
		#endif

//! This forces boost to use CRY_ASSERT, regardless of what it is defined as.
		#define BOOST_ENABLE_ASSERT_HANDLER
namespace boost
{
inline void assertion_failed_msg(char const* const szExpr, char const* const szMsg, char const* const szFunction, char const* const szFile, long const line)
{
		#if !defined(FORCE_STANDARD_ASSERT)
	::Detail::SAssertData const assertData =
	{
		szExpr, szFunction, szFile, line };
	static ::Detail::SAssertCond assertCond =
	{
		false, true };
	if (::Detail::CryAssertHandler(assertData, assertCond, szMsg))
		CryDebugBreak();
		#else
	CRY_ASSERT_TRACE(false, ("An assertion failed in boost: expr=%s, msg=%s, function=%s, file=%s, line=%d", szExpr, szMsg, szFunction, szFile, (int)line));
		#endif // FORCE_STANDARD_ASSERT
}

inline void assertion_failed(char const* const szExpr, char const* const szFunction, char const* const szFile, long const line)
{
	assertion_failed_msg(szExpr, "BOOST_ASSERT", szFunction, szFile, line);
}
} // namespace boost
	#else
// Used in release configuration.
		#define CRY_WAS_USE_ASSERT_SET 0
		#undef USE_CRY_ASSERT
		#undef CRY_ASSERT_TRACE
		#undef CRY_ASSERT_MESSAGE
		#undef CRY_ASSERT
		#define CRY_ASSERT_TRACE(condition, parenthese_message) ((void)0)
		#define CRY_ASSERT_MESSAGE(condition, ...)              ((void)0)
		#define CRY_ASSERT(condition)                           ((void)0)

		#if defined(FORCE_STANDARD_ASSERT)
// This empty assert is needed by RC in Release configuration.
			#define assert(expression) ((void)0)
		#endif
	#endif // _RELEASE

namespace Cry
{
template<typename T, typename ... Args>
inline T const& VerifyWithMessage(T const& expr, Args&& ... args)
{
	CRY_ASSERT_MESSAGE(expr, std::forward<Args>(args) ...);
	return expr;
}
} // namespace Cry

	#define CRY_VERIFY(expr)                   Cry::VerifyWithMessage(expr, # expr)
	#define CRY_VERIFY_WITH_MESSAGE(expr, ...) Cry::VerifyWithMessage(expr, __VA_ARGS__)

#endif   // CRY_ASSERT_H_INCLUDED

// Note: This ends the "once per compilation unit" part of this file, from here on, the code is included every time
// See the top of this file on the reasoning behind this.
#if defined(USE_CRY_ASSERT)
	#if CRY_WAS_USE_ASSERT_SET != 1
		#error USE_CRY_ASSERT value changed between includes of CryAssert.h (was undefined, now defined)
	#endif
#else
	#if CRY_WAS_USE_ASSERT_SET != 0
		#error USE_CRY_ASSERT value changed between includes of CryAssert.h (was defined, now undefined)
	#endif
#endif

// This is for every code base that wants to use CRY_ASSERT instead of standard assert.
// Other tools such as RC define FORCE_STANDARD_ASSERT to use the standard assert in non-release configuration.
// In Release configuration they'll have to define an empty assert.
#if !defined(FORCE_STANDARD_ASSERT)
	#undef assert
	#define assert CRY_ASSERT
#endif
