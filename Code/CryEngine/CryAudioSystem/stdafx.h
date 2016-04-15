// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#pragma warning( disable:4786 )

#include <CryCore/Project/CryModuleDefs.h>
#define eCryModule eCryM_AudioSystem
#include <CryCore/Platform/platform.h>

#if !defined(_RELEASE)
	#define INCLUDE_AUDIO_PRODUCTION_CODE
	#define ENABLE_AUDIO_LOGGING
#endif // _RELEASE

#include <CryCore/StlUtils.h>
#include <CryCore/Project/ProjectDefines.h>
#include <SoundAllocator.h>
#include <AudioLogger.h>

extern CSoundAllocator g_audioMemoryPoolPrimary;
extern CAudioLogger g_audioLogger;
extern CTimeValue g_lastMainThreadFrameStartTime;

#define AUDIO_ALLOCATOR_MEMORY_POOL g_audioMemoryPoolPrimary

#include <STLSoundAllocator.h>

// Windows or Durango
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
#endif

// Windows32
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_WINDOWS && CRY_PLATFORM_32BIT
#endif

// Windows64
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_WINDOWS && CRY_PLATFORM_64BIT
#endif

// Durango
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_DURANGO
//#include <xdk.h>
#endif

//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_ORBIS
#endif

// Mac
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_MAC
#endif

// Android
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_ANDROID
#endif

// iOS
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_IOS
#endif

// Linux
//////////////////////////////////////////////////////////////////////////
#if CRY_PLATFORM_LINUX
#endif
