// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "IResCompiler.h"
#include "XMLConverter.h"
#include "IRCLog.h"
#include "XML/xml.h"

#include <CryCore/Platform/CryLibrary.h>

// Must be included only once in DLL module.
#include <platform_implRC.inl>

static HMODULE g_hInst;

XmlStrCmpFunc g_pXmlStrCmp = &stricmp;

BOOL APIENTRY DllMain(
	HANDLE hModule, 
	DWORD  ul_reason_for_call, 
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = (HMODULE)hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

ICryXML* LoadICryXML()
{
	HMODULE hXMLLibrary = CryLoadLibraryDefName("CryXML");
	if (NULL == hXMLLibrary)
	{
		RCLogError("Unable to load xml library (CryXML" CrySharedLibraryExtension ").");
		return 0;
	}

	FnGetICryXML pfnGetICryXML = (FnGetICryXML)CryGetProcAddress(hXMLLibrary, "GetICryXML");
	if (pfnGetICryXML == 0)
	{
		RCLogError("Unable to load xml library (CryXML" CrySharedLibraryExtension ") - cannot find exported function GetICryXML().");
		return 0;
	}

	return pfnGetICryXML();
}

void __stdcall RegisterConverters( IResourceCompiler *pRC )
{
	SetRCLog(pRC->GetIRCLog());

	ICryXML* pCryXML = LoadICryXML();
	if (pCryXML == 0)
	{
		RCLogError("Loading xml library failed - not registering xml converter.");
		return;
	}

	pCryXML->AddRef();

	pRC->RegisterConverter("XMLConverter", new XMLConverter(pCryXML));

	pRC->RegisterKey("xmlFilterFile", "specify file with special commands to filter out unneeded XML elements and attributes");	

	pCryXML->Release();
}

