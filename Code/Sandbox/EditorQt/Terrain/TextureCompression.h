// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryRenderer/ITexture.h>
#include <CryThreading/CryThread.h>

class CFile;

class CTextureCompression
{
public:
	explicit CTextureCompression(const bool bHighQuality);

	void WriteDDSToFile(CFile& toFile, unsigned char* dat, int w, int h, int Size,
	                    ETEX_Format eSrcF, ETEX_Format eDstF, int NumMips, const bool bHeader, const bool bDither);

private:
	static void SaveCompressedMipmapLevel(const void* data, size_t size, void* userData);

	bool                      m_bHighQuality; // true:slower but better quality, false=use hardware instead
	static CFile*             m_pFile;
	static CryCriticalSection m_sFileLock;
};
