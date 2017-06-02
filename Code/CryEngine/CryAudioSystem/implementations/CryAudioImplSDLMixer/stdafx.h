// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Project/CryModuleDefs.h>
#define eCryModule eCryM_AudioImplementation
#include <CryCore/Platform/platform.h>
#include <CryCore/StlUtils.h>
#include <CryCore/Project/ProjectDefines.h>
#include <CrySystem/ISystem.h>
#include <AudioLogger.h>

extern CryAudio::CLogger g_implLogger;

#if !defined(_RELEASE)
	#define INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
	#define ENABLE_AUDIO_LOGGING
#endif
