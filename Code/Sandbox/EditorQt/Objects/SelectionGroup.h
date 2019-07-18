// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

class CBrushObject;
class CGeomEntity;
struct SDisplayContext;
#include <Objects/BaseObject.h>

#include "SandboxAPI.h"
#include "ObjectEvent.h"
#include "Objects/ISelectionGroup.h"
#include <CryCore/smartptr.h>
#include <vector>

/*!
 *	CSelectionGroup is a selection group of objects.
 */
class SANDBOX_API CSelectionGroup : public ISelectionGroup
{
public:
	//! Set name of selection.
	void          SetName(const string& name) { m_name = name; }
	//! Get name of selection.
	const string& GetName() const             { return m_name; }

	//! Reserves space for X amount of objects in the selection list
	void                      Reserve(size_t count);
	//! Adds object into selection list.
	void                      AddObject(CBaseObject* obj);
	//! Remove object from selection list.
	void                      RemoveObject(CBaseObject* obj);
	//! Remove all objects from selection.
	void                      RemoveAll();
	//! Check if object contained in selection list.
	bool                      IsContainObject(CBaseObject* obj) const;
	//! Return true if selection does not contain any object.
	bool                      IsEmpty() const override;
	//! Number of selected object.
	int                       GetCount() const;
	//! Get object at given index.
	CBaseObject*              GetObject(int index) const;
	//! Get all selected objects.
	void                      GetObjects(std::vector<CBaseObject*>& objects) const;
	//! Get object from a GUID
	CBaseObject*              GetObjectByGuid(CryGUID guid) const;
	//! Get object from a GUID in a prefab
	CBaseObject*              GetObjectByGuidInPrefab(CryGUID guid) const;
	//! Get all the objects that satisfy the filter function
	std::vector<CBaseObject*> GetObjectsByFilter(std::function<bool(CBaseObject* pObject)> filter) const override;
	//! Get matrix suitable for manipulator
	bool                      GetManipulatorMatrix(Matrix34& tm) const override;

	//! Get mass center of selected objects.
	Vec3 GetCenter() const;

	//! Get Bounding box of selection.
	AABB GetBounds() const;

	void Copy(const CSelectionGroup& from);

	//! Remove from selection group all objects which have parent also in selection group.
	//! And save resulting objects to saveTo selection.
	void         FilterParents() const override;
	//TODO : This is extremely dangerous as m_filtered now has several potential states and meanings
	void         FilterLinkedObjects() const;
	//! Get number of child filtered objects.
	int          GetFilteredCount() const       { return m_filtered.size(); }
	CBaseObject* GetFilteredObject(int i) const { return m_filtered[i]; }
	//! Saves original positions of filtered objects
	void         SaveFilteredTransform() const override;

	//! Send ENTITY_EVENT_XFORM_FINISHED_EDITOR to selected objects
	void ObjectModified() const override;

	//////////////////////////////////////////////////////////////////////////
	// Operations on selection group.
	//////////////////////////////////////////////////////////////////////////
	//! Move objects in selection by offset.
	void Move(const Vec3& offset, int moveFlags, const CPoint& point = CPoint(-1, -1), bool bFromInitPos = false) const override;
	//! Rotate objects in selection by given quaternion.
	void Rotate(const Quat& qRot) const;
	//! Rotate objects in selection by given angle.
	void Rotate(const Ang3& angles) const;
	//! Rotate objects in selection by given rotation matrix.
	void Rotate(const Matrix34& matRot) const;
	//! Resets rotation and scale to identity and (1.0f, 1.0f, 1.0f)
	void ResetTransformation() const;
	//! Scale objects in selection by given scale.
	void Scale(const Vec3& scale) const;
	void SetScale(const Vec3& scale) const;
	//! Align objects in selection to surface normal
	void Align() const override;

	//! Same as Copy but will copy all objects from hierarchy of current selection to new selection group.
	void FlattenHierarchy(CSelectionGroup& newGroup, bool flattenGroups = false);

	// Send event to all objects in selection group.
	void SendEvent(ObjectEvent event) const;

	void FinishChanges() const override;
private:
	// struct that stores information about initial transforms of objects
	struct STransformElementInit
	{
		// initial position in world space
		Vec3     position;
		// since scale is applied first in transform chain, it is always in local space
		Vec3     scale;
		Matrix33 worldRotation;
		Quat     localRotation;
	};

	string                 m_name;
	typedef std::vector<_smart_ptr<CBaseObject>> Objects;
	Objects                m_objects;
	// Objects set, for fast searches.
	std::set<CBaseObject*> m_objectsSet;

	//! Selection list with child objects filtered out
	//! Is mutable because this list will be invalidated if the selection changes
	//TODO: This should not be a field, FilterParents should simply return a temporary array, as this design is very risky
	mutable std::vector<CBaseObject*>          m_filtered;
	mutable std::vector<STransformElementInit> m_initElementTransforms;
};