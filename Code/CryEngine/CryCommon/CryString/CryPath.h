// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   CryPath.h
//  Version:     v1.00
//  Created:     19/10/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: Defines namespace PathUtil for operations on files paths.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CryPath_h__
#define __CryPath_h__
#pragma once

#include <CrySystem/ISystem.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IConsole.h>

#if CRY_PLATFORM_POSIX
	#define CRY_NATIVE_PATH_SEPSTR "/"
#else
	#define CRY_NATIVE_PATH_SEPSTR "\\"
#endif

namespace PathUtil
{
inline string GetGameFolder()
{
	return (gEnv->pCryPak->GetGameFolder());
}

inline string GetLocalizationFolder()
{
	return gEnv->pCryPak->GetLocalizationFolder();
}

//! Convert a path to the uniform form.
inline string ToUnixPath(const string& strPath)
{
	if (strPath.find('\\') != string::npos)
	{
		string path = strPath;
		path.replace('\\', '/');
		return path;
	}
	return strPath;
}

//! Convert a path to the uniform form in place on stack.
inline void ToUnixPath(stack_string& rConv)
{
	const char* const cpEnd = &(rConv.c_str()[rConv.size()]);
	char* __restrict pC = rConv.begin();
	while (pC != cpEnd)
	{
		char c = *pC;
		if (c == '\\')
			c = '/';
		*pC++ = c;
	}
}

//! Convert a path to the DOS form.
inline string ToDosPath(const string& strPath)
{
	if (strPath.find('/') != string::npos)
	{
		string path = strPath;
		path.replace('/', '\\');
		return path;
	}
	return strPath;
}

//! Convert a path to lowercase form.
inline string ToLower(const string& strPath)
{
	string path = strPath;
	path.MakeLower();
	return path;
}

//! Split full file name to path and filename.
//! \param[in] filepath Full file name including path.
//! \param[out] path Extracted file path.
//! \param[out] filename Extracted file (without extension).
//! \param[out] ext Extracted files extension.
inline void Split(const string& filepath, string& path, string& filename, string& fext)
{
	path = filename = fext = string();
	if (filepath.empty())
		return;
	const char* str = filepath.c_str();
	const char* pext = str + filepath.length() - 1;
	const char* p;
	for (p = str + filepath.length() - 1; p >= str; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			path = filepath.substr(0, p - str + 1);
			filename = filepath.substr(p - str + 1, pext - p);
			return;
		case '.':
			// there's an extension in this file name
			fext = filepath.substr(p - str + 1);
			pext = p;
			break;
		}
	}
	filename = filepath.substr(p - str + 1, pext - p);
}

//! Split full file name to path and filename.
//! \param[in] filepath Full file name inclusing path.
//! \param[out] path Extracted file path.
//! \param[out] file Extracted file (with extension).
inline void Split(const string& filepath, string& path, string& file)
{
	string fext;
	Split(filepath, path, file, fext);
	file += fext;
}

//! Extract extension from full specified file path.
//! \return Pointer to the extension (without .) or pointer to an empty 0-terminated string.
inline const char* GetExt(const char* filepath)
{
	const char* str = filepath;
	size_t len = strlen(filepath);
	for (const char* p = str + len - 1; p >= str; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			// we've reached a path separator - it means there's no extension in this name
			return "";
		case '.':
			// there's an extension in this file name
			return p + 1;
		}
	}
	return "";
}

//! Extract path from full specified file path.
inline string GetPath(const string& filepath)
{
	const char* str = filepath.c_str();
	for (const char* p = str + filepath.length() - 1; p >= str; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			return filepath.substr(0, p - str + 1);
		}
	}
	return "";
}

//! Extract path from full specified file path.
inline string GetPath(const char* filepath)
{
	return GetPath(string(filepath));
}

//! Extract path from full specified file path.
inline stack_string GetPath(const stack_string& filepath)
{
	const char* str = filepath.c_str();
	for (const char* p = str + filepath.length() - 1; p >= str; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			return filepath.substr(0, p - str + 1);
		}
	}
	return "";
}

//! Extract file name with extension from full specified file path.
inline string GetFile(const string& filepath)
{
	const char* str = filepath.c_str();
	for (const char* p = str + filepath.length() - 1; p >= str; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			return filepath.substr(p - str + 1);
		}
	}
	return filepath;
}

inline const char* GetFile(const char* filepath)
{
	const size_t len = strlen(filepath);
	for (const char* p = filepath + len - 1; p >= filepath; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			return p + 1;
		}
	}
	return filepath;
}

//! Replace extension for given file.
inline void RemoveExtension(string& filepath)
{
	const char* str = filepath.c_str();
	for (const char* p = str + filepath.length() - 1; p >= str; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			// we've reached a path separator - it means there's no extension in this name
			return;
		case '.':
			// there's an extension in this file name
			filepath = filepath.substr(0, p - str);
			return;
		}
	}
	// it seems the file name is a pure name, without path or extension
}

//! Replace extension for given file.
inline void RemoveExtension(stack_string& filepath)
{
	const char* str = filepath.c_str();
	for (const char* p = str + filepath.length() - 1; p >= str; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			// we've reached a path separator - it means there's no extension in this name
			return;
		case '.':
			// there's an extension in this file name
			filepath = filepath.substr(0, p - str);
			return;
		}
	}
	// it seems the file name is a pure name, without path or extension
}

//! Extract file name without extension from full specified file path.
inline string GetFileName(const string& filepath)
{
	string file = filepath;
	RemoveExtension(file);
	return GetFile(file);
}

//! Removes the trailing slash or backslash from a given path.
inline string RemoveSlash(const string& path)
{
	if (path.empty() || (path[path.length() - 1] != '/' && path[path.length() - 1] != '\\'))
		return path;
	return path.substr(0, path.length() - 1);
}

inline string GetEnginePath()
{
	char szEngineRootDir[_MAX_PATH];
	CryFindEngineRootFolder(CRY_ARRAY_COUNT(szEngineRootDir), szEngineRootDir);
	return RemoveSlash(szEngineRootDir);
}

inline string GetSlash()
{
	return CRY_NATIVE_PATH_SEPSTR;
}

//! Add a backslash if necessary.
inline string AddSlash(const string& path)
{
	if (path.empty() || path[path.length() - 1] == '/')
		return path;
	if (path[path.length() - 1] == '\\')
		return path.substr(0, path.length() - 1) + "/";
	return path + "/";
}

//! Add a backslash if necessary.
inline stack_string AddSlash(const stack_string& path)
{
	if (path.empty() || path[path.length() - 1] == '/')
		return path;
	if (path[path.length() - 1] == '\\')
		return path.substr(0, path.length() - 1) + "/";
	return path + "/";
}

//! Add a backslash if necessary.
inline string AddSlash(const char* path)
{
	return AddSlash(string(path));
}

inline stack_string ReplaceExtension(const stack_string& filepath, const char* ext)
{
	stack_string str = filepath;
	if (ext != 0)
	{
		RemoveExtension(str);
		if (ext[0] != 0 && ext[0] != '.')
		{
			str += ".";
		}
		str += ext;
	}
	return str;
}

//! Replace extension for given file.
inline string ReplaceExtension(const string& filepath, const char* ext)
{
	string str = filepath;
	if (ext != 0)
	{
		RemoveExtension(str);
		if (ext[0] != 0 && ext[0] != '.')
		{
			str += ".";
		}
		str += ext;
	}
	return str;
}

//! Replace extension for given file.
inline string ReplaceExtension(const char* filepath, const char* ext)
{
	return ReplaceExtension(string(filepath), ext);
}

//! Makes a fully specified file path from path and file name.
inline string Make(const string& path, const string& file)
{
	return AddSlash(path) + file;
}

//! Makes a fully specified file path from path and file name.
inline string Make(const string& dir, const string& filename, const string& ext)
{
	string path = ReplaceExtension(filename, ext);
	path = AddSlash(dir) + path;
	return path;
}

//! Makes a fully specified file path from path and file name.
inline string Make(const string& dir, const string& filename, const char* ext)
{
	return Make(dir, filename, string(ext));
}

//! Makes a fully specified file path from path and file name.
inline stack_string Make(const stack_string& path, const stack_string& file)
{
	return AddSlash(path) + file;
}

//! Makes a fully specified file path from path and file name.
inline stack_string Make(const stack_string& dir, const stack_string& filename, const stack_string& ext)
{
	stack_string path = ReplaceExtension(filename, ext);
	path = AddSlash(dir) + path;
	return path;
}

//! Makes a fully specified file path from path and file name.
inline string Make(const char* path, const string& file)
{
	return Make(string(path), file);
}

//! Makes a fully specified file path from path and file name.
inline string Make(const string& path, const char* file)
{
	return Make(path, string(file));
}

//! Makes a fully specified file path from path and file name.
inline string Make(const char path[], const char file[])
{
	return Make(string(path), string(file));
}

//! Makes a fully specified file path from path and file name.
inline string Make(const char* path, const char* file, const char* ext)
{
	return Make(string(path), string(file), string(ext));
}

//! Makes a fully specified file path from path and file name.
inline string MakeFullPath(const string& relativePath)
{
	return relativePath;
}

//////////////////////////////////////////////////////////////////////////
//! Make a game correct path out of any input path.
inline string MakeGamePath(const string& path)
{
	string fullpath = ToUnixPath(path);
	string rootDataFolder = ToUnixPath(AddSlash(PathUtil::GetGameFolder()));
	if (fullpath.length() > rootDataFolder.length() && strnicmp(fullpath.c_str(), rootDataFolder.c_str(), rootDataFolder.length()) == 0)
	{
		return fullpath.substr(rootDataFolder.length(), fullpath.length() - rootDataFolder.length());
	}
	//fullpath = GetRelativePath(path);
	return fullpath;
}

//////////////////////////////////////////////////////////////////////////
//! Make a game correct path out of any input path.
inline stack_string MakeGamePath(const stack_string& path)
{
	stack_string fullpath = ToUnixPath(path).c_str();
	stack_string rootDataFolder = ToUnixPath(AddSlash(PathUtil::GetGameFolder())).c_str();
	if (fullpath.length() > rootDataFolder.length() && strnicmp(fullpath.c_str(), rootDataFolder.c_str(), rootDataFolder.length()) == 0)
	{
		return fullpath.substr(rootDataFolder.length(), fullpath.length() - rootDataFolder.length());
	}
	//fullpath = GetRelativePath(path);
	return fullpath;
}

inline string GetParentDirectory(const string& strFilePath, int nGeneration = 1)
{
	for (const char* p = strFilePath.c_str() + strFilePath.length() - 2;   // -2 is for the possible trailing slash: there always must be some trailing symbol which is the file/directory name for which we should get the parent
	     p >= strFilePath.c_str();
	     --p)
	{
		switch (*p)
		{
		case ':':
			return string(strFilePath.c_str(), p);
		case '/':
		case '\\':
			// we've reached a path separator - return everything before it.
			if (!--nGeneration)
				return string(strFilePath.c_str(), p);
			break;
		}
	}
	;
	// it seems the file name is a pure name, without path or extension
	return string();
}

template<typename T, size_t SIZE>
inline CryStackStringT<T, SIZE> GetParentDirectoryStackString(const CryStackStringT<T, SIZE>& strFilePath, int nGeneration = 1)
{
	for (const char* p = strFilePath.c_str() + strFilePath.length() - 2;   // -2 is for the possible trailing slash: there always must be some trailing symbol which is the file/directory name for which we should get the parent
	     p >= strFilePath.c_str();
	     --p)
	{
		switch (*p)
		{
		case ':':
			return CryStackStringT<T, SIZE>(strFilePath.c_str(), p);
		case '/':
		case '\\':
			// we've reached a path separator - return everything before it.
			if (!--nGeneration)
				return CryStackStringT<T, SIZE>(strFilePath.c_str(), p);
			break;
		}
	}
	;
	// it seems the file name is a pure name, without path or extension
	return CryStackStringT<T, SIZE>();
}

//! \return True if the string matches the wildcard.
inline bool MatchWildcard(const char* szString, const char* szWildcard)
{
	const char* pString = szString, * pWildcard = szWildcard;
	// skip the obviously the same starting substring
	while (*pWildcard && *pWildcard != '*' && *pWildcard != '?')
		if (*pString != *pWildcard)
			return false;   // must be exact match unless there's a wildcard character in the wildcard string
		else
			++pString, ++pWildcard;

	if (!*pString)
	{
		// this will only match if there are no non-wild characters in the wildcard
		for (; *pWildcard; ++pWildcard)
			if (*pWildcard != '*' && *pWildcard != '?')
				return false;
		return true;
	}

	switch (*pWildcard)
	{
	case '\0':
		return false;   // the only way to match them after the leading non-wildcard characters is !*pString, which was already checked

	// we have a wildcard with wild character at the start.
	case '*':
		{
			// merge consecutive ? and *, since they are equivalent to a single *
			while (*pWildcard == '*' || *pWildcard == '?')
				++pWildcard;

			if (!*pWildcard)
				return true;   // the rest of the string doesn't matter: the wildcard ends with *

			for (; *pString; ++pString)
				if (MatchWildcard(pString, pWildcard))
					return true;

			return false;
		}
	case '?':
		return MatchWildcard(pString + 1, pWildcard + 1) || MatchWildcard(pString, pWildcard + 1);
	default:
		assert(0);
		return false;
	}
}

};

#endif //__CryPath_h__
