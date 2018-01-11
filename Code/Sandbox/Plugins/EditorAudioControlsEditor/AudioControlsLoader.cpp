// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioControlsLoader.h"

#include "SystemAssetsManager.h"
#include "SystemControlsModel.h"
#include "AudioControlsEditorPlugin.h"

#include <CryString/StringUtils.h>
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryString/CryPath.h>
#include <QtUtil.h>
#include <IEditor.h>
#include <ConfigurationManager.h>

#include <QRegularExpression>

// This file is deprecated and only used for backwards compatibility. It will be removed before March 2019.
namespace ACE
{
string const CAudioControlsLoader::s_controlsLevelsFolder = "levels" CRY_NATIVE_PATH_SEPSTR;
string const CAudioControlsLoader::s_assetsFolderPath = AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "ace" CRY_NATIVE_PATH_SEPSTR;

//////////////////////////////////////////////////////////////////////////
ESystemItemType TagToType_BackwardsComp(string const& tag)
{
	ESystemItemType type;

	if (tag == "ATLSwitch")
	{
		type = ESystemItemType::Switch;
	}
	else if (tag == "ATLSwitchState")
	{
		type = ESystemItemType::State;
	}
	else if (tag == "ATLEnvironment")
	{
		type = ESystemItemType::Environment;
	}
	else if (tag == "ATLRtpc")
	{
		type = ESystemItemType::Parameter;
	}
	else if (tag == "ATLTrigger")
	{
		type = ESystemItemType::Trigger;
	}
	else if (tag == "ATLPreloadRequest")
	{
		type = ESystemItemType::Preload;
	}
	else
	{
		type = ESystemItemType::Invalid;
	}

	return type;
}

//////////////////////////////////////////////////////////////////////////
CAudioControlsLoader::CAudioControlsLoader(CSystemAssetsManager* const pAssetsManager)
	: m_pAssetsManager(pAssetsManager)
	, m_errorCodeMask(EErrorCode::NoError)
	, m_loadOnlyDefaultControls(false)
{}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadAll(bool const loadOnlyDefaultControls /*= false*/)
{
	m_loadOnlyDefaultControls = loadOnlyDefaultControls;
	LoadScopes();
	LoadControls(s_assetsFolderPath);
	LoadControls(m_pAssetsManager->GetConfigFolderPath());
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadControls(string const& folderPath)
{
	// load the global controls
	LoadAllLibrariesInFolder(folderPath, "");

	// load the level specific controls
	_finddata_t fd;
	ICryPak* pCryPak = gEnv->pCryPak;
	intptr_t handle = pCryPak->FindFirst(folderPath + s_controlsLevelsFolder + "*.*", &fd);

	if (handle != -1)
	{
		do
		{
			if (fd.attrib & _A_SUBDIR)
			{
				string name = fd.name;

				if (name != "." && name != "..")
				{
					LoadAllLibrariesInFolder(folderPath, name);

					if (!m_pAssetsManager->ScopeExists(fd.name))
					{
						// if the control doesn't exist it
						// means it is not a real level in the
						// project so it is flagged as LocalOnly
						m_pAssetsManager->AddScope(fd.name, true);
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadAllLibrariesInFolder(string const& folderPath, string const& level)
{
	string path = folderPath;

	if (!level.empty())
	{
		path = path + s_controlsLevelsFolder + level + CRY_NATIVE_PATH_SEPSTR;
	}

	string const searchPath = path + "*.xml";
	ICryPak* const pCryPak = gEnv->pCryPak;
	_finddata_t fd;
	intptr_t const handle = pCryPak->FindFirst(searchPath, &fd);

	if (handle != -1)
	{
		do
		{
			string filename = path + fd.name;
			XmlNodeRef const root = GetISystem()->LoadXmlFromFile(filename);

			if (root != nullptr)
			{
				string const tag = root->getTag();

				if (tag == "ATLConfig")
				{
					m_loadedFilenames.insert(filename.MakeLower());
					string file = fd.name;

					if (root->haveAttr("atl_name"))
					{
						file = root->getAttr("atl_name");
					}

					int atlVersion = 1;
					root->getAttr("atl_version", atlVersion);
					PathUtil::RemoveExtension(file);
					LoadControlsLibrary(root, folderPath, level, file, atlVersion);
				}
			}
			else
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] Failed parsing game sound file %s", filename);
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
CSystemAsset* CAudioControlsLoader::AddUniqueFolderPath(CSystemAsset* pParent, QString const& path)
{
	QStringList folderNames = path.split(QRegularExpression(R"((\\|\/))"), QString::SkipEmptyParts);

	int const size = folderNames.length();

	for (int i = 0; i < size; ++i)
	{
		if (!folderNames[i].isEmpty())
		{
			CSystemAsset* const pChild = m_pAssetsManager->CreateFolder(QtUtil::ToString(folderNames[i]), pParent);

			if (pChild != nullptr)
			{
				pParent = pChild;
			}
		}
	}

	return pParent;
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadControlsLibrary(XmlNodeRef const pRoot, string const& filepath, string const& level, string const& filename, uint32 const version)
{
  // Always create a library file, even if no proper formatting is present.
	CSystemLibrary* const pLibrary = m_pAssetsManager->CreateLibrary(filename);

	if (pLibrary != nullptr)
	{
		int const controlTypeCount = pRoot->getChildCount();

		for (int i = 0; i < controlTypeCount; ++i)
		{
			XmlNodeRef const pNode = pRoot->getChild(i);

			if (pNode != nullptr)
			{
				if (pNode->isTag("EditorData"))
				{
					LoadEditorData(pNode, *pLibrary);
				}
				else
				{
					Scope const scope = level.empty() ? Utils::GetGlobalScope() : m_pAssetsManager->GetScope(level);
					int const numControls = pNode->getChildCount();

					for (int j = 0; j < numControls; ++j)
					{
						if (m_loadOnlyDefaultControls)
						{
							LoadDefaultControl(pNode->getChild(j), scope, version, pLibrary);
						}
						else
						{
							LoadControl(pNode->getChild(j), scope, version, pLibrary);
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CSystemControl* CAudioControlsLoader::LoadControl(XmlNodeRef const pNode, Scope const scope, uint32 const version, CSystemAsset* const pParentItem)
{
	CSystemControl* pControl = nullptr;

	if (pNode != nullptr)
	{
		CSystemAsset* const pFolderItem = AddUniqueFolderPath(pParentItem, QtUtil::ToQString(pNode->getAttr("path")));

		if (pFolderItem != nullptr)
		{
			string const name = pNode->getAttr("atl_name");
			ESystemItemType const controlType = TagToType_BackwardsComp(pNode->getTag());

			if (!((controlType == ESystemItemType::Switch) && ((name == "ObstrOcclCalcType") ||
				(name == "object_velocity_tracking") ||
				(name == "object_doppler_tracking") ||
				(name == CryAudio::s_szAbsoluteVelocityTrackingSwitchName) ||
				(name == CryAudio::s_szRelativeVelocityTrackingSwitchName))) &&
				!((controlType == ESystemItemType::Trigger) && (name == CryAudio::s_szDoNothingTriggerName)))
			{
				pControl = m_pAssetsManager->FindControl(name, controlType);

				if (pControl == nullptr)
				{
					pControl = m_pAssetsManager->CreateControl(name, controlType, pFolderItem);

					if (pControl != nullptr)
					{
						switch (controlType)
						{
						case ESystemItemType::Trigger:
						{
							float radius = 0.0f;
							pNode->getAttr("atl_radius", radius);
							pControl->SetRadius(radius);
							LoadConnections(pNode, pControl);
						}
						break;
						case ESystemItemType::Switch:
						{
							int const stateCount = pNode->getChildCount();

							for (int i = 0; i < stateCount; ++i)
							{
								LoadControl(pNode->getChild(i), scope, version, pControl);
							}
						}
						break;
						case ESystemItemType::Preload:
							LoadPreloadConnections(pNode, pControl, version);
							break;
						default:
							LoadConnections(pNode, pControl);
							break;
						}

						pControl->SetScope(scope);
						pControl->SetModified(true, true);
					}
				}
			}
		}
	}

	return pControl;
}

//////////////////////////////////////////////////////////////////////////
CSystemControl* CAudioControlsLoader::LoadDefaultControl(XmlNodeRef const pNode, Scope const scope, uint32 const version, CSystemAsset* const pParentItem)
{
	CSystemControl* pControl = nullptr;

	if (pNode != nullptr)
	{
		CSystemAsset* const pFolderItem = AddUniqueFolderPath(pParentItem, QtUtil::ToQString(pNode->getAttr("path")));

		if (pFolderItem != nullptr)
		{
			string const name = pNode->getAttr("atl_name");
			ESystemItemType const controlType = TagToType_BackwardsComp(pNode->getTag());

			if ((controlType == ESystemItemType::Trigger) && (m_defaultTriggerNames.find(name) != m_defaultTriggerNames.end()))
			{
				pControl = m_pAssetsManager->FindControl(name, controlType);

				if (pControl == nullptr)
				{
					pControl = m_pAssetsManager->CreateControl(name, controlType, pFolderItem);

					if (pControl != nullptr)
					{
						float radius = 0.0f;
						pNode->getAttr("atl_radius", radius);
						pControl->SetRadius(radius);
						LoadConnections(pNode, pControl);
						pControl->SetModified(true, true);
					}
				}
			}
			else if ((controlType == ESystemItemType::Parameter) && (m_defaultParameterNames.find(name) != m_defaultParameterNames.end()))
			{
				pControl = m_pAssetsManager->FindControl(name, controlType);

				if (pControl == nullptr)
				{
					pControl = m_pAssetsManager->CreateControl(name, controlType, pFolderItem);

					if (pControl != nullptr)
					{
						LoadConnections(pNode, pControl);

						if (pControl->GetName() == "object_speed")
						{
							pControl->SetName(CryAudio::s_szAbsoluteVelocityParameterName);
						}
						else if (pControl->GetName() == "object_doppler")
						{
							pControl->SetName(CryAudio::s_szRelativeVelocityParameterName);
						}

						pControl->SetModified(true, true);
					}
				}
			}
		}
	}

	return pControl;
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadScopes()
{
	LoadScopesImpl(s_controlsLevelsFolder);
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadScopesImpl(string const& sLevelsFolder)
{
	// TODO: consider moving the file enumeration to a background thread to speed up the editor startup time.

	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(sLevelsFolder + CRY_NATIVE_PATH_SEPSTR "*.*", &fd);

	if (handle != -1)
	{
		do
		{
			string name = fd.name;

			if ((name != ".") && (name != "..") && !name.empty())
			{
				if (fd.attrib & _A_SUBDIR)
				{
					LoadScopesImpl(sLevelsFolder + CRY_NATIVE_PATH_SEPSTR + name);
				}
				else
				{
					if (strcmp(PathUtil::GetExt(name), "level") == 0)
					{
						PathUtil::RemoveExtension(name);
						m_pAssetsManager->AddScope(name);
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
std::set<string> CAudioControlsLoader::GetLoadedFilenamesList()
{
	return m_loadedFilenames;
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadConnections(XmlNodeRef const pRoot, CSystemControl* const pControl)
{
	// The radius might change because of the attenuation matching option
	// so we check here to inform the user if their data is outdated.
	float const radius = pControl->GetRadius();

	int const numChildren = pRoot->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pNode = pRoot->getChild(i);
		pControl->LoadConnectionFromXML(pNode);
	}

	if (radius != pControl->GetRadius())
	{
		m_errorCodeMask |= EErrorCode::NonMatchedActivityRadius;
		pControl->SetModified(true, true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadPreloadConnections(XmlNodeRef const pNode, CSystemControl* const pControl, uint32 const version)
{
	string const type = pNode->getAttr("atl_type");

	if (type.compare("AutoLoad") == 0)
	{
		pControl->SetAutoLoad(true);
	}
	else
	{
		pControl->SetAutoLoad(false);
	}

	// Read the connection information for each of the platform groups
	std::vector<dll_string> const& platforms = GetIEditor()->GetConfigurationManager()->GetPlatformNames();
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		// Skip unused data from previous format
		XmlNodeRef const pGroupNode = pNode->getChild(i);
		string const tag = pGroupNode->getTag();

		if ((version == 1) && (tag.compare("ATLConfigGroup") != 0))
		{
			continue;
		}

		// Get the index for that platform name
		int platformIndex = -1;
		string const platformName = pGroupNode->getAttr("atl_name");
		size_t const size = platforms.size();

		for (size_t j = 0; j < size; ++j)
		{
			if (strcmp(platformName.c_str(), platforms[j].c_str()) == 0)
			{
				platformIndex = j;
				break;
			}
		}

		if (platformIndex == -1)
		{
			m_errorCodeMask |= EErrorCode::UnkownPlatform;
			pControl->SetModified(true, true);
		}

		int const numConnections = pGroupNode->getChildCount();

		for (int j = 0; j < numConnections; ++j)
		{
			XmlNodeRef const pConnectionNode = pGroupNode->getChild(j);

			if (pConnectionNode != nullptr)
			{
				pControl->LoadConnectionFromXML(pConnectionNode, platformIndex);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadEditorData(XmlNodeRef const pEditorDataNode, CSystemAsset& library)
{
	int const size = pEditorDataNode->getChildCount();

	for (int i = 0; i < size; ++i)
	{
		XmlNodeRef const pChild = pEditorDataNode->getChild(i);

		if (pChild->isTag("Library"))
		{
			LoadLibraryEditorData(pChild, library);
		}
		else if (pChild->isTag("Folders"))
		{
			LoadAllFolders(pChild, library);
		}
		else if (pChild->isTag("Controls"))
		{
			LoadAllControlsEditorData(pChild);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadLibraryEditorData(XmlNodeRef const pLibraryNode, CSystemAsset& library)
{
	string description = "";
	pLibraryNode->getAttr("description", description);

	if (!description.IsEmpty())
	{
		library.SetDescription(description);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadAllFolders(XmlNodeRef const pFoldersNode, CSystemAsset& library)
{
	if (pFoldersNode != nullptr)
	{
		int const size = pFoldersNode->getChildCount();

		for (int i = 0; i < size; ++i)
		{
			LoadFolderData(pFoldersNode->getChild(i), library);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadFolderData(XmlNodeRef const pFolderNode, CSystemAsset& parentAsset)
{
	CSystemAsset* const pItem = AddUniqueFolderPath(&parentAsset, pFolderNode->getAttr("name"));

	if (pItem != nullptr)
	{
		string description = "";
		pFolderNode->getAttr("description", description);

		if (!description.IsEmpty())
		{
			pItem->SetDescription(description);
		}

		int const size = pFolderNode->getChildCount();

		for (int i = 0; i < size; ++i)
		{
			LoadFolderData(pFolderNode->getChild(i), *pItem);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadAllControlsEditorData(XmlNodeRef const pControlsNode)
{
	if (pControlsNode != nullptr)
	{
		int const size = pControlsNode->getChildCount();

		for (int i = 0; i < size; ++i)
		{
			LoadControlsEditorData(pControlsNode->getChild(i));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioControlsLoader::LoadControlsEditorData(XmlNodeRef const pParentNode)
{
	if (pParentNode != nullptr)
	{
		ESystemItemType const controlType = TagToType_BackwardsComp(pParentNode->getTag());
		string description = "";
		pParentNode->getAttr("description", description);

		if ((controlType != ESystemItemType::Invalid) && !description.IsEmpty())
		{
			 CSystemControl* const pControl = m_pAssetsManager->FindControl(pParentNode->getAttr("name"), controlType);

			if (pControl != nullptr)
			{
				pControl->SetDescription(description);
			}
		}
	}
}
} // namespace ACE
