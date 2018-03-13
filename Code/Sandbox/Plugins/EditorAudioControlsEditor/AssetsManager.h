// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Assets.h"

#include <CrySandbox/CrySignal.h>
#include <array>

namespace ACE
{
struct IImplItem;

class CAssetsManager final
{
public:

	CAssetsManager();
	~CAssetsManager();

	void      Initialize();
	void      Clear();

	CLibrary* CreateLibrary(string const& name);
	CLibrary* GetLibrary(size_t const index) const { return m_libraries[index]; }
	size_t    GetLibraryCount() const              { return m_libraries.size(); }

	CAsset*   CreateFolder(string const& name, CAsset* const pParent = nullptr);
	CControl* CreateControl(string const& name, EAssetType const type, CAsset* const pParent = nullptr);
	void      CreateDefaultControl(string const& name, EAssetType const type, CAsset* const pParent, bool& wasModified, string const& description);
	void      DeleteItem(CAsset* const pAsset);

	CControl* GetControlByID(ControlId const id) const;
	CControl* FindControl(string const& name, EAssetType const type, CAsset* const pParent = nullptr) const;

	using Controls = std::vector<CControl*>;
	Controls const& GetControls() const { return m_controls; }

	void            ClearScopes();
	void            AddScope(string const& name, bool const isLocalOnly = false);
	bool            ScopeExists(string const& name) const;
	Scope           GetScope(string const& name) const;
	SScopeInfo      GetScopeInfo(Scope const id) const;
	void            GetScopeInfoList(ScopeInfoList& scopeList) const;

	void            ClearAllConnections();
	void            BackupAndClearAllConnections();
	void            ReloadAllConnections();
	void            MoveItems(CAsset* const pParent, std::vector<CAsset*> const& assets);
	void            CreateAndConnectImplItems(IImplItem* const pIImplItem, CAsset* const pParent);

	bool            IsTypeDirty(EAssetType const type) const;
	bool            IsDirty() const;
	void            ClearDirtyFlags();
	bool            IsLoading() const { return m_isLoading; }

	void            SetAssetModified(CAsset* const pAsset, bool const isModified);
	void            SetTypeModified(EAssetType const type, bool const isModified);

	void            UpdateAllConnectionStates();
	void            UpdateLibraryConnectionStates(CAsset* pAsset);
	void            UpdateAssetConnectionStates(CAsset* const pAsset);

	void            OnControlAboutToBeModified(CControl* const pControl);
	void            OnControlModified(CControl* const pControl);
	void            OnConnectionAdded(CControl* const pControl, IImplItem* const pIImplItem);
	void            OnConnectionRemoved(CControl* const pControl, IImplItem* const pIImplItem);
	void            OnAssetRenamed(CAsset* const pAsset);

	void            UpdateFolderPaths();
	string const&   GetConfigFolderPath() const;
	string const&   GetAssetFolderPath() const;

	FileNames       GetModifiedLibraries() const { return m_modifiedLibraryNames; }

	CCrySignal<void(bool)>             SignalIsDirty;
	CCrySignal<void()>                 SignalLibraryAboutToBeAdded;
	CCrySignal<void(CLibrary*)>        SignalLibraryAdded;
	CCrySignal<void(CLibrary*)>        SignalLibraryAboutToBeRemoved;
	CCrySignal<void()>                 SignalLibraryRemoved;
	CCrySignal<void(CAsset*)>          SignalItemAboutToBeAdded;
	CCrySignal<void(CAsset*)>          SignalItemAdded;
	CCrySignal<void(CAsset*)>          SignalItemAboutToBeRemoved;
	CCrySignal<void(CAsset*, CAsset*)> SignalItemRemoved;
	CCrySignal<void(CAsset*)>          SignalAssetRenamed;
	CCrySignal<void(CControl*)>        SignalControlModified;
	CCrySignal<void(CControl*)>        SignalConnectionAdded;
	CCrySignal<void(CControl*)>        SignalConnectionRemoved;

private:

	CAsset*   CreateAndConnectImplItemsRecursively(IImplItem* const pIImplItem, CAsset* const pParent);
	ControlId GenerateUniqueId() { return m_nextId++; }

	using ScopesInfo = std::map<Scope, SScopeInfo>;
	using Libraries = std::vector<CLibrary*>;
	using ModifiedSystemTypes = std::set<EAssetType>;

	Libraries           m_libraries;
	static ControlId    m_nextId;
	Controls            m_controls;
	ScopesInfo          m_scopes;
	ModifiedSystemTypes m_modifiedTypes;
	FileNames           m_modifiedLibraryNames;
	bool                m_isLoading = false;

	string              m_configFolderPath;
	string              m_assetFolderPath;
};

namespace Utils
{
Scope   GetGlobalScope();
string  GenerateUniqueName(string const& name, EAssetType const type, CAsset* const pParent);
string  GenerateUniqueLibraryName(string const& name);
string  GenerateUniqueControlName(string const& name, EAssetType const type);
CAsset* GetParentLibrary(CAsset* pAsset);
void    SelectTopLevelAncestors(std::vector<CAsset*> const& source, std::vector<CAsset*>& dest);
} // namespace Utils
} // namespace ACE
