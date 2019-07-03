#pragma once

#include <unordered_set>
#include <CryCore/StlUtils.h>
#include "../../../Sandbox/Plugins/EditorCommon/PathUtils.h"

class CAnimList
{
public:
	void AddWildcardFile(const char* szFilePattern);
	void AddFile(const char* szFile);
	bool MatchFile(const char* filename) const;
private:
	typedef std::unordered_set<string, stl::hash_stricmp<string>> TFilePatternSet;
	TFilePatternSet m_plainFilesSet;
	TFilePatternSet m_wildcardFilesSet;
};


inline void CAnimList::AddWildcardFile(const char* szFilePattern)
{
	if (szFilePattern && szFilePattern[0])
	{
		stack_string filePatternLowercase = szFilePattern;
		filePatternLowercase.MakeLower();
		m_wildcardFilesSet.emplace(filePatternLowercase);
	}
}

inline void CAnimList::AddFile(const char* szFile)
{
	if (szFile && szFile[0])
	{
		stack_string fileLowercase = szFile;
		fileLowercase.MakeLower();
		m_plainFilesSet.emplace(fileLowercase);
	}
}

inline static bool MatchAnimListWildcard(const char* szFilePath, const char* szWildcard)
{
	const char* szFileWithoutPath = PathUtil::GetFile(szFilePath);
	const char* szWildcardPatternWithoutPath = PathUtil::GetFile(szWildcard);
	if (!PathUtil::MatchWildcard(szFileWithoutPath, szWildcardPatternWithoutPath))
	{
		return false;
	}
	stack_string fileDirectory(szFilePath, szFileWithoutPath);
	stack_string wildcardDirectory(szWildcard, szWildcardPatternWithoutPath);

	size_t starPosition = wildcardDirectory.find('*');
	if (starPosition != stack_string::npos)
	{
		// in case there is a wildcard, we compare the two root directory paths
		// note: any subfolder of the root path is fine.
		// note2: any character after the directory wildcard is ignored.

		if (starPosition > fileDirectory.length())
		{
			return false;
		}

		stack_string fileRootPath = fileDirectory.substr(0, starPosition);
		stack_string wildcardRootPath = wildcardDirectory.substr(0, starPosition);

		return fileRootPath == wildcardRootPath;
	}
	else
	{
		// in case there's no wildcard we just directly compare the two directories
		if (fileDirectory == wildcardDirectory)
		{
			return true;
		}
	}
	return false;
}

inline bool CAnimList::MatchFile(const char* szFilePath) const
{
	stack_string filePathLowercase = szFilePath;
	filePathLowercase.MakeLower();

	if (filePathLowercase.length() == 0)
	{
		return false;
	}

	// If the file is an i_caf file we are actually looking at a .caf directive in the AnimList
	if (!stricmp(PathUtil::GetExt(filePathLowercase), "i_caf"))
	{
		// Requested file is .i_caf, in this case the animation list will include .caf directives as it's looking for the compiled files."
		filePathLowercase = PathUtil::ReplaceExtension(filePathLowercase, "caf");
	}

	if (m_plainFilesSet.find(filePathLowercase) != m_plainFilesSet.end())
		return true;

	for (TFilePatternSet::const_reference el : m_wildcardFilesSet)
	{
		stack_string wildcardPattern = el.c_str();
		wildcardPattern.MakeLower();

		if (MatchAnimListWildcard(filePathLowercase, wildcardPattern))
		{
			return true;
		}
	}

	return false;
}
