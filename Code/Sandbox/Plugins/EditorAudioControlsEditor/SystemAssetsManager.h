// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemAssets.h"

#include <CrySandbox/CrySignal.h>
#include <array>

namespace ACE
{
struct IImplItem;

class CSystemAssetsManager
{
public:

	CSystemAssetsManager();
	~CSystemAssetsManager();

	void            Initialize();
	void            Clear();

	CSystemLibrary* CreateLibrary(string const& name);
	CSystemLibrary* GetLibrary(size_t const index) const { return m_systemLibraries[index]; }
	size_t          GetLibraryCount() const              { return m_systemLibraries.size(); }

	CSystemAsset*   CreateFolder(string const& name, CSystemAsset* const pParent = nullptr);
	CSystemControl* CreateControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent = nullptr);
	void            CreateDefaultControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent, bool& wasModified, string const& description);
	void            DeleteItem(CSystemAsset* const pItem);

	CSystemControl* GetControlByID(CID const id) const;
	CSystemControl* FindControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent = nullptr) const;

	using Controls = std::vector<CSystemControl*>;
	Controls const& GetControls() const { return m_controls; }

	void            ClearScopes();
	void            AddScope(string const& name, bool const isLocalOnly = false);
	bool            ScopeExists(string const& name) const;
	Scope           GetScope(string const& name) const;
	SScopeInfo      GetScopeInfo(Scope const id) const;
	void            GetScopeInfoList(ScopeInfoList& scopeList) const;

	void            ClearAllConnections();
	void            ReloadAllConnections();
	void            MoveItems(CSystemAsset* const pParent, std::vector<CSystemAsset*> const& items);
	void            CreateAndConnectImplItems(IImplItem* const pImplItem, CSystemAsset* const pParent);

	bool            IsTypeDirty(ESystemItemType const type) const;
	bool            IsDirty() const;
	void            ClearDirtyFlags();
	bool            IsLoading() const { return m_isLoading; }

	void            SetAssetModified(CSystemAsset* const pAsset, bool const isModified);
	void            SetTypeModified(ESystemItemType const type, bool const isModified);

	void            UpdateAllConnectionStates();
	void            UpdateLibraryConnectionStates(CSystemAsset* pAsset);
	void            UpdateAssetConnectionStates(CSystemAsset* const pAsset);

	void            OnControlAboutToBeModified(CSystemControl* const pControl);
	void            OnControlModified(CSystemControl* const pControl);
	void            OnConnectionAdded(CSystemControl* const pControl, IImplItem* const pImplItem);
	void            OnConnectionRemoved(CSystemControl* const pControl, IImplItem* const pImplItem);
	void            OnAssetRenamed();

	void            UpdateFolderPaths();
	string const&   GetConfigFolderPath() const;
	string const&   GetAssetFolderPath() const;

	using ModifiedLibraryNames = std::set<string>;
	ModifiedLibraryNames GetModifiedLibraries() const { return m_modifiedLibraryNames; }

	CCrySignal<void(bool)>                         SignalIsDirty;
	CCrySignal<void()>                             SignalLibraryAboutToBeAdded;
	CCrySignal<void(CSystemLibrary*)>              SignalLibraryAdded;
	CCrySignal<void(CSystemLibrary*)>              SignalLibraryAboutToBeRemoved;
	CCrySignal<void()>                             SignalLibraryRemoved;
	CCrySignal<void(CSystemAsset*)>                SignalItemAboutToBeAdded;
	CCrySignal<void(CSystemAsset*)>                SignalItemAdded;
	CCrySignal<void(CSystemAsset*)>                SignalItemAboutToBeRemoved;
	CCrySignal<void(CSystemAsset*, CSystemAsset*)> SignalItemRemoved;
	CCrySignal<void()>                             SignalAssetRenamed;
	CCrySignal<void(CSystemControl*)>              SignalControlModified;
	CCrySignal<void(CSystemControl*)>              SignalConnectionAdded;
	CCrySignal<void(CSystemControl*)>              SignalConnectionRemoved;

private:

	CSystemAsset* CreateAndConnectImplItemsRecursively(IImplItem* const pImplItem, CSystemAsset* const pParent);
	CID           GenerateUniqueId() { return m_nextId++; }

	using ScopesInfo = std::map<Scope, SScopeInfo>;
	using SystemLibraries = std::vector<CSystemLibrary*>;
	using ModifiedSystemTypes = std::set<ESystemItemType>;

	SystemLibraries      m_systemLibraries;
	static CID           m_nextId;
	Controls             m_controls;
	ScopesInfo           m_scopes;
	ModifiedSystemTypes  m_modifiedTypes;
	ModifiedLibraryNames m_modifiedLibraryNames;
	bool                 m_isLoading = false;

	string               m_configFolderPath;
	string               m_assetFolderPath;
};

namespace Utils
{
Scope         GetGlobalScope();
string        GenerateUniqueName(string const& name, ESystemItemType const type, CSystemAsset* const pParent);
string        GenerateUniqueLibraryName(string const& name, CSystemAssetsManager const& assetManager);
string        GenerateUniqueControlName(string const& name, ESystemItemType const type, CSystemAssetsManager const& assetManager);
CSystemAsset* GetParentLibrary(CSystemAsset* pAsset);
void          SelectTopLevelAncestors(std::vector<CSystemAsset*> const& source, std::vector<CSystemAsset*>& dest);
} // namespace Utils
} // namespace ACE
