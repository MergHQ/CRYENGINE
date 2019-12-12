// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SkeletonLoader.h"

#include "../../../CryXML/IXMLSerializer.h"
#include "../../../CryXML/ICryXML.h"
#include <CryCore/CryCrc32.h>
#include "TempFilePakExtraction.h"
#include "StringHelpers.h"

//////////////////////////////////////////////////////////////////////////
static XmlNodeRef LoadXml( const char *filename, ICryXML* pXMLParser )
{
	// Get the xml serializer.
	IXMLSerializer* pSerializer = pXMLParser->GetXMLSerializer();

	// Read in the input file.
	XmlNodeRef root;
	{
		const bool bRemoveNonessentialSpacesFromContent = false;
		char szErrorBuffer[1024];
		root = pSerializer->Read(FileXmlBufferSource(filename), bRemoveNonessentialSpacesFromContent, sizeof(szErrorBuffer), szErrorBuffer);
		if (!root)
		{
			//RCLogError("Cannot open XML file '%s': %s\n", filename, szErrorBuffer);
			return 0;
		}
	}
	return root;
}


//////////////////////////////////////////////////////////////////////////
static bool LoadIKSetup(const char* szChrParamsFileName, XmlNodeRef rootNode, CSkinningInfo* pModelSkeleton, ICryXML* pXMLParser)
{
	uint32 numIKTypes = rootNode->getChildCount();
	for (uint32 iktype=0; iktype<numIKTypes; iktype++) 
	{
		XmlNodeRef IKNode = rootNode->getChild(iktype);


		const char* IKTypeTAG = IKNode->getTag();
		//-----------------------------------------------------------
		//check LookIK-Setup  
		//-----------------------------------------------------------
		if (!stricmp(IKTypeTAG,"LookIK_Definition"))
		{

			uint32 numChilds = IKNode->getChildCount();
			for (uint32 c=0; c<numChilds; c++) 
			{
				XmlNodeRef node = IKNode->getChild(c);
				const char* strTag = node->getTag();

				//----------------------------------------------------------------------------
				if (stricmp(strTag,"RotationList")==0) 
				{
					uint32 num = node->getChildCount();
					pModelSkeleton->m_LookIK_Rot.reserve(num);
					for (uint32 i=0; i<num; i++) 
					{
						SJointsAimIK_Rot LookIK_Rot;
						XmlNodeRef nodeRot = node->getChild(i);
						const char* RotTag = nodeRot->getTag();
						if (stricmp(RotTag,"Rotation")==0) 
						{
							const char* pJointName = nodeRot->getAttr( "JointName" );
							int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
							if (jidx>0)
							{
								LookIK_Rot.m_nJointIdx = jidx;
								LookIK_Rot.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
							}
							//		assert(jidx>0);
							nodeRot->getAttr( "Primary",    LookIK_Rot.m_nPreEvaluate );
							nodeRot->getAttr( "Additive",   LookIK_Rot.m_nAdditive );
							//	LookIK_Rot.m_nAdditive=0;
							pModelSkeleton->m_LookIK_Rot.push_back( LookIK_Rot );
						}
					}
				}

				//----------------------------------------------------------------------------------------

				if (stricmp(strTag,"PositionList")==0) 
				{
					uint32 num = node->getChildCount();
					pModelSkeleton->m_LookIK_Pos.reserve(num);
					for (uint32 i=0; i<num; i++) 
					{
						SJointsAimIK_Pos LookIK_Pos;
						XmlNodeRef nodePos = node->getChild(i);
						const char* PosTag = nodePos->getTag();
						if (stricmp(PosTag,"Position")==0) 
						{
							const char* pJointName = nodePos->getAttr( "JointName" );
							int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
							if (jidx>0)
							{
								LookIK_Pos.m_nJointIdx = jidx;
								LookIK_Pos.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
							}
							//	assert(jidx>0);
							pModelSkeleton->m_LookIK_Pos.push_back( LookIK_Pos );
						}
					}
				}

				//----------------------------------------------------------------------------------------

				if (stricmp(strTag,"DirectionalBlends")==0) 
				{
					uint32 num = node->getChildCount();
					pModelSkeleton->m_LookDirBlends.reserve(num);
					for (uint32 i=0; i<num; i++) 
					{
						DirectionalBlends DirBlend;
						XmlNodeRef nodePos = node->getChild(i);
						const char* DirTag = nodePos->getTag();
						if (stricmp(DirTag,"Joint")==0) 
						{
							const char* pAnimToken = nodePos->getAttr( "AnimToken" );
							DirBlend.m_AnimToken = pAnimToken;
							DirBlend.m_AnimTokenCRC32 = CCrc32::ComputeLowercase(pAnimToken);

							const char* pJointName = nodePos->getAttr( "ParameterJoint" );
							int jidx1 = pModelSkeleton->GetJointIDByName(pJointName);
							if (jidx1>0)
							{
								DirBlend.m_nParaJointIdx = jidx1;
								DirBlend.m_strParaJointName = pModelSkeleton->GetJointNameByID(jidx1);
							}
							else
							{
								RCLogWarning("Error while parsing chrparams file '%s': The'ParameterJoint' parameter at line '%d' refers to joint '%s', but the model doesn't seem to have a joint with that name! Lookposes for this character might not work as expected.", szChrParamsFileName, nodePos->getLine(), pJointName);
							}

							const char* pStartJointName = nodePos->getAttr( "StartJoint" );
							int jidx2 = pModelSkeleton->GetJointIDByName(pStartJointName);
							if (jidx2>0)
							{
								DirBlend.m_nStartJointIdx	= jidx2;
								DirBlend.m_strStartJointName = pModelSkeleton->GetJointNameByID(jidx2);
							}
							else
							{
								RCLogWarning("Error while parsing chrparams file '%s': The'StartJoint' parameter at line '%d' refers to joint '%s', but the model doesn't seem to have a joint with that name! Lookposes for this character might not work as expected.", szChrParamsFileName, nodePos->getLine(), pStartJointName);
							}

							pModelSkeleton->m_LookDirBlends.push_back( DirBlend );
						}
					}
				}
			}
		}



		//-----------------------------------------------------------
		//check AimIK-Setup  
		//-----------------------------------------------------------
		if (!stricmp(IKTypeTAG,"AimIK_Definition"))
		{
			uint32 numChilds = IKNode->getChildCount();
			for (uint32 c=0; c<numChilds; c++) 
			{
				XmlNodeRef node = IKNode->getChild(c);
				const char* strTag = node->getTag();

				//----------------------------------------------------------------------------
				if (stricmp(strTag,"RotationList")==0) 
				{
					uint32 num = node->getChildCount();
					pModelSkeleton->m_AimIK_Rot.reserve(num);
					for (uint32 i=0; i<num; i++) 
					{
						SJointsAimIK_Rot AimIK_Rot;
						XmlNodeRef nodeRot = node->getChild(i);
						const char* RotTag = nodeRot->getTag();
						if (stricmp(RotTag,"Rotation")==0) 
						{
							const char* pJointName = nodeRot->getAttr( "JointName" );
							int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
							if (jidx>0)
							{
								AimIK_Rot.m_nJointIdx = jidx;
								AimIK_Rot.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
							}
							//		assert(jidx>0);
							nodeRot->getAttr( "Primary",    AimIK_Rot.m_nPreEvaluate );
							nodeRot->getAttr( "Additive",   AimIK_Rot.m_nAdditive );
							//	AimIK_Rot.m_nAdditive=0;
							pModelSkeleton->m_AimIK_Rot.push_back( AimIK_Rot );
						}
					}
				}

				//----------------------------------------------------------------------------------------

				if (stricmp(strTag,"PositionList")==0) 
				{
					uint32 num = node->getChildCount();
					pModelSkeleton->m_AimIK_Pos.reserve(num);
					for (uint32 i=0; i<num; i++) 
					{
						SJointsAimIK_Pos AimIK_Pos;
						XmlNodeRef nodePos = node->getChild(i);
						const char* PosTag = nodePos->getTag();
						if (stricmp(PosTag,"Position")==0) 
						{
							const char* pJointName = nodePos->getAttr( "JointName" );
							int32 jidx = pModelSkeleton->GetJointIDByName(pJointName);
							if (jidx>0)
							{
								AimIK_Pos.m_nJointIdx = jidx;
								AimIK_Pos.m_strJointName = pModelSkeleton->GetJointNameByID(jidx);
							}
							//	assert(jidx>0);
							pModelSkeleton->m_AimIK_Pos.push_back( AimIK_Pos );
						}
					}
				}

				//----------------------------------------------------------------------------------------

				if (stricmp(strTag,"DirectionalBlends")==0) 
				{
					uint32 num = node->getChildCount();
					pModelSkeleton->m_AimDirBlends.reserve(num);
					for (uint32 i=0; i<num; i++) 
					{
						DirectionalBlends DirBlend;
						XmlNodeRef nodePos = node->getChild(i);
						const char* DirTag = nodePos->getTag();
						if (stricmp(DirTag,"Joint")==0) 
						{
							const char* pAnimToken = nodePos->getAttr( "AnimToken" );
							DirBlend.m_AnimToken = pAnimToken;
							DirBlend.m_AnimTokenCRC32 = CCrc32::ComputeLowercase(pAnimToken);

							const char* pJointName = nodePos->getAttr( "ParameterJoint" );
							int jidx1 = pModelSkeleton->GetJointIDByName(pJointName);
							if (jidx1>0)
							{
								DirBlend.m_nParaJointIdx = jidx1;
								DirBlend.m_strParaJointName = pModelSkeleton->GetJointNameByID(jidx1);
							}
							else
							{
								RCLogWarning("Error while parsing chrparams file '%s': The'ParameterJoint' parameter at line '%d' refers to joint '%s', but the model doesn't seem to have a joint with that name! Aimposes for this character might not work as expected.", szChrParamsFileName, nodePos->getLine(), pJointName);
							}

							const char* pStartJointName = nodePos->getAttr( "StartJoint" );
							int jidx2 = pModelSkeleton->GetJointIDByName(pStartJointName);
							if (jidx2>0)
							{
								DirBlend.m_nStartJointIdx	= jidx2;
								DirBlend.m_strStartJointName = pModelSkeleton->GetJointNameByID(jidx2);
							}
							else
							{
								RCLogWarning("Error while parsing chrparams file '%s': The'StartJoint' parameter at line '%d' refers to joint '%s', but the model doesn't seem to have a joint with that name! Aimposes for this character might not work as expected.", szChrParamsFileName, nodePos->getLine(), pStartJointName);
							}

							pModelSkeleton->m_AimDirBlends.push_back( DirBlend );
						}
					}
				}
			}
		}
		//-----------------------------------------------------------------------
	}
	return true;
}

static bool LoadAnimList(const char* szChrParamsFileName, const char* szAssetRootDir, XmlNodeRef rootNode, CAnimList* pOutAnimList, ICryXML* pXmlParser, const char* strDefaultDirName = nullptr)
{
	constexpr uint32 MAX_STRLEN = 512;

	stack_string strAnimDirName;
	if (strDefaultDirName == nullptr || strDefaultDirName[0] == '\0')
	{
		stack_string fileNameNoExt = szChrParamsFileName;
		fileNameNoExt.replace(".chrparams", "");

		strAnimDirName = PathUtil::GetParentDirectory(fileNameNoExt, 3);
		if (!strAnimDirName.empty())
			strAnimDirName += "/animations";
		else
			strAnimDirName += "animations";
	}
	else
	{
		strAnimDirName = strDefaultDirName;
	}

	if (!rootNode)
		return false;

	bool bSuccess = true;

	uint32 xmlChildrenCount = rootNode->getChildCount();
	for (uint32 i = 0; i < xmlChildrenCount; ++i)
	{
		const XmlNodeRef currentChild = rootNode->getChild(i);

		CryFixedStringT<MAX_STRLEN> animPath;
		CryFixedStringT<MAX_STRLEN> animName;
		CryFixedStringT<MAX_STRLEN> currentPathToken;

		animName.append(currentChild->getAttr("name"));
		animPath.append(currentChild->getAttr("path"));

		int32 tokenizeCharIndex = 0;
		currentPathToken = animPath.Tokenize(" \t\n\r=", tokenizeCharIndex);

		// now only needed for aim poses
		char szLine[MAX_STRLEN];
		cry_strcpy(szLine, animPath.c_str());

		if (currentPathToken.empty() || currentPathToken.at(0) == '?')
		{
			continue;
		}

		if (0 == stricmp(animName, "#filepath"))
		{
			strAnimDirName = PathUtil::ToUnixPath(animPath.c_str());
			strAnimDirName.TrimRight('/'); // delete the trailing slashes
			continue;
		}

		if (0 == stricmp(animName, "#ParseSubFolders"))
		{
			RCLogWarning("Error while parsing chrparams file '%s': Ignoring deprecated #ParseSubFolders directive", szChrParamsFileName);
			continue;
		}

		// remove first '\' and replace '\' with '/'
		currentPathToken.replace('\\', '/');
		currentPathToken.TrimLeft('/');

		// process the possible directives
		if (animName.at(0) == '$')
		{
			if (!stricmp(animName, "$AnimationDir") || !stricmp(animName, "$AnimDir") || !stricmp(animName, "$AnimationDirectory") || !stricmp(animName, "$AnimDirectory"))
			{
				RCLogWarning("Error while parsing chrparams file '%s': Deprecated directive \"%s\"", szChrParamsFileName, animName.c_str());
			}
			else if (!stricmp(animName, "$AnimEventDatabase") || !stricmp(animName, "$TracksDatabase") || !stricmp(animName, "$FaceLib"))
			{
				// Ignoring anim event database, tracks database and FaceLib for RC purposes
			}
			else if (!stricmp(animName, "$Include"))
			{
				// load the new params file, but only parse the AnimationList section
				string fullPath = PathUtil::Make(szAssetRootDir, currentPathToken);
				XmlNodeRef topRoot = LoadXml(fullPath.c_str(), pXmlParser);
				if (topRoot)
				{
					const char* nodeTag = topRoot->getTag();
					if (stricmp(nodeTag, "Params") == 0)
					{
						int32 numChildren = topRoot->getChildCount();
						for (int32 iChild = 0; iChild < numChildren; ++iChild)
						{
							XmlNodeRef node = topRoot->getChild(iChild);

							const char* newNodeTag = node->getTag();
							if (stricmp(newNodeTag, "AnimationList") == 0)
							{
								bSuccess &= LoadAnimList(szChrParamsFileName, szAssetRootDir, node, pOutAnimList, pXmlParser, strAnimDirName);
							}
						}
					}
				}
			}
			else
			{
				RCLogWarning("Error while parsing chrparams file '%s': Unknown directive in '%s'", szChrParamsFileName, animName.c_str());
			}
		}
		else
		{
			// Check whether the filename is a facial animation, by checking the extension.
			const char* szExtension = PathUtil::GetExt(currentPathToken);
			stack_string fileName;
			fileName.Format("%s/%s", strAnimDirName.c_str(), currentPathToken.c_str());

			// is there any wildcard in the file name?
			if (strchr(currentPathToken, '*') != NULL || strchr(currentPathToken, '?') != NULL)
			{
				pOutAnimList->AddWildcardFile(fileName);
			}
			else
			{
				if (szExtension != nullptr && stricmp("fsq", szExtension) == 0)
				{
					// Ignoring facial animation for RC purposes
				}
				else
				{
					pOutAnimList->AddFile(fileName);
				}
			}
		}
	}
	return bSuccess;
}

static bool LoadChrParams(const char* szFileNameXML, const char* szAssetRootDir, CAnimList* pOutAnimList, CSkinningInfo* pModelSkeleton, ICryXML* pXMLParser)
{
	XmlNodeRef TopRoot = LoadXml(szFileNameXML, pXMLParser);
	if (TopRoot == 0)
	{
		return false;
	}

	bool bSuccess = true;
	uint32 numChildren = TopRoot->getChildCount();
	for (uint32 i = 0; i < numChildren; i++)
	{
		XmlNodeRef currentChildNode = TopRoot->getChild(i);
		const char* currentXmlTag = currentChildNode->getTag();
		if (stricmp(currentXmlTag, "IK_Definition") == 0)
		{
			bSuccess &= LoadIKSetup(szFileNameXML, currentChildNode, pModelSkeleton, pXMLParser);
		}
		if (stricmp(currentXmlTag, "AnimationList") == 0)
		{
			bSuccess &= LoadAnimList(szFileNameXML, szAssetRootDir, currentChildNode, pOutAnimList, pXMLParser);
		}
	}
	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
SkeletonLoader::SkeletonLoader()
	: m_bLoaded(false)
{
}


//////////////////////////////////////////////////////////////////////////
CSkeletonInfo* SkeletonLoader::Load(const char* szFilename, const char* szAssetRootDir, IPakSystem* pakSystem, ICryXML* xml, const string& tempPath)
{
	std::auto_ptr<TempFilePakExtraction> fileProxyCHR(new TempFilePakExtraction(szFilename, tempPath.c_str(), pakSystem ));
	m_tempFileName = fileProxyCHR->GetTempName();

	const bool bChr = StringHelpers::EndsWithIgnoreCase(szFilename, "chr");

	stack_string filenameChrParams = szFilename;
	filenameChrParams.replace(".chr",".chrparams");
	std::auto_ptr<TempFilePakExtraction> fileProxyChrParams(new TempFilePakExtraction( filenameChrParams.c_str(), tempPath.c_str(), pakSystem ));

	if (bChr && m_skeletonInfo.LoadFromChr(m_tempFileName.c_str()))
	{
		LoadChrParams(fileProxyChrParams->GetTempName().c_str(), szAssetRootDir, &m_skeletonInfo.m_animList, &m_skeletonInfo.m_SkinningInfo, xml);
		m_bLoaded = true;
		return &m_skeletonInfo;
	}
	else if (!bChr && m_skeletonInfo.LoadFromCga(m_tempFileName.c_str()))
	{
		m_bLoaded = true;
		return &m_skeletonInfo;
	}
	else
	{
		m_bLoaded = false;
		return 0;
	}
}
