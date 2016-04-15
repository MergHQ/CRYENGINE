/*
	SH allocator implementations
*/

#include "stdafx.h"
#include <PRT/SHAllocator.h>
#include <PRT/ISHLog.h>	//top be able to log 
#include <string>


#undef min
#undef max


extern NSH::ISHLog& GetSHLog();

#if defined(USE_MEM_ALLOCATOR)
	static HMODULE gsSystem = 0;
	static FNC_SHMalloc gspSHAllocFnc = 0;
	static FNC_SHFreeSize gspSHFreeSizeFnc = 0;
#endif

#if defined(USE_MEM_ALLOCATOR)
//instantiate simple byte allocator
CSHAllocator<unsigned char> gsByteAllocator;

void LoadAllocatorModule(FNC_SHMalloc& rpfMalloc, FNC_SHFreeSize& rpfFreeSize)
{
	if(!gsSystem)
	{
		CRITICAL_SECTION cs;
		InitializeCriticalSection(&cs);
		EnterCriticalSection(&cs);
#if defined(WIN64)
		gsSystem = LoadLibrary("SHAllocator.dll"); 
#else
		gsSystem = LoadLibrary("SHAllocator.dll"); 
#endif
		if(!gsSystem)
		{
			//now assume current dir is one level below
#if defined(WIN64)
			gsSystem = LoadLibrary("bin64/SHAllocator.dll"); 
#else
			gsSystem = LoadLibrary("bin32/SHAllocator.dll");
#endif
		}
		if(!gsSystem)
		{
			//support rc, is one level above
#if defined(WIN64)
			gsSystem = LoadLibrary("../SHAllocator_x64.dll");
#else
			gsSystem = LoadLibrary("../SHAllocator.dll");
#endif
		}
		gspSHAllocFnc			=(FNC_SHMalloc)GetProcAddress((HINSTANCE)gsSystem,"SHMalloc"); 
		gspSHFreeSizeFnc	=(FNC_SHFreeSize)GetProcAddress((HINSTANCE)gsSystem,"SHFreeSize"); 
		if(!gsSystem || !gspSHAllocFnc || !gspSHFreeSizeFnc)
		{
			char szMasterCDDir[256];
			GetCurrentDirectory(256,szMasterCDDir);
			GetSHLog().LogError("Could not access SHAllocator.dll (check working dir, currently: %s)\n", szMasterCDDir);
			std::string buf("Could not access SHAllocator.dll (check working dir, currently: ");
			buf += szMasterCDDir;
			buf += ")";
			MessageBox(NULL, buf.c_str(), "PRT Memory Manager", MB_OK);
			if(gsSystem)
				::FreeLibrary(gsSystem);
			exit(1);
		}; 
		LeaveCriticalSection(&cs);
	}
	rpfFreeSize = gspSHFreeSizeFnc;
	rpfMalloc = gspSHAllocFnc;
}
#endif 