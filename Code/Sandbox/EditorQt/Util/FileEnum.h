// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CFileEnum
{
public:
	CFileEnum();
	virtual ~CFileEnum();
	bool        GetNextFile(struct __finddata64_t* pFile);
	bool        StartEnumeration(const char* szEnumPathAndPattern, __finddata64_t* pFile);
	bool        StartEnumeration(const char* szEnumPath, const char* szEnumPattern, __finddata64_t* pFile);
	static bool ScanDirectory(
	  const string& path,
	  const string& file,
	  std::vector<string>& files,
	  bool bRecursive = true,
	  bool bSkipPaks = false);

protected:
	intptr_t m_hEnumFile;
};
