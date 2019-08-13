// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ObjectLayer.h"
#include <Objects/IObjectLayerManager.h>
#include <CrySandbox/CrySignal.h>
#include <CryExtension/CryGUID.h>

class CObjectArchive;
class CObjectManager;

namespace Private_ObjectLayerManager
{
	class CUndoLayerCreateDelete;
}

struct CLayerChangeEvent
{
	enum EventType
	{
		LE_BEFORE_ADD,
		LE_AFTER_ADD,
		LE_BEFORE_REMOVE,
		LE_AFTER_REMOVE,
		LE_MODIFY,
		LE_VISIBILITY,
		LE_FROZEN,
		LE_RENAME,
		LE_LAYERCOLOR,
		LE_SELECT,
		LE_BEFORE_PARENT_CHANGE,
		LE_AFTER_PARENT_CHANGE,
		LE_UPDATE_ALL //no assumptions, everything could have changed
	};

	CLayerChangeEvent(EventType type, CObjectLayer* layer = nullptr, CObjectLayer* parent = nullptr)
		: m_type(type), m_layer(layer), m_savedParent(parent) {}

	void Send() const;

	EventType     m_type;
	CObjectLayer* m_layer;       //valid for all events except UPDATEALL
	CObjectLayer* m_savedParent; //only valid for PARENT_CHANGE events, represents NEW parent for BEFORE events, OLD parent for AFTER events
};

/** Manager of objects layers.
    Instance of these hosted by CObjectManager class.
 */
class SANDBOX_API CObjectLayerManager : public IObjectLayerManager, public IEditorNotifyListener
{
public:
	static const char* GetLayerExtension() { return ".lyr"; }

	CCrySignal<void(const CLayerChangeEvent&)> signalChangeEvent;

	CObjectLayerManager(CObjectManager* pObjectManager);
	virtual ~CObjectLayerManager();

	void OnEditorNotifyEvent(EEditorNotifyEvent event);

	//! Mark level layer model for backing up. Modified state should not be altered when generating a backup
	void          StartBackup() { m_bCanModifyLayers = false; }
	//! Mark level layer model for backing up. Modified state should not be altered when generating a backup
	void          EndBackup()   { m_bCanModifyLayers = true; }
	//! Ensures that the editor has finished loading a level before allowing layers to be modified.
	bool          CanModifyLayers() const;
	//! Create a layer of the given type and attach it to the parent layer if provided
	CObjectLayer* CreateLayer(EObjectLayerType layerType = eObjectLayerType_Layer, CObjectLayer* pParent = nullptr);
	//! Create a layer of the given name and type, and attach it to the parent layer if provided
	CObjectLayer* CreateLayer(const char* szName, EObjectLayerType layerType = eObjectLayerType_Layer, CObjectLayer* pParent = nullptr);
	//! Check if it's possible to delete a layer.
	bool          CanDeleteLayer(CObjectLayer* pLayer);
	//! Delete layer from layer manager.
	bool          DeleteLayer(IObjectLayer* pLayer, bool bNotify = true, bool deleteFileOnSave = true) override;
	//! Delete all layers from layer manager.
	void          ClearLayers(bool bNotify = true);
	//! Returns true if there are no layers. Note that this should not happen in normal circumstances
	uint          HasLayers() const;

	//! Can layer be renamed to the new name
	bool CanRename(const CObjectLayer* pLayer, const char* szNewName) const;
	//! Sets layer's name. Also adds path to set to be removed on save
	void SetLayerName(CObjectLayer* pLayer, const char* szNewName, bool IsUpdateDepends = false);

	//! Can layer be reparented to new parent
	bool CanMove(const CObjectLayer* pLayer, const CObjectLayer* pTargetParent) const;

	//! Find layer by layer GUID.
	virtual CObjectLayer* FindLayer(CryGUID guid) const override;

	//! Search for layer by name.
	virtual CObjectLayer* FindLayerByFullName(const string& layerFullName) const override;

	//! Search for layer by name, where the layer type is eObjectLayerType_Layer.
	virtual CObjectLayer* FindLayerByName(const string& layerName) const override;

	//! Search for folder by name, where where the folder type is eObjectLayerType_Folder.
	CObjectLayer* FindFolderByName(const string& layerName) const;

	//! Search for layer by name independently of type.
	CObjectLayer* FindAnyLayerByName(const string& layerName) const;

	//! Search for layer by name and type.
	CObjectLayer* FindLayerByNameAndType(const string& layerName, EObjectLayerType type) const;

	//! Returns true if at least one layer matches the type.
	bool IsAnyLayerOfType(EObjectLayerType type) const;

	//! Get a list of all managed layers.
	virtual const std::vector<IObjectLayer*>& GetLayers() const override;
	
	virtual IObjectLayer* GetLayerByFileIfOpened(const string& layerFile) const override;

	virtual bool          IsLayerFileOfOpenedLevel(const string& layerFile) const override;

	//! Set this layer is current.
	void                   SetCurrentLayer(CObjectLayer* pCurrLayer);
	virtual CObjectLayer*  GetCurrentLayer() const override;

	const string& GetLayersPath() const { return m_layersPath; }
	void          SetLayersPath(const string& layersPath) { m_layersPath = layersPath; }

	void          NotifyLayerChange(const CLayerChangeEvent& event);

	//! Export layer to objects archive.
	void          ExportLayer(CObjectArchive& ar, CObjectLayer* pLayer, bool bExportExternalChilds);
	//! Import layer from objects archive.
	CObjectLayer* ImportLayer(CObjectArchive& ar, const string& filePath, bool bNotify = true);

	//! Import layer from file. If globalArchive, caller is responsible for both Object and Layer resolution!
	CObjectLayer* ImportLayerFromFile(const string& filePath, bool bNotify = true, CObjectArchive* globalArchive = nullptr) override;

	// Serialize layer manager (called from object manager).
	void Serialize(CObjectArchive& ar);

	//! Resolves layer's parent.
	void ResolveParentFor(const string& fullName, CObjectLayer* pLayer, bool notify = true);

	bool InitLayerSwitches(bool isOnlyClear = false);
	void ExportLayerSwitches(XmlNodeRef& node);
	void SetupLayerSwitches(bool isOnlyClear = false, bool isOnlyRenderNodes = false);

	void SetGameMode(bool inGame);

	//! Freeze read-only layers.
	void FreezeROnly();
	//! Returns if the item can be isolated or if it's already isolated
	bool ShouldToggleFreezeAllBut(CObjectLayer* pLayer) const;
	//! Freeze all layers except for the one specified
	void ToggleFreezeAllBut(CObjectLayer* pLayer);
	//! Returns if the item can be isolated or if it's already isolated
	bool ShouldToggleHideAllBut(CObjectLayer* pLayer) const;
	//! Hide all layers except for the one specified
	void ToggleHideAllBut(CObjectLayer* pLayer);

	void SetAllFrozen(bool bFrozen);
	void SetAllVisible(bool bVisible);

	//! Reloads the layer from disk
	bool ReloadLayer(IObjectLayer* pLayer) override;

	//! Saves the layer to disk
	void       SaveLayer(CObjectLayer* pLayer);

	XmlNodeRef GenerateDynTexSrcLayerInfo() const;

	// iterate over all layers and make sure all render nodes have valid IDs
	void AssignLayerIDsToRenderNodes();

	//! Returns the list of file that belong to layers.
	std::vector<string> GetFiles() const;

private:
	// friend undo because we don't want anyone adding layers to the layer manager. If a layer needs to be added, then call Create
	friend class Private_ObjectLayerManager::CUndoLayerCreateDelete;

	//! Add already created layer to manager.
	bool          AddLayer(CObjectLayer* pLayer, bool bNotify = true, bool loading = false);
	CObjectLayer* CreateLayerInstance() { return new CObjectLayer(); }

	void          SaveLayer(CObjectArchive* pArchive, CObjectLayer* pLayer);
	CObjectLayer* FindOrCreateFolderChain(const string& folderChain, bool bNotify = true);

	//! Physically deletes layers that pend deletion.
	void DeletePendingLayers();

	//////////////////////////////////////////////////////////////////////////
	//! Map of layer GUID to layer pointer.
	typedef std::map<CryGUID, TSmartPtr<CObjectLayer>> LayersMap;
	LayersMap m_layersMap;

	// At the moment this is the simplest implementation of layers' caching.
	// Ideally we would need to make it work together with the map maybe by having map keep indices of layers in the vector
	// instead of pointers. Maintaining this would be more involved. That's why for now we leave the simple solution
	mutable std::vector<IObjectLayer*> m_layersCache;
	//! List of layer paths to be deleted on save (product of a rename)
	std::set<string> m_toBeDeleted;

	//! Pointer to currently active layer.
	TSmartPtr<CObjectLayer> m_pCurrentLayer;
	//////////////////////////////////////////////////////////////////////////

	CObjectManager* m_pObjectManager;
	//! Layers path relative to level folder.
	string          m_layersPath;

	bool            m_bOverwriteDuplicates;
	bool            m_bCanModifyLayers;

	// support Layer Switches in Flow Graph
	std::vector<CObjectLayer*> m_layerSwitches;

	// visible set for restoring states in Isolate()
	CryGUID m_visibleSetLayer;
};
