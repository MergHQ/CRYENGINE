// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CAbstractMenu;
class CObjectArchive;

struct IObjectLayerManager
{
	virtual ~IObjectLayerManager() {}

	//! Find layer by layer GUID.
	virtual IObjectLayer* FindLayer(CryGUID guid) const = 0;

	//! Search for layer by name.
	virtual IObjectLayer* FindLayerByFullName(const string& layerFullName) const = 0;

	//! Search for layer by name.
	virtual IObjectLayer* FindLayerByName(const string& layerName) const = 0;

	//! Get this layer is current.
	virtual IObjectLayer* GetCurrentLayer() const = 0;
	
	//! Delete layer from layer manager.
	virtual bool          DeleteLayer(IObjectLayer* pLayer, bool bNotify = true, bool deleteFileOnSave = true) = 0;

	//! Reloads the layer from disk
	virtual bool          ReloadLayer(IObjectLayer* pLayer) = 0;

	//! Import layer from file. If globalArchive, caller is responsible for both Object and Layer resolution!
	virtual IObjectLayer* ImportLayerFromFile(const string& filename, bool bNotify = true, CObjectArchive* globalArchive = nullptr) = 0;

	//! Get a list of all managed layers.
	virtual const std::vector<IObjectLayer*>& GetLayers() const = 0;

	CCrySignal<void(CAbstractMenu&, const std::vector<CBaseObject*>&, const std::vector<IObjectLayer*>& layers, const std::vector<IObjectLayer*>& folders)> signalContextMenuRequested;
	CCrySignal<void(IObjectLayer& layer)> signalLayerSaved;
};
