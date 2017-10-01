// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAssetsManager.h"

#include "AudioControlsEditorUndo.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "IAudioSystemEditor.h"
#include "IAudioSystemItem.h"

#include <CryString/StringUtils.h>
#include <CryString/CryPath.h>
#include <FilePathUtil.h>
#include <IEditor.h>

namespace ACE
{
CID CAudioAssetsManager::m_nextId = 1;

//////////////////////////////////////////////////////////////////////////
CAudioAssetsManager::CAudioAssetsManager()
{
	ClearDirtyFlags();
	m_scopeMap[Utils::GetGlobalScope()] = SScopeInfo("global", false);
	m_controls.reserve(8192);
}

//////////////////////////////////////////////////////////////////////////
CAudioAssetsManager::~CAudioAssetsManager()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::signalAboutToLoad.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::signalLoaded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::Initialize()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.Connect([&]()
		{
			ClearAllConnections();
		}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect([&]()
		{
			ReloadAllConnections();
		}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::signalAboutToLoad.Connect([&]()
		{
			m_isLoading = true;
		}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::signalLoaded.Connect([&]()
		{
			m_isLoading = false;
		}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
CAudioControl* CAudioAssetsManager::CreateControl(string const& name, EItemType const type, CAudioAsset* const pParent)
{
	if ((pParent != nullptr) && !name.empty())
	{
		EItemType const parentType = pParent->GetType();

		if ((parentType == EItemType::Folder || parentType == EItemType::Library) || (parentType == EItemType::Switch && type == EItemType::State))
		{
			CAudioControl* const pFoundControl = FindControl(name, type, pParent);

			if (pFoundControl != nullptr)
			{
				return pFoundControl;
			}

			signalItemAboutToBeAdded(pParent);

			CAudioControl* const pControl = new CAudioControl(name, GenerateUniqueId(), type);

			m_controls.push_back(pControl);

			pControl->SetParent(pParent);
			pParent->AddChild(pControl);

			signalItemAdded(pControl);
			pControl->SetModified(true);

			return pControl;
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] Controls must be created on either Libraries or Folders!");
		}
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] No valid parent or name to create control!");
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::DeleteItem(CAudioAsset* const pItem)
{
	if (pItem != nullptr)
	{
		// Recursively remove all children as well
		while (pItem->ChildCount() > 0)
		{
			DeleteItem(pItem->GetChild(0));
		}

		EItemType const type = pItem->GetType();

		// Inform that we're about to remove the item
		if (type == EItemType::Library)
		{
			signalLibraryAboutToBeRemoved(static_cast<CAudioLibrary*>(pItem));
		}
		else
		{
			signalItemAboutToBeRemoved(pItem);
		}

		// Remove/detach item from the tree
		CAudioAsset* const pParent = pItem->GetParent();

		if (pParent != nullptr)
		{
			pParent->RemoveChild(pItem);
			pItem->SetParent(nullptr);
		}

		if (type == EItemType::Library)
		{
			m_audioLibraries.erase(std::remove(m_audioLibraries.begin(), m_audioLibraries.end(), static_cast<CAudioLibrary*>(pItem)), m_audioLibraries.end());
			signalLibraryRemoved();
		}
		else
		{
			if (type != EItemType::Folder)
			{
				// Must be a control
				CAudioControl* const pControl = static_cast<CAudioControl*>(pItem);

				if (pControl != nullptr)
				{
					pControl->ClearConnections();
					m_controls.erase(std::remove_if(m_controls.begin(), m_controls.end(), [&](auto pIterControl) { return pIterControl->GetId() == pControl->GetId(); }), m_controls.end());
				}
			}
			signalItemRemoved(pParent, pItem);
		}

		pItem->SetModified(true);
		delete pItem;
	}
}

//////////////////////////////////////////////////////////////////////////
CAudioControl* CAudioAssetsManager::GetControlByID(CID const id) const
{
	for (auto const pControl : m_controls)
	{
		if (pControl->GetId() == id)
		{
			return pControl;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::ClearScopes()
{
	m_scopeMap.clear();

	// The global scope must always exist
	m_scopeMap[Utils::GetGlobalScope()] = SScopeInfo("global", false);
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::AddScope(string const& name, bool const isLocalOnly)
{
	string scopeName = name;
	m_scopeMap[CCrc32::Compute(scopeName.MakeLower())] = SScopeInfo(scopeName, isLocalOnly);
}

//////////////////////////////////////////////////////////////////////////
bool CAudioAssetsManager::ScopeExists(string const& name) const
{
	string scopeName = name;
	return m_scopeMap.find(CCrc32::Compute(scopeName.MakeLower())) != m_scopeMap.end();
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::GetScopeInfoList(ScopeInfoList& scopeList) const
{
	stl::map_to_vector(m_scopeMap, scopeList);
}

//////////////////////////////////////////////////////////////////////////
Scope CAudioAssetsManager::GetScope(string const& name) const
{
	string scopeName = name;
	scopeName.MakeLower();
	return CCrc32::Compute(scopeName);
}

//////////////////////////////////////////////////////////////////////////
SScopeInfo CAudioAssetsManager::GetScopeInfo(Scope const id) const
{
	return stl::find_in_map(m_scopeMap, id, SScopeInfo());
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::Clear()
{
	std::vector<CAudioLibrary*> const libraries = m_audioLibraries;

	for (auto const pLibrary : libraries)
	{
		DeleteItem(pLibrary);
	}

	CRY_ASSERT(m_controls.empty());
	CRY_ASSERT(m_audioLibraries.empty());
	ClearScopes();
	ClearDirtyFlags();
}

//////////////////////////////////////////////////////////////////////////
CAudioLibrary* CAudioAssetsManager::CreateLibrary(string const& name)
{
	if (!name.empty())
	{
		size_t const size = m_audioLibraries.size();

		for (size_t i = 0; i < size; ++i)
		{
			CAudioLibrary* const pLibrary = m_audioLibraries[i];

			if ((pLibrary != nullptr) && (name.compareNoCase(pLibrary->GetName()) == 0))
			{
				return pLibrary;
			}
		}

		signalLibraryAboutToBeAdded();
		CAudioLibrary* const pLibrary = new CAudioLibrary(name);
		m_audioLibraries.push_back(pLibrary);
		signalLibraryAdded(pLibrary);
		pLibrary->SetModified(true);
		return pLibrary;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CAudioAsset* CAudioAssetsManager::CreateFolder(string const& name, CAudioAsset* const pParent)
{
	if ((pParent != nullptr) && !name.empty())
	{
		if ((pParent->GetType() == EItemType::Folder) || (pParent->GetType() == EItemType::Library))
		{
			size_t const size = pParent->ChildCount();

			for (size_t i = 0; i < size; ++i)
			{
				CAudioAsset* const pItem = pParent->GetChild(i);

				if ((pItem != nullptr) && (pItem->GetType() == EItemType::Folder) && (name.compareNoCase(pItem->GetName()) == 0))
				{
					return pItem;
				}
			}

			CAudioFolder* const pFolder = new CAudioFolder(name);

			if (pFolder != nullptr)
			{
				signalItemAboutToBeAdded(pParent);
				pParent->AddChild(pFolder);
				pFolder->SetParent(pParent);
			}

			signalItemAdded(pFolder);
			pFolder->SetModified(true);
			return pFolder;
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] Folders must be created on either Libraries or Folders!");
		}
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] No valid parent or name to create folder!");
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::OnControlAboutToBeModified(CAudioControl* const pControl)
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::OnConnectionAdded(CAudioControl* const pControl, IAudioSystemItem* const pMiddlewareControl)
{
	signalConnectionAdded(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::OnConnectionRemoved(CAudioControl* const pControl, IAudioSystemItem* const pMiddlewareControl)
{
	signalConnectionRemoved(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::OnControlModified(CAudioControl* const pControl)
{
	signalControlModified(pControl);
	m_controlTypesModified.emplace_back(pControl->GetType());
	signalIsDirty(true);
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::SetAssetModified(CAudioAsset* const pAsset)
{
	UpdateLibraryConnectionStates(pAsset);
	m_controlTypesModified.emplace_back(pAsset->GetType());
	signalIsDirty(true);
}

//////////////////////////////////////////////////////////////////////////
bool CAudioAssetsManager::IsDirty() const
{
	return !m_controlTypesModified.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CAudioAssetsManager::IsTypeDirty(EItemType const type) const
{
	return std::find(m_controlTypesModified.begin(), m_controlTypesModified.end(), type) != m_controlTypesModified.end();
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::ClearDirtyFlags()
{
	m_controlTypesModified.clear();
	signalIsDirty(false);
}

//////////////////////////////////////////////////////////////////////////
CAudioControl* CAudioAssetsManager::FindControl(string const& controlName, EItemType const type, CAudioAsset* const pParent) const
{
	if (pParent == nullptr)
	{
		for (auto const pControl : m_controls)
		{
			if ((pControl != nullptr) && (pControl->GetName() == controlName) && (pControl->GetType() == type))
			{
				return pControl;
			}
		}
	}
	else
	{
		size_t const size = pParent->ChildCount();

		for (size_t i = 0; i < size; ++i)
		{
			CAudioControl* const pControl = static_cast<CAudioControl*>(pParent->GetChild(i));

			if ((pControl != nullptr) && (pControl->GetName() == controlName) && (pControl->GetType() == type))
			{
				return pControl;
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::ClearAllConnections()
{
	m_isLoading = true;

	for (auto const pControl : m_controls)
	{
		if (pControl != nullptr)
		{
			pControl->ClearConnections();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::ReloadAllConnections()
{
	for (auto const pControl : m_controls)
	{
		if (pControl != nullptr)
		{
			pControl->ClearConnections();
			pControl->ReloadConnections();
		}
	}
	
	m_isLoading = false;
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::UpdateAllConnectionStates()
{
	for (auto const pLibrary : m_audioLibraries)
	{
		UpdateAssetConnectionStates(pLibrary);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::UpdateLibraryConnectionStates(CAudioAsset* pAsset)
{
	while ((pAsset != nullptr) && (pAsset->GetType() != EItemType::Library))
	{
		pAsset = pAsset->GetParent();
	}

	if (pAsset != nullptr)
	{
		UpdateAssetConnectionStates(pAsset);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::UpdateAssetConnectionStates(CAudioAsset* const pAsset)
{
	if (pAsset != nullptr)
	{
		EItemType const type = pAsset->GetType();

		if ((type == EItemType::Library) || (type == EItemType::Folder) || (type == EItemType::Switch))
		{
			bool hasPlaceholder = false;
			bool hasNoConnection = false;
			bool hasControl = false;
			int const childCount = pAsset->ChildCount();

			for (int i = 0; i < childCount; ++i)
			{
				auto const pChild = pAsset->GetChild(i);

				UpdateAssetConnectionStates(pChild);

				if (pChild->HasPlaceholderConnection())
				{
					hasPlaceholder = true;
				}

				if (!pChild->HasConnection())
				{
					hasNoConnection = true;
				}
				
				if (pChild->HasControl())
				{
					hasControl = true;
				}
			}

			pAsset->SetHasPlaceholderConnection(hasPlaceholder);
			pAsset->SetHasConnection(!hasNoConnection);
			pAsset->SetHasControl(hasControl);
		}
		else if (type != EItemType::Invalid)
		{
			CAudioControl* const pControl = static_cast<CAudioControl*>(pAsset);

			if (pControl != nullptr)
			{
				pControl->SetHasControl(true);
				bool hasPlaceholder = false;
				bool hasConnection = false;
				int const connectionCount = pControl->GetConnectionCount();

				for (int i = 0; i < connectionCount; ++i)
				{
					hasConnection = true;
					IAudioSystemEditor const* const pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManger()->GetImplementation();

					if (pAudioSystemImpl != nullptr)
					{
						IAudioSystemItem const* const pAudioSystemControl = pAudioSystemImpl->GetControl(pControl->GetConnectionAt(i)->GetID());

						if (pAudioSystemControl != nullptr)
						{
							if (pAudioSystemControl->IsPlaceholder())
							{
								hasPlaceholder = true;
							}
						}
					}
				}

				pControl->SetHasPlaceholderConnection(hasPlaceholder);
				pControl->SetHasConnection(hasConnection);
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::MoveItems(CAudioAsset* const pParent, std::vector<CAudioAsset*> const& items)
{
	if (pParent != nullptr)
	{
		for (auto const pItem : items)
		{
			if (pItem != nullptr)
			{
				CAudioAsset* const pPreviousParent = pItem->GetParent();

				if (pPreviousParent != nullptr)
				{
					signalItemAboutToBeRemoved(pItem);
					pPreviousParent->RemoveChild(pItem);
					pItem->SetParent(nullptr);
					signalItemRemoved(pPreviousParent, pItem);
					pPreviousParent->SetModified(true);
				}

				signalItemAboutToBeAdded(pParent);
				pParent->AddChild(pItem);
				pItem->SetParent(pParent);
				signalItemAdded(pItem);
				pItem->SetModified(true);
			}
		}

		pParent->SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioAssetsManager::CreateAndConnectImplItems(IAudioSystemItem* const pImplItem, CAudioAsset* const pParent)
{
	signalItemAboutToBeAdded(pParent);
	CAudioAsset* pItem = CreateAndConnectImplItemsRecursively(pImplItem, pParent);
	signalItemAdded(pItem);
}

//////////////////////////////////////////////////////////////////////////
CAudioAsset* CAudioAssetsManager::CreateAndConnectImplItemsRecursively(IAudioSystemItem* const pImplItem, CAudioAsset* const pParent)
{
	CAudioAsset* pItem = nullptr;

	// Create the new control and connect it to the one dragged in externally
	IAudioSystemEditor* const pImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

	string name = pImplItem->GetName();
	EItemType const type = pImpl->ImplTypeToATLType(pImplItem->GetType());

	if (type != EItemType::Invalid)
	{
		PathUtil::RemoveExtension(name);

		if (type != EItemType::State)
		{
			name = Utils::GenerateUniqueControlName(name, type, *this);
		}
		else
		{
			name = Utils::GenerateUniqueName(name, type, pParent);
		}

		CAudioControl* const pControl = new CAudioControl(name, GenerateUniqueId(), type);
		pControl->SetParent(pParent);
		pParent->AddChild(pControl);
		m_controls.push_back(pControl);

		ConnectionPtr const pAudioConnection = pImpl->CreateConnectionToControl(pControl->GetType(), pImplItem);

		if (pAudioConnection != nullptr)
		{
			pControl->AddConnection(pAudioConnection);
		}

		pItem = pControl;
	}
	else
	{
		// If the type of the control is invalid then it must be a folder or container
		name = Utils::GenerateUniqueName(name, EItemType::Folder, pParent);
		CAudioFolder* const pFolder = new CAudioFolder(name);
		pParent->AddChild(pFolder);
		pFolder->SetParent(pParent);

		pItem = pFolder;
	}

	size_t const size = pImplItem->ChildCount();

	for (size_t i = 0; i < size; ++i)
	{
		CreateAndConnectImplItemsRecursively(pImplItem->GetChildAt(i), pItem);
	}

	return pItem;
}

//////////////////////////////////////////////////////////////////////////
namespace Utils
{
//////////////////////////////////////////////////////////////////////////
Scope GetGlobalScope()
{
	static Scope const globalScopeId = CCrc32::Compute("global");
	return globalScopeId;
}

//////////////////////////////////////////////////////////////////////////
string GenerateUniqueName(string const& name, EItemType const type, CAudioAsset* const pParent)
{
	string finalName = name;

	if (pParent != nullptr)
	{
		size_t const size = pParent->ChildCount();
		std::vector<string> names;
		names.reserve(size);

		for (size_t i = 0; i < size; ++i)
		{
			CAudioAsset* const pChild = pParent->GetChild(i);

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
string GenerateUniqueLibraryName(string const& name, CAudioAssetsManager const& assetManager)
{
	size_t const size = assetManager.GetLibraryCount();
	std::vector<string> names;
	names.reserve(size);

	for (size_t i = 0; i < size; ++i)
	{
		CAudioLibrary* const pLibrary = assetManager.GetLibrary(i);

		if (pLibrary != nullptr)
		{
			names.emplace_back(pLibrary->GetName());
		}
	}

	return PathUtil::GetUniqueName(name, names);
}

//////////////////////////////////////////////////////////////////////////
string GenerateUniqueControlName(string const& name, EItemType const type, CAudioAssetsManager const& assetManager)
{
	CAudioAssetsManager::Controls const& controls(assetManager.GetControls());
	std::vector<string> names;
	names.reserve(controls.size());

	for (auto const pControl : controls)
	{
		names.emplace_back(pControl->GetName());
	}

	return PathUtil::GetUniqueName(name, names);
}

//////////////////////////////////////////////////////////////////////////
CAudioAsset* GetParentLibrary(CAudioAsset* pAsset)
{
	while ((pAsset != nullptr) && (pAsset->GetType() != EItemType::Library))
	{
		pAsset = pAsset->GetParent();
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
void SelectTopLevelAncestors(std::vector<CAudioAsset*> const& source, std::vector<CAudioAsset*>& dest)
{
	for (auto const pItem : source)
	{
		// Check if item has ancestors that are also selected
		bool isAncestorAlsoSelected = false;

		for (auto const pOtherItem : source)
		{
			if (pItem != pOtherItem)
			{
				// Find if pOtherItem is the ancestor of pItem
				CAudioAsset const* pParent = pItem->GetParent();

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
			dest.push_back(pItem);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
string const& GetAssetFolder()
{
	static string const path = AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "ace" CRY_NATIVE_PATH_SEPSTR;
	return path;
}
} // namespace Utils
} // namespace ACE
