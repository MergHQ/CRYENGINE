// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "VegetationObject.h"
#include <Util/Image.h>
#include <CrySandbox/CrySignal.h>
#include <list>
#include <unordered_map>
#include <vector>

#define RAD2BYTE(x) ((x) * 255.0f / float(g_PI2))
#define BYTE2RAD(x) ((x) * float(g_PI2) / 255.0f)

//////////////////////////////////////////////////////////////////////////
/** CVegetationMap stores static objects distributed over terrain.
    It keeps a list of all allocated geometry objects and all places instances.
 */
class SANDBOX_API CVegetationMap
{
public:
	CVegetationMap();
	~CVegetationMap();

	//////////////////////////////////////////////////////////////////////////
	// Map
	//////////////////////////////////////////////////////////////////////////
	//! Allocate sectors map.
	//! @param mapSize is size of terrain in meters.
	void Allocate(int nMapSize, bool bKeepData);

	//! Get number of sectors at side.
	int GetNumSectors() const { return m_numSectors; }

	//! Get total Size of vegetation map.
	int GetSize() const;

	//! Convert world coordinate to sector coordinate.
	int WorldToSector(float worldCoord) const;

	//! Place all objects in vegetation map to 3d engine terrain.
	void PlaceObjectsOnTerrain();

	//! Get total number of vegetation instances.
	int GetNumInstances() const { return m_numInstances; }

	//////////////////////////////////////////////////////////////////////////
	// Vegetation Objects
	//////////////////////////////////////////////////////////////////////////
	//! Get number of use vegetation objects.
	int                GetObjectCount() const { return m_objects.size(); }
	//! Get vegetation object.
#pragma push_macro("GetObject")
#undef GetObject
	CVegetationObject* GetObject(int i) const { return m_objects[i]; }
#pragma pop_macro("GetObject")
	//! Get vegetation object by id.
	CVegetationObject* GetObjectById(int id) const;

	//! Create new object.
	//! @param prev Source object to clone from.
	CVegetationObject* CreateObject(CVegetationObject* prev = 0);

	// Generates a vegetation object id, when available.
	// If no vegetation object id is available, returns -1.
	int GenerateVegetationObjectId();

	// Inserts an object, assigning a new ID to it.
	// Should be used only with objects which ID is not registered.
	// One such case, is while Undo.
	bool InsertObject(CVegetationObject* obj);

	//! Remove object.
	void RemoveObject(CVegetationObject* obj);
	//! Replace one vegetation object with another.
	void ReplaceObject(CVegetationObject* pOldObject, CVegetationObject* pNewObject);
	//! Remove all objects.
	void ClearObjects();

	//! Hide all instances of this object.
	void HideObject(CVegetationObject* object, bool bHide);
	void HideAllObjects(bool bHide);

	//! Merge 2 object to first one
	void CVegetationMap::MergeObjects(CVegetationObject* object, CVegetationObject* objectMerged);

	//! Save vegetation to xml file
	void Save(bool bBackup = false);

	//! Load vegetation from xml file
	bool Load();

	//! Export static object and all its instances.
	//! @return Number of serialized instances.
	int  ExportObject(CVegetationObject* object, XmlNodeRef& node, CRect* saveRect = NULL);
	void ImportObject(XmlNodeRef& node, const Vec3& offset = Vec3(0, 0, 0));

	void ImportObjectsFromXml(const string& filename);
	void ExportObjectsToXml(const string& filename);

	//! Export part of vegetation map.
	void ExportBlock(const CRect& rect, CXmlArchive& ar);
	//! Import part of vegetation map.
	void ImportBlock(CXmlArchive& ar, CPoint placeOffset = CPoint(0, 0));

	//! Unload all rendering geometry from objects.
	void UnloadObjectsGeometry();

	//! Rather ugly way to ensure the radius is synchronized between editor objects and
	//! IStatObjects. Must change after E3 - hassle Danny if you see it after this!
	void SetAIRadius(IStatObj* pObj, float radius);

	//! Gets selected objects
	std::vector<CVegetationObject*> GetSelectedObjects() const;

	//////////////////////////////////////////////////////////////////////////
	// Object Painting.
	//////////////////////////////////////////////////////////////////////////

	//! Place single object at specified location.
	CVegetationInstance* PlaceObjectInstance(const Vec3& worldPos, CVegetationObject* brush);
	//! Clone object instance
	CVegetationInstance* CloneInstance(CVegetationInstance* pOriginal);
	void                 DeleteObjInstance(CVegetationInstance* pInst);
	void                 RemoveDuplVegetation(int x1 = -1, int y1 = -1, int x2 = -1, int y2 = -1);
	//! Find object instances closest to the point within given radius.
	CVegetationInstance* GetNearestInstance(const Vec3& worldPos, float radius);
	void                 GetObjectInstances(float x1, float y1, float x2, float y2, std::vector<CVegetationInstance*>& instances);
	void                 GetAllInstances(std::vector<CVegetationInstance*>& instances);
	//! Move instance to new position.
	bool                 MoveInstance(CVegetationInstance* pInst, const Vec3& newPos, bool bTerrainAlign = true);
	//! Remove object from 3D engine and place it back again.
	void                 RepositionObject(CVegetationObject* object);

	//! Update vegetation position z coordinate
	void OnHeightMapChanged();

	//! Remove objects within specified area from 3D engine and then place it back again on top of terrain.
	void RepositionArea(const AABB& box, const Vec3& offset = Vec3(0, 0, 0), int nRot = 0, bool isCopy = false);

	//! Scale all instances of this objects.
	void ScaleObjectInstances(CVegetationObject* object, float fScale, AABB* outModifiedArea = NULL);
	//! Randomly rotate all instances of this objects.
	void RandomRotateInstances(CVegetationObject* object);
	//! Clear rotation for all instances of this objects.
	void ClearRotateInstances(CVegetationObject* object);
	//! Distribute all instances of this object.
	void DistributeVegetationObject(CVegetationObject* object);
	//! Clear all instances of this object.
	void ClearVegetationObject(CVegetationObject* object);
	//! Merges the selected vegetation objects.
	void MergeVegetationObjects(const std::vector<CVegetationObject*>& objects);

	//! Paint objects on rectangle using given brush.
	void PaintBrush(CRect& rc, bool bCircle, CVegetationObject* brush, Vec3* pPos = 0);

	//! Clear objects in rectangle using given brush.
	//! @param brush Object to remove, if NULL all object will be cleared.
	void ClearBrush(CRect& rc, bool bCircle, CVegetationObject* pObject);

	//! Clear all object within mask.
	void ClearMask(const string& maskFile);

	//! Sets this brightness to all objects within specified rectangle.
	//! x,y,w,h are specified in world units (meters).
	//! \param brightness 0..255 brightness without ground texture, with hill shadows but without object shadows
	//! \param brightness_shadowmap 0..255 brightness without ground texture, with hill shadows and with object shadows
	void PaintBrightness(float x, float y, float w, float h, uint8 brightness, uint8 brightness_shadowmap);

	//////////////////////////////////////////////////////////////////////////
	//! Serialize vegetation map to archive.
	void Serialize(CXmlArchive& xmlAr);

	//! Serialize vegetation objects
	void SerializeObjects(XmlNodeRef& vegetationNode);

	//! Serialize vegetation instances
	void SerializeInstances(CXmlArchive& xmlAr, CRect* rect = NULL);

	//! Serialize segment in memory block
	void ClearSegment(const AABB& bb);
	void ImportSegment(CMemoryBlock& mem, const Vec3& vOfs);
	void ExportSegment(CMemoryBlock& mem, const AABB& bb, const Vec3& vOfs);

	//! Generate shadows from static objects and place them in shadow map bitarray.
	void GenerateShadowMap(CByteImage& shadowmap, float shadowAmmount, const Vec3& sunVector);

	//! Draw sectors to texture.
	void DrawToTexture(uint32* texture, int textureWidth, int textureHeight, int srcX, int srcY);

	//! Record undo info for vegetation instance.
	void RecordUndo(CVegetationInstance* pInst);

	// Calculates texture memory usage for the vegetation objects.
	int  GetTexureMemoryUsage(bool bOnlySelectedObjects);

	void GetMemoryUsage(ICrySizer* pSizer);

	// Set engine params for all objects
	void SetEngineObjectsParams();

	// Calculate Height for vegetation instance if it placed on the brush.
	float CalcHeightOnBrushes(const Vec3& p, const Vec3& posUpper, bool& bHitBrush);

	// Call this function if global config spec is changed
	void        UpdateConfigSpec();

	void        ReloadGeometry();

	static void GenerateBillboards(IConsoleCmdArgs*);
	bool        SaveBillboardTIFF(const CString& texName, ITexture* pTexture, const char* szPreset, bool bConvertToSRGB);

	bool        IsAreaEmpty(const AABB& bbox);

	//! States for optimal storing Undo
	enum EStoreUndo
	{
		eStoreUndo_Normal,
		eStoreUndo_Begin,
		eStoreUndo_Once,
		eStoreUndo_End,
	};
	void StoreBaseUndo(EStoreUndo state = eStoreUndo_Normal);

	void RegisterInstance(CVegetationInstance* pInst);

	const char* GetDataFilename() const;
	
	CCrySignal<void(CVegetationObject*)> signalVegetationObjectChanged;
	CCrySignal<void(bool)>               signalAllVegetationObjectsChanged; //bool - Reload vegetation objects in VegetationModel
	CCrySignal<void()>                   signalVegetationObjectsMerged;

private:
	typedef std::list<CVegetationInstance*> SectorInfo;

	//! Get sector by world coordinates.
	SectorInfo* GetVegSector(const Vec3& worldPos);

	//! Get sector by 2d map coordinates.
	SectorInfo& GetVegSector(int x, int y);

	//! Remove all object in vegetation map from 3d engine terrain.
	void RemoveObjectsFromTerrain();

	//! Create new object instance in map.
	CVegetationInstance* CreateObjInstance(CVegetationObject* object, const Vec3& pos, CVegetationInstance* pCopy = nullptr, SectorInfo* si = nullptr);
	void                 DeleteObjInstanceImpl(CVegetationInstance* pInst);
	//! Only to be used by undo/redo.
	void                 AddObjInstance(CVegetationInstance* pInst);

	//! Return true if there is no specified objects within radius from given position.
	bool CanPlace(CVegetationObject* object, const Vec3& pos, float radius);

	//! Returns true if theres no any objects within radius from given position.
	bool  IsPlaceEmpty(const Vec3& pos, float radius, CVegetationInstance* ignore);

	void  ClearAll();
	void  ClearSectors();

	void  LoadOldStuff(CXmlArchive& xmlAr);

	void  UpdateGroundDecal(CVegetationInstance* pInst);

	float GenerateRotation(CVegetationObject* pObject, const Vec3& vPos);

	//Instances
	std::vector<SectorInfo> m_sectors;         // 2D Array of sectors that store vegetation objects
	int                     m_sectorSize;      // Size of single sector in meters
	int                     m_numSectors;      // Number of sectors in each dimension in map (Resolution of sectors map)
	int                     m_mapSize;         // Size of all map in meters
	float                   m_minimalDistance; // Minimal distance between two objects. Objects cannot be placed closer together than this distance
	float                   m_worldToSector;   // World to sector scaling ratio

	int                     m_numInstances;

	// Vegetation objects
	typedef std::vector<TSmartPtr<CVegetationObject>> Objects;
	Objects m_objects;
	std::unordered_map<int, CVegetationObject*> m_idToObject; // Used for mapping id to object taken group ids

	int        m_nVersion;

	EStoreUndo m_storeBaseUndoState; // For optimization: Update UI once while Undo

	friend class CUndoVegInstanceCreate;
};
