// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// Note: Can't use #pragma once here, since (like assert.h) this file CAN be included more than once.
// Each time it's included, it will re-define assert to match CRY_ASSERT.
// This behavior can be used to "fix" 3rdParty headers that #include <assert.h> (by including CryAssert.h after the 3rdParty header).
// In the case where assert's definition gets trampled, just including CryAssert.h should fix that problem for the remainder of the file.
#ifndef CRY_ASSERT_H_INCLUDED
	#define CRY_ASSERT_H_INCLUDED

// FORCE_STANDARD_ASSERT is set by some tools (like RC)
	#if !defined(_RELEASE) && !defined(FORCE_STANDARD_ASSERT)
		#define USE_CRY_ASSERT
	#else
		#undef USE_CRY_ASSERT
	#endif

//-----------------------------------------------------------------------------------------------------
// Use like this:
// CRY_ASSERT(expression);
// CRY_ASSERT_MESSAGE(expression,"Useful message");
// CRY_ASSERT_TRACE(expression,("This should never happen because parameter %d named %s is %f",iParameter,szParam,fValue));
//-----------------------------------------------------------------------------------------------------

	#if defined(USE_CRY_ASSERT)

void CryAssertSetGlobalFlagAddress(int*);
bool CryAssertIsEnabled();
void CryAssertTrace(const char*, ...);
bool CryAssert(const char*, const char*, unsigned int, bool*);
void CryDebugBreak();

//! The code to insert when assert is used.
		#define CRY_ASSERT_TRACE(condition, parenthese_message)                  \
		  do                                                                     \
		  {                                                                      \
		    static bool s_bIgnoreAssert = false;                                 \
		    if (!s_bIgnoreAssert && !(condition) && CryAssertIsEnabled())        \
		    {                                                                    \
		      CryAssertTrace parenthese_message;                                 \
		      if (CryAssert( # condition, __FILE__, __LINE__, &s_bIgnoreAssert)) \
		      {                                                                  \
		        __debugbreak();                                                  \
		      }                                                                  \
		    }                                                                    \
		  } while (0)
		#define CRY_ASSERT_MESSAGE(condition, message) CRY_ASSERT_TRACE(condition, (message))
		#define CRY_ASSERT(condition)                  CRY_ASSERT_MESSAGE(condition, nullptr)

	#else

//! Use the platform's default assert.
		#include <assert.h>
		#define CRY_ASSERT_TRACE(condition, parenthese_message) assert(condition)
		#define CRY_ASSERT_MESSAGE(condition, message)          assert(condition)
		#define CRY_ASSERT(condition)                           assert(condition)

	#endif

	#ifdef IS_EDITOR_BUILD
		#define Q_ASSERT(cond)                CRY_ASSERT_MESSAGE(cond, "Q_ASSERT")
		#define Q_ASSERT_X(cond, where, what) CRY_ASSERT_MESSAGE(cond, "Q_ASSERT_X" where what)
	#endif

//! This forces boost to use CRY_ASSERT, regardless of what it is defined as.
	#define BOOST_ENABLE_ASSERT_HANDLER
namespace boost
{
inline void assertion_failed_msg(const char* expr, const char* msg, const char* function, const char* file, long line)
{
	CRY_ASSERT_TRACE(false, ("An assertion failed in boost: expr=%s, msg=%s, function=%s, file=%s, line=%d", expr, msg, function, file, (int)line));
}
inline void assertion_failed(const char* expr, const char* function, const char* file, long line)
{
	assertion_failed_msg(expr, "BOOST_ASSERT", function, file, line);
}
}

// Remember the value of USE_CRY_ASSERT
	#if defined(USE_CRY_ASSERT)
		#define CRY_WAS_USE_ASSERT_SET 1
	#else
		#define CRY_WAS_USE_ASSERT_SET 0
	#endif

#endif

// Note: This ends the "once per compilation unit" part of this file, from here on, the code is included every time
// See the top of this file on the reasoning behind this.
#if defined(USE_CRY_ASSERT)
	#if CRY_WAS_USE_ASSERT_SET != 1
		#error USE_CRY_ASSERT value changed between includes of CryAssert.h (was undefined, now defined)
	#endif
	#undef assert
	#define assert CRY_ASSERT
#else
	#if CRY_WAS_USE_ASSERT_SET != 0
		#error USE_CRY_ASSERT value changed between includes of CryAssert.h (was defined, now undefined)
	#endif
#endif
