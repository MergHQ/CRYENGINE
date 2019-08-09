// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PrefabItem.h"

#include "HyperGraph/FlowGraph.h"
#include "HyperGraph/FlowGraphManager.h"
#include "Objects/EntityObject.h"
#include "Objects/PrefabObject.h"
#include "Objects/SelectionGroup.h"
#include "Prefabs/PrefabEvents.h"
#include "Prefabs/PrefabLibrary.h"
#include "Prefabs/PrefabManager.h"
#include "BaseLibraryManager.h"

#include <IEditor.h>
#include "GameEngine.h"
#include <AssetSystem/AssetManager.h>

#include <Objects/IObjectLayer.h>
#include <Objects/ObjectLoader.h>
#include <Preferences/SnappingPreferences.h>
#include <IUndoManager.h>

#include <CryMath/Cry_Math.h>
#include <queue>

enum EPrefabVersions
{
	/*
	   Prefabs are saved in a flattened hierarchy, group members are too.For example
	   <Object Type = "Group" Layer = "Main" LayerGUID = "561c3d54-fd84-4a25-3fa2-2781f36492a8" Id = "a440327f-e262-3aa9-6520-619fe689071e" Name = "Group-2" Pos = "509.65161,501.59622,31.999756" Rotate = "1,0,0,0" Scale = "1,1,1" ColorRGB = "65280" UseCustomLevelLayerColor = "0" Opened = "0" / >
	   <Object Type = "Brush" Layer = "Main" LayerGUID = "561c3d54-fd84-4a25-3fa2-2781f36492a8" Id = "27120ffe-ab2b-6c95-c25d-171d8ec67aaf" Name = "primitive_sphere-1" Parent = "a440327f-e262-3aa9-6520-619fe689071e" Pos = "-508.02625,-501.48993,-31.999756" Rotate = "1,0,0,0" Scale = "1,1,1" ColorRGB = "16777215" UseCustomLevelLayerColor = "0" MatLayersMask = "0" Prefab = "objects/default/primitive_sphere.cgf" IgnoreVisareas = "0" CastShadowMaps = "1" GIMode = "1" RainOccluder = "1" SupportSecondVisarea = "0" DynamicDistanceShadows = "0" Hideable = "0" LodRatio = "100" ViewDistRatio = "100" NotTriangulate = "0" NoDynamicWater = "0" AIRadius = "-1" NoStaticDecals = "0" RecvWind = "0" Occluder = "0" DrawLast = "0" ShadowLodBias = "0" IgnoreTerrainLayerBlend = "1" IgnoreDecalBlend = "1" RndFlags = "60000608">
	   <CollisionFiltering>
	   <Type collision_class_terrain = "0" collision_class_wheeled = "0" collision_class_living = "0" collision_class_articulated = "0" collision_class_soft = "0" collision_class_particle = "0" gcc_player_capsule = "0" gcc_player_body = "0" gcc_vehicle = "0" gcc_large_kickable = "0" gcc_ragdoll = "0" gcc_rigid = "0" gcc_vtol = "0" gcc_ai = "0" / >
	   <Ignore collision_class_terrain = "0" collision_class_wheeled = "0" collision_class_living = "0" collision_class_articulated = "0" collision_class_soft = "0" collision_class_particle = "0" gcc_player_capsule = "0" gcc_player_body = "0" gcc_vehicle = "0" gcc_large_kickable = "0" gcc_ragdoll = "0" gcc_rigid = "0" gcc_vtol = "0" gcc_ai = "0" / >
	   < / CollisionFiltering>
	   < / Object>
	   "primitive_sphere-1" is a child of "Group-2", how do you know ? Because it has a parent attribute with the group guid, top level objects in prefab don't have a parent, it's implicitly given at load time
	 */
	e_FlattenedHierarchy,
	/*
	   1 : No more flattened hierarchy, groups are serialized exactly as they are in the level file (using CGroup::Serialize)
	   <Object Type="Group" Layer="Main" LayerGUID="5af8763b-0eb4-f6b2-c9cd-bf985e9fa3dd" Id="b5621401-679a-2850-f761-ae381c539261" Name="Group-4" Pos="-6.7286377,1.715271,0" Rotate="1,0,0,0" Scale="1,1,1" ColorRGB="65280" UseCustomLevelLayerColor="0" Opened="1">
	   <Objects>
	   <Object Type="Brush" Layer="Main" LayerGUID="5af8763b-0eb4-f6b2-c9cd-bf985e9fa3dd" Id="92354280-6ddb-a34b-ddf8-fb1d67141eb7" Name="primitive_pyramid-1" Parent="b5621401-679a-2850-f761-ae381c539261" Pos="5.1032715,0.83911133,0" Rotate="1,0,0,0" Scale="1,1,1" ColorRGB="16777215" UseCustomLevelLayerColor="0" MatLayersMask="0" Prefab="objects/default/primitive_pyramid.cgf" IgnoreVisareas="0" CastShadowMaps="1" GIMode="1" RainOccluder="1" SupportSecondVisarea="0" DynamicDistanceShadows="0" Hideable="0" LodRatio="100" ViewDistRatio="100" NotTriangulate="0" NoDynamicWater="0" AIRadius="-1" NoStaticDecals="0" RecvWind="0" Occluder="0" DrawLast="0" ShadowLodBias="0" IgnoreTerrainLayerBlend="1" IgnoreDecalBlend="1" RndFlags="60000408">
	   <CollisionFiltering>
	   <Type collision_class_terrain="0" collision_class_wheeled="0" collision_class_living="0" collision_class_articulated="0" collision_class_soft="0" collision_class_particle="0" gcc_player_capsule="0" gcc_player_body="0" gcc_vehicle="0" gcc_large_kickable="0" gcc_ragdoll="0" gcc_rigid="0" gcc_vtol="0" gcc_ai="0"/>
	   <Ignore collision_class_terrain="0" collision_class_wheeled="0" collision_class_living="0" collision_class_articulated="0" collision_class_soft="0" collision_class_particle="0" gcc_player_capsule="0" gcc_player_body="0" gcc_vehicle="0" gcc_large_kickable="0" gcc_ragdoll="0" gcc_rigid="0" gcc_vtol="0" gcc_ai="0"/>
	   </CollisionFiltering>
	   </Object>
	   </Objects>
	   </Object>
	   Group members are properly serialized via CGroup::SerializeMembers instead of flattening (note the <Objects> tag)
	 */
	e_FullHierarchy,
};

#define CURRENT_VERSION EPrefabVersions::e_FullHierarchy

CPrefabItem::CPrefabItem() :
	//init to current version
	m_version(CURRENT_VERSION)
{
	m_PrefabClassName = PREFAB_OBJECT_CLASS_NAME;
	m_objectsNode = XmlHelpers::CreateXmlNode("Objects");

}

void CPrefabItem::SetPrefabClassName(string prefabClassNameString)
{
	m_PrefabClassName = prefabClassNameString;
}

void CPrefabItem::SetName(const string& name)
{
	// There is no much point of creating undo here, because setting a new prefab name should always be done along with renaming the prefab library, which does not support undo.
	CBaseLibraryItem::SetName(name, true);

	signalNameChanged();
}

void CPrefabItem::Serialize(SerializeContext& ctx)
{
	CBaseLibraryItem::Serialize(ctx);
	XmlNodeRef node = ctx.node;
	if (ctx.bLoading)
	{
		m_objectsNode = node->findChild("Objects");
		//we are loading a very old prefab, set version to 0
		if (!node->getAttr("Version", m_version))
		{
			m_version = e_FlattenedHierarchy;
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "This prefab has been created with an old file version, on modify it will be updated to latest");
		}
	}
	else
	{
		if (m_objectsNode)
		{
			node->addChild(m_objectsNode);
			node->setAttr("Version", m_version);
		}
	}
}

void CPrefabItem::ResetObjectsNode()
{
	m_objectsNode = XmlHelpers::CreateXmlNode("Objects");
}

void CPrefabItem::Update()
{
	// Mark library as modified.
	SetModified();
}

void CPrefabItem::MakeFromSelection(const CSelectionGroup& selection)
{
	selection.FilterParents();
	std::vector<CBaseObject*> objects;
	objects.reserve(selection.GetFilteredCount());
	for (auto i = 0; i < selection.GetFilteredCount(); i++)
	{
		objects.push_back(selection.GetFilteredObject(i));
	}

	if (!CGroup::CanCreateFrom(objects))
		return;

	CPrefabObject::CreateFrom(objects, selection.GetCenter(), this);
}

CPrefabItem* CPrefabItem::CreateCopy() const
{
	CPrefabItem* pCopy = new CPrefabItem();
	const XmlString xml = m_objectsNode->getXML();
	pCopy->m_objectsNode = XmlHelpers::LoadXmlFromBuffer(xml.c_str(), xml.length());
	CRY_ASSERT(pCopy->m_objectsNode);
	return pCopy;
}

void CPrefabItem::SaveLinkedObjects(CObjectArchive& ar, CBaseObject* pObject, bool bAllowOwnedByPrefab)
{
	for (auto i = 0; i < pObject->GetLinkedObjectCount(); ++i)
	{
		CBaseObject* pLinkedObject = pObject->GetLinkedObject(i);
		if (bAllowOwnedByPrefab || !pLinkedObject->IsPartOfPrefab())
		{
			ar.SaveObject(pLinkedObject, false, true);
			SaveLinkedObjects(ar, pLinkedObject, bAllowOwnedByPrefab);
		}
	}
}

void CPrefabItem::CollectLinkedObjects(CBaseObject* pObject, std::vector<CBaseObject*>& linkedObjects, CSelectionGroup& selection)
{
	for (auto i = 0; i < pObject->GetLinkedObjectCount(); ++i)
	{
		CBaseObject* pLinkedObject = pObject->GetLinkedObject(i);
		if (selection.IsContainObject(pLinkedObject))
			selection.RemoveObject(pLinkedObject);

		linkedObjects.push_back(pLinkedObject);

		CollectLinkedObjects(pLinkedObject, linkedObjects, selection);
	}
}

void CPrefabItem::CheckVersionAndUpgrade()
{
	if (m_version < EPrefabVersions::e_FullHierarchy)
	{
		//in version 0 all prefabs objects are in a flat list, meaning all objects are added to the top level of the prefab (even group members are flattened), this needs to be updated to a proper hierarchy
		std::vector<XmlNodeRef> objects;
		FindAllObjectsInLibrary(m_objectsNode, objects);

		for (int i = 0; i < m_objectsNode->getChildCount(); i++)
		{
			CryGUID parentGuid;
			XmlNodeRef objectRef = m_objectsNode->getChild(i);

			//parent in top level means we are in a group (or some other object that has a parent somewhere else in the prefab), which in turn means that we have to relocate it to the correct place in the hierarchy
			if (objectRef->getAttr("Parent", parentGuid))
			{
				XmlNodeRef parentNode = FindObjectByGuidInFlattenedNodes(objects, parentGuid);

				//remove node from old parent
				objectRef->getParent()->removeChild(objectRef);

				//and add to new parent, note that groups have an <Objects> child before the actual objects, we need to get that
				XmlNodeRef groupChildRoot = parentNode->findChild("Objects");
				if (groupChildRoot)
				{
					groupChildRoot->addChild(objectRef);
				}//if it's the first time we are adding to a group we have to create the <Objects> child as it does not exist in the flattened version of the prefab file
				else if (parentNode->haveAttr("Type") && strcmp(parentNode->getAttr("Type"), "Group") == 0)
				{
					XmlNodeRef child = parentNode->createNode("Objects");
					parentNode->addChild(child);
					child->addChild(objectRef);
				}
			}
		}
	}

	m_version = CURRENT_VERSION;
}

void CPrefabItem::UpdateFromPrefabObject(CPrefabObject* pPrefabObject, const SObjectChangedContext& context)
{
	CPrefabManager* const pPrefabManager = GetIEditorImpl()->GetPrefabManager();
	//make sure that we are not modifying a prefab on level load
	const bool documentReady = GetIEditor()->IsDocumentReady();
	// Skip other prefab types
	if (!documentReady || !pPrefabObject || pPrefabObject->IsModifyInProgress()
	    || !pPrefabManager || pPrefabManager->ShouldSkipPrefabUpdate())
	{
		return;
	}

	IObjectManager* const pObjectManager = GetIEditorImpl()->GetObjectManager();
	CBaseObject* pObject = pObjectManager->FindObject(context.m_modifiedObjectGlobalId);
	if (!pObject)
	{
		return;
	}

	CheckVersionAndUpgrade();

	pPrefabObject->SetModifyInProgress(true);
	// Prevent recursive calls to this function when modifying other instances of this same prefab
	CPrefabManager::SkipPrefabUpdate skipUpdates;

	CSelectionGroup descendantsSelection;
	CSelectionGroup flatSelection;

	// Get all objects part of the prefab
	pPrefabObject->GetAllPrefabFlagedDescendants(descendantsSelection);

	// While modifying the instances we don't want the FG manager to send events since they can call into GUI components and cause a crash
	// e.g. we are modifying one FG but in fact we are working on multiple instances of this FG the MFC code does not cope well with this
	CFlowGraphManager* const pFlowMan = GetIEditorImpl()->GetFlowGraphManager();
	pFlowMan->DisableNotifyListeners(true);

	{
		//////////////////////////////////////////////////////////////////////////
		// Save all objects in flat selection to XML.
		descendantsSelection.FlattenHierarchy(flatSelection);

		// Serialize in the prefab lib (the main XML representation of the prefab)
		{
			CScopedSuspendUndo suspendUndo;
			TObjectIdMapping mapping;
			ExtractObjectsPrefabIDtoGuidMapping(flatSelection, mapping);
			ModifyLibraryPrefab(flatSelection, pPrefabObject, context, mapping);
		}

		SetModified();

		if (context.m_operation == eOCOT_ModifyTransformInLibOnly)
		{
			pFlowMan->DisableNotifyListeners(false);
			pPrefabObject->SetModifyInProgress(false);
			return;
		}

		SObjectChangedContext modifiedContext = context;

		// If modify transform case
		if (context.m_operation == eOCOT_ModifyTransform)
			modifiedContext.m_localTM = pObject->GetLocalTM();

		// Now change all the rest instances of this prefab in the level (skipping this one, since it is already been modified)
		pPrefabManager->SetSelectedItem(this);
		pFlowMan->SetGUIControlsProcessEvents(false, false);

		{
			CScopedSuspendUndo suspendUndo;

			CBaseObjectsArray allInstancedPrefabs;
			pObjectManager->FindObjectsOfType(RUNTIME_CLASS(CPrefabObject), allInstancedPrefabs);

			for (size_t i = 0, prefabsCount = allInstancedPrefabs.size(); i < prefabsCount; ++i)
			{
				CPrefabObject* const pInstancedPrefab = static_cast<CPrefabObject*>(allInstancedPrefabs[i]);
				if (pPrefabObject != pInstancedPrefab && pInstancedPrefab->GetPrefabGuid() == pPrefabObject->GetPrefabGuid())
				{
					CSelectionGroup instanceDescendantsSelection;
					CSelectionGroup instanceFlatSelection;

					pInstancedPrefab->GetAllPrefabFlagedDescendants(instanceDescendantsSelection);

					instanceDescendantsSelection.FlattenHierarchy(instanceFlatSelection);

					TObjectIdMapping mapping;
					ExtractObjectsPrefabIDtoGuidMapping(instanceFlatSelection, mapping);

					// Modify instanced prefabs in the level
					pInstancedPrefab->SetModifyInProgress(true);
					ModifyInstancedPrefab(instanceFlatSelection, pInstancedPrefab, modifiedContext, mapping);
					pInstancedPrefab->SetModifyInProgress(false);
				}
			}
		} // ~CScopedSuspendUndo suspendUndo;

		pFlowMan->SetGUIControlsProcessEvents(true, true);
	}

	pFlowMan->DisableNotifyListeners(false);

	pPrefabObject->SetModifyInProgress(false);
}

void CPrefabItem::UpdateObjects()
{
	std::vector<CBaseObject*> pPrefabObjects;
	GetIEditorImpl()->GetObjectManager()->FindObjectsOfType(RUNTIME_CLASS(CPrefabObject), pPrefabObjects);
	for (int i = 0; i < pPrefabObjects.size(); ++i)
	{
		//SetPrefab actually changes the prefabs hierarchy, some objects wont be prefabs anymore
		if (!pPrefabObjects[i]->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
		{
			continue;
		}

		CPrefabObject* pPrefabObject = (CPrefabObject*)pPrefabObjects[i];
		if (pPrefabObject->GetPrefabGuid() == GetGUID())
		{
			// Even though this is not undo process, the undo flags should be true to avoid crashes caused by light anim sequence.
			pPrefabObject->SetPrefab(this, true);
		}
	}
}

bool CPrefabItem::ScanForDuplicateObjects()
{
	std::vector<CryGUID> guids;

	//find all objects in this item
	std::vector<XmlNodeRef> objects;
	FindAllObjectsInLibrary(GetObjectsNode(), objects);

	bool duplicateFound = false;
	int duplicatedItems = 0;

	//Check all the object nodes for recurring ids
	for (XmlNodeRef node : objects)
	{
		//Make sure we are only dealing with objects, this is necessary because area nodes have an 'Id' property that's named the same as object node Ids. 
		//The issue is that area nodes use a completely different id naming scheme, so this needs to be skipped
		if (strcmp(node->getTag(), "Object"))
		{
			continue;
		}

		CryGUID id = CryGUID::FromString(node->getAttr("Id"));

		bool hasId = std::any_of(guids.begin(), guids.end(), [id](const CryGUID& guidToCheck)
		{
			//if the high part of this id is the same as another id in the ids list then we need to warn the user about it
			return id.hipart == guidToCheck.hipart;
		});

		if (hasId && node->haveAttr("Name"))
		{
			CryWarning(EValidatorModule::VALIDATOR_MODULE_EDITOR, EValidatorSeverity::VALIDATOR_ERROR, " Found duplicated hipart in object %s, id is %s", node->getAttr("Name"), id.ToString().c_str());
			if (!duplicateFound)
			{
				duplicatedItems++;
			}

			duplicateFound = true;
		}
		else
		{
			guids.push_back(id);
		}
	}

	if (duplicateFound)
	{
		CryWarning(EValidatorModule::VALIDATOR_MODULE_EDITOR, EValidatorSeverity::VALIDATOR_ERROR, "This prefab item %s has duplicate hipart ids, this will cause a crash on prefab swap, run prefab.fix_duplicates_in_items to recompute prefab ids", GetName().c_str());
	}

	return !duplicateFound;
}

bool CPrefabItem::FixDuplicateObjects()
{
	bool canRun = true;

	//If the level is loaded check if we have loaded prefab items
	if (GetIEditor()->GetGameEngine()->IsLevelLoaded() && GetIEditor()->GetPrefabManager()->GetLibraryCount())
	{
		//If a level is open make sure that no instances of this item are present in the level
		if (GetIEditor()->GetPrefabManager()->FindAllInstancesOfItem(this).size())
		{
			canRun = false;
		}
	}

	if (!canRun)
	{
		CryWarning(EValidatorModule::VALIDATOR_MODULE_EDITOR, EValidatorSeverity::VALIDATOR_ERROR, "This command changes prefab ids and cannot be run while any prefab instances are present in the level");
		return false;
	}

	//list of the ids loaded up until now
	std::vector<CryGUID> guids;

	//find all objects in this item
	std::vector<XmlNodeRef> objects;
	FindAllObjectsInLibrary(GetObjectsNode(), objects);

	bool duplicateFound = false;

	//Check all the object nodes for recurring ids
	for (XmlNodeRef node : objects)
	{
		//Make sure we are only dealing with objects, this is necessary because area nodes have an 'Id' property that's named the same as object node Ids. 
		//The issue is that area nodes use a completely different id naming scheme, so this needs to be skipped
		if (strcmp(node->getTag(), "Object") != 0)
		{
			continue;
		}
		
		CryGUID id = CryGUID::FromString(node->getAttr("Id"));

		bool hasGuid = std::any_of(guids.begin(), guids.end(), [id](const CryGUID& guidToCheck)
		{
			//if the high part of this id is the same as another id in the ids list then we need to regenerate it
			return id.hipart == guidToCheck.hipart;
		});

		if (hasGuid)
		{
			//regenerate this id and store it in the XML node
			node->setAttr("Id", CryGUID::Create());
			duplicateFound = true;
		}
		else
		{
			guids.push_back(id);
		}
	}

	//if we changed any id mark the item (and the asset) as modified
	if (duplicateFound)
	{
		SetModified();
	}

	return duplicateFound;
}

void CPrefabItem::ModifyLibraryPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping)
{
	IObjectManager* const pObjectManager = GetIEditorImpl()->GetObjectManager();

	std::vector<XmlNodeRef> objects;
	//Find all the objects in this prefab item
	FindAllObjectsInLibrary(m_objectsNode, objects);
	//If this object actually exists in the instance
	if (CBaseObject* const pObject = objectsInPrefabAsFlatSelection.GetObjectByGuidInPrefab(context.m_modifiedObjectGuidInPrefab))
	{
		// Add a new member to the to level of the prefab
		if (context.m_operation == eOCOT_Add)
		{
			// If we are trying to add something which already exists in the prefab definition we are trying to modify it
			if (FindObjectByGuid(pObject->GetIdInPrefab(), true))
			{
				SObjectChangedContext correctedContext = context;
				correctedContext.m_operation = eOCOT_Modify;
				ModifyLibraryPrefab(objectsInPrefabAsFlatSelection, pPrefabObject, correctedContext, guidMapping);
				return;
			}
			//Create a new archive and serialize the object in it
			CObjectArchive ar(pObjectManager, m_objectsNode, false);

			ar.SaveObject(pObject, false, true);

			//Update all the linked objects
			SaveLinkedObjects(ar, pObject, true);

			//Update all the ids to link to the correct prefab objects (parents/ecc)
			XmlNodeRef savedObj = m_objectsNode->getChild(m_objectsNode->getChildCount() - 1);
			RemapIDsInNodeAndChildren(savedObj, guidMapping, false);
		}
		// Member MODIFY
		else if (context.m_operation == eOCOT_Modify || context.m_operation == eOCOT_ModifyTransform || context.m_operation == eOCOT_ModifyTransformInLibOnly)
		{
			//This object could be everywhere in the hierarchy, we do not want to modify it all, just find it in the flattened version (aka the list of all the objects in the XML) and reserialize
			// Find -> remove previous definition of the object and serialize the new changes
			if (XmlNodeRef object = FindObjectByGuidInFlattenedNodes(objects, pObject->GetIdInPrefab(), true))
			{
				//Parent could be every level in the hierarchy, like an <Objects> node (we are in a group), or the top level of a prefab
				XmlNodeRef parent = object->getParent();
				parent->removeChild(object);

				CObjectArchive ar(pObjectManager, parent, false);
				ar.SaveObject(pObject, false, true);

				XmlNodeRef savedObj = parent->getChild(parent->getChildCount() - 1);
				RemapIDsInNodeAndChildren(savedObj, guidMapping, false);
			}
		}
		// Member DELETE
		else if (context.m_operation == eOCOT_Delete)
		{
			//Just find an object from the flattened hierarchy and then remove it
			if (XmlNodeRef object = FindObjectByGuidInFlattenedNodes(objects, pObject->GetIdInPrefab(), true))//FindObjectByGuid(pObj->GetIdInPrefab(), true))
			{
				XmlNodeRef parent = object->getParent();
				parent->removeChild(object);
				// A parent was deleted so just remove the parent attribute
				for (int i = 0, count = parent->getChildCount(); i < count; ++i)
				{
					CryGUID parentGuid = CryGUID::Null();
					if (parent->getChild(i)->getAttr("Parent", parentGuid) && parentGuid == pObject->GetIdInPrefab())
					{
						parent->getChild(i)->delAttr("Parent");
					}
				}
			}
		} 
	} 
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::FindAllObjectsInLibrary(XmlNodeRef nodeRef, std::vector<XmlNodeRef>& objects)
{
	for (int i = nodeRef->getChildCount() - 1; i >= 0; --i)
	{
		XmlNodeRef childNode = nodeRef->getChild(i);
		if (childNode->isTag("Object"))
		{
			objects.push_back(childNode);
		}

		if (childNode->getChildCount())
		{
			FindAllObjectsInLibrary(childNode, objects);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//This updates all the instanced objects with the exception of the original prefab instance that is calling the prefab item update (as it's obviously already up to date)
void CPrefabItem::ModifyInstancedPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping)
{
	IObjectManager* const pObjectManager = GetIEditorImpl()->GetObjectManager();
	std::vector<XmlNodeRef> objects;
	//we re generate the list it again because it can be modified if the prefab item m_objectsNode is changed (which happens in ModifyPrefab)
	FindAllObjectsInLibrary(m_objectsNode, objects);

	if (context.m_operation == eOCOT_Add)
	{
		if (CBaseObject* const pObject = pObjectManager->FindObject(context.m_modifiedObjectGlobalId))
		{
			//NB : Note that we search only in the top level hierarchy, for hierarchies the parents (aka a Group) will deal with addition/removal of children
			if (XmlNodeRef changedObject = FindObjectByGuid(pObject->GetIdInPrefab(), false))
			{
				// See if we already have this object as part of the instanced prefab
				std::vector<CBaseObject*> descendants;
				pPrefabObject->GetAllPrefabFlagedDescendants(descendants);
				CBaseObject* pModified = FindObjectByPrefabGuid(descendants, pObject->GetIdInPrefab());
				if (pModified)
				{
					SObjectChangedContext correctedContext = context;
					correctedContext.m_operation = eOCOT_Modify;
					ModifyInstancedPrefab(objectsInPrefabAsFlatSelection, pPrefabObject, correctedContext, guidMapping);
					return;
				}
				//Clone the objects, fix the flags and add it to the current item, this works only for TOP LEVEL ITEMS,
				//groups will call modify on themselves and regenerate all the objects inside them if something is added
				CObjectArchive loadAr(pObjectManager, changedObject, true);
				loadAr.EnableProgressBar(false);

				// PrefabGUID -> GUID
				RemapIDsInNodeAndChildren(changedObject, guidMapping, true);

				/*
				   Here we set the guid provider to null so that it will not attempt to regenerate GUIDS in LoadObject(specifically in the call to NewObject).
				   We need the serialized Id to stay the same as the prefab xml Id until AddMember is called
				   AddMember will "slide" the current Id to IdInPrefab (which we need for prefab instance and library update) and generate a new unique Id for the prefab instance
				 */
				loadAr.SetGuidProvider(nullptr);
				CBaseObject* pClone = loadAr.LoadObject(changedObject);
				loadAr.ResolveObjects();

				RegisterPrefabEventFlowNodes(pClone);

				pPrefabObject->SetObjectPrefabFlagAndLayer(pClone);

				//Actually set the guid provider now
				CPrefabChildGuidProvider guidProvider(pPrefabObject);
				loadAr.SetGuidProvider(&guidProvider);
				if (!pClone->GetParent())
					pPrefabObject->AddMember(pClone, false);

				// GUID -> PrefabGUID
				RemapIDsInNodeAndChildren(changedObject, guidMapping, false);
			}
		}
	}
	else if (CBaseObject* const pObject = objectsInPrefabAsFlatSelection.GetObjectByGuidInPrefab(context.m_modifiedObjectGuidInPrefab))
	{
		//We update the state of all members in the top level of the prefab, members in the lower level are updated by their parent (prefabs/group) serialize method
		if (context.m_operation == eOCOT_Modify)
		{
			if (XmlNodeRef changedObject = FindObjectByGuidInFlattenedNodes(objects, pObject->GetIdInPrefab(), false))
			{
				//Deserialize the object and attach it to the prefab instance
				//when this is sent to groups it will recreate the whole group as the CGroup::Serialize deletes and regenerates all it's members
				//Note that in this case no remapping is needed, especially for groups the GenerateGUIDsForObjectAndChildren function called on AddMember in archive resolve (happens on archive destruction, aka end of this function) will properly assign guids to this object and its children

				// Build load archive
				CObjectArchive loadAr(pObjectManager, changedObject, true);

				// Load ids remapping info into the load archive
				for (int j = 0, count = guidMapping.size(); j < count; ++j)
					loadAr.RemapID(guidMapping[j].from, guidMapping[j].to);

				// Set parent attr if we dont have any (because we don't store this in the prefab XML since it is implicitly given)

				if (!changedObject->haveAttr("Parent") && !changedObject->haveAttr("LinkedTo"))
				{
				
					changedObject->setAttr("Parent", pPrefabObject->GetId());
				}

				// Back-up flags before serializing. We need some flags to be persistent after serialization. Ex: Selection flags
				auto flags = pObject->GetFlags();

				// Load the object
				pObject->Serialize(loadAr);
				pObject->PostLoad(loadAr);

				// Set old flags
				pObject->SetFlags(flags);

				RegisterPrefabEventFlowNodes(pObject);

				pPrefabObject->SetObjectPrefabFlagAndLayer(pObject);

				// Remove parent attribute if it is directly a child of the prefab
				CryGUID parentId = CryGUID::Null();
				changedObject->getAttr("Parent", parentId);
				if (parentId == pPrefabObject->GetId())
				{
					changedObject->delAttr("Parent");
				}
			}
		}
		else if (context.m_operation == eOCOT_ModifyTransform)
		{
			if (XmlNodeRef changedObject = FindObjectByGuidInFlattenedNodes(objects, pObject->GetIdInPrefab(), false))//FindObjectByGuid(pObj->GetIdInPrefab(), false))
			{
				//just reset the TM matrix
				pObject->SetLocalTM(context.m_localTM, eObjectUpdateFlags_Undo);
			}
		}
		else if (context.m_operation == eOCOT_Delete)
		{
			pObjectManager->DeleteObject(pObject);
		}
	}
}

XmlNodeRef CPrefabItem::FindObjectByGuid(const CryGUID& guid, bool forwardSearch)
{
	CryGUID objectId = CryGUID::Null();

	if (forwardSearch)
	{
		for (int i = 0, count = m_objectsNode->getChildCount(); i < count; ++i)
		{
			m_objectsNode->getChild(i)->getAttr("Id", objectId);
			if (objectId == guid)
				return m_objectsNode->getChild(i);
		}
	}
	else
	{
		for (int i = m_objectsNode->getChildCount() - 1; i >= 0; --i)
		{
			m_objectsNode->getChild(i)->getAttr("Id", objectId);
			if (objectId == guid)
				return m_objectsNode->getChild(i);
		}
	}

	return nullptr;
}

XmlNodeRef CPrefabItem::FindObjectByGuidInFlattenedNodes(std::vector<XmlNodeRef>& objects, const CryGUID& guid, bool fowardSearch /*= true*/)
{
	CryGUID objectId = CryGUID::Null();

	if (fowardSearch)
	{
		for (int i = 0, count = objects.size(); i < count; ++i)
		{
			objects[i]->getAttr("Id", objectId);
			if (objectId == guid)
				return objects[i];
		}
	}
	else
	{
		for (int i = objects.size() - 1; i >= 0; --i)
		{
			objects[i]->getAttr("Id", objectId);
			if (objectId == guid)
				return objects[i];
		}
	}

	return nullptr;
}

std::set<CryGUID> CPrefabItem::FindPrefabsGUIDsInChildren()
{
	std::set<CryGUID> guids;
	for (int i = 0, count = m_objectsNode->getChildCount(); i < count; ++i)
	{
		string type;
		m_objectsNode->getChild(i)->getAttr("Type", type);
		if (type == "Prefab")
		{
			CryGUID guid;
			m_objectsNode->getChild(i)->getAttr("PrefabGUID", guid);
			guids.insert(guid);
		}
	}

	return guids;
}

std::set<CPrefabItem*> CPrefabItem::FindPrefabItemsInChildren()
{
	std::set<CryGUID> guids = FindPrefabsGUIDsInChildren();
	std::set<CPrefabItem*> items;

	for (CryGUID guid : guids)
	{
		IDataBaseItem* pItem = GetIEditor()->GetPrefabManager()->LoadItem(guid);
		//This can happen if the asset is in the asset browser but the prefab object has been deleted
		//(for example undo after creation)
		if (!pItem)
		{
			continue;
		}
		CPrefabItem* pPrefabItem = static_cast<CPrefabItem*>(pItem);
		items.insert(pPrefabItem);
	}

	return items;
}

std::set<CryGUID> CPrefabItem::FindAllPrefabsGUIDsInChildren(const std::set<CryGUID>& toIgnore)
{
	std::set<CryGUID> finalGuids = FindPrefabsGUIDsInChildren();
	std::set<CryGUID> guids = FindPrefabsGUIDsInChildren();
	for (const CryGUID& guid : guids)
	{
		if (std::find(toIgnore.begin(), toIgnore.end(), guid) != toIgnore.end())
		{
			continue;
		}

		IDataBaseItem* pItem = GetIEditor()->GetPrefabManager()->LoadItem(guid);
		//This can happen if the asset is in the asset browser but the prefab object has been deleted
		//(for example undo after creation)
		if (!pItem)
		{
			continue;
		}
		CPrefabItem* pPrefabItem = static_cast<CPrefabItem*>(pItem);
		std::set<CryGUID> childGuids = pPrefabItem->FindAllPrefabsGUIDsInChildren(toIgnore);
		finalGuids.insert(childGuids.begin(), childGuids.end());
	}

	return finalGuids;
}

CBaseObjectPtr CPrefabItem::FindObjectByPrefabGuid(const std::vector<CBaseObject*>& objects, CryGUID guidInPrefab)
{
	for (auto it = objects.cbegin(), end = objects.cend(); it != end; ++it)
	{
		if ((*it)->GetIdInPrefab() == guidInPrefab)
			return (*it);
	}

	return nullptr;
}

void CPrefabItem::ExtractObjectsPrefabIDtoGuidMapping(CSelectionGroup& objects, TObjectIdMapping& mapping)
{
	mapping.reserve(objects.GetCount());
	std::queue<CBaseObject*> groups;

	for (int i = 0, count = objects.GetCount(); i < count; ++i)
	{
		CBaseObject* pObject = objects.GetObject(i);
		mapping.push_back(SObjectIdMapping(pObject->GetIdInPrefab(), pObject->GetId()));
	}
}

CryGUID CPrefabItem::ResolveID(const TObjectIdMapping& prefabIdToGuidMapping, CryGUID id, bool prefabIdToGuidDirection)
{
	if (prefabIdToGuidDirection)
	{
		for (int j = 0, count = prefabIdToGuidMapping.size(); j < count; ++j)
		{
			if (prefabIdToGuidMapping[j].from == id)
				return prefabIdToGuidMapping[j].to;
		}
	}
	else
	{
		for (int j = 0, count = prefabIdToGuidMapping.size(); j < count; ++j)
		{
			if (prefabIdToGuidMapping[j].to == id)
				return prefabIdToGuidMapping[j].from;
		}
	}

	return id;
}

void CPrefabItem::RemapIDsInNodeAndChildren(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection)
{
	std::queue<XmlNodeRef> objects;
	const char* kFlowGraphTag = "FlowGraph";

	if (!objectNode->isTag(kFlowGraphTag))
		RemapIDsInNode(objectNode, mapping, prefabIdToGuidDirection);

	for (int i = 0; i < objectNode->getChildCount(); ++i)
	{
		objects.push(objectNode->getChild(i));
		// Recursively traverse all objects and exclude flowgraph parts
		while (!objects.empty())
		{
			XmlNodeRef object = objects.front();
			// Skip flowgraph
			if (object->isTag(kFlowGraphTag))
			{
				objects.pop();
				continue;
			}

			RemapIDsInNode(object, mapping, prefabIdToGuidDirection);
			objects.pop();

			for (int j = 0, childCount = object->getChildCount(); j < childCount; ++j)
			{
				XmlNodeRef child = object->getChild(j);
				if (child->isTag(kFlowGraphTag))
					continue;

				RemapIDsInNode(child, mapping, prefabIdToGuidDirection);

				if (child->getChildCount())
					objects.push(child);
			}
		}
	}
}

void CPrefabItem::RemapIDsInNode(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection)
{
	CryGUID patchedId;
	if (objectNode->getAttr("Id", patchedId))
	{
		CryGUID resolvedId = ResolveID(mapping, patchedId, prefabIdToGuidDirection);
		// this is sort of a work-around for a specific case.
		// The problem was that whenever you create a prefab it goes through all the children in it's xml objects'
		// description, finds and replaces (or remaps) all attributes "Id". It expects all of them to be of CryGUID type.
		// If it doesn't find mapped value for the given CryGUID, it will return the given CryGUID.
		// For some objects (like Area) ids are not represented as CryGUID but rather just plain integer.
		// And therefore theirs ids will be overridden by invalid CryGUIDs.
		if (patchedId != resolvedId)
		{
			objectNode->setAttr("Id", resolvedId);
		}
	}
	if (objectNode->getAttr("Parent", patchedId))
		objectNode->setAttr("Parent", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
	if (objectNode->getAttr("LinkedTo", patchedId))
		objectNode->setAttr("LinkedTo", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
	if (objectNode->getAttr("LookAt", patchedId))
		objectNode->setAttr("LookAt", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
	if (objectNode->getAttr("TargetId", patchedId))
		objectNode->setAttr("TargetId", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
}

void CPrefabItem::RemoveAllChildrenOf(CryGUID guid)
{
	std::deque<XmlNodeRef> childrenToRemove;
	std::deque<CryGUID> guidsToCheckAgainst;
	CryGUID parentId = CryGUID::Null();
	CryGUID linkedToId = CryGUID::Null();

	for (int i = 0, count = m_objectsNode->getChildCount(); i < count; ++i)
	{
		XmlNodeRef child = m_objectsNode->getChild(i);
		if (child->getAttr("Parent", parentId) && guid == parentId ||
		    child->getAttr("LinkedTo", linkedToId) && guid == linkedToId)
		{
			childrenToRemove.push_front(child);
			string objType;
			if (child->getAttr("Type", objType) && !objType.CompareNoCase("Group"))
			{
				CryGUID groupId;
				child->getAttr("Id", groupId);
				guidsToCheckAgainst.push_back(groupId);
			}
		}
	}

	for (std::deque<XmlNodeRef>::iterator it = childrenToRemove.begin(), end = childrenToRemove.end(); it != end; ++it)
	{
		m_objectsNode->removeChild((*it));
	}

	for (int i = 0, count = guidsToCheckAgainst.size(); i < count; ++i)
	{
		RemoveAllChildrenOf(guidsToCheckAgainst[i]);
	}
}

void CPrefabItem::RegisterPrefabEventFlowNodes(CBaseObject* const pEntityObj)
{
	if (pEntityObj->IsKindOf(RUNTIME_CLASS(CEntityObject)))
	{
		CEntityObject* pEntity = static_cast<CEntityObject*>(pEntityObj);
		if (CHyperFlowGraph* pFG = pEntity->GetFlowGraph())
		{
			IHyperGraphEnumerator* pEnumerator = pFG->GetNodesEnumerator();
			IHyperNode* pNode = pEnumerator->GetFirst();
			while (pNode)
			{
				GetIEditorImpl()->GetPrefabManager()->GetPrefabEvents()->OnHyperGraphManagerEvent(EHG_NODE_ADD, pNode->GetGraph(), pNode);
				pNode = pEnumerator->GetNext();
			}
			pEnumerator->Release();
		}
	}
}
