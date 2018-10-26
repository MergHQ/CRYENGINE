// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FileWriter.h"

#include "Common.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "Common/IConnection.h"
#include "Common/IItem.h"

#include <CryString/StringUtils.h>
#include <CrySystem/File/CryFile.h>

namespace ACE
{
uint32 const CFileWriter::s_currentFileVersion = 3;

//////////////////////////////////////////////////////////////////////////
char const* TypeToTag(EAssetType const assetType)
{
	char const* szTag = nullptr;

	switch (assetType)
	{
	case EAssetType::Parameter:
		szTag = CryAudio::s_szParameterTag;
		break;
	case EAssetType::Trigger:
		szTag = CryAudio::s_szTriggerTag;
		break;
	case EAssetType::Switch:
		szTag = CryAudio::s_szSwitchTag;
		break;
	case EAssetType::State:
		szTag = CryAudio::s_szStateTag;
		break;
	case EAssetType::Preload:
		szTag = CryAudio::s_szPreloadRequestTag;
		break;
	case EAssetType::Environment:
		szTag = CryAudio::s_szEnvironmentTag;
		break;
	case EAssetType::Setting:
		szTag = CryAudio::s_szSettingTag;
		break;
	default:
		szTag = nullptr;
		break;
	}

	return szTag;
}

//////////////////////////////////////////////////////////////////////////
void CountControls(EAssetType const type, SLibraryScope& scope)
{
	switch (type)
	{
	case EAssetType::Trigger:
		{
			++scope.numTriggers;
			break;
		}
	case EAssetType::Parameter:
		{
			++scope.numParameters;
			break;
		}
	case EAssetType::Switch:
		{
			++scope.numSwitches;
			break;
		}
	case EAssetType::State:
		{
			++scope.numStates;
			break;
		}
	case EAssetType::Environment:
		{
			++scope.numEnvironments;
			break;
		}
	case EAssetType::Preload:
		{
			++scope.numPreloads;
			break;
		}
	case EAssetType::Setting:
		{
			++scope.numSettings;
			break;
		}
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CountConnections(EAssetType const type, SLibraryScope& scope)
{
	switch (type)
	{
	case EAssetType::Trigger:
		{
			++scope.numTriggerConnections;
			break;
		}
	case EAssetType::Parameter:
		{
			++scope.numParameterConnections;
			break;
		}
	case EAssetType::State:
		{
			++scope.numStateConnections;
			break;
		}
	case EAssetType::Environment:
		{
			++scope.numEnvironmentConnections;
			break;
		}
	case EAssetType::Preload:
		{
			++scope.numPreloadConnections;
			break;
		}
	case EAssetType::Setting:
		{
			++scope.numSettingConnections;
			break;
		}
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteAll()
{
	size_t const libCount = g_assetsManager.GetLibraryCount();

	for (size_t i = 0; i < libCount; ++i)
	{
		CLibrary& library = *g_assetsManager.GetLibrary(i);
		WriteLibrary(library);
		library.SetModified(false);
	}

	// Delete libraries that don't exist anymore from disk
	FileNames librariesToDelete;
	std::set_difference(m_previousLibraryPaths.begin(), m_previousLibraryPaths.end(), m_foundLibraryPaths.begin(), m_foundLibraryPaths.end(),
	                    std::inserter(librariesToDelete, librariesToDelete.begin()));

	for (auto const& name : librariesToDelete)
	{
		string const fullFilePath = PathUtil::GetGameFolder() + "/" + name;
		DeleteLibraryFile(fullFilePath);
	}

	m_previousLibraryPaths = m_foundLibraryPaths;
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteLibrary(CLibrary& library)
{
	if ((library.GetFlags() & EAssetFlags::IsModified) != 0)
	{
		LibraryStorage libraryXmlNodes;
		size_t const itemCount = library.ChildCount();

		if ((library.GetFlags() & EAssetFlags::IsDefaultControl) != 0)
		{
			for (size_t i = 0; i < itemCount; ++i)
			{
				CAsset* const pAsset = library.GetChild(i);

				if ((pAsset != nullptr) && ((pAsset->GetFlags() & EAssetFlags::IsInternalControl) == 0))
				{
					WriteItem(pAsset, "", libraryXmlNodes);
				}
			}
		}
		else
		{
			for (size_t i = 0; i < itemCount; ++i)
			{
				WriteItem(library.GetChild(i), "", libraryXmlNodes);
			}
		}

		// If empty, force it to write an empty library at the root
		if (libraryXmlNodes.empty())
		{
			libraryXmlNodes[GlobalScopeId].isDirty = true;
		}

		for (auto const& libraryPair : libraryXmlNodes)
		{
			string libraryPath = g_assetsManager.GetConfigFolderPath();
			Scope const scope = libraryPair.first;

			if (scope == GlobalScopeId)
			{
				// no scope, file at the root level
				libraryPath += library.GetName();
			}
			else
			{
				// with scope, inside level folder
				libraryPath += CryAudio::s_szLevelsFolderName;
				libraryPath += "/" + g_assetsManager.GetScopeInfo(scope).name + "/" + library.GetName();
			}

			m_foundLibraryPaths.insert(libraryPath.MakeLower() + ".xml");

			SLibraryScope const& libScope = libraryPair.second;

			if (libScope.isDirty)
			{
				XmlNodeRef pFileNode = GetISystem()->CreateXmlNode(CryAudio::s_szRootNodeTag);
				pFileNode->setAttr(CryAudio::s_szNameAttribute, library.GetName());
				pFileNode->setAttr(CryAudio::s_szVersionAttribute, s_currentFileVersion);

				// Don't write control counts for default library, because default controls are not pooled.
				// But connections of default controls need to get written.
				if ((library.GetFlags() & EAssetFlags::IsDefaultControl) == 0)
				{
					if (libScope.numTriggers > 0)
					{
						pFileNode->setAttr(CryAudio::s_szNumTriggersAttribute, libScope.numTriggers);
					}

					if (libScope.numParameters > 0)
					{
						pFileNode->setAttr(CryAudio::s_szNumParametersAttribute, libScope.numParameters);
					}

					if (libScope.numSwitches > 0)
					{
						pFileNode->setAttr(CryAudio::s_szNumSwitchesAttribute, libScope.numSwitches);
					}

					if (libScope.numStates > 0)
					{
						pFileNode->setAttr(CryAudio::s_szNumStatesAttribute, libScope.numStates);
					}

					if (libScope.numEnvironments > 0)
					{
						pFileNode->setAttr(CryAudio::s_szNumEnvironmentsAttribute, libScope.numEnvironments);
					}

					if (libScope.numPreloads > 0)
					{
						pFileNode->setAttr(CryAudio::s_szNumPreloadsAttribute, libScope.numPreloads);
					}

					if (libScope.numSettings > 0)
					{
						pFileNode->setAttr(CryAudio::s_szNumSettingsAttribute, libScope.numSettings);
					}
				}

				if (libScope.numTriggerConnections > 0)
				{
					pFileNode->setAttr(CryAudio::s_szNumTriggerConnectionsAttribute, libScope.numTriggerConnections);
				}

				if (libScope.numParameterConnections > 0)
				{
					pFileNode->setAttr(CryAudio::s_szNumParameterConnectionsAttribute, libScope.numParameterConnections);
				}

				if (libScope.numStateConnections > 0)
				{
					pFileNode->setAttr(CryAudio::s_szNumStateConnectionsAttribute, libScope.numStateConnections);
				}

				if (libScope.numEnvironmentConnections > 0)
				{
					pFileNode->setAttr(CryAudio::s_szNumEnvironmentConnectionsAttribute, libScope.numEnvironmentConnections);
				}

				if (libScope.numPreloadConnections > 0)
				{
					pFileNode->setAttr(CryAudio::s_szNumPreloadConnectionsAttribute, libScope.numPreloadConnections);
				}

				if (libScope.numSettingConnections > 0)
				{
					pFileNode->setAttr(CryAudio::s_szNumSettingConnectionsAttribute, libScope.numSettingConnections);
				}

				XmlNodeRef const pImplDataNode = g_pIImpl->SetDataNode(CryAudio::s_szImplDataNodeTag);

				if (pImplDataNode != nullptr)
				{
					pFileNode->addChild(pImplDataNode);
				}

				auto const numTypes = static_cast<int>(EAssetType::NumTypes);

				for (int i = 0; i < numTypes; ++i)
				{
					if (i != static_cast<int>(EAssetType::State))   // switch_states are written inside the switches
					{
						XmlNodeRef node = libScope.GetXmlNode((EAssetType)i);

						if ((node != nullptr) && (node->getChildCount() > 0))
						{
							pFileNode->addChild(node);
						}
					}
				}

				// Editor data
				XmlNodeRef const pEditorData = pFileNode->createNode(CryAudio::s_szEditorDataTag);

				if (pEditorData != nullptr)
				{
					XmlNodeRef const pLibraryNode = pEditorData->createNode(s_szLibraryNodeTag);

					if (pLibraryNode != nullptr)
					{
						WriteLibraryEditorData(library, pLibraryNode);
						pEditorData->addChild(pLibraryNode);
					}

					XmlNodeRef const pFoldersNode = pEditorData->createNode(s_szFoldersNodeTag);

					if (pFoldersNode != nullptr)
					{
						WriteFolderEditorData(library, pFoldersNode);
						pEditorData->addChild(pFoldersNode);
					}

					XmlNodeRef const pControlsNode = pEditorData->createNode(s_szControlsNodeTag);

					if (pControlsNode != nullptr)
					{
						WriteControlsEditorData(library, pControlsNode);
						pEditorData->addChild(pControlsNode);
					}

					pFileNode->addChild(pEditorData);
				}

				string const fullFilePath = PathUtil::GetGameFolder() + "/" + libraryPath + ".xml";
				DWORD const fileAttributes = GetFileAttributesA(fullFilePath.c_str());

				if ((fileAttributes & FILE_ATTRIBUTE_READONLY) != 0)
				{
					// file is read-only
					SetFileAttributesA(fullFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
				}

				// TODO: Check out in source control.
				pFileNode->saveToFile(fullFilePath);

				// Update pak status, because it will exist on disk, if writing doesn't fail.
				library.SetPakStatus(EPakStatus::OnDisk, gEnv->pCryPak->IsFileExist(fullFilePath.c_str(), ICryPak::eFileLocation_OnDisk));
			}
		}
	}
	else
	{
		std::unordered_set<Scope> scopes;
		size_t const numChildren = library.ChildCount();

		for (size_t i = 0; i < numChildren; ++i)
		{
			CAsset* const pAsset = library.GetChild(i);
			GetScopes(pAsset, scopes);
		}

		for (auto const scope : scopes)
		{
			string libraryPath = g_assetsManager.GetConfigFolderPath();

			if (scope == GlobalScopeId)
			{
				// no scope, file at the root level
				libraryPath += library.GetName();
			}
			else
			{
				// with scope, inside level folder
				libraryPath += CryAudio::s_szLevelsFolderName;
				libraryPath += "/" + g_assetsManager.GetScopeInfo(scope).name + "/" + library.GetName();
			}

			m_foundLibraryPaths.insert(libraryPath.MakeLower() + ".xml");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteItem(CAsset* const pAsset, string const& path, LibraryStorage& library)
{
	if (pAsset != nullptr)
	{
		if (pAsset->GetType() == EAssetType::Folder)
		{
			size_t const itemCount = pAsset->ChildCount();

			for (size_t i = 0; i < itemCount; ++i)
			{
				// Use forward slash only to ensure cross platform compatibility.
				string newPath = path.empty() ? "" : path + "/";
				newPath += pAsset->GetName();
				WriteItem(pAsset->GetChild(i), newPath, library);
			}

			pAsset->SetModified(false);
		}
		else
		{
			auto const pControl = static_cast<CControl*>(pAsset);

			if (pControl != nullptr)
			{
				SLibraryScope& scope = library[pControl->GetScope()];
				scope.isDirty = true;
				WriteControlToXML(scope.GetXmlNode(pControl->GetType()), pControl, path, scope);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::GetScopes(CAsset const* const pAsset, std::unordered_set<Scope>& scopes)
{
	if (pAsset->GetType() == EAssetType::Folder)
	{
		size_t const numChildren = pAsset->ChildCount();

		for (size_t i = 0; i < numChildren; ++i)
		{
			GetScopes(pAsset->GetChild(i), scopes);
		}
	}
	else
	{
		auto const pControl = static_cast<CControl const*>(pAsset);

		if (pControl != nullptr)
		{
			scopes.insert(pControl->GetScope());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteControlToXML(XmlNodeRef const pNode, CControl* const pControl, string const& path, SLibraryScope& scope)
{
	EAssetType const type = pControl->GetType();
	XmlNodeRef const pChildNode = pNode->createNode(TypeToTag(type));
	pChildNode->setAttr(CryAudio::s_szNameAttribute, pControl->GetName());
	CountControls(type, scope);

	if (!path.empty())
	{
		pChildNode->setAttr(s_szPathAttribute, path);
	}

	if (type == EAssetType::Switch)
	{
		size_t const numChildren = pControl->ChildCount();

		for (size_t i = 0; i < numChildren; ++i)
		{
			CAsset* const pAsset = pControl->GetChild(i);

			if ((pAsset != nullptr) && (pAsset->GetType() == EAssetType::State))
			{
				WriteControlToXML(pChildNode, static_cast<CControl*>(pAsset), "", scope);
			}
		}
	}
	else if ((type == EAssetType::Preload) || (type == EAssetType::Setting))
	{
		if (pControl->IsAutoLoad())
		{
			pChildNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::s_szDataLoadType);
		}

		size_t const numPlatforms = g_platforms.size();

		for (size_t i = 0; i < numPlatforms; ++i)
		{
			XmlNodeRef const pFileNode = pChildNode->createNode(CryAudio::s_szPlatformTag);
			pFileNode->setAttr(CryAudio::s_szNameAttribute, g_platforms[i]);
			WriteConnectionsToXML(pFileNode, pControl, scope, i);

			if (pFileNode->getChildCount() > 0)
			{
				pChildNode->addChild(pFileNode);
			}
		}
	}
	else
	{
		WriteConnectionsToXML(pChildNode, pControl, scope);
	}

	pControl->SetModified(false);
	pNode->addChild(pChildNode);
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteConnectionsToXML(XmlNodeRef const pNode, CControl* const pControl, SLibraryScope& scope, int const platformIndex /*= -1*/)
{
	EAssetType const type = pControl->GetType();
	size_t const numConnections = pControl->GetConnectionCount();

	for (size_t i = 0; i < numConnections; ++i)
	{
		IConnection const* const pIConnection = pControl->GetConnectionAt(i);

		if (pIConnection != nullptr)
		{
			if (((type != EAssetType::Preload) && (type != EAssetType::Setting)) || (pIConnection->IsPlatformEnabled(static_cast<PlatformIndexType>(platformIndex))))
			{
				XmlNodeRef const pChild = g_pIImpl->CreateXMLNodeFromConnection(pIConnection, type);

				if (pChild != nullptr)
				{
					// Don't add identical nodes!
					bool shouldAddNode = true;
					int const numNodeChilds = pNode->getChildCount();

					for (int j = 0; j < numNodeChilds; ++j)
					{
						XmlNodeRef const pTempNode = pNode->getChild(j);

						if ((pTempNode != nullptr) && (_stricmp(pTempNode->getTag(), pChild->getTag()) == 0))
						{
							int const numAttributes1 = pTempNode->getNumAttributes();
							int const numAttributes2 = pChild->getNumAttributes();

							if (numAttributes1 == numAttributes2)
							{
								shouldAddNode = false;
								char const* key1 = nullptr;
								char const* val1 = nullptr;
								char const* key2 = nullptr;
								char const* val2 = nullptr;

								for (int k = 0; k < numAttributes1; ++k)
								{
									pTempNode->getAttributeByIndex(k, &key1, &val1);
									pChild->getAttributeByIndex(k, &key2, &val2);

									if ((_stricmp(key1, key2) != 0) || (_stricmp(val1, val2) != 0))
									{
										shouldAddNode = true;
										break;
									}
								}

								if (!shouldAddNode)
								{
									break;
								}
							}
						}
					}

					if (shouldAddNode)
					{
						pNode->addChild(pChild);
						CountConnections(type, scope);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::DeleteLibraryFile(string const& filepath)
{
	// TODO: Mark for delete in source control.
	DWORD const fileAttributes = GetFileAttributesA(filepath.c_str());

	if ((fileAttributes & FILE_ATTRIBUTE_READONLY) != 0)
	{
		// file is read-only
		SetFileAttributesA(filepath.c_str(), FILE_ATTRIBUTE_NORMAL);
	}

	if ((fileAttributes == INVALID_FILE_ATTRIBUTES) || !DeleteFile(filepath.c_str()))
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] Failed to delete file %s", filepath);
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteLibraryEditorData(CAsset const& library, XmlNodeRef const pParentNode) const
{
	string const& description = library.GetDescription();

	if (!description.IsEmpty() && (library.GetFlags() & EAssetFlags::IsDefaultControl) == 0)
	{
		pParentNode->setAttr(s_szDescriptionAttribute, description);
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteFolderEditorData(CAsset const& library, XmlNodeRef const pParentNode) const
{
	size_t const itemCount = library.ChildCount();

	for (size_t i = 0; i < itemCount; ++i)
	{
		CAsset const* const pAsset = library.GetChild(i);

		if (pAsset->GetType() == EAssetType::Folder)
		{
			XmlNodeRef const pFolderNode = pParentNode->createNode(s_szFolderTag);

			if (pFolderNode != nullptr)
			{
				pFolderNode->setAttr(CryAudio::s_szNameAttribute, pAsset->GetName());
				string const description = pAsset->GetDescription();

				if (!description.IsEmpty() && ((pAsset->GetFlags() & EAssetFlags::IsDefaultControl) == 0))
				{
					pFolderNode->setAttr(s_szDescriptionAttribute, description);
				}

				WriteFolderEditorData(*pAsset, pFolderNode);
				pParentNode->addChild(pFolderNode);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileWriter::WriteControlsEditorData(CAsset const& parentAsset, XmlNodeRef const pParentNode) const
{
	size_t const itemCount = parentAsset.ChildCount();

	for (size_t i = 0; i < itemCount; ++i)
	{
		CAsset const& asset = *parentAsset.GetChild(i);
		EAssetType const type = asset.GetType();
		char const* const szTag = TypeToTag(type);

		if (szTag != nullptr)
		{
			XmlNodeRef const pControlNode = pParentNode->createNode(TypeToTag(type));

			if (pControlNode != nullptr)
			{
				string const& description = asset.GetDescription();

				if (!description.IsEmpty() && ((asset.GetFlags() & EAssetFlags::IsDefaultControl) == 0))
				{
					pControlNode->setAttr(CryAudio::s_szNameAttribute, asset.GetName());
					pControlNode->setAttr(s_szDescriptionAttribute, description);
					pParentNode->addChild(pControlNode);
				}
			}
		}

		if ((type == EAssetType::Folder) || (type == EAssetType::Switch))
		{
			WriteControlsEditorData(asset, pParentNode);
		}
	}
}
} // namespace ACE
