// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SandboxAPI.h"
#include "Terrain/Heightmap.h"
#include "Terrain/Layer.h"
#include "Terrain/SurfaceType.h"
#include "DocMultiArchive.h"
#include <CrySandbox/CrySignal.h>
#include <CryCore/smartptr.h>
#include <vector>

class CSurfaceType;

class SANDBOX_API CTerrainManager
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Surface Types.
	CSurfaceType* GetSurfaceTypePtr(int i) const { if (i >= 0 && i < m_surfaceTypes.size()) return m_surfaceTypes[i]; return nullptr; }
	int           GetSurfaceTypeCount() const    { return m_surfaceTypes.size(); }
	//! Find surface type by name, return -1 if name not found.
	int           FindSurfaceType(const string& name);
	void          RemoveSurfaceType(CSurfaceType* pSrfType);
	int           AddSurfaceType(CSurfaceType* srf);
	void          RemoveAllSurfaceTypes();
	void          SerializeSurfaceTypes(CXmlArchive& xmlAr, bool bUpdateEngineTerrain = true);
	void          ConsolidateSurfaceTypes();

	// Put surface types from Editor to game.
	void ReloadSurfaceTypes(bool bUpdateEngineTerrain = true, bool bUpdateHeightmap = true);

	//////////////////////////////////////////////////////////////////////////
	// Layers
	int     GetLayerCount() const     { return static_cast<int>(m_layers.size()); }

	CLayer* GetLayer(int layer) const { return m_layers[layer]; }
	CLayer* FindLayer(const char* sLayerName) const;
	CLayer* FindLayerByLayerId(const uint32 dwLayerId) const;

	void    SelectLayer(int layerIndex);
	int     GetSelectedLayerIndex();
	CLayer* GetSelectedLayer();

	// If index == -1, place it as the last one
	void AddLayer(CLayer* pLayer, int index = -1);
	void RemoveSelectedLayer();
	void DuplicateSelectedLayer();
	void MoveLayer(int oldPos, int newPos);
	void InvalidateLayers();
	void ClearLayers();
	void SerializeLayerSettings(CXmlArchive& xmlAr);
	void MarkUsedLayerIds(bool bFree[e_layerIdUndefined]) const;

	void CreateDefaultLayer();

	// slow
	// Returns:
	//   0xffffffff if the method failed (internal error)
	uint32 GetDetailIdLayerFromLayerId(const uint32 dwLayerId);

	// needed to convert from old layer structure to the new one
	bool ConvertLayersToRGBLayer();

	//////////////////////////////////////////////////////////////////////////
	// Heightmap
	CHeightmap*      GetHeightmap()      { return &m_heightmap; }
	CRGBLayer*       GetRGBLayer();
	const CRGBLayer* GetRGBLayer() const { return const_cast<CTerrainManager*>(this)->GetRGBLayer(); }

	void             SetTerrainSize(int resolution, float unitSize);
	void             ResetHeightMap();

	void             Save(bool bBackup = false);
	bool             Load();

	void             SaveTexture(bool bBackup = false);
	bool             LoadTexture();

	void             SerializeTerrain(TDocMultiArchive& arrXmlAr);
	void             SerializeTerrain(CXmlArchive& xmlAr);
	void             SerializeTexture(CXmlArchive& xmlAr);

	void             SetModified(int x1 = 0, int y1 = 0, int x2 = 0, int y2 = 0);

	void             GetTerrainMemoryUsage(ICrySizer* pSizer);
	//////////////////////////////////////////////////////////////////////////

	// Get the number of terrain data files in the level data folder.
	// \sa GetIEditorImpl()->GetLevelDataFolder().
	int         GetDataFilesCount() const;
	const char* GetDataFilename(int i) const;

	CCrySignal<void(CLayer*)> signalLayerAboutToDelete;
	CCrySignal<void()>        signalLayersChanged;
	CCrySignal<void()>        signalTerrainChanged;
	CCrySignal<void(CLayer*)> signalSelectedLayerChanged;

	static const int          s_invalidLayerIndex = -1;

private:
	std::vector<_smart_ptr<CSurfaceType>> m_surfaceTypes;
	std::vector<CLayer*>                  m_layers;
	CHeightmap                            m_heightmap;
};
