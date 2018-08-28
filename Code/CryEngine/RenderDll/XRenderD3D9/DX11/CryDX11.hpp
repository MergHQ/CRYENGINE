// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 

#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern int g_nPrintDX11;

#if !_RELEASE
	#define DX11_ERROR(...) \
		do { CryLog("DX11 Error: " __VA_ARGS__); } while (0)
	#define DX11_ASSERT(cond, ...) \
		do { if (!(cond)) { DX11_ERROR(__VA_ARGS__); CRY_ASSERT_MESSAGE(0, __VA_ARGS__); } } while (0)
#else
	#define DX11_ERROR(...)        do {} while (0)
	#define DX11_ASSERT(cond, ...) do {} while (0)
#endif

#ifdef _DEBUG
	#define DX11_LOG(cond, ...) \
		do { if (cond || g_nPrintDX11) { CryLog("DX11 Log: " __VA_ARGS__); } } while (0)
	#define DX11_WARNING(cond, ...) \
		do { if (!(cond)) { DX11_LOG(__VA_ARGS__); } } while (0)
	#define DX11_ASSERT_DEBUG(cond, ...) DX11_ASSERT(cond, __VA_ARGS__)
#else
	#define DX11_LOG(cond, ...)          do {} while (0)
	#define DX11_WARNING(cond, ...)      do {} while (0)
	#define DX11_ASSERT_DEBUG(cond, ...) do {} while (0)
#endif

#define DX11_NOT_IMPLEMENTED DX11_ASSERT(0, "Not implemented!");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "API/DX11Base.hpp"

#if CRY_PLATFORM_WINDOWS
HRESULT WINAPI DX11CreateDevice(
	IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	ID3D11Device** ppDevice,
	D3D_FEATURE_LEVEL* pFeatureLevel,
	ID3D11DeviceContext** ppImmediateContext);
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void ClearDebugName(ID3D11DeviceChild* pNativeResource)
{
#if !defined(RELEASE) && CRY_PLATFORM_WINDOWS
	if (!pNativeResource)
		return;

	pNativeResource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
#endif
}

inline void SetDebugName(ID3D11DeviceChild* pNativeResource, const char* name, ...)
{
#if !defined(RELEASE) && CRY_PLATFORM_WINDOWS
	if (!pNativeResource)
		return;

	va_list args;
	va_start(args, name);

	char* buffer = (char*)_alloca(512);
	if (_vsnprintf(buffer, 512, name, args) < 0)
		return;

	pNativeResource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
	pNativeResource->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(buffer), buffer);

	va_end(args);
#endif
}

inline std::string GetDebugName(ID3D11DeviceChild* pNativeResource)
{
#if !defined(RELEASE) && CRY_PLATFORM_WINDOWS
	if (!pNativeResource)
		return "nullptr";

	do
	{
		UINT length = 512;
		char* buffer = (char*)_alloca(length);
		HRESULT hr = pNativeResource->GetPrivateData(WKPDID_D3DDebugObjectName, &length, buffer);
		if (hr == S_OK)
			return buffer;
		if (hr != D3D11_MESSAGE_ID_GETPRIVATEDATA_MOREDATA)
			return "failure";
		
		length += 512;
	}
	while (true);
#endif

	return "";
}
