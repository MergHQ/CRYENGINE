// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// -------------------------------------------------------------------------
//  Description: Interface for the platform specific function libraries
//               Include this file instead of windows.h and similar platform specific header files
// -------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////
//! This define allows including the detail headers which are setting platform specific settings.
#define CRYPLATFROM_ALLOW_DETAIL_INCLUDES

////////////////////////////////////////////////////////////////////////////
// some ifdef selection to include the correct platform implementation
#if CRY_PLATFORM_WINDOWS
	#include <CryCore/Platform/CryPlatform.Win64.h>
#elif CRY_PLATFORM_DURANGO
	#include <CryCore/Platform/CryPlatform.Durango.h>
#elif CRY_PLATFORM_ORBIS
	#include <CryCore/Platform/CryPlatform.Orbis.h>
#elif CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID
	#include <CryCore/Platform/CryPlatform.Linux.h>
#elif CRY_PLATFORM_APPLE
	#include "CryCore/Platform/CryPlatform.Darwin.h"
#else
	#error Current Platform not supported by CryPlatform. Please provide a concrete implementation library.
#endif

///////////////////////////////////////////////////////////////////////////
// Verify that all required macros are set.
///////////////////////////////////////////////////////////////////////////
#if !defined(__DETAIL__LINK_THIRD_PARTY_LIBRARY)
	#error __DETAIL__LINK_THIRD_PARTY_LIBRARY not defined for current platform
#endif

#if !defined(__DETAIL__LINK_SYSTEM_PARTY_LIBRARY)
	#error __DETAIL__LINK_SYSTEM_PARTY_LIBRARY not defined for current platform
#endif

#ifdef CRY_NO_PRAGMA_LIB
	#define LINK_THIRD_PARTY_LIBRARY(name)
	#define LINK_SYSTEM_LIBRARY(name)
#else

////////////////////////////////////////////////////////////////////////////
//! Include a third party library. The path has to be specified relative to the Code/ folder.
//! In addition the path has to be specified as a literal, not as a string, and forward slashes have to be used.
//! For example: LINK_THIRD_PARTY_LIBRARY("SDK/MyLib/lib/MyLib.a").
	#define LINK_THIRD_PARTY_LIBRARY(name) __DETAIL__LINK_THIRD_PARTY_LIBRARY(name)

////////////////////////////////////////////////////////////////////////////
//! Include a platform library.
	#define LINK_SYSTEM_LIBRARY(name) __DETAIL__LINK_SYSTEM_PARTY_LIBRARY(name)

#endif //CRY_NO_PRAGMA_LIB

////////////////////////////////////////////////////////////////////////////
//! Disallow including of detail header.
#undef CRYPLATFROM_ALLOW_DETAIL_INCLUDES
