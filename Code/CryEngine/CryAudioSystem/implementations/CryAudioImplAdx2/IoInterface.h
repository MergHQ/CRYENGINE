// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <cri_file_system.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
extern CriFsIoInterface g_IoInterface;

static CriFsIoError IoFileExists(CriChar8 const* szPath, CriBool* pResult);
static CriFsIoError IoFileOpen(CriChar8 const* szPath, CriFsFileMode mode, CriFsFileAccess access, CriFsFileHn* pFileHn);
static CriFsIoError IoFileClose(CriFsFileHn pFileHn);
static CriFsIoError IoFileGetFileSize(CriFsFileHn pFileHn, CriSint64* pFileSize);
static CriFsIoError IoFileRead(CriFsFileHn pFileHn, CriSint64 offset, CriSint64 readSize, void* pBuffer, CriSint64 bufferSize);
static CriFsIoError IoFileIsReadComplete(CriFsFileHn pFileHn, CriBool* pResult);
static CriFsIoError IoFileGetReadSize(CriFsFileHn pFileHn, CriSint64* pReadSize);
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
