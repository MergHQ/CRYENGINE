// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include <CryString/CryPath.h>

namespace FileUtils
{
//! Removes the file or directory at the given path. If the path points to a directory it will be removed recursively (does not use CryPak).
EDITOR_COMMON_API bool Remove(const char* szPath);

//! Removes the file at the given file path (does not use CryPak).
EDITOR_COMMON_API bool RemoveFile(const char* szFilePath);

//! Moves the file at the given file path to the new file path allowing overwriting of the target (does not use CryPak).
EDITOR_COMMON_API bool MoveFileAllowOverwrite(const char* szOldFilePath, const char* szNewFilePath);

//! Copies the file at the given file path to the target file path allowing overwriting of the target (does not use CryPak).
EDITOR_COMMON_API bool CopyFileAllowOverwrite(const char* szSourceFilePath, const char* szDestinationFilePath);

//! Recursively copies the directory from 'source' to 'destination' directory. Existing files in 'destination' will not be overwritten
EDITOR_COMMON_API bool CopyDirectory(const char* szSource, const char* szDestination);

//! Recursively copies the contents of directory from 'source' to 'destination' directory. Existing files in 'destination' will not be overwritten
EDITOR_COMMON_API bool CopyDirectoryContents(const char* szSource, const char* szDestination);

//! Non-recursively copies the files in a directory from 'source' to 'destination' directory. Existing files in 'destination' will not be overwritten
EDITOR_COMMON_API bool CopyFiles(const char* szSource, const char* szDestination);

//! Recursively moves the directory from 'source' to 'destination' directory. Existing files in 'destination' will not be overwritten
EDITOR_COMMON_API bool MoveDirectory(const char* szSource, const char* szDestination);

//! Recursively moves the contents of directory from 'source' to 'destination' directory. Existing files in 'destination' will not be overwritten
EDITOR_COMMON_API bool MoveDirectoryContents(const char* szSource, const char* szDestination);

//! Non-recursively moves the files in a directory from 'source' to 'destination' directory. Existing files in 'destination' will not be overwritten
EDITOR_COMMON_API bool MoveFiles(const char* szSource, const char* szDestination);

//! Removes the directory at the given path (does not use CryPak).
EDITOR_COMMON_API bool RemoveDirectory(const char* szPath, bool bRecursive = true);

//! Sets the file permission from read only to read-write (does not use CryPak).
EDITOR_COMMON_API bool MakeFileWritable(const char* szFilePath);

//! Returns true if the file exists on disk (does not use CryPak)
EDITOR_COMMON_API bool FileExists(const string& path);

//! Returns true if the path (file of folder) exists on disk (does not use CryPak)
EDITOR_COMMON_API bool PathExists(const string& path);

//! Returns true if the gives is folder that exists on disk (does not use CryPak).
EDITOR_COMMON_API bool FolderExists(const string& path);

//! Makes a backup file (does not use CryPak).
EDITOR_COMMON_API void BackupFile(const char* szFilePath);

//! Returns the list of files or folders inside given directory (does not use CryPak).
//! \param depthLevel How deep should recursion go. Default is 0 which means no limit.
//! \param includeFolders Specifies if folders should be included in the result list.
EDITOR_COMMON_API std::vector<string> GetDirectorysContent(const string& dirPath, int depthLevel = 0, bool includeFolders = false);

//! Returns the list of files or folders inside given directory (does not use CryPak).
//! \param depthLevel How deep should recursion go. Default is 0 which means no limit.
//! \param includeFolders Specifies if folders should be included in the result list.
EDITOR_COMMON_API std::vector<string> GetDirectorysContent(const QString& dirPath, int depthLevel = 0, bool includeFolders = false);

// Functions that respect CryPak system.
namespace Pak
{
//! Returns true if the file exists only in paks.
EDITOR_COMMON_API bool IsFileInPakOnly(const string& path);
//! Returns true if the file exists either in paks or on file system.
EDITOR_COMMON_API bool IsFileInPakOrOnDisk(const string& path);
// Returns true if the files have the same content, false otherwise.
EDITOR_COMMON_API bool CompareFiles(const string& filePath1, const string& filePath2);
//! Copies the file at the given file path to the target file path allowing overwriting of the target.

EDITOR_COMMON_API bool CopyFileAllowOverwrite(const char* szSourceFilePath, const char* szDestinationFilePath);

//! Extracts all the files from a pak file. The pak should already be open by GetISystem()->GetIPak()->OpenPack();
//! The function will silently overwrite existing files.
//! \param szArchivePath Path to the existing pak file to be unpacked.
//! \param szDestPath Path to the destination folder.
//! \param progress. A function object that is called each time another portion of the pak file has been extracted. The progress value is in the range 0..1 inclusive.
//! \see ICryPak::OpenPack()
EDITOR_COMMON_API void Unpak(const char* szArchivePath, const char* szDestPath, std::function<void(float)> progress);
}

}
