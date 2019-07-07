// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "MFCToolsDefines.h"

class CImageEx;

class MFC_TOOLS_PLUGIN_API CImageTIF
{
public:
	bool               Load(const string& fileName, CImageEx& outImage);
	bool               SaveRAW(const string& fileName, const void* pData, int width, int height, int bytesPerChannel, int numChannels, bool bFloat, const char* preset);
	static const char* GetPreset(const string& fileName);
};
