// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryCore/Platform/platform.h>
#include <CryCore/Platform/platform_impl.inl>
#include "AudioSystemEditor_fmod.h"
#include <CrySystem/ISystem.h>

using namespace ACE;

CAudioSystemEditor_fmod* g_pFmodInterface;

//------------------------------------------------------------------
extern "C" ACE_API IAudioSystemEditor * GetAudioInterface(ISystem * pSystem)
{
	ModuleInitISystem(pSystem, "EditorFmod");
	if (!g_pFmodInterface)
	{
		g_pFmodInterface = new CAudioSystemEditor_fmod();
	}
	return g_pFmodInterface;
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
		if (g_pFmodInterface)
		{
			delete g_pFmodInterface;
			g_pFmodInterface = nullptr;
		}
		break;
	}
	return TRUE;
}
