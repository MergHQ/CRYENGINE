// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include "FileUtils.h"

#include "PathUtils.h"
#include "QtUtil.h"
#include <CryString/CryPath.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <IEditor.h>
#include <QDirIterator>
#include <stack>

namespace Private_FileUtils
{

void AddDirectorysContent(const QString& dirPath, std::vector<string>& result, int currentLevel, int levelLimit, bool includeFolders)
{
	if (currentLevel >= levelLimit)
	{
		return;
	}
	QDir dir(dirPath);
	QFileInfoList infoList = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
	for (const QFileInfo& fileInfo : infoList)
	{
		const QString absolutePath = fileInfo.absoluteFilePath();
		if (fileInfo.isDir())
		{
			if (includeFolders)
			{
				result.push_back(PathUtil::ToGamePath(QtUtil::ToString(absolutePath)));
			}
			AddDirectorysContent(absolutePath, result, currentLevel + 1, levelLimit, includeFolders);
		}
		else
		{
			result.push_back(PathUtil::ToGamePath(QtUtil::ToString(absolutePath)));
		}
	}
}

} // namespace Private_FileUtils

namespace FileUtils
{

bool RemoveFile(const char* szFilePath)
{
	return QFile::remove(szFilePath);
}

bool Remove(const char* szPath)
{
	QFileInfo info(szPath);

	if (info.isDir())
		return RemoveDirectory(szPath);
	else
		return RemoveFile(szPath);
}

bool MoveFileAllowOverwrite(const char* szOldFilePath, const char* szNewFilePath)
{
	if (QFile::rename(szOldFilePath, szNewFilePath))
	{
		return true;
	}

	// Try to overwrite existing file.
	return QFile::remove(szNewFilePath) && QFile::rename(szOldFilePath, szNewFilePath);
}

bool CopyFileAllowOverwrite(const char* szSourceFilePath, const char* szDestinationFilePath)
{
	GetISystem()->GetIPak()->MakeDir(PathUtil::GetDirectory(szDestinationFilePath));

	if (QFile::copy(szSourceFilePath, szDestinationFilePath))
	{
		return true;
	}

	// Try to overwrite existing file.
	return QFile::remove(szDestinationFilePath) && QFile::copy(szSourceFilePath, szDestinationFilePath);
}

bool RemoveDirectory(const char* szPath, bool bRecursive /* = true*/)
{
	QDir dir(szPath);

	if (!bRecursive)
	{
		const QString dirName = dir.dirName();
		if (dir.cdUp())
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Unable to remove directory: %s", szPath);
			return false;
		}

		if (dir.remove(dirName))
			return true;
	}

	if (dir.removeRecursively())
		return true;

	CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Unable to remove directory: %s", szPath);
	return false;
}

bool MakeFileWritable(const char* szFilePath)
{
	QFile f(QtUtil::ToQString(szFilePath));
	return f.setPermissions(f.permissions() | QFileDevice::WriteOwner);
}

// The pak should be opened.
void Unpak(const char* szArchivePath, const char* szDestPath, std::function<void(float)> progress)
{
	const string pakFolder = PathUtil::GetDirectory(szArchivePath);

	float progressValue = 0; //(0, 1);
	std::vector<char> buffer(1 << 24);

	std::stack<string> stack;
	stack.push("");
	while (!stack.empty())
	{
		const CryPathString mask = PathUtil::Make(stack.top(), "*");
		const CryPathString folder = stack.top();
		stack.pop();

		GetISystem()->GetIPak()->ForEachArchiveFolderEntry(szArchivePath, mask, [szDestPath, &pakFolder, &stack, &folder, &buffer, &progressValue, progress](const ICryPak::ArchiveEntryInfo& entry)
			{
				const CryPathString path(PathUtil::Make(folder.c_str(), entry.szName));
				if (entry.bIsFolder)
				{
				  stack.push(path);
				  return;
				}

				ICryPak* const pPak = GetISystem()->GetIPak();
				FILE* file = pPak->FOpen(PathUtil::Make(pakFolder, path), "rbx");
				if (!file)
				{
				  return;
				}

				if (!pPak->MakeDir(PathUtil::Make(szDestPath, folder)))
				{
				  return;
				}

				buffer.resize(pPak->FGetSize(file));
				const size_t numberOfBytesRead = pPak->FReadRawAll(buffer.data(), buffer.size(), file);
				pPak->FClose(file);

				CryPathString destPath(PathUtil::Make(szDestPath, path));
				QFile destFile(QtUtil::ToQString(destPath.c_str()));
				destFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
				destFile.write(buffer.data(), numberOfBytesRead);

				if (progress)
				{
				  progressValue = std::min(1.0f, progressValue + 0.01f);
				  progress(progressValue);
				}
			});
	}
}

bool PathExists(const string& path)
{
	return QFileInfo(QtUtil::ToQString(path)).exists();
}

bool FileExists(const string& path)
{
	QFileInfo inf(QtUtil::ToQString(path));
	return inf.exists() && inf.isFile();
}

bool FolderExists(const string& path)
{
	QFileInfo inf(QtUtil::ToQString(path));
	return inf.exists() && inf.isDir();
}

std::vector<string> GetDirectorysContent(const string& dirPath, int depthLevel /*= 0*/, bool includeFolders /*= false*/)
{
	return GetDirectorysContent(QtUtil::ToQString(dirPath), depthLevel, includeFolders);
}

std::vector<string> GetDirectorysContent(const QString& dirPath, int depthLevel /*= 0*/, bool includeFolders /*= false*/)
{
	using namespace Private_FileUtils;
	std::vector<string> result;
	AddDirectorysContent(dirPath, result, 0, depthLevel == 0 ? std::numeric_limits<int>::max() : depthLevel, includeFolders);
	return result;
}


bool IsFileInPakOnly(const string& path)
{
	return !FileExists(path) && GetISystem()->GetIPak()->IsFileExist(PathUtil::AbsolutePathToGamePath(path), ICryPak::eFileLocation_InPak);
}

}
