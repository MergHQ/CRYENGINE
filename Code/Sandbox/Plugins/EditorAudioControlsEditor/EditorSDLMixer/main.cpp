// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryCore/Platform/platform.h>
#include <CryCore/Platform/platform_impl.inl>
#include "AudioSystemEditor_sdlmixer.h"
#include <CrySystem/ISystem.h>

using namespace ACE;

CAudioSystemEditor_sdlmixer* g_pSDLMixerInterface;

//------------------------------------------------------------------
extern "C" ACE_API IAudioSystemEditor * GetAudioInterface(ISystem * pSystem)
{
	ModuleInitISystem(pSystem, "EditorSDLMixer");
	if (!g_pSDLMixerInterface)
	{
		g_pSDLMixerInterface = new CAudioSystemEditor_sdlmixer();
	}
	return g_pSDLMixerInterface;
}

//------------------------------------------------------------------
HINSTANCE g_hInstance = 0;
BOOL __stdcall DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hInstance = hinstDLL;
		break;
	case DLL_PROCESS_DETACH:
		if (g_pSDLMixerInterface)
		{
			delete g_pSDLMixerInterface;
			g_pSDLMixerInterface = nullptr;
		}
		break;
	}
	return TRUE;
}
