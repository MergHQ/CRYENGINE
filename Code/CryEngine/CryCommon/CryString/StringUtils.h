// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CryStringUtils.h"
#include "CryString.h"
#include "UnicodeFunctions.h"

#if !defined(RESOURCE_COMPILER)
	#include <CryCore/CryCrc32.h>
#endif

#if CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE
	#include <ctype.h>
#endif

#include <algorithm>  // std::replace, std::min

namespace CryStringUtils
{

enum
{
	CRY_DEFAULT_HASH_SEED = 40503,    //!< This is a large 16 bit prime number (perfect for seeding).
};

//! Removes the extension from the file path by truncating the string.
//! This function is Unicode agnostic and locale agnostic.
//! \note If the file has multiple extensions, only the last extension is removed.
//! \return A pointer to the removed extension (if found, without the .), or NULL otherwise.
inline char* StripFileExtension(char* szFilePath)
{
	for (char* p = szFilePath + (int)strlen(szFilePath) - 1; p >= szFilePath; --p)
	{
		switch (*p)
		{
		case ':':
		case '/':
		case '\\':
			// we've reached a path separator - it means there's no extension in this name
			return NULL;
		case '.':
			// there's an extension in this file name
			*p = '\0';
			return p + 1;
		}
	}
	// it seems the file name is a pure name, without path or extension
	return NULL;
}

//! The returned path is WITHOUT the trailing slash.
//! If the input path has a trailing slash, it's ignored.
//! This function is Unicode agnostic and locale agnostic.
//! \param nGeneration - is the number of parents to scan up.
//! \note A drive specifier (if any) will always be kept (Windows-specific).
//! \note If the specified path does not contain enough folders to satisfy the request, an empty string is returned.
//! \return The parent directory of the given file or directory.
template<class StringCls>
StringCls GetParentDirectory(const StringCls& strFilePath, int nGeneration = 1)
{
	// -2 is for the possible trailing slash: there always must be some trailing symbol which is the file/directory name for which we should get the parent
	for (const char* p = strFilePath.c_str() + strFilePath.length() - 2;
	     p >= strFilePath.c_str();
	     --p)
	{
		switch (*p)
		{
		case ':':
			return StringCls(strFilePath.c_str(), p);
			break;
		case '/':
		case '\\':
			// we've reached a path separator - return everything before it.
			if (!--nGeneration)
				return StringCls(strFilePath.c_str(), p);
			break;
		}
	}
	;
	// it seems the file name is a pure name, without path or extension
	return StringCls();
}

//! Converts all ASCII characters to lower case.
//! This function is ASCII-only and locale agnostic.
//! \note Any non-ASCII characters are left unchanged.
inline string toLower(const string& str)
{
	string temp = str;

#ifndef NOT_USE_CRY_STRING
	temp.MakeLower();
#else
	std::transform(temp.begin(), temp.end(), temp.begin(), toLowerAscii); // STL MakeLower
#endif

	return temp;
}

//! Converts all ASCII characters to upper case.
//! This function is ASCII-only and locale agnostic.
//! \note Any non-ASCII characters are left unchanged.
inline string toUpper(const string& str)
{
	string temp = str;

#ifndef NOT_USE_CRY_STRING
	temp.MakeUpper();
#else
	std::transform(temp.begin(), temp.end(), temp.begin(), toUpperAscii); // STL MakeLower
#endif

	return temp;
}

//! Searches and returns the pointer to the extension of the given file.
//! If no extension is found, the function returns a pointer to the terminating NULL (ie, empty string).
//! This function is Unicode agnostic and locale agnostic.
//! \note Do not pass a full path, since the function does not account for drives and directories.
inline const char* FindExtension(const char* szFileName)
{
	const char* szEnd = szFileName + (int)strlen(szFileName);
	for (const char* p = szEnd - 1; p >= szFileName; --p)
		if (*p == '.')
			return p + 1;

	return szEnd;
}

//! Searches and returns the pointer to the file name in the given file path.
//! This function is Unicode agnostic and locale agnostic.
//! \note This function assumes the provided path contains a filename, if it doesn't it returns the parent directory (or an empty string if the path ends with a slash).
inline const char* FindFileNameInPath(const char* szFilePath)
{
	for (const char* p = szFilePath + (int)strlen(szFilePath) - 1; p >= szFilePath; --p)
		if (*p == '\\' || *p == '/')
			return p + 1;
	return szFilePath;
}

//! Works like strstr, but is case-insensitive.
//! This function does not perform Unicode collation and uses the current CRT locale to perform case conversion.
inline const char* stristr(const char* szString, const char* szSubstring)
{
	int nSuperstringLength = (int)strlen(szString);
	int nSubstringLength = (int)strlen(szSubstring);

	for (int nSubstringPos = 0; nSubstringPos <= nSuperstringLength - nSubstringLength; ++nSubstringPos)
	{
		if (strnicmp(szString + nSubstringPos, szSubstring, nSubstringLength) == 0)
			return szString + nSubstringPos;
	}
	return NULL;
}

#ifndef NOT_USE_CRY_STRING

//! Converts \ to / and replaces ASCII characters to lower-case (A-Z only).
//! This function is ASCII-only and Unicode agnostic.
inline void UnifyFilePath(stack_string& strPath)
{
	strPath.replace('\\', '/');
	strPath.MakeLower();
}

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

#endif

//! Converts the number to a string.
//! These functions are Unicode agnostic and locale agnostic (integral) or uses the current CRT locale (float, vector, quat, matrix).
inline string toString(unsigned nNumber)
{
	char szNumber[16];
	cry_sprintf(szNumber, "%u", nNumber);
	return szNumber;
}

inline string toString(signed int nNumber)
{
	char szNumber[16];
	cry_sprintf(szNumber, "%d", nNumber);
	return szNumber;
}

inline string toString(float nNumber)
{
	char szNumber[128];
	cry_sprintf(szNumber, "%f", nNumber);
	return szNumber;
}

inline string toString(bool nNumber)
{
	char szNumber[4];
	cry_sprintf(szNumber, "%i", (int)nNumber);
	return szNumber;
}

#ifdef MATRIX_H
inline string toString(const Matrix44& m)
{
	char szBuf[512];
	cry_sprintf(szBuf, "{%g,%g,%g,%g}{%g,%g,%g,%g}{%g,%g,%g,%g}{%g,%g,%g,%g}",
	            m(0, 0), m(0, 1), m(0, 2), m(0, 3),
	            m(1, 0), m(1, 1), m(1, 2), m(1, 3),
	            m(2, 0), m(2, 1), m(2, 2), m(2, 3),
	            m(3, 0), m(3, 1), m(3, 2), m(3, 3));
	return szBuf;
}
#endif

#ifdef _CRYQUAT_H
inline string toString(const CryQuat& q)
{
	char szBuf[256];
	cry_sprintf(szBuf, "{%g,{%g,%g,%g}}", q.w, q.v.x, q.v.y, q.v.z);
	return szBuf;
}
#endif

#ifdef VECTOR_H
inline string toString(const Vec3& v)
{
	char szBuf[128];
	cry_sprintf(szBuf, "{%g,%g,%g}", v.x, v.y, v.z);
	return szBuf;
}
#endif

//! Does the same as strstr, but the szString is allowed to be no more than the specified size.
//! This function is Unicode agnostic and locale agnostic.
inline const char* strnstr(const char* szString, const char* szSubstring, int nSuperstringLength)
{
	int nSubstringLength = (int)strlen(szSubstring);
	if (!nSubstringLength)
		return szString;

	for (int nSubstringPos = 0; szString[nSubstringPos] && nSubstringPos < nSuperstringLength - nSubstringLength; ++nSubstringPos)
	{
		if (strncmp(szString + nSubstringPos, szSubstring, nSubstringLength) == 0)
			return szString + nSubstringPos;
	}
	return NULL;
}

//! Finds the string in the array of strings.
//! Comparison is case-sensitive.
//! The string array end is demarked by the NULL value.
//! This function is Unicode agnostic (but no Unicode collation is performed for equality test) and locale agnostic.
//! \return 0-based index, or -1 if not found.
inline int findString(const char* szString, const char* arrStringList[])
{
	for (const char** p = arrStringList; *p; ++p)
	{
		if (0 == strcmp(*p, szString))
			return (int)(p - arrStringList);
	}
	return -1; // string was not found
}

// Cuts the string and adds leading ... if it's longer than specified maximum length.
// This function is ASCII-only and locale agnostic.
inline string cutString(const string& strPath, unsigned nMaxLength)
{
	if (strPath.length() > nMaxLength && nMaxLength > 3)
		return string("...") + string(strPath.c_str() + strPath.length() - (nMaxLength - 3));
	else
		return strPath;
}

//! Supports wildcard ? (matches one code-point) and * (matches zero or more code-points).
//! This function is Unicode aware and locale agnostic.
//! \note ANSI input is not supported, ASCII is fine since it's a subset of UTF-8.
//! \return true if the string matches the wildcard.
inline bool MatchWildcard(const char* szString, const char* szWildcard)
{
	return CryStringUtils_Internal::MatchesWildcards_Tpl<CryStringUtils_Internal::SCharComparatorCaseSensitive>(szString, szWildcard);
}

//! Supports wildcard ? (matches one code-point) and * (matches zero or more code-points).
//! This function is Unicode aware and uses the "C" locale for case comparison.
//! \note ANSI input is not supported, ASCII is fine since it's a subset of UTF-8.
//! \return true if the string matches the wildcard.
inline bool MatchWildcardIgnoreCase(const char* szString, const char* szWildcard)
{
	return CryStringUtils_Internal::MatchesWildcards_Tpl<CryStringUtils_Internal::SCharComparatorCaseInsensitive>(szString, szWildcard);
}

#if !defined(RESOURCE_COMPILER)

//! Calculates a hash value for a given string.
inline uint32 CalculateHash(const char* str)
{
	return CCrc32::Compute(str);
}

//! Calculates a hash value for the lower case version of a given string.
inline uint32 CalculateHashLowerCase(const char* str)
{
	return CCrc32::ComputeLowercase(str);
}

//! This function is Unicode and locale agnostic.
inline uint32 HashStringSeed(const char* string, const uint32 seed)
{
	// A string hash taken from the FRD/Crysis2 (game) code with low probability of clashes.
	// Recommend you use the CRY_DEFAULT_HASH_SEED (see HashString).
	const char* p;
	uint32 hash = seed;
	for (p = string; *p != '\0'; p++)
	{
		hash += *p;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

//! This function is ASCII-only and uses the standard "C" locale for case conversion.
inline uint32 HashStringLowerSeed(const char* string, const uint32 seed)
{
	// Computes the hash of 'string' converted to lower case.
	// Also see the comment in HashStringSeed.
	const char* p;
	uint32 hash = seed;
	for (p = string; *p != '\0'; p++)
	{
		hash += toLowerAscii(*p);
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

//! This function is Unicode agnostic and locale agnostic.
inline uint32 HashString(const char* string)
{
	return HashStringSeed(string, CRY_DEFAULT_HASH_SEED);
}

//! This function is ASCII-only and uses the standard "C" locale for case conversion.
inline uint32 HashStringLower(const char* string)
{
	return HashStringLowerSeed(string, CRY_DEFAULT_HASH_SEED);
}
#endif

//! Converts all ASCII characters in a string to lower case - avoids memory allocation.
//! This function is ASCII-only (Unicode remains unchanged) and uses the "C" locale for case conversion (A-Z only).
inline void toLowerInplace(string& str)
{
#ifndef NOT_USE_CRY_STRING
	str.MakeLower();
#else
	std::transform(str.begin(), str.end(), str.begin(), toLowerAscii); // STL MakeLower
#endif
}

//! Converts all characters in a null-terminated string to lower case - avoids memory allocation.
//! This function is ASCII-only (Unicode remains unchanged) and uses the "C" locale for case conversion (A-Z only).
inline void toLowerInplace(char* str)
{
	for (char* s = str; *s != 0; s++)
	{
		*s = toLowerAscii(*s);
	}
}

#ifndef NOT_USE_CRY_STRING

//! Converts a wide string (can be UTF-16 or UTF-32 depending on platform) to UTF-8.
//! This function is Unicode aware and locale agnostic.
template<typename T>
inline void WStrToUTF8(const wchar_t* str, T& dstr)
{
	string utf8;
	Unicode::Convert(utf8, str);

	// Note: This function expects T to have assign(ptr, len) function
	dstr.assign(utf8.c_str(), utf8.length());
}

//! Converts a wide string (can be UTF-16 or UTF-32 depending on platform) to UTF-8.
//! This function is Unicode aware and locale agnostic.
inline string WStrToUTF8(const wchar_t* str)
{
	return Unicode::Convert<string>(str);
}

//! Converts an UTF-8 string to wide string (can be UTF-16 or UTF-32 depending on platform).
//! This function is Unicode aware and locale agnostic.
template<typename T>
inline void UTF8ToWStr(const char* str, T& dstr)
{
	wstring wide;
	Unicode::Convert(wide, str);

	// Note: This function expects T to have assign(ptr, len) function
	dstr.assign(wide.c_str(), wide.length());
}

//! Converts an UTF-8 string to wide string (can be UTF-16 or UTF-32 depending on platform).
//! This function is Unicode aware and locale agnostic.
inline wstring UTF8ToWStr(const char* str)
{
	return Unicode::Convert<wstring>(str);
}

#endif // NOT_USE_CRY_STRING

//! The type used to parse a yes/no string.
enum YesNoType
{
	nYNT_Yes,
	nYNT_No,
	nYNT_Invalid
};

//! Parse the yes/no string.
//! This function only accepts ASCII input, on Unicode content will return nYNT_Invalid.
inline YesNoType toYesNoType(const char* szString)
{
	if (!stricmp(szString, "yes")
	    || !stricmp(szString, "enable")
	    || !stricmp(szString, "true")
	    || !stricmp(szString, "1"))
		return nYNT_Yes;
	if (!stricmp(szString, "no")
	    || !stricmp(szString, "disable")
	    || !stricmp(szString, "false")
	    || !stricmp(szString, "0"))
		return nYNT_No;
	return nYNT_Invalid;
}

//! This function checks if the provided filename is valid.
//! This function only accepts ASCII input, on Unicode content will return false.
inline bool IsValidFileName(const char* fileName)
{
	size_t i = 0;
	for (;; )
	{
		const char c = fileName[i++];
		if (c == 0)
		{
			return true;
		}
		if (!((c >= '0' && c <= '9')
		      || (c >= 'A' && c <= 'Z')
		      || (c >= 'a' && c <= 'z')
		      || c == '.' || c == '-' || c == '_'))
		{
			return false;
		}
	}
}

/**************************************************************************
 *_splitpath() - split a path name into its individual components
 *
 * Purpose:
 *       to split a path name into its individual components
 *
 * Entry:
 *       path  - pointer to path name to be parsed
 *       drive - pointer to buffer for drive component, if any
 *       dir   - pointer to buffer for subdirectory component, if any
 *       fname - pointer to buffer for file base name component, if any
 *       ext   - pointer to buffer for file name extension component, if any
 *
 * Exit:
 *       drive - pointer to drive string.  Includes ':' if a drive was given.
 *       dir   - pointer to subdirectory string. Includes leading and trailing '/' or '\', if any.
 *       fname - pointer to file base name
 *       ext   - pointer to file extension, if any.  Includes leading '.'.
 *
 *******************************************************************************/
ILINE void portable_splitpath(const char* path, char* drive, char* dir, char* fname, char* ext)
{
	char* p;
	char* last_slash = NULL, * dot = NULL;
	unsigned len;
	PREFAST_ASSUME(path);

	/* we assume that the path argument has the following form, where any
	 * or all of the components may be missing.
	 *
	 *  <drive><dir><fname><ext>
	 *
	 * and each of the components has the following expected form(s)
	 *
	 *  drive:
	 *  0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
	 *  ':'
	 *  dir:
	 *  0 to _MAX_DIR-1 characters in the form of an absolute path
	 *  (leading '/' or '\') or relative path, the last of which, if
	 *  any, must be a '/' or '\'.  E.g -
	 *  absolute path:
	 *      \top\next\last\     ; or
	 *      /top/next/last/
	 *  relative path:
	 *      top\next\last\  ; or
	 *      top/next/last/
	 *  Mixed use of '/' and '\' within a path is also tolerated
	 *  fname:
	 *  0 to _MAX_FNAME-1 characters not including the '.' character
	 *  ext:
	 *  0 to _MAX_EXT-1 characters where, if any, the first must be a
	 *  '.'
	 *
	 */

	/* extract drive letter and :, if any */

	if ((strlen(path) >= (_MAX_DRIVE - 2)) && (*(path + _MAX_DRIVE - 2) == (':')))
	{
		if (drive)
		{
			cry_strcpy(drive, _MAX_DRIVE, path);
		}
		path += _MAX_DRIVE - 1;
	}
	else if (drive)
	{
		*drive = ('\0');
	}

	/* extract path string, if any.  Path now points to the first character
	 * of the path, if any, or the filename or extension, if no path was
	 * specified.  Scan ahead for the last occurence, if any, of a '/' or
	 * '\' path separator character.  If none is found, there is no path.
	 * We will also note the last '.' character found, if any, to aid in
	 * handling the extension.
	 */

	for (last_slash = NULL, p = (char*)path; *p; p++)
	{
		if (*p == ('/') || *p == ('\\'))
			/* point to one beyond for later copy */
			last_slash = p + 1;
		else if (*p == ('.'))
			dot = p;
	}

	if (last_slash)
	{

		/* found a path - copy up through last_slash or max. characters
		 * allowed, whichever is smaller
		 */

		if (dir)
		{
			len = (std::min)((unsigned int)(((char*)last_slash - (char*)path) / sizeof(char)), (unsigned int)(_MAX_DIR - 1));
			memcpy(dir, path, len);
			*(dir + len) = ('\0');
		}
		path = last_slash;
	}
	else if (dir)
	{

		/* no path found */

		*dir = ('\0');
	}

	/* extract file name and extension, if any.  Path now points to the
	 * first character of the file name, if any, or the extension if no
	 * file name was given.  Dot points to the '.' beginning the extension,
	 * if any.
	 */

	if (dot && (dot >= path))
	{
		/* found the marker for an extension - copy the file name up to
		 * the '.'.
		 */
		if (fname)
		{
			len = (std::min)((unsigned int)(((char*)dot - (char*)path) / sizeof(char)), (unsigned int)(_MAX_FNAME - 1));
			memcpy(fname, path, len);
			*(fname + len) = ('\0');
		}
		/* now we can get the extension - remember that p still points
		 * to the terminating nul character of path.
		 */
		if (ext)
		{
			len = (std::min)((unsigned int)(((char*)p - (char*)dot) / sizeof(char)), (unsigned int)(_MAX_EXT - 1));
			memcpy(ext, dot, len);
			*(ext + len) = ('\0');
		}
	}
	else
	{
		/* found no extension, give empty extension and copy rest of
		 * string into fname.
		 */
		if (fname)
		{
			len = (std::min)((unsigned int)(((char*)p - (char*)path) / sizeof(char)), (unsigned int)(_MAX_FNAME - 1));
			memcpy(fname, path, len);
			*(fname + len) = ('\0');
		}
		if (ext)
		{
			*ext = ('\0');
		}
	}
}

/**************************************************************************
 * void _makepath() - build path name from components
 *
 * Purpose:
 *       create a path name from its individual components
 *
 * Entry:
 *       char *path  - pointer to buffer for constructed path
 *       char *drive - pointer to drive component, may or may not contain trailing ':'
 *       char *dir   - pointer to subdirectory component, may or may not include leading and/or trailing '/' or '\' characters
 *       char *fname - pointer to file base name component
 *       char *ext   - pointer to extension component, may or may not contain a leading '.'.
 *
 * Exit:
 *       path - pointer to constructed path name
 *
 *******************************************************************************/
ILINE void portable_makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext)
{
	const char* p;

	/* we assume that the arguments are in the following form (although we
	 * do not diagnose invalid arguments or illegal filenames (such as
	 * names longer than 8.3 or with illegal characters in them)
	 *
	 *  drive:
	 *      A           ; or
	 *      A:
	 *  dir:
	 *      \top\next\last\     ; or
	 *      /top/next/last/     ; or
	 *      either of the above forms with either/both the leading
	 *      and trailing / or \ removed.  Mixed use of '/' and '\' is
	 *      also tolerated
	 *  fname:
	 *      any valid file name
	 *  ext:
	 *      any valid extension (none if empty or null )
	 */

	/* copy drive */

	if (drive && *drive)
	{
		*path++ = *drive;
		*path++ = (':');
	}

	/* copy dir */

	if ((p = dir) && *p)
	{
		do
		{
			*path++ = *p++;
		}
		while (*p);
		if (*(p - 1) != '/' && *(p - 1) != ('\\'))
		{
			*path++ = ('\\');
		}
	}

	/* copy fname */

	if (p = fname)
	{
		while (*p)
		{
			*path++ = *p++;
		}
	}

	/* copy ext, including 0-terminator - check to see if a '.' needs
	 * to be inserted.
	 */

	if (p = ext)
	{
		if (*p && *p != ('.'))
		{
			*path++ = ('.');
		}
		while (*path++ = *p++)
			;
	}
	else
	{
		/* better add the 0-terminator */
		*path = ('\0');
	}
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

} // namespace CryStringUtils
