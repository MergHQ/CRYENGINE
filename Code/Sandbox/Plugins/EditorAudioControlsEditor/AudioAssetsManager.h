// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioAssets.h"

#include <CrySandbox/CrySignal.h>
#include <array>

#include <QVariant>

namespace ACE
{
class IAudioSystemItem;

static QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	if (orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	if (role == Qt::DisplayRole)
	{
		return "Name";
	}

	return QVariant();
}

class CAudioAssetsManager
{
	friend class IUndoControlOperation;
	friend class CUndoControlModified;
	friend class CAudioControl;

public:

	CAudioAssetsManager();
	~CAudioAssetsManager();

	void Initialize();
	void Clear();

	// Libraries
	CAudioLibrary* CreateLibrary(string const& name);
	CAudioLibrary* GetLibrary(size_t const index) const { return m_audioLibraries[index]; }
	size_t         GetLibraryCount() const              { return m_audioLibraries.size(); }

	//
	CAudioAsset*   CreateFolder(string const& name, CAudioAsset* const pParent = nullptr);
	CAudioControl* CreateControl(string const& controlName, EItemType const type, CAudioAsset* const pParent = nullptr);
	void           DeleteItem(CAudioAsset* const pItem);

	CAudioControl* GetControlByID(CID const id) const;
	CAudioControl* FindControl(string const& controlName, EItemType const type, CAudioAsset* const pParent = nullptr) const;

	using Controls = std::vector<CAudioControl*>;
	Controls const& GetControls() const { return m_controls; }

	// Scope
	void       ClearScopes();
	void       AddScope(string const& name, bool const isLocalOnly = false);
	bool       ScopeExists(string const& name) const;
	Scope      GetScope(string const& name) const;
	SScopeInfo GetScopeInfo(Scope const id) const;
	void       GetScopeInfoList(ScopeInfoList& scopeList) const;

	// Helper functions
	void ClearAllConnections();
	void ReloadAllConnections();
	void MoveItems(CAudioAsset* const pParent, std::vector<CAudioAsset*> const& items);
	void CreateAndConnectImplItems(IAudioSystemItem* const pImplItem, CAudioAsset* const pParent);

	bool IsTypeDirty(EItemType const type) const;
	bool IsDirty() const;
	void ClearDirtyFlags();
	bool IsLoading() const { return m_isLoading; }

	void SetAssetModified(CAudioAsset* const pAsset);

	void UpdateAllConnectionStates();
	void UpdateLibraryConnectionStates(CAudioAsset* pAsset);
	void UpdateAssetConnectionStates(CAudioAsset* const pAsset);

	// Dirty flags signal
	CCrySignal<void(bool)> signalIsDirty;

	// Library signals
	CCrySignal<void()>               signalLibraryAboutToBeAdded;
	CCrySignal<void(CAudioLibrary*)> signalLibraryAdded;
	CCrySignal<void(CAudioLibrary*)> signalLibraryAboutToBeRemoved;
	CCrySignal<void()>               signalLibraryRemoved;

	// Items signals
	CCrySignal<void(CAudioAsset*)>               signalItemAboutToBeAdded;
	CCrySignal<void(CAudioAsset*)>               signalItemAdded;
	CCrySignal<void(CAudioAsset*)>               signalItemAboutToBeRemoved;
	CCrySignal<void(CAudioAsset*, CAudioAsset*)> signalItemRemoved;

	CCrySignal<void(CAudioControl*)>             signalControlModified;
	CCrySignal<void(CAudioControl*)>             signalConnectionAdded;
	CCrySignal<void(CAudioControl*)>             signalConnectionRemoved;

private:

	CAudioAsset* CreateAndConnectImplItemsRecursively(IAudioSystemItem* const pImplItem, CAudioAsset* const pParent);
	void         OnControlAboutToBeModified(CAudioControl* const pControl);
	void         OnControlModified(CAudioControl* const pControl);
	void         OnConnectionAdded(CAudioControl* const pControl, IAudioSystemItem* const pMiddlewareControl);
	void         OnConnectionRemoved(CAudioControl* const pControl, IAudioSystemItem* const pMiddlewareControl);

	CID          GenerateUniqueId() { return m_nextId++; }

	std::vector<CAudioLibrary*> m_audioLibraries;

	static CID                  m_nextId;
	Controls                    m_controls;
	std::map<Scope, SScopeInfo> m_scopeMap;
	std::vector<EItemType>      m_controlTypesModified;
	bool                        m_isLoading = false;
};

namespace Utils
{
Scope         GetGlobalScope();
string        GenerateUniqueName(string const& name, EItemType const type, CAudioAsset* const pParent);
string        GenerateUniqueLibraryName(string const& name, CAudioAssetsManager const& assetManager);
string        GenerateUniqueControlName(string const& name, EItemType const type, CAudioAssetsManager const& assetManager);
CAudioAsset*  GetParentLibrary(CAudioAsset* pAsset);
void          SelectTopLevelAncestors(std::vector<CAudioAsset*> const& source, std::vector<CAudioAsset*>& dest);
string const& GetAssetFolder();
} // namespace Utils
} // namespace ACE
