// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "IoInterface.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
CriFsIoInterface g_IoInterface = {
	IoFileExists,
	CRI_NULL,
	CRI_NULL,
	IoFileOpen,
	IoFileClose,
	IoFileGetFileSize,
	IoFileRead,
	IoFileIsReadComplete,
	CRI_NULL,
	IoFileGetReadSize,
	CRI_NULL,
	CRI_NULL,
	CRI_NULL,
	CRI_NULL,
	CRI_NULL, };

struct SFileObject
{
	SFileObject() = default;

	FILE*     pFile = nullptr;
	CriSint64 fileSize = 0;
	CriSint64 readSize = 0;
};

//////////////////////////////////////////////////////////////////////////
CriFsIoError IoFileExists(CriChar8 const* szPath, CriBool* pResult)
{
	CRY_ASSERT_MESSAGE(szPath != nullptr, "szPath is null pointer");
	CRY_ASSERT_MESSAGE(pResult != nullptr, "pResult is null pointer");

	*pResult = (gEnv->pCryPak->IsFileExist(static_cast<char const*>(szPath)) ? CRI_TRUE : CRI_FALSE);

	return CRIFS_IO_ERROR_OK;
}

//////////////////////////////////////////////////////////////////////////
CriFsIoError IoFileOpen(CriChar8 const* szPath, CriFsFileMode mode, CriFsFileAccess access, CriFsFileHn* pFileHn)
{
	CRY_ASSERT_MESSAGE(szPath != nullptr, "szPath is null pointer");
	CRY_ASSERT_MESSAGE(pFileHn != nullptr, "pFileHn is null pointer");

	CriFsIoError result = CRIFS_IO_ERROR_OK;

	if ((mode == CRIFS_FILE_MODE_OPEN) && (access == CRIFS_FILE_ACCESS_READ))
	{
		auto const pFileObject = new SFileObject();

		if (pFileObject != nullptr)
		{
			auto const szFileName = static_cast<char const*>(szPath);
			pFileObject->pFile = gEnv->pCryPak->FOpen(szFileName, "rbx");

			if (pFileObject->pFile != nullptr)
			{
				gEnv->pCryPak->FSeek(pFileObject->pFile, 0, SEEK_END);
				pFileObject->fileSize = static_cast<CriSint64>(gEnv->pCryPak->FTell(pFileObject->pFile));
				gEnv->pCryPak->FSeek(pFileObject->pFile, 0, SEEK_SET);

				*pFileHn = static_cast<CriFsFileHn>(pFileObject);
			}
			else
			{
				delete pFileObject;
				result = CRIFS_IO_ERROR_NG;
			}
		}
		else
		{
			result = CRIFS_IO_ERROR_NG;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
CriFsIoError IoFileClose(CriFsFileHn pFileHn)
{
	CRY_ASSERT_MESSAGE(pFileHn != nullptr, "pFileHn is null pointer");

	CriFsIoError result = CRIFS_IO_ERROR_NG;

	auto const pFileObject = static_cast<SFileObject*>(pFileHn);

	if (pFileObject->pFile != nullptr)
	{
		gEnv->pCryPak->FClose(pFileObject->pFile);
		pFileObject->pFile = nullptr;
		result = CRIFS_IO_ERROR_OK;
	}

	delete pFileObject;

	return result;
}

//////////////////////////////////////////////////////////////////////////
CriFsIoError IoFileGetFileSize(CriFsFileHn pFileHn, CriSint64* pFileSize)
{
	CRY_ASSERT_MESSAGE(pFileHn != nullptr, "pFileHn is null pointer");
	CRY_ASSERT_MESSAGE(pFileSize != nullptr, "pFileSize is null pointer");

	auto const pFileObject = static_cast<SFileObject*>(pFileHn);
	*pFileSize = pFileObject->fileSize;

	return CRIFS_IO_ERROR_OK;
}

//////////////////////////////////////////////////////////////////////////
CriFsIoError IoFileRead(CriFsFileHn pFileHn, CriSint64 offset, CriSint64 readSize, void* pBuffer, CriSint64 bufferSize)
{
	CRY_ASSERT_MESSAGE(pFileHn != nullptr, "pFileHn is null pointer");
	CRY_ASSERT_MESSAGE(offset >= 0, "offset is < 0");
	CRY_ASSERT_MESSAGE(readSize >= 0, "readSize is < 0");
	CRY_ASSERT_MESSAGE(pBuffer != nullptr, "pBuffer is null pointer");
	CRY_ASSERT_MESSAGE(bufferSize >= 0, "bufferSize is < 0");

	CriFsIoError result = CRIFS_IO_ERROR_NG;

	auto const pFileObject = static_cast<SFileObject*>(pFileHn);

	if (gEnv->pCryPak->FSeek(pFileObject->pFile, static_cast<long>(offset), SEEK_SET) == 0)
	{
		auto const reqByte = static_cast<size_t>(std::min(readSize, bufferSize));
		auto const rdByte = static_cast<size_t>(gEnv->pCryPak->FReadRaw(pBuffer, 1, reqByte, pFileObject->pFile));

		if (gEnv->pCryPak->FError(pFileObject->pFile) == 0)
		{
			pFileObject->readSize = static_cast<CriSint64>(rdByte);
			result = CRIFS_IO_ERROR_OK;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
CriFsIoError IoFileIsReadComplete(CriFsFileHn pFileHn, CriBool* pResult)
{
	CRY_ASSERT_MESSAGE(pFileHn != nullptr, "pFileHn is null pointer");
	CRY_ASSERT_MESSAGE(pResult != nullptr, "pResult is null pointer");

	*pResult = CRI_TRUE;

	return CRIFS_IO_ERROR_OK;
}

//////////////////////////////////////////////////////////////////////////
CriFsIoError IoFileGetReadSize(CriFsFileHn pFileHn, CriSint64* pReadSize)
{
	CRY_ASSERT_MESSAGE(pFileHn != nullptr, "pFileHn is null pointer");
	CRY_ASSERT_MESSAGE(pReadSize != nullptr, "pReadSize is null pointer");

	auto const pFileObject = static_cast<SFileObject*>(pFileHn);
	*pReadSize = pFileObject->readSize;

	return CRIFS_IO_ERROR_OK;
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
