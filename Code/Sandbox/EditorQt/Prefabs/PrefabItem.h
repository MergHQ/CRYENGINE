// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseLibraryItem.h"
#include <Objects/BaseObject.h>
#include <CryExtension/CryGUID.h>

class CPrefabObject;
class CSelectionGroup;

// Helpers for object ID mapping for better code readability
struct SObjectIdMapping
{
	CryGUID from;
	CryGUID to;
	SObjectIdMapping() : from(CryGUID::Null()), to(CryGUID::Null()) {}
	SObjectIdMapping(CryGUID src, CryGUID dst) : from(src), to(dst) {}
};

typedef std::vector<SObjectIdMapping> TObjectIdMapping;

/*! CPrefabItem contain definition of particle system spawning parameters.
 *
 */
class SANDBOX_API CPrefabItem : public CBaseLibraryItem
{
	friend class CPrefabObject;
public:
	CPrefabItem();

	virtual EDataBaseItemType GetType() const { return EDB_TYPE_PREFAB; }

	virtual void              SetName(const string& name) override;

	void                      Serialize(SerializeContext& ctx);

	//! Resets the item by xml representation by removing all the objects from the m_objectsNode node (note that this does not remove objects form prefab object instances)
	void ResetObjectsNode();

	//! Returns xml node containing prefab objects.
	XmlNodeRef GetObjectsNode() { return m_objectsNode; }

	//! Make prefab from selection of objects.
	void MakeFromSelection(const CSelectionGroup& selection);

	//! Makes a copy of this instance with a new guid.
	CPrefabItem* CreateCopy() const;

	//! Called when something changed in a prefab the pPrefabObject is the changed object
	void UpdateFromPrefabObject(CPrefabObject* pPrefabObject, const SObjectChangedContext& context);

	//! Called after particle parameters where updated.
	void   Update();

	string GetPrefabObjectClassName() { return m_PrefabClassName; }
	void   SetPrefabClassName(string prefabClassNameString);

	void   UpdateObjects();
	//! Go through all the objects in the xml archive and find if there are xml nodes with tag object. In those nodes check the prefab id to make sure they don't have common highparts
	//! This check is important because the id resolve process will end up generating the same instance ids if the objects have the same highpart in the prefab id
	bool ScanForDuplicateObjects();
	//! Fix all duplicate Id in child objects by assigning random guids to the duplicated Id fields
	bool FixDuplicateObjects();

	//! Searches and finds the XmlNode with a specified Id in m_objectsNode (the XML representation of this prefab in the prefab library)
	XmlNodeRef             FindObjectByGuid(const CryGUID& guid, bool fowardSearch = true);
	//! Searches and finds the XmlNode with a specified Id in an xml library (the XML representation of this prefab in the prefab library)
	XmlNodeRef             FindObjectByGuidInFlattenedNodes(std::vector<XmlNodeRef>& objects, const CryGUID& guid, bool fowardSearch = true);
	//!Returns the guids of all the direct children of this prefab that are also prefabs
	std::set<CryGUID>      FindPrefabsGUIDsInChildren();
	//!Like FindPrefabsGUIDsInChildren, but also loads the CPrefabItem
	std::set<CPrefabItem*> FindPrefabItemsInChildren();
	//!Recursively goes through the whole hierarchy of children
	//!@param toIgnore the list of children GUIDs to exclude from the search (with all their children)
	std::set<CryGUID> FindAllPrefabsGUIDsInChildren(const std::set<CryGUID>& toIgnore);

	void              CheckVersionAndUpgrade();

	//! Called when the item name changed.
	//! \sa SetName
	CCrySignal<void()> signalNameChanged;

private:
	//! Function to serialize changes to the main prefab lib (this changes only the internal XML representation m_objectsNode)
	void           ModifyLibraryPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping);
	//Go through the whole hierarchy serialized in this XML and find all the entries tagged as object, basically flatten the whole hierarchy of the prefab
	void           FindAllObjectsInLibrary(XmlNodeRef nodeRef, std::vector<XmlNodeRef>& objects);
	//! Function to update a instanced prefabs in the level
	void           ModifyInstancedPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping);
	//! Registers prefab event flowgraph nodes from all prefab instances
	void           RegisterPrefabEventFlowNodes(CBaseObject* const pEntityObj);
	//! Searches for a object inside the container by a prefabId
	CBaseObjectPtr FindObjectByPrefabGuid(const std::vector<CBaseObject*>& objects, CryGUID guidInPrefab);
	//! Extracts all object mapping from a selection (prefabID -> uniqueID)
	void           ExtractObjectsPrefabIDtoGuidMapping(CSelectionGroup& objects, TObjectIdMapping& mapping);
	//! Remaps id to another id, or it returns the id unchanged if no mapping was found
	CryGUID        ResolveID(const TObjectIdMapping& prefabIdToGuidMapping, CryGUID id, bool prefabIdToGuidDirection);
	//! Goes through objectNode and all its children (except Flowgraph) and remaps the ids in the direction specified by the prefabIdToGuidDirection parameter
	void           RemapIDsInNodeAndChildren(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection);
	//! Function changes the ids XML attributes of objectNode in the direction specified by the prefabIdToGuidDirection parameter
	void           RemapIDsInNode(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection);
	//! Removes all XML child nodes in m_objectsNode, which has a Parent with the specified GUID
	void           RemoveAllChildrenOf(CryGUID guid);

	void           SaveLinkedObjects(CObjectArchive& ar, CBaseObject* pObject, bool bAllowOwnedByPrefab);
	void           CollectLinkedObjects(CBaseObject* pObject, std::vector<CBaseObject*>& linkedObjects, CSelectionGroup& selection);
private:
	XmlNodeRef m_objectsNode;
	string     m_PrefabClassName;
	uint32     m_version;
};