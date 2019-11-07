// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SKELETON_MANAGER__H__
#define __SKELETON_MANAGER__H__

#include <unordered_map>
#include "SkeletonLoader.h"

struct IPakSystem;
struct ICryXML;
struct IResourceCompiler;

class SkeletonManager
{
public:
	SkeletonManager(IPakSystem* pPakSystem, ICryXML* pXMLParser, IResourceCompiler* pRc);

	bool Initialize(const string& rootPath);
	const CSkeletonInfo* FindSkeletonByAnimFile(const char* filename, bool bRelative = false) const;
	const CSkeletonInfo* FindSkeleton(const char* filename) const;
	const CSkeletonInfo* LoadSkeleton(const char* filename);

private:
	const CSkeletonInfo* LoadSkeletonInfo(const char* filename, bool bErrorsAsWarnings = false);

private:
	IPakSystem* m_pPakSystem;
	ICryXML* m_pXmlParser;

	string m_rootPath;
	string m_tmpPath;

	typedef std::unordered_map<string, SkeletonLoader, stl::hash_stricmp<string>> TFileToSkeletonMap;
	TFileToSkeletonMap m_fileToSkeletonInfo;
};

#endif
