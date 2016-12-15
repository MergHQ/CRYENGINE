#include "stdafx.h"

#include "systemutilities.h"
#include "GeomDecalClassDesc.h"

extern "C" // Declare extern C to avoid name mangling.
{
	// This function is called by Windows when the DLL is loaded.  This 
	// function may also be called many times during time critical operations
	// like rendering.  Therefore developers need to be careful what they
	// do inside this function.  In the code below, note how after the DLL is
	// loaded the first time only a few statements are executed.
	BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID /*lpvReserved*/)
	{
		if (fdwReason == DLL_PROCESS_ATTACH) // This will only be called once, when the DLL is first loaded.
		{
			MaxSDK::Util::UseLanguagePackLocale();
			// Hang on to this DLL's instance handle.
			hInstance = hinstDLL;
			DisableThreadLibraryCalls(hInstance);
		}
		return TRUE;
	}

	__declspec(dllexport) const TCHAR* LibDescription()
	{
		return GetString(IDS_LIBDESC);
	}

	DEFINE_PLUGIN_CLASS_DESCRIPTIONS(GeomDecalClassDesc::GetInstance());

	// Returns a pre-defined constant indicating the version of 
	// the system under which it was compiled.  It is used to allow the system
	// to catch obsolete DLLs.
	__declspec(dllexport) ULONG LibVersion()
	{
		return VERSION_3DSMAX;
	}
}