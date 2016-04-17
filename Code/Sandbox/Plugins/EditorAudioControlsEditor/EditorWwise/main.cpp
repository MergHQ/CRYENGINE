// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryCore/Platform/platform.h>
#include <CryCore/Platform/platform_impl.inl>
#include "AudioSystemEditor_wwise.h"
#include <CrySystem/ISystem.h>

using namespace ACE;

CAudioSystemEditor_wwise* g_pWwiseInterface;

//------------------------------------------------------------------
extern "C" ACE_API IAudioSystemEditor * GetAudioInterface(ISystem * pSystem)
{
	ModuleInitISystem(pSystem, "EditorWwise");
	if (!g_pWwiseInterface)
	{
		g_pWwiseInterface = new CAudioSystemEditor_wwise();
	}
	return g_pWwiseInterface;
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
		if (g_pWwiseInterface)
		{
			delete g_pWwiseInterface;
			g_pWwiseInterface = nullptr;
		}
		break;
	}
	return TRUE;
}
