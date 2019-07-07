// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// Note: we use '#define xxx 1' because in this case both '#if xxx' and '#if defined(xxx)' work for checking.

//! This define was added just to let programmers check that CryPlatformDefines.h is
//! already included (so CRY_PLATFORM_xxx are already available).
#define CRY_PLATFORM 1

// Detecting CPU. See:
// http://nadeausoftware.com/articles/2012/02/c_c_tip_how_detect_processor_type_using_compiler_predefined_macros
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0491c/BABJFEFG.html
#if defined(__x86_64__) || defined(_M_X64)
	#define CRY_PLATFORM_X64       1
	#define CRY_PLATFORM_SSE2      1
#elif defined(__i386) || defined(_M_IX86) || defined(__arm__)
	#error 32-bit platforms are not supported.
#elif defined(__aarch64__)
	#define CRY_PLATFORM_ARM    1
#else
	#define CRY_PLATFORM_UNKNOWNCPU 1
#endif

#if defined(__APPLE__)

	#define CRY_PLATFORM_APPLE 1
	#define CRY_PLATFORM_POSIX 1
	#include <TargetConditionals.h>
	#if TARGET_OS_IPHONE
		#define CRY_PLATFORM_MOBILE 1
		#define CRY_PLATFORM_IOS    1
		#if !CRY_PLATFORM_UNKNOWNCPU
			#error iOS: Unsupported CPU
		#endif
	#elif TARGET_OS_MAC
		#define CRY_PLATFORM_DESKTOP 1
		#define CRY_PLATFORM_MAC     1
		#if !CRY_PLATFORM_X64
			#error Unsupported Mac CPU (the only supported is x86-64).
		#endif
	#else
		#error Unknown Apple platform.
	#endif

#elif defined(_DURANGO) || defined(_XBOX_ONE)

	#define CRY_PLATFORM_CONSOLE 1
	#define CRY_PLATFORM_DURANGO 1
	#define CRY_PLATFORM_WINAPI  1
	#if !CRY_PLATFORM_X64
		#error Unsupported Durango CPU (the only supported is x86-64).
	#endif
	#define CRY_PLATFORM_SSE4    1
	#define CRY_PLATFORM_F16C    1
	#define CRY_PLATFORM_BMI1    1
	#define CRY_PLATFORM_TBM     1

#elif defined(_ORBIS) || defined(__ORBIS__)

	#define CRY_PLATFORM_CONSOLE 1
	#define CRY_PLATFORM_ORBIS   1
	#define CRY_PLATFORM_POSIX   1
	#if !CRY_PLATFORM_X64
		#error Unsupported Orbis CPU (the only supported is x86-64).
	#endif
	#define CRY_PLATFORM_SSE4    1
	#define CRY_PLATFORM_F16C    1
	#define CRY_PLATFORM_BMI1    1
	#define CRY_PLATFORM_TBM     0

#elif defined(ANDROID) || defined(__ANDROID__)

	#define CRY_PLATFORM_MOBILE  1
	#define CRY_PLATFORM_ANDROID 1
	#define CRY_PLATFORM_POSIX   1

#elif defined(_WIN32)

	#define CRY_PLATFORM_DESKTOP 1
	#define CRY_PLATFORM_WINDOWS 1
	#define CRY_PLATFORM_WINAPI  1
	#if !defined(_WIN64)
		#error Unsupported Windows 64 CPU (the only supported is x86-64).
	#endif

#elif defined(__linux__) || defined(__linux)

	#define CRY_PLATFORM_DESKTOP 1
	#define CRY_PLATFORM_LINUX   1
	#define CRY_PLATFORM_POSIX   1
	#if !CRY_PLATFORM_X64
		#error Unsupported Linux CPU (the only supported are x86-64).
	#endif

#else

	#error Unknown target platform.

#endif

#if CRY_PLATFORM_AVX
#define CRY_PLATFORM_ALIGNMENT 32
#elif CRY_PLATFORM_SSE2 || CRY_PLATFORM_SSE4 || CRY_PLATFORM_NEON
#define CRY_PLATFORM_ALIGNMENT 16U
#else
#define CRY_PLATFORM_ALIGNMENT 1U
#endif

// Validation

#if defined(CRY_PLATFORM_X64) && CRY_PLATFORM_X64 != 1
	#error Wrong value of CRY_PLATFORM_X64.
#endif

#if defined(CRY_PLATFORM_ARM) && CRY_PLATFORM_ARM != 1
	#error Wrong value of CRY_PLATFORM_ARM.
#endif

#if defined(CRY_PLATFORM_UNKNOWN_CPU) && CRY_PLATFORM_UNKNOWN_CPU != 1
	#error Wrong value of CRY_PLATFORM_UNKNOWN_CPU.
#endif

#if CRY_PLATFORM_X64 + CRY_PLATFORM_ARM + CRY_PLATFORM_UNKNOWNCPU != 1
	#error Invalid CPU type.
#endif

#if defined(CRY_PLATFORM_DESKTOP) && CRY_PLATFORM_DESKTOP != 1
	#error Wrong value of CRY_PLATFORM_DESKTOP.
#endif

#if defined(CRY_PLATFORM_MOBILE) && CRY_PLATFORM_MOBILE != 1
	#error Wrong value of CRY_PLATFORM_MOBILE.
#endif

#if defined(CRY_PLATFORM_CONSOLE) && CRY_PLATFORM_CONSOLE != 1
	#error Wrong value of CRY_PLATFORM_CONSOLE.
#endif

#if CRY_PLATFORM_DESKTOP + CRY_PLATFORM_CONSOLE + CRY_PLATFORM_MOBILE != 1
	#error Invalid CRY_PLATFORM_(DESKTOP/CONSOLE/MOBILE)
#endif

#if defined(__clang__)
	#include <CryCore/Compiler/Clangspecific.h>
#elif defined(__GNUC__)
	#include <CryCore/Compiler/GCCspecific.h>
#elif defined(_MSC_VER)
	#include <CryCore/Compiler/MSVCspecific.h>
#else
	#error Unsupported compiler was used.
#endif
