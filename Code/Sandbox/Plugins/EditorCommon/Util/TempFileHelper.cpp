// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TempFileHelper.h"

#include "FileUtils.h"
#include "PathUtils.h"

CTempFileHelper::CTempFileHelper(const char* szFileName)
{
	// Do not change casing of the existing files.
	if (FileUtils::FileExists(szFileName))
	{
		m_fileName = PathUtil::MatchAbsolutePathToCaseOnFileSystem(szFileName);
	}
	else
	{
		m_fileName = PathUtil::AdjustCasing(szFileName).c_str();
	}

	m_tempFileName = PathUtil::ReplaceExtension(m_fileName.GetString(), "tmp");

	FileUtils::MakeFileWritable(m_tempFileName);
	FileUtils::RemoveFile(m_tempFileName);
}

CTempFileHelper::~CTempFileHelper()
{
	FileUtils::RemoveFile(m_tempFileName);
}

bool CTempFileHelper::UpdateFile(bool backup)
{
	// First, check if the files are actually different
	if (!FileUtils::Pak::CompareFiles(m_tempFileName, m_fileName))
	{
		// If the file changed, make sure the destination file is writable
		if (FileUtils::FileExists(m_fileName) && !FileUtils::MakeFileWritable(m_fileName))
		{
			FileUtils::RemoveFile(m_tempFileName);
			return false;
		}

		// Back up the current file if requested
		if (backup)
		{
			FileUtils::BackupFile(m_fileName);
		}

		// Move the temp file over the top of the destination file
		return FileUtils::MoveFileAllowOverwrite(m_tempFileName, m_fileName);
	}
	// If the files are the same, just delete the temp file and return.
	else
	{
		FileUtils::RemoveFile(m_tempFileName);
		return true;
	}
}
