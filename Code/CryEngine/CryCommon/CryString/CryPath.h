// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/Platform/CryWindows.h>
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
	CRY_ASSERT(gEnv && gEnv->pCryPak);
	return gEnv->pCryPak->GetGameFolder();
}

inline string GetLocalizationFolder()
{
	CRY_ASSERT(gEnv && gEnv->pCryPak);
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

template<size_t SIZE>
inline CryStackStringT<char, SIZE> ToUnixPath(const CryStackStringT<char, SIZE>& strPath)
{
	if (strPath.find('\\') != CryStackStringT<char, SIZE>::npos)
	{
		CryStackStringT<char, SIZE> path = strPath;
		path.replace('\\', '/');
		return path;
	}
	return strPath;
}

inline void ToUnixPath(char*& szPath)
{
	const size_t length = strlen(szPath);
	std::replace(szPath, szPath + length, '\\', '/');
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

inline void ToDosPath(char*& szPath)
{
	const size_t length = strlen(szPath);
	std::replace(szPath, szPath + length, '/', '\\');
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
template<size_t SIZE>
inline CryStackStringT<char, SIZE> GetPath(const CryStackStringT<char, SIZE>& filepath)
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

template<size_t SIZE>
inline CryStackStringT<char, SIZE> GetFile(const CryStackStringT<char, SIZE>& filepath)
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

//! Remove extension for given file.
inline void RemoveExtension(char* szFilePath)
{
	for (char* p = szFilePath + strlen(szFilePath) - 1; p >= szFilePath; --p)
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
			*p = '\0';
			return;
		}
	}
	// it seems the file name is a pure name, without extension
}

//! Remove extension for given file.
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
	// it seems the file name is a pure name, without extension
}

//! Replace extension for given file.
template<size_t SIZE>
inline void RemoveExtension(CryStackStringT<char, SIZE>& filepath)
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

inline string RemoveExtension(const string& filepath)
{
	string result = filepath;
	RemoveExtension(result);
	return result;
}

//! Extract file name without extension from full specified file path.
inline string GetFileName(const string& filepath)
{
	string file = filepath;
	RemoveExtension(file);
	return GetFile(file);
}

inline string GetFileName(const char* szFilepath)
{
	return GetFileName(string(szFilepath));
}

template<size_t SIZE>
inline CryStackStringT<char, SIZE> GetFileName(const CryStackStringT<char, SIZE>& filepath)
{
	CryStackStringT<char, SIZE> file = filepath;
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

//! Add a slash if necessary.
inline string AddSlash(const string& path)
{
	if (path.empty() || path[path.length() - 1] == '/')
		return path;
	if (path[path.length() - 1] == '\\')
		return path.substr(0, path.length() - 1) + "/";
	return path + "/";
}

//! Add a slash if necessary.
template<size_t SIZE>
inline CryStackStringT<char, SIZE> AddSlash(const CryStackStringT<char, SIZE>& path)
{
	if (path.empty() || path[path.length() - 1] == '/')
		return path;
	if (path[path.length() - 1] == '\\')
		return path.substr(0, path.length() - 1) + "/";
	return path + "/";
}

//! Add a backslash if necessary.
inline string AddBackslash(const string& path)
{
	if (path.empty() || path[path.length() - 1] == '\\')
		return path;
	if (path[path.length() - 1] == '/')
		return path.substr(0, path.length() - 1) + "\\";
	return path + "\\";
}

template<size_t SIZE>
inline CryStackStringT<char, SIZE> ReplaceExtension(const CryStackStringT<char, SIZE>& filepath, const char* ext)
{
	CryStackStringT<char, SIZE> str = filepath;
	if (ext != nullptr)
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
	if (ext != nullptr)
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
template<size_t SIZE>
inline CryStackStringT<char, SIZE> Make(const CryStackStringT<char, SIZE>& path, const CryStackStringT<char, SIZE>& file)
{
	return AddSlash(path) + file;
}

//! Makes a fully specified file path from path and file name.
template<size_t SIZE>
inline CryStackStringT<char, SIZE> Make(const CryStackStringT<char, SIZE>& dir, const CryStackStringT<char, SIZE>& filename, const CryStackStringT<char, SIZE>& ext)
{
	CryStackStringT<char, SIZE> path = ReplaceExtension(filename, ext);
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
inline string Make(const char* path, const char* file)
{
	return Make(string(path), string(file));
}

//! Makes a fully specified file path from path and file name.
inline string Make(const char* path, const char* file, const char* ext)
{
	return Make(string(path), string(file), string(ext));
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
template<size_t SIZE>
inline CryStackStringT<char, SIZE> MakeGamePath(const CryStackStringT<char, SIZE>& path)
{
	CryStackStringT<char, SIZE> fullpath = ToUnixPath(path);
	CryStackStringT<char, SIZE> rootDataFolder = ToUnixPath(AddSlash(PathUtil::GetGameFolder()));
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
inline CryStackStringT<T, SIZE> GetParentDirectory(const CryStackStringT<T, SIZE>& strFilePath, int nGeneration = 1)
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

inline bool IsDirectory(const char* szPath)
{
	return (GetFileAttributes(szPath) & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

inline std::vector<string> GetDirectoryStructure(const string& path)
{
	if (path.empty())
	{
		return std::vector<string>();
	}

	string currentDirectoryName;
	const char* pchCurrentPosition = path.c_str();
	const char* pchLastPosition = path.c_str();

	// It removes as many slashes the path has in its start...
	// MAYBE and just maybe we should consider paths starting with
	// more than 2 slashes invalid paths...
	while ((*pchLastPosition == '\\') || (*pchLastPosition == '/'))
	{
		++pchLastPosition;
		++pchCurrentPosition;
	}

	std::vector<string> result;
	do
	{
		pchCurrentPosition = strpbrk(pchLastPosition, "\\/");
		if (pchCurrentPosition == nullptr)
		{
			break;
		}
		currentDirectoryName.assign(pchLastPosition, pchCurrentPosition);
		pchLastPosition = pchCurrentPosition + 1;
		// Again, here we are skipping as many consecutive slashes.
		while ((*pchLastPosition == '\\') || (*pchLastPosition == '/'))
		{
			++pchLastPosition;
		}

		result.push_back(currentDirectoryName);

	}
	while (true);

	return result;
}

enum EPathStyle
{
	//! Allows Posix paths:
	//! - absolute path (/foo/bar)
	//! - relative path (foo, ../bar) paths
	//! Output slashes are '/'
	ePathStyle_Posix,

	//! Allows Posix paths as well as these Windows specific paths:
	//! - UNC path (\\server\foo\bar)
	//! - drive-absolute path (c:\foo\bar)
	//! - drive-relative path (c:foo, c:..\bar)
	//! Output slashes are '\'
	ePathStyle_Windows,

#if CRY_PLATFORM_WINAPI
	ePathStyle_Native = ePathStyle_Windows,
#elif CRY_PLATFORM_POSIX
	ePathStyle_Native = ePathStyle_Posix,
#else
	#error Native path style is not supported
#endif
};

//! Simplifies a given file path, such that . and .. are removed (if possible).
//! Slashes are unified according to the path style and redundant and trailing slashes are removed.
//! The path-style also determines what inputs are accepted by the function. See EPathStyle enumeration values for more information.
//! If an error occurs or if the buffer is too small, the buffer will contain an empty string and the function returns false.
//! \note The output will always be shorter or equal length to the input, so a buffer of the same size as the input will always work.
inline bool SimplifyFilePath(const char* const szInput, char* const szBuf, const size_t bufLength, const EPathStyle pathStyle)
{

#define CRY_SIMPLIFY_REJECT { *szBuf = 0; return false; \
  }
#define CRY_SIMPLIFY_EMIT(expr) { if (--pOut < szBuf) { *szBuf = 0; return false; } *pOut = expr; }

	if (szBuf == nullptr || bufLength == 0)
	{
		return false;
	}
	if (szInput == nullptr || (*szInput == 0))
	{
		CRY_SIMPLIFY_REJECT;
	}

	const bool bWinApi = pathStyle == ePathStyle_Windows;
	const char kSlash = bWinApi ? '\\' : '/';
	const char* pIn = szInput + strlen(szInput);
	const char* pLast = pIn - 1;
	const char* pLastSlash = pIn;
	char* pOut = szBuf + bufLength;
	size_t skipElements = 0;
	bool bDots = true;
	char driveRelative = 0;

	CRY_SIMPLIFY_EMIT(0); // null-terminator
	while (pIn != szInput)
	{
		assert(pIn >= szInput);
		const char c = *--pIn;
		switch (c)
		{
		case '\\':
		case '/':
			if ((pIn == szInput + 1) && ((szInput[0] == '\\') || (szInput[0] == '/'))) // UNC path
			{
				if (!bWinApi || bDots || skipElements != 0)
				{
					CRY_SIMPLIFY_REJECT;
				}
				CRY_SIMPLIFY_EMIT(kSlash);
				CRY_SIMPLIFY_EMIT(kSlash);
				pIn = szInput;
				continue;
			}
			else if (bDots) // handle redundant slashes and . and .. elements
			{
				const size_t numDots = pLastSlash - pIn - 1;
				if (numDots == 2)
				{
					++skipElements;
				}
				else if ((numDots != 0) && (numDots != 1) && (pIn != pLast))
				{
					CRY_SIMPLIFY_REJECT;
				}
			}
			else if (skipElements != 0) // mark eaten element
			{
				--skipElements;
			}
			if ((*pOut != kSlash) && (skipElements == 0))
			{
				if (*pOut != '\0') // don't emit trailing slashes
				{
					CRY_SIMPLIFY_EMIT(kSlash);
				}
				else if (pIn == szInput || (bWinApi && pIn[-1] == ':')) // exception for single slash input '/' and 'c:\'
				{
					CRY_SIMPLIFY_EMIT(kSlash);
				}
			}
			pLastSlash = pIn;
			bDots = true;
			continue;

		case '.':
			if (bDots) // count dots
			{
				continue;
			}
			break;

		case ':':
			if (bWinApi) // ':' should belong to a drive, otherwise it is not an allowed char in win
			{
				if ((pIn != szInput + 1) || ((pLastSlash == szInput + 2) && (skipElements != 0)))
				{
					CRY_SIMPLIFY_REJECT;
				}
				else // handle drive identifier
				{
					const char driveLetter = pIn[-1];
					if (!(driveLetter >= 'a' && driveLetter <= 'z') && !(driveLetter >= 'A' && driveLetter <= 'Z'))
					{
						CRY_SIMPLIFY_REJECT;
					}
					if (pLastSlash == szInput + 2)
					{
						if (skipElements != 0)
						{
							CRY_SIMPLIFY_REJECT;
						}
					}
					else if (bDots)
					{
						const size_t numDots = pLastSlash - pIn - 1;
						if (numDots == 2)
						{
							CRY_SIMPLIFY_EMIT('.');
							CRY_SIMPLIFY_EMIT('.');
						}
						else if (numDots != 1)
						{
							CRY_SIMPLIFY_REJECT;
						}
					}
					else if (skipElements != 0)
					{
						--skipElements;
					}
					driveRelative = driveLetter;
					pIn = szInput;
					bDots = false;
					continue;
				}
			}
		// fall-through
		default:
			if (bDots)
			{
				if (skipElements == 0)
				{
					const size_t numDots = pLastSlash - pIn - 1;
					for (size_t i = 0; i < numDots; ++i)
					{
						CRY_SIMPLIFY_EMIT('.');
					}
				}
				bDots = false;
			}
			break;
		}
		if (!skipElements)
		{
			CRY_SIMPLIFY_EMIT(c);
		}
	}

	if (bDots) // record remaining dots
	{
		const size_t numDots = pLastSlash - szInput;
		if (numDots == 2)
		{
			++skipElements;
		}
		else if (numDots == 1)
		{
			if ((*pOut == kSlash) && (skipElements == 0))
			{
				++pOut; // leading dot should eat a slash
			}
			else if (*pOut == 0)
			{
				CRY_SIMPLIFY_EMIT('.'); // special case, the input is only a dot, keep it
			}
		}
		else if (skipElements != 0)
		{
			CRY_SIMPLIFY_REJECT;
		}
	}
	else if (skipElements && !driveRelative) // if not bDots, then we read a relative element that needs to be discounted, e.g. a/..
	{
		--skipElements;
	}

	for (size_t i = 0; i < skipElements; ++i) // flush all pending dots
	{
		CRY_SIMPLIFY_EMIT('.');
		CRY_SIMPLIFY_EMIT('.');
		if (i != skipElements - 1)
		{
			CRY_SIMPLIFY_EMIT(kSlash);
		}
	}

	if (driveRelative != 0) // Fix up non-absolute but drive-relative paths.
	{
		CRY_SIMPLIFY_EMIT(':');
		CRY_SIMPLIFY_EMIT(driveRelative);
	}

	if (pOut != szBuf) // left-align in the buffer
	{
		const size_t resultLength = szBuf + bufLength - pOut;
		assert(resultLength > 0);
		memmove(szBuf, pOut, resultLength);
	}
	return true;

#undef CRY_SIMPLIFY_REJECT
#undef CRY_SIMPLIFY_EMIT
}

//! Converts \ to / and replaces ASCII characters to lower-case (A-Z only).
//! This function is ASCII-only and Unicode agnostic.
template<size_t SIZE>
inline void UnifyFilePath(CryStackStringT<char, SIZE>& strPath)
{
	strPath.replace('\\', '/');
	strPath.MakeLower();
}

inline void UnifyFilePath(string& strPath)
{
	strPath.replace('\\', '/');
	strPath.MakeLower();
}
}
