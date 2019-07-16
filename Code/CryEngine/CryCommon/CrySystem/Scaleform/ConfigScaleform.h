// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifdef INCLUDE_SCALEFORM_SDK
#include <CryCore/Platform/CryWindows.h>
#include <CryCore/Project/ProjectDefines.h>


// Enable Scaleform Video playback implementation
// Note: This requires linking against Scaleform Video library
#define ENABLE_GFX_VIDEO

// Enable Scaleform IME implementation
// Note: This requires linking against Scaleform IME plugin library
#define ENABLE_GFX_IME

#if defined(ENABLE_GFX_VIDEO) && !defined(EXCLUDE_SCALEFORM_VIDEO)
#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO || CRY_PLATFORM_ORBIS
#define USE_GFX_VIDEO
#define GFX_ENABLE_VIDEO
#endif
#endif

#if defined(ENABLE_GFX_IME) && !defined(EXCLUDE_SCALEFORM_IME)
#if CRY_PLATFORM_WINDOWS
#define USE_GFX_IME
#endif
#endif

#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_APPLE || CRY_PLATFORM_MAC) && !defined(GFC_BUILD_SHIPPING)
#define USE_GFX_JPG
#define USE_GFX_PNG
#endif

#if !defined(GFC_USE_LIBJPEG) && defined(USE_GFX_JPG)
#undef USE_GFX_JPG
#endif

#if !defined(GFC_USE_LIBPNG) && defined(USE_GFX_PNG)
#undef USE_GFX_PNG
#endif

#endif // #ifdef INCLUDE_SCALEFORM_SDK
