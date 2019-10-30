// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SkeletonManager.h"

#include "../../../CryXML/IXMLSerializer.h"
#include "../../../CryXML/ICryXML.h"
#include "FileUtil.h"
#include "PathHelpers.h"
#include "PakSystem.h"
#include "PakXmlFileBufferSource.h"
#include "StringHelpers.h"


//////////////////////////////////////////////////////////////////////////
SkeletonManager::SkeletonManager(IPakSystem* pPakSystem, ICryXML* pXmlParser, IResourceCompiler* pRc)
: m_pPakSystem(pPakSystem)
, m_pXmlParser(pXmlParser)
{
	assert(m_pPakSystem);
	assert(m_pXmlParser);

	assert(pRc != 0);
	m_tmpPath = pRc->GetTmpPath();
}


namespace SkelMgr_Internal
{
static string StandardizePath(const char* path)
{
	const string absolutePath = PathUtil::ToUnixPath(PathHelpers::IsRelative(path) ? PathHelpers::GetAbsolutePath(path) : path);
	
	const size_t bufferLength = strlen(absolutePath.c_str()) + 1;
	STACK_ARRAY(char, szSimplifiedPath, bufferLength);

	// We simplify the file path to remove any ., .. or other redundant path operators
	bool bSuccess = PathUtil::SimplifyFilePath(absolutePath.c_str(), szSimplifiedPath, bufferLength, PathUtil::ePathStyle_Posix);
	if (!bSuccess)
	{
		RCLogError("Error while simplifying path '%s'", absolutePath.c_str());
	}

	return string{ szSimplifiedPath };
}
}


//////////////////////////////////////////////////////////////////////////
bool SkeletonManager::Initialize(const string& rootPath)
{
	m_rootPath = PathUtil::AddSlash(SkelMgr_Internal::StandardizePath(rootPath));

	std::vector<string> foundSkeletons;
	FileUtil::ScanDirectory(m_rootPath, "*.chr", foundSkeletons, true, "");
	
	RCLog("Starting preloading of skeletons.");
	for (const string& skeletonFilename : foundSkeletons)
	{
		RCLog("Loading skeleton '%s'", skeletonFilename.c_str());
		LoadSkeletonInfo(skeletonFilename.c_str(), true);
	} 

	RCLog("Finished preloading of skeletons.");
	return true;
}


const CSkeletonInfo* SkeletonManager::FindSkeletonByAnimFile(const char* szAnimationFile, bool bRelative) const
{
	string relativeAnimFilePath;
	if (!bRelative)
	{
		string fullAnimFilePath = SkelMgr_Internal::StandardizePath(szAnimationFile);
		int rootLength = m_rootPath.length();
		if (0 == strcmpi(m_rootPath, fullAnimFilePath.substr(0, rootLength)))
		{
			relativeAnimFilePath = fullAnimFilePath.substr(rootLength, fullAnimFilePath.length() - rootLength);
		}
		else
		{
			RCLogError("Animation file '%s' not part of root directory '%s'!", fullAnimFilePath.c_str(), m_rootPath.c_str());
		}
	}
	else
	{
		relativeAnimFilePath = PathUtil::ToUnixPath(szAnimationFile);
	}	

	RCLog("Looking for skeleton of animation file '%s'", relativeAnimFilePath.c_str());
	for (TFileToSkeletonMap::const_reference el : m_fileToSkeletonInfo)
	{
		if (el.second.Skeleton().m_animList.MatchFile(relativeAnimFilePath.c_str()))
		{
			return &el.second.Skeleton();
		}
	}
	RCLogError("Skeleton with animation file '%s' in its Animation List could not be found. Check that the skeleton's .chrparams file correctly references this file.", relativeAnimFilePath.c_str());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
const CSkeletonInfo* SkeletonManager::LoadSkeletonInfo(const char* szFilename, bool bErrorsAsWarnings)
{
	if (szFilename == nullptr || szFilename[0] == '\0')
	{
		RCLogWarning("Passed a null or empty string to the LoadSkeletonInfo method.");
		return nullptr;
	}

	const bool bIsAlreadyLoaded = (m_fileToSkeletonInfo.find(szFilename) != m_fileToSkeletonInfo.end());
	if (bIsAlreadyLoaded)
	{
		assert(false);
		return nullptr;
	}

	stack_string fullFile = PathUtil::Make(m_rootPath, szFilename);
	RCLog("Loading skeleton '%s'.", fullFile.c_str());

	SkeletonLoader& skeletonInfo = m_fileToSkeletonInfo[szFilename];
	skeletonInfo.Load(fullFile.c_str(), m_rootPath.c_str(), m_pPakSystem, m_pXmlParser, m_tmpPath);
	const bool bLoadSuccess = skeletonInfo.IsLoaded();
	if (!bLoadSuccess)
	{
		(bErrorsAsWarnings ? RCLogWarning : RCLogError)("Failed to load skeleton '%s'.", fullFile.c_str());
		return nullptr;
	}

	return &skeletonInfo.Skeleton();
}

//////////////////////////////////////////////////////////////////////////
const CSkeletonInfo* SkeletonManager::FindSkeleton(const char* szFilename) const
{
	if (szFilename == nullptr || szFilename[0] == '\0')
	{
		RCLogWarning("Passed a null or empty string to the FindSkeleton method.");
		return nullptr;
	}

	TFileToSkeletonMap::const_iterator cit = m_fileToSkeletonInfo.find(szFilename);
	if (cit != m_fileToSkeletonInfo.end())
	{
		const SkeletonLoader& skeletonLoader = cit->second;
		const bool bIsLoaded = skeletonLoader.IsLoaded();
		if (!bIsLoaded)
		{
			return nullptr;
		}

		return &skeletonLoader.Skeleton();
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
const CSkeletonInfo* SkeletonManager::LoadSkeleton(const char* szFilename)
{
	if (szFilename == nullptr || szFilename[0] == '\0')
	{
		RCLogWarning("Passed a null or empty string to the LoadSkeleton method.");
		return nullptr;
	}

	const CSkeletonInfo* skeleton = FindSkeleton(szFilename);
	if (skeleton)
	{
		return skeleton;
	}

	return LoadSkeletonInfo(szFilename);
}
