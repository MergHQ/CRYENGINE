// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AssetUtils.h"

#include "AssetsManager.h"
#include "Common/IConnection.h"
#include "Common/IImpl.h"

#include <PathUtils.h>

namespace ACE
{
namespace AssetUtils
{
//////////////////////////////////////////////////////////////////////////
string GenerateUniqueName(string const& name, EAssetType const type, CAsset* const pParent)
{
	string finalName = name;

	if (pParent != nullptr)
	{
		size_t const numChildren = pParent->ChildCount();
		AssetNames names;
		names.reserve(numChildren);

		for (size_t i = 0; i < numChildren; ++i)
		{
			CAsset const* const pChild = pParent->GetChild(i);

			if ((pChild != nullptr) && (pChild->GetType() == type))
			{
				names.emplace_back(pChild->GetName());
			}
		}

		finalName = PathUtil::GetUniqueName(name, names);
	}

	return finalName;
}

//////////////////////////////////////////////////////////////////////////
string GenerateUniqueLibraryName(string const& name)
{
	size_t const numLibraries = g_assetsManager.GetLibraryCount();
	AssetNames names;
	names.reserve(numLibraries);

	for (size_t i = 0; i < numLibraries; ++i)
	{
		CLibrary const* const pLibrary = g_assetsManager.GetLibrary(i);

		if (pLibrary != nullptr)
		{
			names.emplace_back(pLibrary->GetName());
		}
	}

	return PathUtil::GetUniqueName(name, names);
}

//////////////////////////////////////////////////////////////////////////
string GenerateUniqueControlName(string const& name, EAssetType const type)
{
	Controls const& controls(g_assetsManager.GetControls());
	AssetNames names;
	names.reserve(controls.size());

	for (auto const* const pControl : controls)
	{
		if (type == pControl->GetType())
		{
			names.emplace_back(pControl->GetName());
		}
	}

	return PathUtil::GetUniqueName(name, names);
}

//////////////////////////////////////////////////////////////////////////
CAsset* GetParentLibrary(CAsset* const pAsset)
{
	CAsset* pParent = pAsset;

	while ((pParent != nullptr) && (pParent->GetType() != EAssetType::Library))
	{
		pParent = pParent->GetParent();
	}

	return pParent;
}

//////////////////////////////////////////////////////////////////////////
char const* GetTypeName(EAssetType const type)
{
	char const* szTypeName = nullptr;

	switch (type)
	{
	case EAssetType::Trigger:
		szTypeName = "Trigger";
		break;
	case EAssetType::Parameter:
		szTypeName = "Parameter";
		break;
	case EAssetType::Switch:
		szTypeName = "Switch";
		break;
	case EAssetType::State:
		szTypeName = "State";
		break;
	case EAssetType::Environment:
		szTypeName = "Environment";
		break;
	case EAssetType::Preload:
		szTypeName = "Preload";
		break;
	case EAssetType::Setting:
		szTypeName = "Setting";
		break;
	case EAssetType::Folder:
		szTypeName = "Folder";
		break;
	case EAssetType::Library:
		szTypeName = "Library";
		break;
	default:
		szTypeName = nullptr;
		break;
	}

	return szTypeName;
}

//////////////////////////////////////////////////////////////////////////
void SelectTopLevelAncestors(Assets const& source, Assets& dest)
{
	for (auto const pAsset : source)
	{
		// Check if item has ancestors that are also selected
		bool isAncestorAlsoSelected = false;

		for (auto const pOtherItem : source)
		{
			if (pAsset != pOtherItem)
			{
				// Find if pOtherItem is the ancestor of pAsset
				CAsset const* pParent = pAsset->GetParent();

				while (pParent != nullptr)
				{
					if (pParent == pOtherItem)
					{
						break;
					}

					pParent = pParent->GetParent();
				}

				if (pParent != nullptr)
				{
					isAncestorAlsoSelected = true;
					break;
				}
			}
		}

		if (!isAncestorAlsoSelected)
		{
			dest.push_back(pAsset);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void TryConstructTriggerConnectionNode(
	XmlNodeRef const pTriggerNode,
	IConnection const* const pIConnection,
	CryAudio::ContextId const contextId)
{
	XmlNodeRef const pConnectionNode = g_pIImpl->CreateXMLNodeFromConnection(pIConnection, EAssetType::Trigger, contextId);

	if (pConnectionNode != nullptr)
	{
		// Don't add identical nodes!
		bool shouldAddNode = true;
		int const numNodeChilds = pTriggerNode->getChildCount();

		for (int i = 0; i < numNodeChilds; ++i)
		{
			XmlNodeRef const pTempNode = pTriggerNode->getChild(i);

			if ((pTempNode != nullptr) && (_stricmp(pTempNode->getTag(), pConnectionNode->getTag()) == 0))
			{
				int const numAttributes1 = pTempNode->getNumAttributes();
				int const numAttributes2 = pConnectionNode->getNumAttributes();

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
						pConnectionNode->getAttributeByIndex(k, &key2, &val2);

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
			pTriggerNode->addChild(pConnectionNode);
		}
	}
}
} // namespace AssetUtils
} // namespace ACE
