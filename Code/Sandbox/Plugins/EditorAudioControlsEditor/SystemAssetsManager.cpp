// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemAssetsManager.h"

#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

#include <IEditorImpl.h>
#include <ImplItem.h>

#include <CryAudio/IAudioSystem.h>
#include <CryString/StringUtils.h>
#include <CryString/CryPath.h>
#include <FilePathUtil.h>
#include <IEditor.h>

namespace ACE
{
CID CSystemAssetsManager::m_nextId = 1;

//////////////////////////////////////////////////////////////////////////
CSystemAssetsManager::CSystemAssetsManager()
{
	ClearDirtyFlags();
	m_scopes[Utils::GetGlobalScope()] = SScopeInfo("global", false);
	m_controls.reserve(8192);
}

//////////////////////////////////////////////////////////////////////////
CSystemAssetsManager::~CSystemAssetsManager()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::SignalAboutToLoad.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::SignalLoaded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::Initialize()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.Connect([&]()
		{
			ClearAllConnections();
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.Connect([&]()
		{
			ReloadAllConnections();
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::SignalAboutToLoad.Connect([&]()
		{
			m_isLoading = true;
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::SignalLoaded.Connect([&]()
		{
			m_isLoading = false;
	  }, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
CSystemControl* CSystemAssetsManager::CreateControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent /*= nullptr*/)
{
	CSystemControl* pControl = nullptr;

	if ((pParent != nullptr) && !name.empty())
	{
		ESystemItemType const parentType = pParent->GetType();

		if (((parentType == ESystemItemType::Folder) || (parentType == ESystemItemType::Library)) || ((parentType == ESystemItemType::Switch) && (type == ESystemItemType::State)))
		{
			CSystemControl* const pFoundControl = FindControl(name, type, pParent);

			if (pFoundControl != nullptr)
			{
				pControl = pFoundControl;
			}
			else
			{
				SignalItemAboutToBeAdded(pParent);

				CSystemControl* const pNewControl = new CSystemControl(name, GenerateUniqueId(), type);

				m_controls.emplace_back(pNewControl);

				pNewControl->SetParent(pParent);
				pParent->AddChild(pNewControl);

				SignalItemAdded(pNewControl);
				pNewControl->SetModified(true);

				pControl = pNewControl;
			}
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

	return pControl;
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::CreateDefaultControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent, bool& wasModified, string const& description)
{
	CSystemControl* pControl = FindControl(name, type);

	if (pControl == nullptr)
	{
		pControl = CreateControl(name, type, pParent);
		wasModified = true;
	}
	else if (pControl->GetParent() != pParent)
	{
		pControl->GetParent()->RemoveChild(pControl);
		pParent->AddChild(pControl);
		pControl->SetParent(pParent);
		wasModified = true;
	}

	if (pControl != nullptr)
	{
		pControl->SetDescription(description);
		pControl->SetDefaultControl(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::DeleteItem(CSystemAsset* const pItem)
{
	if (pItem != nullptr)
	{
		// Recursively remove all children as well
		while (pItem->ChildCount() > 0)
		{
			DeleteItem(pItem->GetChild(0));
		}

		ESystemItemType const type = pItem->GetType();

		// Inform that we're about to remove the item
		if (type == ESystemItemType::Library)
		{
			SignalLibraryAboutToBeRemoved(static_cast<CSystemLibrary*>(pItem));
		}
		else
		{
			SignalItemAboutToBeRemoved(pItem);
		}

		// Remove/detach item from the tree
		CSystemAsset* const pParent = pItem->GetParent();

		if (pParent != nullptr)
		{
			pParent->RemoveChild(pItem);
			pItem->SetParent(nullptr);
		}

		if (type == ESystemItemType::Library)
		{
			m_systemLibraries.erase(std::remove(m_systemLibraries.begin(), m_systemLibraries.end(), static_cast<CSystemLibrary*>(pItem)), m_systemLibraries.end());
			SignalLibraryRemoved();
		}
		else
		{
			if (type != ESystemItemType::Folder)
			{
				// Must be a control
				CSystemControl* const pControl = static_cast<CSystemControl*>(pItem);

				if (pControl != nullptr)
				{
					pControl->ClearConnections();
					m_controls.erase(std::remove_if(m_controls.begin(), m_controls.end(), [&](auto pIterControl) { return pIterControl->GetId() == pControl->GetId(); }), m_controls.end());
					SetTypeModified(type, true);
				}
			}

			SignalItemRemoved(pParent, pItem);
		}

		pItem->SetModified(true);
		delete pItem;
	}
}

//////////////////////////////////////////////////////////////////////////
CSystemControl* CSystemAssetsManager::GetControlByID(CID const id) const
{
	CSystemControl* pSystemControl = nullptr;

	for (auto const pControl : m_controls)
	{
		if (pControl->GetId() == id)
		{
			pSystemControl = pControl;
			break;
		}
	}

	return pSystemControl;
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::ClearScopes()
{
	m_scopes.clear();

	// The global scope must always exist
	m_scopes[Utils::GetGlobalScope()] = SScopeInfo("global", false);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::AddScope(string const& name, bool const isLocalOnly /*= false*/)
{
	m_scopes[CryAudio::StringToId(name.c_str())] = SScopeInfo(name, isLocalOnly);
}

//////////////////////////////////////////////////////////////////////////
bool CSystemAssetsManager::ScopeExists(string const& name) const
{
	return m_scopes.find(CryAudio::StringToId(name.c_str())) != m_scopes.end();
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::GetScopeInfoList(ScopeInfoList& scopeList) const
{
	stl::map_to_vector(m_scopes, scopeList);
}

//////////////////////////////////////////////////////////////////////////
Scope CSystemAssetsManager::GetScope(string const& name) const
{
	return CryAudio::StringToId(name.c_str());
}

//////////////////////////////////////////////////////////////////////////
SScopeInfo CSystemAssetsManager::GetScopeInfo(Scope const id) const
{
	return stl::find_in_map(m_scopes, id, SScopeInfo());
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::Clear()
{
	std::vector<CSystemLibrary*> const libraries = m_systemLibraries;

	for (auto const pLibrary : libraries)
	{
		DeleteItem(pLibrary);
	}

	CRY_ASSERT_MESSAGE(m_controls.empty(), "m_controls is not empty.");
	CRY_ASSERT_MESSAGE(m_systemLibraries.empty(), "m_systemLibraries is not empty.");
	ClearScopes();
	ClearDirtyFlags();
}

//////////////////////////////////////////////////////////////////////////
CSystemLibrary* CSystemAssetsManager::CreateLibrary(string const& name)
{
	CSystemLibrary* pSystemLibrary = nullptr;
	bool foundLibrary = false;

	if (!name.empty())
	{
		size_t const size = m_systemLibraries.size();

		for (size_t i = 0; i < size; ++i)
		{
			CSystemLibrary* const pLibrary = m_systemLibraries[i];

			if ((pLibrary != nullptr) && (name.compareNoCase(pLibrary->GetName()) == 0))
			{
				pSystemLibrary = pLibrary;
				foundLibrary = true;
				break;
			}
		}

		if (!foundLibrary)
		{
			SignalLibraryAboutToBeAdded();
			CSystemLibrary* const pLibrary = new CSystemLibrary(name);
			m_systemLibraries.emplace_back(pLibrary);
			SignalLibraryAdded(pLibrary);
			pLibrary->SetModified(true);
			pSystemLibrary = pLibrary;
		}
	}

	return pSystemLibrary;
}

//////////////////////////////////////////////////////////////////////////
CSystemAsset* CSystemAssetsManager::CreateFolder(string const& name, CSystemAsset* const pParent /*= nullptr*/)
{
	CSystemAsset* pAsset = nullptr;
	bool foundFolder = false;

	if ((pParent != nullptr) && !name.empty())
	{
		if ((pParent->GetType() == ESystemItemType::Folder) || (pParent->GetType() == ESystemItemType::Library))
		{
			size_t const size = pParent->ChildCount();

			for (size_t i = 0; i < size; ++i)
			{
				CSystemAsset* const pItem = pParent->GetChild(i);

				if ((pItem != nullptr) && (pItem->GetType() == ESystemItemType::Folder) && (name.compareNoCase(pItem->GetName()) == 0))
				{
					pAsset = pItem;
					foundFolder = true;
					break;
				}
			}

			if (!foundFolder)
			{
				CSystemFolder* const pFolder = new CSystemFolder(name);

				if (pFolder != nullptr)
				{
					SignalItemAboutToBeAdded(pParent);
					pParent->AddChild(pFolder);
					pFolder->SetParent(pParent);
				}

				SignalItemAdded(pFolder);
				pFolder->SetModified(true);
				pAsset = pFolder;
			}
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

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::OnControlAboutToBeModified(CSystemControl* const pControl)
{
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::OnConnectionAdded(CSystemControl* const pControl, CImplItem* const pImplControl)
{
	SignalConnectionAdded(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::OnConnectionRemoved(CSystemControl* const pControl, CImplItem* const pImplControl)
{
	SignalConnectionRemoved(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::OnAssetRenamed()
{
	SignalAssetRenamed();
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::UpdateFolderPaths()
{
	string const& rootPath = AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR;

	if (g_pEditorImpl != nullptr)
	{
		string const& implFolderPath = rootPath + g_pEditorImpl->GetFolderName() + CRY_NATIVE_PATH_SEPSTR;

		m_configFolderPath = implFolderPath;
		m_configFolderPath += CryAudio::s_szConfigFolderName;
		m_configFolderPath += CRY_NATIVE_PATH_SEPSTR;

		m_assetFolderPath = implFolderPath;
		m_assetFolderPath += CryAudio::s_szAssetsFolderName;
		m_assetFolderPath += CRY_NATIVE_PATH_SEPSTR;
	}
	else
	{
		m_configFolderPath = rootPath;
		m_assetFolderPath = rootPath;
	}
}
//////////////////////////////////////////////////////////////////////////
string const& CSystemAssetsManager::GetConfigFolderPath() const
{
	return m_configFolderPath;
}

//////////////////////////////////////////////////////////////////////////
string const& CSystemAssetsManager::GetAssetFolderPath() const
{
	return m_assetFolderPath;
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::OnControlModified(CSystemControl* const pControl)
{
	SignalControlModified(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::SetAssetModified(CSystemAsset* const pAsset, bool const isModified)
{
	if (isModified)
	{
		UpdateLibraryConnectionStates(pAsset);
		auto const type = pAsset->GetType();

		m_modifiedTypes.emplace(type);

		if (type == ESystemItemType::Library)
		{
			m_modifiedLibraryNames.emplace(pAsset->GetName());
		}

		SignalIsDirty(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::SetTypeModified(ESystemItemType const type, bool const isModified)
{
	if (isModified)
	{
		m_modifiedTypes.emplace(type);
	}
}
//////////////////////////////////////////////////////////////////////////
bool CSystemAssetsManager::IsDirty() const
{
	return !m_modifiedTypes.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemAssetsManager::IsTypeDirty(ESystemItemType const type) const
{
	return m_modifiedTypes.find(type) != m_modifiedTypes.end();
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::ClearDirtyFlags()
{
	m_modifiedTypes.clear();
	m_modifiedLibraryNames.clear();
	SignalIsDirty(false);
}

//////////////////////////////////////////////////////////////////////////
CSystemControl* CSystemAssetsManager::FindControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent /*= nullptr*/) const
{
	CSystemControl* pSystemControl = nullptr;

	if (pParent == nullptr)
	{
		for (auto const pControl : m_controls)
		{
			if ((pControl != nullptr) && (pControl->GetName() == name) && (pControl->GetType() == type))
			{
				pSystemControl = pControl;
				break;
			}
		}
	}
	else
	{
		size_t const size = pParent->ChildCount();

		for (size_t i = 0; i < size; ++i)
		{
			CSystemControl* const pControl = static_cast<CSystemControl*>(pParent->GetChild(i));

			if ((pControl != nullptr) && (pControl->GetName() == name) && (pControl->GetType() == type))
			{
				pSystemControl = pControl;
				break;
			}
		}
	}

	return pSystemControl;
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::ClearAllConnections()
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
void CSystemAssetsManager::ReloadAllConnections()
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
void CSystemAssetsManager::UpdateAllConnectionStates()
{
	for (auto const pLibrary : m_systemLibraries)
	{
		UpdateAssetConnectionStates(pLibrary);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::UpdateLibraryConnectionStates(CSystemAsset* pAsset)
{
	while ((pAsset != nullptr) && (pAsset->GetType() != ESystemItemType::Library))
	{
		pAsset = pAsset->GetParent();
	}

	if (pAsset != nullptr)
	{
		UpdateAssetConnectionStates(pAsset);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::UpdateAssetConnectionStates(CSystemAsset* const pAsset)
{
	if (pAsset != nullptr)
	{
		ESystemItemType const type = pAsset->GetType();

		if ((type == ESystemItemType::Library) || (type == ESystemItemType::Folder) || (type == ESystemItemType::Switch))
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
		else if (type != ESystemItemType::Invalid)
		{
			CSystemControl* const pControl = static_cast<CSystemControl*>(pAsset);

			if (pControl != nullptr)
			{
				pControl->SetHasControl(true);

				bool hasPlaceholder = false;
				bool hasConnection = pControl->IsDefaultControl();
				size_t const connectionCount = pControl->GetConnectionCount();

				for (size_t i = 0; i < connectionCount; ++i)
				{
					hasConnection = true;

					if (g_pEditorImpl != nullptr)
					{
						CImplItem const* const pImpleControl = g_pEditorImpl->GetControl(pControl->GetConnectionAt(i)->GetID());

						if (pImpleControl != nullptr)
						{
							if (pImpleControl->IsPlaceholder())
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
void CSystemAssetsManager::MoveItems(CSystemAsset* const pParent, std::vector<CSystemAsset*> const& items)
{
	if (pParent != nullptr)
	{
		for (auto const pItem : items)
		{
			if (pItem != nullptr)
			{
				CSystemAsset* const pPreviousParent = pItem->GetParent();

				if (pPreviousParent != nullptr)
				{
					SignalItemAboutToBeRemoved(pItem);
					pPreviousParent->RemoveChild(pItem);
					pItem->SetParent(nullptr);
					SignalItemRemoved(pPreviousParent, pItem);
					pPreviousParent->SetModified(true);
				}

				SignalItemAboutToBeAdded(pParent);
				ESystemItemType const type = pItem->GetType();

				if ((type == ESystemItemType::State) || (type == ESystemItemType::Folder))
				{
					// To prevent duplicated names of states and folders.
					pItem->UpdateNameOnMove(pParent);
				}

				pParent->AddChild(pItem);
				pItem->SetParent(pParent);
				SignalItemAdded(pItem);
				pItem->SetModified(true);
			}
		}

		pParent->SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystemAssetsManager::CreateAndConnectImplItems(CImplItem* const pImplItem, CSystemAsset* const pParent)
{
	SignalItemAboutToBeAdded(pParent);
	CSystemAsset* pItem = CreateAndConnectImplItemsRecursively(pImplItem, pParent);
	SignalItemAdded(pItem);
}

//////////////////////////////////////////////////////////////////////////
CSystemAsset* CSystemAssetsManager::CreateAndConnectImplItemsRecursively(CImplItem* const pImplItem, CSystemAsset* const pParent)
{
	CSystemAsset* pItem = nullptr;

	// Create the new control and connect it to the one dragged in externally
	string name = pImplItem->GetName();
	ESystemItemType type = g_pEditorImpl->ImplTypeToSystemType(pImplItem);

	if (type != ESystemItemType::Invalid)
	{
		PathUtil::RemoveExtension(name);

		if ((type == ESystemItemType::Parameter) && (pParent->GetType() == ESystemItemType::Switch))
		{
			// Create a state instead of a parameter if the parent is a switch.
			type = ESystemItemType::State;
		}

		if (type != ESystemItemType::State)
		{
			name = Utils::GenerateUniqueControlName(name, type, *this);
		}
		else
		{
			name = Utils::GenerateUniqueName(name, type, pParent);
		}

		CSystemControl* const pControl = new CSystemControl(name, GenerateUniqueId(), type);
		pControl->SetParent(pParent);
		pParent->AddChild(pControl);
		m_controls.emplace_back(pControl);

		ConnectionPtr const pAudioConnection = g_pEditorImpl->CreateConnectionToControl(pControl->GetType(), pImplItem);

		if (pAudioConnection != nullptr)
		{
			pControl->AddConnection(pAudioConnection);
		}

		pItem = pControl;
	}
	else
	{
		// If the type of the control is invalid then it must be a folder or container
		name = Utils::GenerateUniqueName(name, ESystemItemType::Folder, pParent);
		CSystemFolder* const pFolder = new CSystemFolder(name);
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
	static Scope const globalScopeId = CryAudio::StringToId("global");
	return globalScopeId;
}

//////////////////////////////////////////////////////////////////////////
string GenerateUniqueName(string const& name, ESystemItemType const type, CSystemAsset* const pParent)
{
	string finalName = name;

	if (pParent != nullptr)
	{
		size_t const size = pParent->ChildCount();
		std::vector<string> names;
		names.reserve(size);

		for (size_t i = 0; i < size; ++i)
		{
			CSystemAsset const* const pChild = pParent->GetChild(i);

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
string GenerateUniqueLibraryName(string const& name, CSystemAssetsManager const& assetManager)
{
	size_t const size = assetManager.GetLibraryCount();
	std::vector<string> names;
	names.reserve(size);

	for (size_t i = 0; i < size; ++i)
	{
		CSystemLibrary const* const pLibrary = assetManager.GetLibrary(i);

		if (pLibrary != nullptr)
		{
			names.emplace_back(pLibrary->GetName());
		}
	}

	return PathUtil::GetUniqueName(name, names);
}

//////////////////////////////////////////////////////////////////////////
string GenerateUniqueControlName(string const& name, ESystemItemType const type, CSystemAssetsManager const& assetManager)
{
	CSystemAssetsManager::Controls const& controls(assetManager.GetControls());
	std::vector<string> names;
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
CSystemAsset* GetParentLibrary(CSystemAsset* pAsset)
{
	while ((pAsset != nullptr) && (pAsset->GetType() != ESystemItemType::Library))
	{
		pAsset = pAsset->GetParent();
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
void SelectTopLevelAncestors(std::vector<CSystemAsset*> const& source, std::vector<CSystemAsset*>& dest)
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
				CSystemAsset const* pParent = pItem->GetParent();

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
			dest.emplace_back(pItem);
		}
	}
}
} // namespace Utils
} // namespace ACE
