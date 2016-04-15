// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioSystemImplCVars_sdlmixer.h"
#include <CrySystem/IConsole.h>

namespace CryAudio
{
namespace Impl
{

//////////////////////////////////////////////////////////////////////////
CAudioSystemImplCVars::CAudioSystemImplCVars()
	: m_nPrimaryPoolSize(0)
{
}

//////////////////////////////////////////////////////////////////////////
CAudioSystemImplCVars::~CAudioSystemImplCVars()
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemImplCVars::RegisterVariables()
{
#if CRY_PLATFORM_WINDOWS
	m_nPrimaryPoolSize = 128 << 10;     // 128 MiB
#elif CRY_PLATFORM_MAC
	m_nPrimaryPoolSize = 128 << 10;     // 128 MiB
#elif CRY_PLATFORM_LINUX
	m_nPrimaryPoolSize = 128 << 10;     // 128 MiB
#elif CRY_PLATFORM_IOS
	m_nPrimaryPoolSize = 8 << 10;       // 8 MiB
#elif CRY_PLATFORM_ANDROID
	m_nPrimaryPoolSize = 8 << 10;       // 8 MiB
#else
	#error "Undefined platform."
#endif

	REGISTER_CVAR2("s_SDLMixerPrimaryPoolSize", &m_nPrimaryPoolSize, m_nPrimaryPoolSize, VF_REQUIRE_APP_RESTART,
	               "Specifies the size (in KiB) of the memory pool to be used by the SDL Mixer audio system implementation.\n"
	               "Usage: s_SDLMixerPrimaryPoolSize [0/...]\n"
	               "Default PC: 131072 (128 MiB), Mac: 131072 (128 MiB), Linux: 131072 (128 MiB), iOS: 8192 (8 MiB), Android: 8192 (8 MiB)\n");
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemImplCVars::UnregisterVariables()
{
	IConsole* const pConsole = gEnv->pConsole;
	CRY_ASSERT(pConsole);

	pConsole->UnregisterVariable("s_SDLMixerPrimaryPoolSize");
}

}
}
