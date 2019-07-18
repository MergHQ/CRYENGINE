// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ObjectEvent.h"
#include <CryMath/Cry_Math.h>
#include <CryMath/Cry_Geo.h>

// Windows API defines this
#ifdef GetObject
#undef GetObject
#endif

struct ISelectionGroup
{
public:
	enum EMoveSelectionFlag
	{
		eMS_None = 0x00,
		eMS_FollowTerrain = 0x01,
		eMS_FollowGeometry = 0x02,
		eMS_SnapToNormal = 0x04
	};

	virtual ~ISelectionGroup() {}
	//! Adds object into selection list.
	virtual void AddObject(CBaseObject* obj) = 0;
	//! Remove object from selection list.
	virtual void RemoveObject(CBaseObject* obj) = 0;
	//! Get object at given index.
	virtual CBaseObject* GetObject(int index) const = 0;
	//! Get all the objects that satisfy the filter function
	virtual std::vector<CBaseObject*> GetObjectsByFilter(std::function<bool(CBaseObject* pObject)> filter) const = 0;
	//! Number of selected object.
	virtual int GetCount() const = 0;
	//! Return true if selection does not contain any object.
	virtual bool IsEmpty() const = 0;
	//! Get mass center of selected objects.
	virtual Vec3 GetCenter() const = 0;
	//! Get Bounding box of selection.
	virtual AABB GetBounds() const = 0;
	//! Send event to all objects in selection group.
	virtual void SendEvent(ObjectEvent event) const = 0;
	//! Get matrix suitable for manipulator
	virtual bool GetManipulatorMatrix(Matrix34& tm) const = 0;
	//! Used to notify when objects have been fully transformed
	virtual void FinishChanges() const = 0;
	//! Send ENTITY_EVENT_XFORM_FINISHED_EDITOR to selected objects
	virtual void ObjectModified() const = 0;
	//! Move objects in selection by offset.
	virtual void Move(const Vec3& offset, int moveFlags, const CPoint& point = CPoint(-1, -1), bool bFromInitPos = false) const = 0;
	//! Rotate objects in selection by given quaternion.
	virtual void Rotate(const Quat& qRot) const = 0;
	//! Rotate objects in selection by given angle.
	virtual void Rotate(const Ang3& angles) const = 0;
	//! Rotate objects in selection by given rotation matrix.
	virtual void Rotate(const Matrix34& matRot) const = 0;
	//! Resets rotation and scale to identity and (1.0f, 1.0f, 1.0f)
	virtual void ResetTransformation() const = 0;
	//! Scale objects in selection by given scale.
	virtual void Scale(const Vec3& scale) const = 0;
	virtual void SetScale(const Vec3& scale) const = 0;
	//! Align objects in selection to surface normal
	virtual void Align() const = 0;
	//! Remove from selection group all objects which have parent also in selection group.
	//! And save resulting objects to saveTo selection.
	virtual void         FilterParents() const = 0;
	//! Saves original positions of filtered objects
	virtual void         SaveFilteredTransform() const = 0;
};
