// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AssetsManager.h"

#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

#include <IEditorImpl.h>
#include <IImplItem.h>

#include <CryAudio/IAudioSystem.h>
#include <CryString/StringUtils.h>
#include <CryString/CryPath.h>
#include <FilePathUtil.h>
#include <IEditor.h>

namespace ACE
{
ControlId CAssetsManager::m_nextId = 1;

//////////////////////////////////////////////////////////////////////////
CAssetsManager::CAssetsManager()
{
	ClearDirtyFlags();
	m_scopes[Utils::GetGlobalScope()] = SScopeInfo("global", false);
	m_controls.reserve(8192);
}

//////////////////////////////////////////////////////////////////////////
CAssetsManager::~CAssetsManager()
{
	g_implementationManager.SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_implementationManager.SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::SignalAboutToLoad.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::SignalLoaded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::Initialize()
{
	g_implementationManager.SignalImplementationAboutToChange.Connect([this]()
		{
			ClearAllConnections();
	  }, reinterpret_cast<uintptr_t>(this));

	g_implementationManager.SignalImplementationChanged.Connect([this]()
		{
			m_isLoading = false;
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
CControl* CAssetsManager::CreateControl(string const& name, EAssetType const type, CAsset* const pParent /*= nullptr*/)
{
	CControl* pControl = nullptr;

	if ((pParent != nullptr) && !name.empty())
	{
		EAssetType const parentType = pParent->GetType();

		if (((parentType == EAssetType::Folder) || (parentType == EAssetType::Library)) || ((parentType == EAssetType::Switch) && (type == EAssetType::State)))
		{
			CControl* const pFoundControl = FindControl(name, type, pParent);

			if (pFoundControl != nullptr)
			{
				pControl = pFoundControl;
			}
			else
			{
				SignalItemAboutToBeAdded(pParent);

				auto const pNewControl = new CControl(name, GenerateUniqueId(), type);

				m_controls.push_back(pNewControl);

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
void CAssetsManager::CreateDefaultControl(string const& name, EAssetType const type, CAsset* const pParent, bool& wasModified, string const& description)
{
	CControl* pControl = FindControl(name, type);

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
void CAssetsManager::DeleteItem(CAsset* const pAsset)
{
	if (pAsset != nullptr)
	{
		// Recursively remove all children as well
		while (pAsset->ChildCount() > 0)
		{
			DeleteItem(pAsset->GetChild(0));
		}

		EAssetType const type = pAsset->GetType();

		// Inform that we're about to remove the item
		if (type == EAssetType::Library)
		{
			SignalLibraryAboutToBeRemoved(static_cast<CLibrary*>(pAsset));
		}
		else
		{
			SignalItemAboutToBeRemoved(pAsset);
		}

		// Remove/detach item from the tree
		CAsset* const pParent = pAsset->GetParent();

		if (pParent != nullptr)
		{
			pParent->RemoveChild(pAsset);
			pAsset->SetParent(nullptr);
		}

		if (type == EAssetType::Library)
		{
			m_libraries.erase(std::remove(m_libraries.begin(), m_libraries.end(), static_cast<CLibrary*>(pAsset)), m_libraries.end());
			SignalLibraryRemoved();
		}
		else
		{
			if (type != EAssetType::Folder)
			{
				// Must be a control
				CControl* const pControl = static_cast<CControl*>(pAsset);

				if (pControl != nullptr)
				{
					pControl->ClearConnections();
					m_controls.erase(std::remove_if(m_controls.begin(), m_controls.end(), [&](auto pIterControl) { return pIterControl->GetId() == pControl->GetId(); }), m_controls.end());
					SetTypeModified(type, true);
				}
			}

			SignalItemRemoved(pParent, pAsset);
		}

		pAsset->SetModified(true);
		delete pAsset;
	}
}

//////////////////////////////////////////////////////////////////////////
CControl* CAssetsManager::GetControlByID(ControlId const id) const
{
	CControl* pSystemControl = nullptr;

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
void CAssetsManager::ClearScopes()
{
	m_scopes.clear();

	// The global scope must always exist
	m_scopes[Utils::GetGlobalScope()] = SScopeInfo("global", false);
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::AddScope(string const& name, bool const isLocalOnly /*= false*/)
{
	m_scopes[CryAudio::StringToId(name.c_str())] = SScopeInfo(name, isLocalOnly);
}

//////////////////////////////////////////////////////////////////////////
bool CAssetsManager::ScopeExists(string const& name) const
{
	return m_scopes.find(CryAudio::StringToId(name.c_str())) != m_scopes.end();
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::GetScopeInfoList(ScopeInfoList& scopeList) const
{
	stl::map_to_vector(m_scopes, scopeList);
}

//////////////////////////////////////////////////////////////////////////
Scope CAssetsManager::GetScope(string const& name) const
{
	return CryAudio::StringToId(name.c_str());
}

//////////////////////////////////////////////////////////////////////////
SScopeInfo CAssetsManager::GetScopeInfo(Scope const id) const
{
	return stl::find_in_map(m_scopes, id, SScopeInfo("", false));
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::Clear()
{
	std::vector<CLibrary*> const libraries = m_libraries;

	for (auto const pLibrary : libraries)
	{
		DeleteItem(pLibrary);
	}

	CRY_ASSERT_MESSAGE(m_controls.empty(), "m_controls is not empty.");
	CRY_ASSERT_MESSAGE(m_libraries.empty(), "m_systemLibraries is not empty.");
	ClearScopes();
	ClearDirtyFlags();
}

//////////////////////////////////////////////////////////////////////////
CLibrary* CAssetsManager::CreateLibrary(string const& name)
{
	CLibrary* pSystemLibrary = nullptr;
	bool foundLibrary = false;

	if (!name.empty())
	{
		size_t const size = m_libraries.size();

		for (size_t i = 0; i < size; ++i)
		{
			CLibrary* const pLibrary = m_libraries[i];

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
			auto const pLibrary = new CLibrary(name);
			m_libraries.push_back(pLibrary);
			SignalLibraryAdded(pLibrary);
			pLibrary->SetModified(true);
			pSystemLibrary = pLibrary;
		}
	}

	return pSystemLibrary;
}

//////////////////////////////////////////////////////////////////////////
CAsset* CAssetsManager::CreateFolder(string const& name, CAsset* const pParent /*= nullptr*/)
{
	CAsset* pAsset = nullptr;
	bool foundFolder = false;

	if ((pParent != nullptr) && !name.empty())
	{
		if ((pParent->GetType() == EAssetType::Folder) || (pParent->GetType() == EAssetType::Library))
		{
			size_t const size = pParent->ChildCount();

			for (size_t i = 0; i < size; ++i)
			{
				CAsset* const pChild = pParent->GetChild(i);

				if ((pChild != nullptr) && (pChild->GetType() == EAssetType::Folder) && (name.compareNoCase(pChild->GetName()) == 0))
				{
					pAsset = pChild;
					foundFolder = true;
					break;
				}
			}

			if (!foundFolder)
			{
				auto const pFolder = new CFolder(name);

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
void CAssetsManager::OnControlAboutToBeModified(CControl* const pControl)
{
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::OnConnectionAdded(CControl* const pControl, IImplItem* const pIImplItem)
{
	SignalConnectionAdded(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::OnConnectionRemoved(CControl* const pControl, IImplItem* const pIImplItem)
{
	SignalConnectionRemoved(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::OnAssetRenamed(CAsset* const pAsset)
{
	SignalAssetRenamed(pAsset);
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::UpdateFolderPaths()
{
	string const rootPath = AUDIO_SYSTEM_DATA_ROOT "/";

	if (g_pEditorImpl != nullptr)
	{
		string const& implFolderPath = rootPath + g_pEditorImpl->GetFolderName() + "/";
		m_configFolderPath = implFolderPath + CryAudio::s_szConfigFolderName + "/";
		m_assetFolderPath = implFolderPath + CryAudio::s_szAssetsFolderName + "/";
	}
	else
	{
		m_configFolderPath = rootPath;
		m_assetFolderPath = rootPath;
	}
}
//////////////////////////////////////////////////////////////////////////
string const& CAssetsManager::GetConfigFolderPath() const
{
	return m_configFolderPath;
}

//////////////////////////////////////////////////////////////////////////
string const& CAssetsManager::GetAssetFolderPath() const
{
	return m_assetFolderPath;
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::OnControlModified(CControl* const pControl)
{
	SignalControlModified(pControl);
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::SetAssetModified(CAsset* const pAsset, bool const isModified)
{
	if (isModified)
	{
		UpdateLibraryConnectionStates(pAsset);
		auto const type = pAsset->GetType();

		m_modifiedTypes.emplace(type);

		if (type == EAssetType::Library)
		{
			m_modifiedLibraryNames.emplace(pAsset->GetName());
		}

		SignalIsDirty(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::SetTypeModified(EAssetType const type, bool const isModified)
{
	if (isModified)
	{
		m_modifiedTypes.emplace(type);
	}
}
//////////////////////////////////////////////////////////////////////////
bool CAssetsManager::IsDirty() const
{
	return !m_modifiedTypes.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CAssetsManager::IsTypeDirty(EAssetType const type) const
{
	return m_modifiedTypes.find(type) != m_modifiedTypes.end();
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::ClearDirtyFlags()
{
	m_modifiedTypes.clear();
	m_modifiedLibraryNames.clear();
	SignalIsDirty(false);
}

//////////////////////////////////////////////////////////////////////////
CControl* CAssetsManager::FindControl(string const& name, EAssetType const type, CAsset* const pParent /*= nullptr*/) const
{
	CControl* pSystemControl = nullptr;

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
			auto const pControl = static_cast<CControl*>(pParent->GetChild(i));

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
void CAssetsManager::ClearAllConnections()
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
void CAssetsManager::BackupAndClearAllConnections()
{
	m_isLoading = true;

	for (auto const pControl : m_controls)
	{
		if (pControl != nullptr)
		{
			pControl->BackupAndClearConnections();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::ReloadAllConnections()
{
	for (auto const pControl : m_controls)
	{
		if (pControl != nullptr)
		{
			pControl->ReloadConnections();
		}
	}

	m_isLoading = false;

	UpdateAllConnectionStates();
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::UpdateAllConnectionStates()
{
	for (auto const pLibrary : m_libraries)
	{
		UpdateAssetConnectionStates(pLibrary);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::UpdateLibraryConnectionStates(CAsset* pAsset)
{
	while ((pAsset != nullptr) && (pAsset->GetType() != EAssetType::Library))
	{
		pAsset = pAsset->GetParent();
	}

	if (pAsset != nullptr)
	{
		UpdateAssetConnectionStates(pAsset);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::UpdateAssetConnectionStates(CAsset* const pAsset)
{
	if (pAsset != nullptr)
	{
		EAssetType const type = pAsset->GetType();

		if ((type == EAssetType::Library) || (type == EAssetType::Folder) || (type == EAssetType::Switch))
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
		else if (type != EAssetType::None)
		{
			CControl* const pControl = static_cast<CControl*>(pAsset);

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
						IImplItem const* const pIImplItem = g_pEditorImpl->GetItem(pControl->GetConnectionAt(i)->GetID());

						if (pIImplItem != nullptr)
						{
							if (pIImplItem->IsPlaceholder())
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
void CAssetsManager::MoveItems(CAsset* const pParent, std::vector<CAsset*> const& assets)
{
	if (pParent != nullptr)
	{
		for (auto const pAsset : assets)
		{
			if (pAsset != nullptr)
			{
				CAsset* const pPreviousParent = pAsset->GetParent();

				if (pPreviousParent != nullptr)
				{
					SignalItemAboutToBeRemoved(pAsset);
					pPreviousParent->RemoveChild(pAsset);
					pAsset->SetParent(nullptr);
					SignalItemRemoved(pPreviousParent, pAsset);
					pPreviousParent->SetModified(true);
				}

				SignalItemAboutToBeAdded(pParent);
				EAssetType const type = pAsset->GetType();

				if ((type == EAssetType::State) || (type == EAssetType::Folder))
				{
					// To prevent duplicated names of states and folders.
					pAsset->UpdateNameOnMove(pParent);
				}

				pParent->AddChild(pAsset);
				pAsset->SetParent(pParent);
				SignalItemAdded(pAsset);
				pAsset->SetModified(true);
			}
		}

		pParent->SetModified(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAssetsManager::CreateAndConnectImplItems(IImplItem* const pIImplItem, CAsset* const pParent)
{
	SignalItemAboutToBeAdded(pParent);
	CAsset* pAsset = CreateAndConnectImplItemsRecursively(pIImplItem, pParent);
	SignalItemAdded(pAsset);
}

//////////////////////////////////////////////////////////////////////////
CAsset* CAssetsManager::CreateAndConnectImplItemsRecursively(IImplItem* const pIImplItem, CAsset* const pParent)
{
	CAsset* pAsset = nullptr;

	// Create the new control and connect it to the one dragged in externally
	string name = pIImplItem->GetName();
	EAssetType type = g_pEditorImpl->ImplTypeToAssetType(pIImplItem);

	if (type != EAssetType::None)
	{
		PathUtil::RemoveExtension(name);

		if ((type == EAssetType::Parameter) && (pParent->GetType() == EAssetType::Switch))
		{
			// Create a state instead of a parameter if the parent is a switch.
			type = EAssetType::State;
		}

		if (type != EAssetType::State)
		{
			name = Utils::GenerateUniqueControlName(name, type);
		}
		else
		{
			name = Utils::GenerateUniqueName(name, type, pParent);
		}

		auto const pControl = new CControl(name, GenerateUniqueId(), type);
		pControl->SetParent(pParent);
		pParent->AddChild(pControl);
		m_controls.push_back(pControl);

		ConnectionPtr const pAudioConnection = g_pEditorImpl->CreateConnectionToControl(pControl->GetType(), pIImplItem);

		if (pAudioConnection != nullptr)
		{
			pControl->AddConnection(pAudioConnection);
		}

		pAsset = pControl;
	}
	else
	{
		// If the type of the control is invalid then it must be a folder or container
		name = Utils::GenerateUniqueName(name, EAssetType::Folder, pParent);
		auto const pFolder = new CFolder(name);
		pParent->AddChild(pFolder);
		pFolder->SetParent(pParent);

		pAsset = pFolder;
	}

	size_t const size = pIImplItem->GetNumChildren();

	for (size_t i = 0; i < size; ++i)
	{
		CreateAndConnectImplItemsRecursively(pIImplItem->GetChildAt(i), pAsset);
	}

	return pAsset;
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
string GenerateUniqueName(string const& name, EAssetType const type, CAsset* const pParent)
{
	string finalName = name;

	if (pParent != nullptr)
	{
		size_t const size = pParent->ChildCount();
		std::vector<string> names;
		names.reserve(size);

		for (size_t i = 0; i < size; ++i)
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
	size_t const size = g_assetsManager.GetLibraryCount();
	std::vector<string> names;
	names.reserve(size);

	for (size_t i = 0; i < size; ++i)
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
	CAssetsManager::Controls const& controls(g_assetsManager.GetControls());
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
CAsset* GetParentLibrary(CAsset* pAsset)
{
	while ((pAsset != nullptr) && (pAsset->GetType() != EAssetType::Library))
	{
		pAsset = pAsset->GetParent();
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
void SelectTopLevelAncestors(std::vector<CAsset*> const& source, std::vector<CAsset*>& dest)
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
} // namespace Utils
} // namespace ACE
