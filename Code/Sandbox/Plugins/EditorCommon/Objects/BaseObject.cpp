// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <Preferences/ViewportPreferences.h>
#include "Objects/BaseObject.h"
#include "Objects/ObjectLoader.h"
#include "Objects/IObjectLayer.h"
#include "Objects/IObjectLayerManager.h"
#include "IDisplayViewport.h"
#include "Objects/DisplayContext.h"
#include "Objects/ISelectionGroup.h"
#include "IIconManager.h"
#include "IObjectManager.h"
#include "Util/Math.h"
#include "Util/AffineParts.h"
#include "Controls/DynamicPopupMenu.h"
#include "EditTool.h"
#include "UsedResources.h"
#include "Material/IEditorMaterial.h"
#include "Grid.h"

#include "IUndoObject.h"

#include <CryMovie/IMovieSystem.h>
#include <CrySystem/ITimer.h>

// To use the Andrew's algorithm in order to make convex hull from the points, this header is needed.
#include "Util/GeometryUtil.h"

#include <CrySerialization/Serializer.h>
#include <CrySerialization/IArchive.h>
#include <CrySerialization/STL.h>
#include <CrySerialization/Color.h>
#include <CrySerialization/Math.h>
#include <CrySerialization/Enum.h>
#include <Serialization/Decorators/EditorActionButton.h>

#include <CrySystem/ICryLink.h>

#include <EditorFramework/Editor.h>
#include <EditorFramework/Preferences.h>
#include <Preferences/GeneralPreferences.h>
#include <Gizmos/GizmoManager.h>

SERIALIZATION_ENUM_BEGIN(ESystemConfigSpec, "SystemConfigSpec")
SERIALIZATION_ENUM(CONFIG_CUSTOM, "notset", "All");
SERIALIZATION_ENUM(CONFIG_LOW_SPEC, "low", "LowSpec");
SERIALIZATION_ENUM(CONFIG_MEDIUM_SPEC, "med", "MedSpec");
SERIALIZATION_ENUM(CONFIG_HIGH_SPEC, "high", "HighSpec");
SERIALIZATION_ENUM(CONFIG_VERYHIGH_SPEC, "veryhigh", "VeryHighSpec");
SERIALIZATION_ENUM(CONFIG_DURANGO, "durango", "XboxOne");
SERIALIZATION_ENUM(CONFIG_ORBIS, "orbis", "PS4");
SERIALIZATION_ENUM(CONFIG_DETAIL_SPEC, "detail", "Detail");
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN(ObjectType, "Object Type")
SERIALIZATION_ENUM(OBJTYPE_GROUP, "group", "Group")
SERIALIZATION_ENUM(OBJTYPE_TAGPOINT, "tagpoint", "Tag Point");
SERIALIZATION_ENUM(OBJTYPE_AIPOINT, "aipoint", "AI Point");
SERIALIZATION_ENUM(OBJTYPE_ENTITY, "entity", "Entity");
SERIALIZATION_ENUM(OBJTYPE_SHAPE, "shape", "Shape");
SERIALIZATION_ENUM(OBJTYPE_VOLUME, "volume", "Volume");
SERIALIZATION_ENUM(OBJTYPE_BRUSH, "brush", "Brush");
SERIALIZATION_ENUM(OBJTYPE_PREFAB, "prefab", "Prefab");
SERIALIZATION_ENUM(OBJTYPE_SOLID, "solid", "Solid");
SERIALIZATION_ENUM(OBJTYPE_CLOUD, "cloud", "Cloud");
SERIALIZATION_ENUM(OBJTYPE_CLOUDGROUP, "cloudgroup", "Cloud Group");
SERIALIZATION_ENUM(OBJTYPE_VOXEL, "voxel", "Voxel");
SERIALIZATION_ENUM(OBJTYPE_ROAD, "road", "Road");
SERIALIZATION_ENUM(OBJTYPE_OTHER, "other", "Other");
SERIALIZATION_ENUM(OBJTYPE_DECAL, "decal", "Decal");
SERIALIZATION_ENUM(OBJTYPE_DISTANCECLOUD, "distancecloud", "Distance Cloud");
SERIALIZATION_ENUM(OBJTYPE_TELEMETRY, "telemetry", "Telemetry");
SERIALIZATION_ENUM(OBJTYPE_REFPICTURE, "refpicture", "Reference Picture");
SERIALIZATION_ENUM(OBJTYPE_GEOMCACHE, "geomcache", "Geometry Cache");
SERIALIZATION_ENUM(OBJTYPE_VOLUMESOLID, "volumesolid", "Volume Solid");
//Intentionally not serializing "any" type since it was mostly used for UI and is not a valid value
SERIALIZATION_ENUM_END()

namespace {
COLORREF kLinkColorParent = RGB(0, 255, 255);
COLORREF kLinkColorChild = RGB(0, 0, 255);
COLORREF kLinkColorGray = RGB(128, 128, 128);

bool GetUniformScale()
{
	bool uniformScale = true;
	GetIEditor()->GetPersonalizationManager()->GetProperty("BaseObject", "UniformScale", uniformScale);
	return uniformScale;
}

void SetUniformScale(bool uniformScale)
{
	GetIEditor()->GetPersonalizationManager()->SetProperty("BaseObject", "UniformScale", uniformScale);
}

}

#define INVALID_POSITION_EPSILON 100000

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for CBaseObject.
class CUndoBaseObject : public IUndoObject
{
public:
	CUndoBaseObject(CBaseObject* pObj, const char* undoDescription);

protected:
	virtual int         GetSize()        { return sizeof(*this); }
	virtual const char* GetDescription() { return m_undoDescription; };
	virtual const char* GetObjectName();

	virtual void        Undo(bool bUndo);
	virtual void        Redo();

protected:
	string     m_undoDescription;
	CryGUID    m_guid;
	XmlNodeRef m_undo;
	XmlNodeRef m_redo;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for CBaseObject that only stores its transform, color, area and minSpec
class CUndoBaseObjectMinimal : public IUndoObject
{
public:
	CUndoBaseObjectMinimal(CBaseObject* obj, const char* undoDescription, int flags);

protected:
	virtual int         GetSize()        { return sizeof(*this); }
	virtual const char* GetDescription() { return m_undoDescription; };
	virtual const char* GetObjectName();

	virtual void        Undo(bool bUndo);
	virtual void        Redo();

private:
	struct StateStruct
	{
		Vec3     pos;
		Quat     rotate;
		Vec3     scale;
		COLORREF color;
		float    area;
		int      minSpec;
	};

	void SetTransformsFromState(CBaseObject* pObject, const StateStruct& state, bool bUndo);

	CryGUID     m_guid;
	string      m_undoDescription;
	StateStruct m_undoState;
	StateStruct m_redoState;
};

//////////////////////////////////////////////////////////////////////////
//! Undo object for attach/detach changes
class CUndoAttachBaseObject : public IUndoObject
{
public:
	CUndoAttachBaseObject(CBaseObject* pAttachedObject, IObjectLayer* pOldLayer, bool bKeepPos, bool bAttach)
		: m_attachedObjectGUID(pAttachedObject->GetId())
		, m_parentObjectGUID(pAttachedObject->GetParent()->GetId())
		, m_oldLayerGUID(pOldLayer->GetGUID())
		, m_bKeepPos(bKeepPos)
		, m_bAttach(bAttach) {}

	virtual void Undo(bool bUndo) override
	{
		if (m_bAttach) Detach();
		else Attach();
	}

	virtual void Redo() override
	{
		if (m_bAttach) Attach();
		else Detach();
	}

private:
	void Attach()
	{
		IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();
		CBaseObject* pObject = pObjectManager->FindObject(m_attachedObjectGUID);
		CBaseObject* pParentObject = pObjectManager->FindObject(m_parentObjectGUID);

		if (pObject && pParentObject)
		{
			//if (pParentObject->IsKindOf(RUNTIME_CLASS(CGroup)))
			if (GetIEditor()->IsCGroup(pParentObject))
				pParentObject->AddMember(pObject, m_bKeepPos);
			else
				pParentObject->AttachChild(pObject, m_bKeepPos);

			pObject->UpdatePrefab();
		}
	}

	void Detach()
	{
		IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();
		CBaseObject* pObject = pObjectManager->FindObject(m_attachedObjectGUID);
		CBaseObject* pParentObject = pObjectManager->FindObject(m_parentObjectGUID);
		IObjectLayer* pOldLayer = pObjectManager->GetIObjectLayerManager()->FindLayer(m_oldLayerGUID);

		if (pObject)
		{
			CBaseObject* pPrefab = pObject->GetPrefab();

			pObject->DetachThis(m_bKeepPos);
			if (pOldLayer != pObject->GetLayer())
			{
				pObject->SetLayer(pOldLayer);
			}

			if (pPrefab)
			{
				pPrefab->AttachChild(pObject, m_bKeepPos);
				pObject->UpdatePrefab();
			}
		}
	}

	virtual int         GetSize()        { return sizeof(CUndoAttachBaseObject); }
	virtual const char* GetDescription() { return "Attachment Changed"; }

	CryGUID m_attachedObjectGUID;
	CryGUID m_parentObjectGUID;
	CryGUID m_oldLayerGUID;
	bool    m_bKeepPos;
	bool    m_bAttach;
};

//////////////////////////////////////////////////////////////////////////
CUndoBaseObject::CUndoBaseObject(CBaseObject* obj, const char* undoDescription)
{
	// Stores the current state of this object.
	assert(obj != 0);
	m_undoDescription = undoDescription;
	m_guid = obj->GetId();

	m_redo = 0;
	m_undo = XmlHelpers::CreateXmlNode("Undo");
	CObjectArchive ar(GetIEditor()->GetObjectManager(), m_undo, false);
	ar.bUndo = true;
	obj->Serialize(ar);

	obj->SetLayerModified();
}

//////////////////////////////////////////////////////////////////////////
const char* CUndoBaseObject::GetObjectName()
{
	if (CBaseObject* obj = GetIEditor()->GetObjectManager()->FindObject(m_guid))
	{
		return obj->GetName();
	}
	return "";
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObject::Undo(bool bUndo)
{
	CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!pObject)
		return;

	GetIEditor()->GetIUndoManager()->Suspend();

	if (bUndo)
	{
		m_redo = XmlHelpers::CreateXmlNode("Redo");
		// Save current object state.
		CObjectArchive ar(GetIEditor()->GetObjectManager(), m_redo, false);
		ar.bUndo = true;
		pObject->Serialize(ar);
	}

	// Undo object state.
	CObjectArchive ar(GetIEditor()->GetObjectManager(), m_undo, true);
	ar.bUndo = true;
	IObjectLayer* pLayer = pObject->GetLayer();
	pObject->Serialize(ar);
	pObject->SetLayerModified();

	if (pLayer && pLayer != pObject->GetLayer())
	{
		pLayer->SetModified();
	}

	if (bUndo)
	{
		pObject->UpdateGroup();
		pObject->UpdatePrefab();
	}

	GetIEditor()->GetIUndoManager()->Resume();
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObject::Redo()
{
	CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!pObject)
		return;

	GetIEditor()->GetIUndoManager()->Suspend();

	CObjectArchive ar(GetIEditor()->GetObjectManager(), m_redo, true);
	ar.bUndo = true;
	IObjectLayer* pLayer = pObject->GetLayer();
	pObject->Serialize(ar);
	pObject->SetLayerModified();

	if (pLayer && pLayer != pObject->GetLayer())
	{
		pLayer->SetModified();
	}

	pObject->UpdateGroup();
	pObject->UpdatePrefab();

	GetIEditor()->GetIUndoManager()->Resume();
}

//////////////////////////////////////////////////////////////////////////
CUndoBaseObjectMinimal::CUndoBaseObjectMinimal(CBaseObject* pObj, const char* undoDescription, int flags)
{
	// Stores the current state of this object.
	assert(pObj != nullptr);
	m_undoDescription = undoDescription;
	m_guid = pObj->GetId();

	ZeroStruct(m_redoState);

	m_undoState.pos = pObj->GetPos();
	m_undoState.rotate = pObj->GetRotation();
	m_undoState.scale = pObj->GetScale();
	m_undoState.color = pObj->GetColor();
	m_undoState.area = pObj->GetArea();
	m_undoState.minSpec = pObj->GetMinSpec();

	pObj->SetLayerModified();
}

//////////////////////////////////////////////////////////////////////////
const char* CUndoBaseObjectMinimal::GetObjectName()
{
	CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!pObject)
		return "";

	return pObject->GetName();
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObjectMinimal::Undo(bool bUndo)
{
	CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!pObject)
		return;

	if (bUndo)
	{
		m_redoState.pos = pObject->GetPos();
		m_redoState.scale = pObject->GetScale();
		m_redoState.rotate = pObject->GetRotation();
		m_redoState.color = pObject->GetColor();
		m_redoState.area = pObject->GetArea();
		m_redoState.minSpec = pObject->GetMinSpec();
	}

	SetTransformsFromState(pObject, m_undoState, bUndo);

	pObject->ChangeColor(m_undoState.color);
	pObject->SetArea(m_undoState.area);
	pObject->SetMinSpec(m_undoState.minSpec, false);
	pObject->SetLayerModified();

	if (bUndo)
		pObject->UpdateGroup();
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObjectMinimal::Redo()
{
	CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	if (!pObject)
		return;

	SetTransformsFromState(pObject, m_redoState, true);

	pObject->ChangeColor(m_redoState.color);
	pObject->SetArea(m_redoState.area);
	pObject->SetMinSpec(m_redoState.minSpec, false);
	pObject->SetLayerModified();
	pObject->UpdateGroup();
}

//////////////////////////////////////////////////////////////////////////
void CUndoBaseObjectMinimal::SetTransformsFromState(CBaseObject* pObject, const StateStruct& state, bool bUndo)
{
	uint32 flags = eObjectUpdateFlags_Undo;
	if (!bUndo)
	{
		flags |= eObjectUpdateFlags_UserInputUndo;
	}

	pObject->SetPos(state.pos, flags);
	pObject->SetScale(state.scale, flags);
	pObject->SetRotation(state.rotate, flags);
}

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CBaseObject, CObject);

//////////////////////////////////////////////////////////////////////////
void CObjectCloneContext::AddClone(CBaseObject* pFromObject, CBaseObject* pToObject)
{
	m_objectsMap[pFromObject] = pToObject;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectCloneContext::FindClone(CBaseObject* pFromObject)
{
	CBaseObject* pTarget = stl::find_in_map(m_objectsMap, pFromObject, (CBaseObject*) NULL);
	return pTarget;
}

//////////////////////////////////////////////////////////////////////////
// CBaseObject implementation.
//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
{
	m_pos(0, 0, 0);
	m_rotate.SetIdentity();
	m_scale(1, 1, 1);

	m_color = RGB(255, 255, 255);
	m_pMaterial = NULL;

	m_flags = 0;
	m_flattenArea = 0;

	m_numRefs = 0;
	m_guid = CryGUID::Null();
	m_guidInPrefab = CryGUID::Null();

	m_layer = 0;

	m_parent = 0;

	m_lookat = 0;
	m_lookatSource = 0;

	m_bMatrixInWorldSpace = false;
	m_bMatrixValid = false;
	m_bWorldBoxValid = false;
	m_bUseMaterialLayersMask = false;

	m_nTextureIcon = 0;
	m_nMaterialLayersMask = 0;
	m_nMinSpec = 0;

	m_worldTM.SetIdentity();
	m_worldBounds.min.Set(0, 0, 0);
	m_worldBounds.max.Set(0, 0, 0);

	m_floorNumber = -1;

	m_vDrawIconPos = Vec3(0, 0, 0);
	m_nIconFlags = 0;
	m_bInSelectionBox = false;

	m_pTransformDelegate = nullptr;
	m_bSupportsBoxHighlight = true;
}

//////////////////////////////////////////////////////////////////////////
IObjectManager* CBaseObject::GetObjectManager() const
{
	return GetIEditor()->GetObjectManager();
};

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetClassDesc(CObjectClassDesc* classDesc)
{
	m_classDesc = classDesc;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::Init(CBaseObject* prev, const string& file)
{
	ClearFlags(OBJFLAG_DELETED);

	if (prev != 0)
	{
		// Same layer.
		SetLayer(prev->GetLayer());

		SetUniqName(prev->GetName());
		SetLocalTM(prev->GetPos(), prev->GetRotation(), prev->GetScale());
		SetArea(prev->GetArea());
		SetColor(prev->GetColor());
		m_nMaterialLayersMask = prev->m_nMaterialLayersMask;
		SetMaterial(prev->GetMaterial());
		SetMinSpec(prev->GetMinSpec(), false);

		// Copy all basic variables.
		EnableUpdateCallbacks(false);
		CopyVariableValues(prev);
		EnableUpdateCallbacks(true);
		OnSetValues();
	}

	m_nTextureIcon = m_classDesc->GetTextureIconId();
	if (m_classDesc->RenderTextureOnTop())
	{
		SetFlags(OBJFLAG_SHOW_ICONONTOP);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject::~CBaseObject()
{
	for (Childs::iterator c = m_childs.begin(); c != m_childs.end(); c++)
	{
		CBaseObject* child = *c;
		child->m_parent = 0;
	}
	m_childs.clear();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Done()
{
	CBaseObject* pGroup = GetGroup();

	// From parent
	if (pGroup)
	{
		bool bSuspended(GetIEditor()->SuspendUpdateCGroup(pGroup));
		GetIEditor()->RemoveMemberCGroup(pGroup, this);
		if (bSuspended)
			GetIEditor()->ResumeUpdateCGroup(pGroup);
	}
	else
	{
		DetachThis();
	}

	// From children
	DetachAll();

	SetLookAt(0);
	if (m_lookatSource)
	{
		m_lookatSource->SetLookAt(0);
	}
	SetFlags(OBJFLAG_DELETED);

	NotifyListeners(OBJECT_ON_DELETE);
	m_eventListeners.clear();

	if (m_pMaterial)
	{
		m_pMaterial->Release();
		m_pMaterial = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetName(const string& name)
{
	if (name == m_name)
		return;

	StoreUndo("Name");

	m_name = name;
	GetObjectManager()->RegisterObjectName(name);
	SetModified(false);

	NotifyListeners(OBJECT_ON_RENAME);
	GetIEditor()->GetObjectManager()->NotifyObjectListeners(this, OBJECT_ON_RENAME);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetUniqName(const string& name)
{
	if (name == m_name)
		return;
	SetName(GetObjectManager()->GenUniqObjectName(name));
}

//////////////////////////////////////////////////////////////////////////
const string& CBaseObject::GetName() const
{
	return m_name;
}

//////////////////////////////////////////////////////////////////////////
string CBaseObject::GetWarningsText() const
{
	string warnings;

	if (gViewportDebugPreferences.showScaleWarnings)
	{
		const EScaleWarningLevel scaleWarningLevel = GetScaleWarningLevel();
		if (scaleWarningLevel == eScaleWarningLevel_Rescaled)
		{
			warnings += "\\n  Warning: Object Scale is not 100%.";
		}
		else if (scaleWarningLevel == eScaleWarningLevel_RescaledNonUniform)
		{
			warnings += "\\n  Warning: Object has non-uniform scale.";
		}
	}

	if (gViewportDebugPreferences.showRotationWarnings)
	{
		const ERotationWarningLevel rotationWarningLevel = GetRotationWarningLevel();

		if (rotationWarningLevel == eRotationWarningLevel_Rotated)
		{
			warnings += "\\n  Warning: Object is rotated.";
		}
		else if (rotationWarningLevel == eRotationWarningLevel_RotatedNonRectangular)
		{
			warnings += "\\n  Warning: Object is rotated non-orthogonally.";
		}
	}

	return warnings;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsSameClass(CBaseObject* obj)
{
	return GetClassDesc() == obj->GetClassDesc();
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetPos(const Vec3& pos, int flags)
{
	auto currentPos = GetPos();

	bool equal = false;
	if (flags & eObjectUpdateFlags_MoveTool) // very sensitive in case of the move tool
	{
		equal = IsVectorsEqual(currentPos, pos, 0.0f);
	}
	else // less sensitive for others
	{
		equal = IsVectorsEqual(currentPos, pos);
	}

	if (equal)
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Check if position is bad.
	if (fabs(pos.x) > INVALID_POSITION_EPSILON ||
	    fabs(pos.y) > INVALID_POSITION_EPSILON ||
	    fabs(pos.z) > INVALID_POSITION_EPSILON ||
	    !_finite(pos.x) || !_finite(pos.y) || !_finite(pos.z))
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING,
		           "Object %s, SetPos called with invalid position: (%f,%f,%f) %s",
		           (const char*)GetName(), pos.x, pos.y, pos.z,
		           CryLinkService::CCryLinkUriFactory::GetUriV("Editor", "general.select_and_go_to_object %s", GetName()));
		return false;
	}

	if (ShouldNotifyOfUpcomingAABBChanges())
	{
		OnBeforeAreaChange();
	}

	const bool ignoreSelection = (flags & eObjectUpdateFlags_IgnoreSelection) != 0;
	const bool bPositionDelegated = m_pTransformDelegate && m_pTransformDelegate->IsPositionDelegated(ignoreSelection);

	if (m_pTransformDelegate && (flags & eObjectUpdateFlags_Animated) == 0)
	{
		m_pTransformDelegate->SetTransformDelegatePos(pos, ignoreSelection);
	}

	//////////////////////////////////////////////////////////////////////////
	if (!bPositionDelegated && (flags & eObjectUpdateFlags_RestoreUndo) == 0 && (flags & eObjectUpdateFlags_Animated) == 0)
	{
		StoreUndo("Position", true, flags);
	}

	if (!bPositionDelegated)
	{
		m_pos = pos;
	}

	if (!(flags & eObjectUpdateFlags_DoNotInvalidate))
	{
		InvalidateTM(flags | eObjectUpdateFlags_PositionChanged);
	}

	SetModified(true);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetRotation(const Quat& rotate, int flags)
{
	if (!IsRotatable())
	{
		return false;
	}

	const Quat currentRotate = GetRotation();

	if (currentRotate.w == rotate.w && currentRotate.v.x == rotate.v.x
	    && currentRotate.v.y == rotate.v.y && currentRotate.v.z == rotate.v.z)
	{
		return false;
	}

	if (flags & eObjectUpdateFlags_ScaleTool)
	{
		return false;
	}

	if (ShouldNotifyOfUpcomingAABBChanges())
	{
		OnBeforeAreaChange();
	}

	const bool ignoreSelection = (flags & eObjectUpdateFlags_IgnoreSelection) != 0;
	const bool bRotationDelegated = m_pTransformDelegate && m_pTransformDelegate->IsRotationDelegated(ignoreSelection);
	if (m_pTransformDelegate && (flags & eObjectUpdateFlags_Animated) == 0)
	{
		m_pTransformDelegate->SetTransformDelegateRotation(rotate, ignoreSelection);
	}

	if (!bRotationDelegated && (flags & eObjectUpdateFlags_RestoreUndo) == 0 && (flags & eObjectUpdateFlags_Animated) == 0)
	{
		StoreUndo("Rotate", true, flags);
	}

	if (!bRotationDelegated)
	{
		m_rotate = rotate;
	}

	if (m_bMatrixValid && !(flags & eObjectUpdateFlags_DoNotInvalidate))
	{
		InvalidateTM(flags | eObjectUpdateFlags_RotationChanged);
	}

	SetModified(true);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetScale(const Vec3& scale, int flags)
{
	if (!IsScalable())
	{
		return false;
	}

	if (IsVectorsEqual(GetScale(), scale))
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Check if position is bad.
	if (scale.x < 0.01 || scale.y < 0.01 || scale.z < 0.01)
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Object %s, SetScale called with invalid scale: (%f,%f,%f) %s",
			(const char*)GetName(), scale.x, scale.y, scale.z,
			CryLinkService::CCryLinkUriFactory::GetUriV("Editor", "general.select_and_go_to_object %s", GetName()));
		return false;
	}
	//////////////////////////////////////////////////////////////////////////

	if (ShouldNotifyOfUpcomingAABBChanges())
	{
		OnBeforeAreaChange();
	}

	const bool ignoreSelection = (flags & eObjectUpdateFlags_IgnoreSelection) != 0;
	const bool bScaleDelegated = m_pTransformDelegate && m_pTransformDelegate->IsScaleDelegated(ignoreSelection);
	if (m_pTransformDelegate && (flags & eObjectUpdateFlags_Animated) == 0)
	{
		m_pTransformDelegate->SetTransformDelegateScale(scale, ignoreSelection);
	}

	if (!bScaleDelegated && (flags & eObjectUpdateFlags_RestoreUndo) == 0 && (flags & eObjectUpdateFlags_Animated) == 0)
	{
		StoreUndo("Scale", true, flags);
	}

	if (!bScaleDelegated)
	{
		m_scale = scale;
	}

	if (m_bMatrixValid && !(flags & eObjectUpdateFlags_DoNotInvalidate))
	{
		InvalidateTM(flags | eObjectUpdateFlags_ScaleChanged);
	}

	SetModified(true);

	return true;
}

//////////////////////////////////////////////////////////////////////////
const Vec3 CBaseObject::GetPos() const
{
	if (!m_pTransformDelegate)
	{
		return m_pos;
	}

	return m_pTransformDelegate->GetTransformDelegatePos(m_pos);
}

//////////////////////////////////////////////////////////////////////////
const Quat CBaseObject::GetRotation() const
{
	if (!m_pTransformDelegate)
	{
		return m_rotate;
	}

	return m_pTransformDelegate->GetTransformDelegateRotation(m_rotate);
}

//////////////////////////////////////////////////////////////////////////
const Vec3 CBaseObject::GetScale() const
{
	if (!m_pTransformDelegate)
	{
		return m_scale;
	}

	return m_pTransformDelegate->GetTransformDelegateScale(m_scale);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::ChangeColor(COLORREF color)
{
	if (color == m_color)
		return;

	StoreUndo("Color", true);

	SetColor(color);
	SetModified(false);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetColor(COLORREF color)
{
	m_color = color;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetArea(float area)
{
	if (m_flattenArea == area)
		return;

	StoreUndo("Area", true);

	m_flattenArea = area;
	SetModified(false);
};

void CBaseObject::GetDisplayBoundBox(AABB& box)
{
	GetBoundBox(box);
}

//! Get bounding box of object in world coordinate space.
void CBaseObject::GetBoundBox(AABB& box)
{
	if (!m_bWorldBoxValid)
	{
		GetLocalBounds(m_worldBounds);
		if (!m_worldBounds.IsReset() && !m_worldBounds.IsEmpty())
		{
			m_worldBounds.SetTransformedAABB(GetWorldTM(), m_worldBounds);
			m_bWorldBoxValid = true;
		}
	}
	box = m_worldBounds;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::GetLocalBounds(AABB& box)
{
	box.min.Set(0, 0, 0);
	box.max.Set(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateUIVars()
{
	// Just notify inspector, he'll take care of the rest
	NotifyListeners(OBJECT_ON_UI_PROPERTY_CHANGED);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CBaseObject::BeginEditParams(int flags)
{
	SetFlags(OBJFLAG_EDITING);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::EndEditParams()
{
	ClearFlags(OBJFLAG_EDITING);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
	SetFlags(OBJFLAG_EDITING);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::EndEditMultiSelParams()
{
	ClearFlags(OBJFLAG_EDITING);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetModified(bool boModifiedTransformOnly)
{
	GetObjectManager()->OnObjectModified(this, false, boModifiedTransformOnly);
	// Send signal to prefab if this object is part of any
	if (boModifiedTransformOnly)
		UpdatePrefab(eOCOT_ModifyTransform);
	else
		UpdatePrefab();
}

void CBaseObject::DrawDefault(DisplayContext& dc, COLORREF labelColor)
{
	Vec3 wp = GetWorldPos();

	bool bDisplaySelectionHelper = false;
	if (!CanBeDrawn(dc, bDisplaySelectionHelper))
		return;

	// Draw link between parent and child.
	if (dc.flags & DISPLAY_LINKS)
	{
		bool prefabOpenedCheck = true;
		// If we are part of a prefab draw links only if the prefab is in opened state
		if (CheckFlags(OBJFLAG_PREFAB))
		{
			CBaseObject* pPrefab = GetPrefab();
			prefabOpenedCheck = pPrefab && GetIEditor()->IsGroupOpen(pPrefab);
		}

		if (GetParent() && GetParent() != GetGroup() && prefabOpenedCheck)
		{
			dc.DrawLine(GetParentAttachPointWorldTM().GetTranslation(), wp, IsFrozen() ? kLinkColorGray : kLinkColorParent, IsFrozen() ? kLinkColorGray : kLinkColorChild);
		}
		int nChildCount = GetChildCount();
		if (nChildCount && !GetIEditor()->IsCGroup(this))
		{
			for (int i = 0; i < nChildCount; ++i)
			{
				const CBaseObject* pChild = GetChild(i);
				if (prefabOpenedCheck)
				{
					dc.DrawLine(pChild->GetParentAttachPointWorldTM().GetTranslation(), pChild->GetWorldPos(), pChild->IsFrozen() ? kLinkColorGray : kLinkColorParent, pChild->IsFrozen() ? kLinkColorGray : kLinkColorChild);
				}
			}
		}
	}

	// Draw Bounding box
	if (dc.flags & DISPLAY_BBOX)
	{
		AABB box;
		GetBoundBox(box);
		dc.SetColor(Vec3(1, 1, 1));
		dc.DrawWireBox(box.min, box.max);
	}

	if (gViewportPreferences.displaySelectedObjectOrientation && IsSelected())
	{
		float textSize = 1.4;
		const Matrix34& m = GetWorldTM();
		float scale = dc.view->GetScreenScaleFactor(m.GetTranslation()) * gGizmoPreferences.axisGizmoSize * 0.5f;
		Vec3 xVec = m.GetTranslation() + scale * m.GetColumn0().GetNormalized();
		Vec3 yVec = m.GetTranslation() + scale * m.GetColumn1().GetNormalized();
		Vec3 zVec = m.GetTranslation() + scale * m.GetColumn2().GetNormalized();

		unsigned int oldFlags = dc.GetState();
		dc.DepthTestOff();
		dc.SetLineWidth(2.0f);

		dc.SetColor(Vec3(1, 0, 0));
		dc.DrawLine(m.GetTranslation(), xVec);

		dc.SetColor(Vec3(0, 1, 0));
		dc.DrawLine(m.GetTranslation(), yVec);

		dc.SetColor(Vec3(0, 0, 1));
		dc.DrawLine(m.GetTranslation(), zVec);

		if (gGizmoPreferences.axisGizmoText)
		{
			dc.SetColor(Vec3(1, 1, 1));
			dc.DrawTextLabel(ConvertToTextPos(xVec, Matrix34::CreateIdentity(), dc.view, dc.flags & DISPLAY_2D), textSize, "x");
			dc.DrawTextLabel(ConvertToTextPos(yVec, Matrix34::CreateIdentity(), dc.view, dc.flags & DISPLAY_2D), textSize, "y");
			dc.DrawTextLabel(ConvertToTextPos(zVec, Matrix34::CreateIdentity(), dc.view, dc.flags & DISPLAY_2D), textSize, "z");
		}

		dc.SetState(oldFlags);
	}

	if (IsHighlighted())
	{
		DrawHighlight(dc);
	}

	if (IsSelected())
	{
		DrawArea(dc);

		ISelectionGroup* pSelection = GetIEditor()->GetISelectionGroup();

		// If the number of selected object is over 2, the merged boundbox should be used to render the measurement axis.
		if (!pSelection || (pSelection && pSelection->GetCount() == 1))
		{
			DrawDimensions(dc);
		}
	}

	if (bDisplaySelectionHelper)
	{
		DrawSelectionHelper(dc, wp, labelColor, 1.0f);
	}
	else if (!(dc.flags & DISPLAY_HIDENAMES))
	{
		DrawLabel(dc, wp, labelColor);
	}

	SetDrawTextureIconProperties(dc, wp);
	DrawTextureIcon(dc, wp);
	DrawWarningIcons(dc, wp);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawDimensions(DisplayContext& dc, AABB* pMergedBoundBox)
{
	if (HasMeasurementAxis() && gViewportPreferences.displayDimension)
	{
		AABB localBoundBox;
		GetLocalBounds(localBoundBox);
		DrawDimensionsImpl(dc, localBoundBox, pMergedBoundBox);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawDimensionsImpl(DisplayContext& dc, const AABB& localBoundBox, AABB* pMergedBoundBox)
{
	AABB boundBox;
	Matrix34 rotatedTM;
	bool bHave2Axis(false);
	float width(0);
	float height(0);
	float depth(0);

	if (pMergedBoundBox)
	{
		rotatedTM = Matrix34::CreateIdentity();
		boundBox = *pMergedBoundBox;
		width = boundBox.max.x - boundBox.min.x;
		height = boundBox.max.z - boundBox.min.z;
		depth = boundBox.max.y - boundBox.min.y;
	}
	else
	{
		rotatedTM = GetWorldRotTM();
		Matrix34 scaledTranslatedTM = GetWorldScaleTM();
		scaledTranslatedTM.SetTranslation(GetWorldPos());
		boundBox.SetTransformedAABB(scaledTranslatedTM, localBoundBox);

		IVariable* pVarWidth(NULL);
		IVariable* pVarDepth(NULL);
		IVariable* pVarHeight(NULL);

		IVariable* pVarDimX(NULL);
		IVariable* pVarDimY(NULL);
		IVariable* pVarDimZ(NULL);

		auto scale = GetScale();

		CVarBlock* pVarBlock(GetVarBlock());
		if (pVarBlock)
		{
			pVarWidth = pVarBlock->FindVariable("Width");
			pVarDepth = pVarBlock->FindVariable("Length");
			pVarHeight = pVarBlock->FindVariable("Height");
			pVarDimX = pVarBlock->FindVariable("DimX");
			pVarDimY = pVarBlock->FindVariable("DimY");
			pVarDimZ = pVarBlock->FindVariable("DimZ");
		}

		width = boundBox.max.x - boundBox.min.x;
		height = boundBox.max.z - boundBox.min.z;
		depth = boundBox.max.y - boundBox.min.y;

		if (pVarDimX && pVarDimY && pVarDimZ)
		{
			pVarDimX->Get(width);
			pVarDimZ->Get(height);
			pVarDimY->Get(depth);
			width *= scale.x;
			height *= scale.z;
			depth *= scale.y;
		}
		else if (pVarWidth && pVarDepth && pVarHeight)
		{
			// A case of an area box.
			pVarWidth->Get(width);
			pVarHeight->Get(height);
			pVarDepth->Get(depth);
			width *= scale.x;
			height *= scale.z;
			depth *= scale.y;
		}
		else if (!pVarWidth && !pVarDepth && pVarHeight)
		{
			// A case of an area shape.
			pVarHeight->Get(height);
			height *= scale.z;
		}
	}

	const float kMinimumLimitation(0.4f);
	if (width < kMinimumLimitation && height < kMinimumLimitation && depth < kMinimumLimitation)
		return;

	const float kEpsilon(0.001f);
	bHave2Axis = GetType() == OBJTYPE_DECAL || fabs(height) < kEpsilon;

	Vec3 basePoints[] = {
		Vec3(boundBox.min.x, boundBox.min.y, boundBox.min.z),
		Vec3(boundBox.min.x, boundBox.max.y, boundBox.min.z),
		Vec3(boundBox.max.x, boundBox.max.y, boundBox.min.z),
		Vec3(boundBox.max.x, boundBox.min.y, boundBox.min.z),
		Vec3(boundBox.min.x, boundBox.min.y, boundBox.max.z),
		Vec3(boundBox.min.x, boundBox.max.y, boundBox.max.z),
		Vec3(boundBox.max.x, boundBox.max.y, boundBox.max.z),
		Vec3(boundBox.max.x, boundBox.min.y, boundBox.max.z)
	};
	const int kElementSize(sizeof(basePoints) / sizeof(*basePoints));
	Vec3 axisDirections[kElementSize] = { Vec3(1, 1, 1), Vec3(1, -1, 1), Vec3(-1, -1, 1), Vec3(-1, 1, 1), Vec3(1, 1, -1), Vec3(1, -1, -1), Vec3(-1, -1, -1), Vec3(-1, 1, -1) };
	int nLoopCount = bHave2Axis ? (kElementSize / 2) : kElementSize;
	if (bHave2Axis)
	{
		for (int i = 0; i < nLoopCount; ++i)
			basePoints[i].z = 0.5f * (boundBox.min.z + boundBox.max.z);
	}

	// Find out the nearest base point of a boundbox from a camera position and use it as a pivot.
	const CCamera& camera = gEnv->pRenderer->GetCamera();
	Vec3 cameraPos(camera.GetPosition());
	Vec3 pivot(rotatedTM.TransformVector(basePoints[0] - GetWorldPos()) + GetWorldPos());
	float fNearestDist = (cameraPos - pivot).GetLength();
	int nNearestAxisIndex(0);
	bool bPrevVisible(camera.IsPointVisible(pivot));

	for (int i = 1; i < nLoopCount; ++i)
	{
		Vec3 candidatePivot(rotatedTM.TransformVector(basePoints[i] - GetWorldPos()) + GetWorldPos());
		float candidateLength = (candidatePivot - cameraPos).GetLength();
		bool bVisible = camera.IsPointVisible(candidatePivot);
		if (bVisible)
		{
			if (!bPrevVisible || candidateLength < fNearestDist)
			{
				fNearestDist = candidateLength;
				pivot = candidatePivot;
				nNearestAxisIndex = i;
			}
			bPrevVisible = bVisible;
		}
	}

	float fOriginArrowScale = dc.view->GetScreenScaleFactor(pivot) * 0.04f;

	Vec3 vX(width, 0, 0);
	Vec3 vY(0, depth, 0);
	Vec3 vZ(0, 0, height);

	vX = vX * axisDirections[nNearestAxisIndex].x;
	vY = vY * axisDirections[nNearestAxisIndex].y;
	vZ = vZ * axisDirections[nNearestAxisIndex].z;
	vX = rotatedTM.TransformVector(vX);
	vY = rotatedTM.TransformVector(vY);
	vZ = rotatedTM.TransformVector(vZ);

	const float kArrowPivotOffset = 0.1f;
	pivot = pivot + (-(vX + vY + vZ)).GetNormalized() * kArrowPivotOffset;

	Vec3 centerPt(boundBox.GetCenter());

	// Display texts of width, height and depth
	float fTextScale(1.3f);
	dc.SetColor(RGB(200, 200, 200));
	string str;

	const float kBrightness(0.35f);
	const float kTextBrightness(0.7f);
	const ColorF kXColor(1.0f, kBrightness, kBrightness, 0.9f);
	const ColorF kYColor(kBrightness, 1.0f, kBrightness, 0.9f);
	const ColorF kZColor(kBrightness, kBrightness, 1.0f, 0.9f);
	const ColorF kTextXColor(1.0f, kTextBrightness, kTextBrightness, 0.9f);
	const ColorF kTextYColor(kTextBrightness, 1.0f, kTextBrightness, 0.9f);
	const ColorF kTextZColor(kTextBrightness, kTextBrightness, 1.0f, 0.9f);
	const ColorF TextBoxColor(0, 0, 0, 0.75f);

	ColorB backupcolor = dc.GetColor();
	uint32 backupstate = dc.GetState();
	int backupThickness = dc.GetLineWidth();

	dc.SetState(backupstate | e_DepthTestOff);

	Vec3 vNX = vX.GetNormalized();
	Vec3 vNY = vY.GetNormalized();
	Vec3 vNZ = vZ.GetNormalized();

	const float kMinimumOffset(0.20f);
	const float kMaximumOffset(30.0f);
	float fMaximumOffset[3] = { kMaximumOffset, kMaximumOffset, kMaximumOffset };
	if (width > kMaximumOffset * 0.5f)
		fMaximumOffset[0] = width * 3.0f;
	if (depth > kMaximumOffset * 0.5f)
		fMaximumOffset[1] = depth * 3.0f;
	if (height > kMaximumOffset * 0.5f)
		fMaximumOffset[2] = height * 3.0f;
	Vec3 textPos[3] = { pivot, pivot, pivot };
	Vec3 textMinPos[3] = { pivot + vNX * kMinimumOffset, pivot + vNY * kMinimumOffset, pivot + vNZ * kMinimumOffset };
	Vec3 textCenterPos[3] = { pivot + vX * 0.5f, pivot + vY * 0.5f, pivot + vZ * 0.5f };
	Vec3 textMaxPos[3] = { pivot + vNX * fMaximumOffset[0], pivot + vNY * fMaximumOffset[1], pivot + vNZ * fMaximumOffset[2] };

	const Vec3& cameraDir(camera.GetViewdir());

	for (int i = 0; i < 3; ++i)
	{
		Vec3 d = (textMaxPos[i] - cameraPos).GetNormalized();
		float fCameraDir = d.Dot(cameraDir);
		if (fCameraDir < 0)
			fCameraDir = 0;
		textPos[i] = textMinPos[i] + (textCenterPos[i] - textPos[i]) * fCameraDir;
	}

	SRayHitInfo hitInfo;
	bool bVisiableText[3] = { true, true, true };
	for (int i = 0; i < 3; ++i)
	{
		if (GetIEditor()->PickObject(cameraPos, textPos[i] - cameraPos, hitInfo, this))
		{
			if (hitInfo.fDistance < cameraPos.GetDistance(textPos[i]))
				bVisiableText[i] = false;
		}
	}

	if (bVisiableText[0])
	{
		str.Format("%.3f", width);
		DrawTextOn2DBox(dc, textPos[0], str, fTextScale, kTextXColor, TextBoxColor);
	}
	if (!bHave2Axis)
	{
		if (bVisiableText[2])
		{
			str.Format("%.3f", height);
			DrawTextOn2DBox(dc, textPos[2], str, fTextScale, kTextZColor, TextBoxColor);
		}
	}
	if (bVisiableText[1])
	{
		str.Format("%.3f", depth);
		DrawTextOn2DBox(dc, textPos[1], str, fTextScale, kTextYColor, TextBoxColor);
	}

	dc.SetState(backupstate | e_DepthTestOn);
	dc.SetLineWidth(4);

	// Draw arrows of each axis.
	dc.SetColor(kXColor);
	Vec3 xVec = pivot + vX;
	float fxArrowScale = dc.view->GetScreenScaleFactor(xVec) * 0.04f;
	dc.DrawArrow(pivot, xVec, fxArrowScale);
	dc.DrawArrow(xVec, pivot, fOriginArrowScale);
	if (!bHave2Axis)
	{
		dc.SetColor(kZColor);
		Vec3 zVec = pivot + vZ;
		float fzArrowScale = dc.view->GetScreenScaleFactor(zVec) * 0.04f;
		dc.DrawArrow(pivot, zVec, fzArrowScale);
		dc.DrawArrow(zVec, pivot, fOriginArrowScale);
	}
	dc.SetColor(kYColor);
	Vec3 yVec = pivot + vY;
	float fyArrowScale = dc.view->GetScreenScaleFactor(yVec) * 0.04f;
	dc.DrawArrow(pivot, yVec, fyArrowScale);
	dc.DrawArrow(yVec, pivot, fOriginArrowScale);

	dc.SetState(backupstate);
	dc.SetColor(backupcolor);
	dc.SetLineWidth(backupThickness);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawTextOn2DBox(DisplayContext& dc, const Vec3& pos, const char* text, float textScale, const ColorF& TextColor, const ColorF& TextBackColor)
{
	Vec3 worldPos = dc.ToWorldPos(pos);
	int vx, vy, vw, vh;
	gEnv->pRenderer->GetViewport(&vx, &vy, &vw, &vh);

	const CCamera& camera = gEnv->pRenderer->GetCamera();
	Vec3 screenPos;
	camera.Project(worldPos, screenPos, Vec2i(0, 0), Vec2i(0, 0));

	//! Font size information doesn't seem to exist so the proper size is used
	int textlen = strlen(text);
	float fontsize = 7.5f;
	float textwidth = fontsize * textlen;
	float textheight = 16.0f;

	screenPos.x = screenPos.x - textwidth * 0.5f;

	Vec3 textregion[4] = {
		Vec3(screenPos.x,             screenPos.y,              screenPos.z),
		Vec3(screenPos.x + textwidth, screenPos.y,              screenPos.z),
		Vec3(screenPos.x + textwidth, screenPos.y + textheight, screenPos.z),
		Vec3(screenPos.x,             screenPos.y + textheight, screenPos.z)
	};

	Vec3 textworldreign[4];
	Matrix34 dcInvTm = dc.GetMatrix().GetInverted();

	Matrix44A mProj, mView;
	mathMatrixPerspectiveFov(&mProj, camera.GetFov(), camera.GetProjRatio(), camera.GetNearPlane(), camera.GetFarPlane());
	mathMatrixLookAt(&mView, camera.GetPosition(), camera.GetPosition() + camera.GetViewdir(), Vec3(0, 0, 1));
	Matrix44A mInvViewProj = (mView * mProj).GetInverted();

	for (int i = 0; i < 4; ++i)
	{
		Vec4 projectedpos = Vec4((textregion[i].x - vx) / vw * 2.0f - 1.0f,
		                         -((textregion[i].y - vy) / vh) * 2.0f + 1.0f,
		                         textregion[i].z,
		                         1.0f);

		Vec4 wp = projectedpos * mInvViewProj;
		wp.x /= wp.w;
		wp.y /= wp.w;
		wp.z /= wp.w;
		textworldreign[i] = dcInvTm.TransformPoint(Vec3(wp.x, wp.y, wp.z));
	}

	ColorB backupcolor = dc.GetColor();
	uint32 backupstate = dc.GetState();

	dc.SetColor(TextBackColor);
	dc.SetDrawInFrontMode(true);

	dc.DrawQuad(textworldreign[3], textworldreign[2], textworldreign[1], textworldreign[0]);
	dc.SetColor(TextColor);
	dc.DrawTextLabel(pos, textScale, text);

	dc.SetDrawInFrontMode(false);
	dc.SetColor(backupcolor);
	dc.SetState(backupstate);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawSelectionHelper(DisplayContext& dc, const Vec3& pos, COLORREF labelColor, float alpha)
{
	DrawLabel(dc, pos, labelColor);

	dc.SetColor(GetColor());
	if (IsHighlighted() || IsSelected() || IsInSelectionBox())
		dc.SetColor(dc.GetSelectedColor());

	uint32 nPrevState = dc.GetState();
	dc.DepthTestOff();
	float r = dc.view->GetScreenScaleFactor(pos) * 0.006f;
	dc.DrawWireBox(pos - Vec3(r, r, r), pos + Vec3(r, r, r));
	dc.SetState(nPrevState);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetDrawTextureIconProperties(DisplayContext& dc, const Vec3& pos, float alpha)
{
	if (gViewportPreferences.showIcons || gViewportPreferences.showSizeBasedIcons)
	{
		if (IsHighlighted())
			dc.SetColor(RGB(255, 120, 0), 0.8f * alpha);
		else if (IsSelected())
			dc.SetSelectedColor(alpha);
		else if (IsFrozen())
			dc.SetFreezeColor();
		else
			dc.SetColor(RGB(255, 255, 255), alpha);

		m_vDrawIconPos = pos;

		int nIconFlags = 0;
		if (CheckFlags(OBJFLAG_SHOW_ICONONTOP))
		{
			Vec3 objectPos = GetWorldPos();

			AABB box;
			GetBoundBox(box);
			m_vDrawIconPos.z = (m_vDrawIconPos.z - objectPos.z) + box.max.z;
			nIconFlags = DisplayContext::TEXICON_ALIGN_BOTTOM;
		}

		m_nIconFlags = nIconFlags;
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawTextureIcon(DisplayContext& dc, const Vec3& pos, float alpha)
{
	if (m_nTextureIcon && (gViewportPreferences.showIcons || gViewportPreferences.showSizeBasedIcons))
	{
		dc.DrawTextureLabel(GetTextureIconDrawPos(), OBJECT_TEXTURE_ICON_SIZEX, OBJECT_TEXTURE_ICON_SIZEY,
		                    GetTextureIcon(), GetTextureIconFlags(), 0, 0,
		                    gViewportPreferences.distanceScaleIcons, OBJECT_TEXTURE_ICON_SCALE);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawWarningIcons(DisplayContext& dc, const Vec3& pos)
{
	// Don't draw warning icons if they are beyond draw distance
	if ((dc.camera->GetPosition() - pos).GetLength() > gViewportDebugPreferences.warningIconsDrawDistance)
		return;

	if (gViewportPreferences.showIcons || gViewportPreferences.showSizeBasedIcons)
	{
		const int warningIconSizeX = OBJECT_TEXTURE_ICON_SIZEX / 2;
		const int warningIconSizeY = OBJECT_TEXTURE_ICON_SIZEY / 2;

		const int iconOffsetX = m_nTextureIcon ? (-OBJECT_TEXTURE_ICON_SIZEX / 2) : 0;
		const int iconOffsetY = m_nTextureIcon ? (-OBJECT_TEXTURE_ICON_SIZEY / 2) : 0;

		if (gViewportDebugPreferences.showScaleWarnings)
		{
			const EScaleWarningLevel scaleWarningLevel = GetScaleWarningLevel();

			if (scaleWarningLevel != eScaleWarningLevel_None)
			{
				dc.SetColor(RGB(255, scaleWarningLevel == eScaleWarningLevel_RescaledNonUniform ? 50 : 255, 50), 1.0f);
				dc.DrawTextureLabel(GetTextureIconDrawPos(), warningIconSizeX, warningIconSizeY,
				                    GetIEditor()->GetIconManager()->GetIconTexture(eIcon_ScaleWarning), GetTextureIconFlags(),
				                    -warningIconSizeX / 2, iconOffsetX - (warningIconSizeY / 2));
			}
		}

		if (gViewportDebugPreferences.showRotationWarnings)
		{
			const ERotationWarningLevel rotationWarningLevel = GetRotationWarningLevel();
			if (rotationWarningLevel != eRotationWarningLevel_None)
			{
				dc.SetColor(RGB(255, rotationWarningLevel == eRotationWarningLevel_RotatedNonRectangular ? 50 : 255, 50), 1.0f);
				dc.DrawTextureLabel(GetTextureIconDrawPos(), warningIconSizeX, warningIconSizeY,
				                    GetIEditor()->GetIconManager()->GetIconTexture(eIcon_RotationWarning), GetTextureIconFlags(),
				                    warningIconSizeX / 2, iconOffsetY - (warningIconSizeY / 2));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawLabel(DisplayContext& dc, const Vec3& pos, COLORREF labelColor, float alpha, float size)
{
	// Check if our group is not closed.
	if (!GetGroup() || GetIEditor()->IsGroupOpen(GetGroup()))
	{
		AABB box;
		GetBoundBox(box);

		//p.z = box.max.z + 0.2f;
		if ((dc.flags & DISPLAY_2D) && labelColor == RGB(255, 255, 255))
		{
			labelColor = RGB(0, 0, 0);
		}
	}

	float camDist = dc.camera->GetPosition().GetDistance(pos);
	float maxDist = gViewportPreferences.labelsDistance;
	if (camDist < gViewportPreferences.labelsDistance || (dc.flags & DISPLAY_SELECTION_HELPERS))
	{
		float range = maxDist / 2.0f;
		Vec3 c = Rgb2Vec(labelColor);
		if (IsSelected())
			c = Rgb2Vec(dc.GetSelectedColor());

		float col[4] = { c.x, c.y, c.z, 1 };
		if (dc.flags & DISPLAY_SELECTION_HELPERS)
		{
			if (IsHighlighted())
				c = Rgb2Vec(dc.GetSelectedColor());
			col[0] = c.x;
			col[1] = c.y;
			col[2] = c.z;
		}
		else if (camDist > range)
		{
			col[3] = col[3] * (1.0f - (camDist - range) / range);
		}

		dc.SetColor(col[0], col[1], col[2], col[3] * alpha);
		dc.DrawTextLabel(pos, size, GetName());
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawHighlight(DisplayContext& dc)
{
	if (!m_nTextureIcon && m_bSupportsBoxHighlight)
	{
		AABB box;
		GetLocalBounds(box);

		dc.PushMatrix(GetWorldTM());
		dc.DrawWireBox(box.min, box.max);
		dc.SetLineWidth(1);
		dc.PopMatrix();
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawBudgetUsage(DisplayContext& dc, COLORREF color)
{
	AABB box;
	GetLocalBounds(box);

	dc.SetColor(color);

	dc.PushMatrix(GetWorldTM());
	dc.DrawWireBox(box.min, box.max);
	dc.PopMatrix();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DrawAxis(DisplayContext& dc, const Vec3& pos, float size)
{
	/*
	   dc.renderer->EnableDepthTest(false);
	   Vec3 x(size,0,0);
	   Vec3 y(0,size,0);
	   Vec3 z(0,0,size);

	   bool bWorldSpace = false;
	   if (dc.flags & DISPLAY_WORLDSPACEAXIS)
	   bWorldSpace = true;

	   Matrix tm = GetWorldTM();
	   Vec3 org = tm.TransformPoint( pos );

	   if (!bWorldSpace)
	   {
	   tm.NoScale();
	   x = tm.TransformVector(x);
	   y = tm.TransformVector(y);
	   z = tm.TransformVector(z);
	   }

	   float fScreenScale = dc.view->GetScreenScaleFactor(org);
	   x = x * fScreenScale;
	   y = y * fScreenScale;
	   z = z * fScreenScale;

	   float col[4] = { 1,1,1,1 };
	   float hcol[4] = { 1,0,0,1 };
	   dc.renderer->DrawLabelEx( org+x,1.2f,col,true,true,"X" );
	   dc.renderer->DrawLabelEx( org+y,1.2f,col,true,true,"Y" );
	   dc.renderer->DrawLabelEx( org+z,1.2f,col,true,true,"Z" );

	   Vec3 colX(1,0,0),colY(0,1,0),colZ(0,0,1);
	   if (s_highlightAxis)
	   {
	   float col[4] = { 1,0,0,1 };
	   if (s_highlightAxis == 1)
	   {
	    colX.Set(1,1,0);
	    dc.renderer->DrawLabelEx( org+x,1.2f,col,true,true,"X" );
	   }
	   if (s_highlightAxis == 2)
	   {
	    colY.Set(1,1,0);
	    dc.renderer->DrawLabelEx( org+y,1.2f,col,true,true,"Y" );
	   }
	   if (s_highlightAxis == 3)
	   {
	    colZ.Set(1,1,0);
	    dc.renderer->DrawLabelEx( org+z,1.2f,col,true,true,"Z" );
	   }
	   }

	   x = x * 0.8f;
	   y = y * 0.8f;
	   z = z * 0.8f;
	   float fArrowScale = fScreenScale * 0.07f;
	   dc.SetColor( colX );
	   dc.DrawArrow( org,org+x,fArrowScale );
	   dc.SetColor( colY );
	   dc.DrawArrow( org,org+y,fArrowScale );
	   dc.SetColor( colZ );
	   dc.DrawArrow( org,org+z,fArrowScale );

	   //dc.DrawLine( org,org+x,colX,colX );
	   //dc.DrawLine( org,org+y,colY,colY );
	   //dc.DrawLine( org,org+z,colZ,colZ );

	   dc.renderer->EnableDepthTest(true);
	   ///dc.SetColor( 0,1,1,1 );
	   //dc.DrawLine( p,p+dc.view->m_constructionPlane.m_normal*10.0f );
	 */
}

void CBaseObject::DrawArea(DisplayContext& dc)
{
	float area = m_flattenArea;
	if (area > 0)
	{
		dc.SetColor(RGB(5, 5, 255), 1.f); // make it different color from the AI sight radius
		Vec3 wp = GetWorldPos();
		float z = GetIEditor()->GetTerrainElevation(wp.x, wp.y);
		if (fabs(wp.z - z) < 5)
			dc.DrawTerrainCircle(wp, area, 0.2f);
		else
			dc.DrawCircle(wp, area);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::CanBeDrawn(const DisplayContext& dc, bool& outDisplaySelectionHelper) const
{
	bool bResult = true;
	outDisplaySelectionHelper = false;

	if (dc.flags & DISPLAY_SELECTION_HELPERS)
	{
		// Check if this object type is masked for selection.
		if ((GetType() & gViewportSelectionPreferences.objectSelectMask) && !IsFrozen())
		{
			if (IsSkipSelectionHelper())
				return bResult;
			if (CanBeHightlighted())
				outDisplaySelectionHelper = true;
		}
		else
		{
			// Object helpers should not be displayed when object is not for selection.
			bResult = false;
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsInCameraView(const CCamera& camera)
{
	AABB bbox;
	GetDisplayBoundBox(bbox);
	return (camera.IsAABBVisible_F(AABB(bbox.min, bbox.max)));
}

//////////////////////////////////////////////////////////////////////////
float CBaseObject::GetCameraVisRatio(const CCamera& camera)
{
	AABB bbox;
	GetBoundBox(bbox);

	static const float defaultVisRatio = 1000.0f;

	const float objectHeightSq = max(1.0f, (bbox.max - bbox.min).GetLengthSquared());
	const float camdistSq = (bbox.min - camera.GetPosition()).GetLengthSquared();
	float visRatio = defaultVisRatio;
	if (camdistSq > FLT_EPSILON)
		visRatio = objectHeightSq / camdistSq;

	return visRatio;
}

//////////////////////////////////////////////////////////////////////////
int CBaseObject::MouseCreateCallback(IDisplayViewport* view, EMouseEvent event, CPoint& point, int flags)
{
	if (event == eMouseMove || event == eMouseLDown)
	{
		Vec3 pos = view->MapViewToCP(point, 0, true, GetCreationOffsetFromTerrain());
		SetPos(pos);

		if (event == eMouseLDown)
		{
			return MOUSECREATE_OK;
		}
	}
	else if (event == eMouseWheel)
	{
		double angle = 1;
		if (gSnappingPreferences.angleSnappingEnabled())
			angle = gSnappingPreferences.angleSnap();

		Quat rot = GetRotation();
		rot.SetRotationXYZ(Ang3(0, 0, rot.GetRotZ() + DEG2RAD(point.x > 0 ? angle * (-1) : angle)));
		SetRotation(rot);
	}

	return MOUSECREATE_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnEvent(ObjectEvent event)
{
	switch (event)
	{
	case EVENT_CONFIG_SPEC_CHANGE:
		UpdateVisibility(!IsHidden());
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetShared(bool bShared)
{

}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetHidden(bool bHidden, bool bAnimated)
{
	// test first if the current layer is hidden beforehand AND we're not currently loading a level
	if (!GetLayer()->IsVisible() && !GetIEditor()->GetObjectManager()->IsLoading())
	{
		CUndo undo("Show Object Along With Layer");
		// collect children of the same layer
		CBaseObjectsArray children;
		GetIEditor()->GetObjectManager()->GetObjects(children, BaseObjectFilterFunctor(&CBaseObject::FilterByLayer, GetLayer()));
		for (auto child : children)
		{
			if (child == this)
				// unhide myself
				child->ClearFlags(OBJFLAG_HIDDEN);
			else
				// hide all other children
				child->SetFlags(OBJFLAG_HIDDEN);
		}
		// then unhide the layer
		IObjectLayer* layer = GetLayer();
		do
		{
			layer->SetVisible(true, false);
			layer = layer->GetParentIObjectLayer();
		} while (layer);
		for (auto child : children)
			child->UpdateVisibility(!child->IsHidden());
	}
	else if (CheckFlags(OBJFLAG_HIDDEN) != bHidden)
	{
		if (!bAnimated)
		{
			StoreUndo("Hide Object");
		}

		if (bHidden)
			SetFlags(OBJFLAG_HIDDEN);
		else
			ClearFlags(OBJFLAG_HIDDEN);
		UpdateVisibility(!IsHidden());
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::FilterByLayer(CBaseObject const& obj, void* pLayer)
{
	if (obj.CheckFlags(OBJFLAG_DELETED))
		return false;

	if (obj.GetLayer() == pLayer)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetFrozen(bool bFrozen)
{
	IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();

	// test first if the current layer is actually frozen beforehand AND we're not currently loading a level
	if (GetLayer()->IsFrozen() && !pObjectManager->IsLoading())
	{
		// the only case this situation is currently valid is when a child
		// object is frozen because of the parent layer - and the object's
		// actual "frozen" state might be masked off by the layer

		CUndo undo("Unfreeze Object Along With Layer");
		// collect children of the same layer
		CBaseObjectsArray children;
		pObjectManager->GetObjects(children, BaseObjectFilterFunctor(&CBaseObject::FilterByLayer, GetLayer()));
		for (auto child : children)
		{
			if (child == this)
				// unfreeze myself
				child->ClearFlags(OBJFLAG_FROZEN);
			else
				// freeze all other children
				child->SetFlags(OBJFLAG_FROZEN);
		}
		// then unfreeze the layer alone
		GetLayer()->SetFrozen(false, false);
	}
	else if (CheckFlags(OBJFLAG_FROZEN) != bFrozen)
	{
		// First unselect, so the selection state is saved before freezing. If we don't do that
		// selection won't be restored properly if we are restoring from undo,
		// since undos get restored in reverse order and object will still think it's frozen
		if (bFrozen && IsSelected())
		{
			pObjectManager->UnselectObject(this);
		}

		StoreUndo("Freeze Object");
		if (bFrozen)
			SetFlags(OBJFLAG_FROZEN);
		else
			ClearFlags(OBJFLAG_FROZEN);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetHighlight(bool bHighlight)
{
	if (bHighlight)
		SetFlags(OBJFLAG_HIGHLIGHT);
	else
		ClearFlags(OBJFLAG_HIGHLIGHT);

	UpdateHighlightPassState(IsSelected(), bHighlight);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetSelected(bool bSelect)
{
	if (bSelect)
	{
		SetFlags(OBJFLAG_SELECTED);
		NotifyListeners(OBJECT_ON_SELECT);

		//CLogFile::FormatLine( "Selected: %s, ID=%u",(const char*)m_name,m_id );
	}
	else
	{
		ClearFlags(OBJFLAG_SELECTED);
		NotifyListeners(OBJECT_ON_UNSELECT);
	}
	UpdateHighlightPassState(bSelect, IsHighlighted());
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsHiddenBySpec() const
{
	if (!gViewportPreferences.applyConfigSpec)
		return false;

	// Check against min spec.
	auto configSpec = GetIEditor()->GetEditorConfigSpec();
	if (m_nMinSpec == CONFIG_DETAIL_SPEC && configSpec == CONFIG_LOW_SPEC)
		return true;

	return (m_nMinSpec != 0 && configSpec != 0 && m_nMinSpec > configSpec);
}

//////////////////////////////////////////////////////////////////////////
//! Returns true if object hidden.
bool CBaseObject::IsHidden() const
{
	return (CheckFlags(OBJFLAG_HIDDEN)) ||
	       (!m_layer->IsVisible()) ||
	       (gViewportDebugPreferences.GetObjectHideMask() & GetType());
}

//////////////////////////////////////////////////////////////////////////
//! Returns true if object frozen.
bool CBaseObject::IsFrozen() const
{
	return CheckFlags(OBJFLAG_FROZEN) || m_layer->IsFrozen();
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsSelectable() const
{
	// Not selectable if hidden.
	if (IsHidden())
		return false;

	// Not selectable if frozen.
	if (IsFrozen())
		return false;

	// Not selectable if in closed group.
	CBaseObject* group = GetGroup();
	if (group)
	{
		if (!GetIEditor()->IsGroupOpen(group))
			return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Serialize(CObjectArchive& ar)
{
	XmlNodeRef xmlNode = ar.node;

	ITransformDelegate* pTransformDelegate = m_pTransformDelegate;
	m_pTransformDelegate = nullptr;

	if (ar.bLoading)
	{
		// Loading.
		if (ar.ShouldResetInternalMembers())
		{
			m_flags = 0;
			m_flattenArea = 0.0f;
			m_nMinSpec = 0;
			m_scale.Set(1.0f, 1.0f, 1.0f);
		}

		int flags = 0;
		int oldFlags = m_flags;

		IObjectLayer* pLayer = 0;
		// Find layer name.
		CryGUID layerGUID;
		string layerName;

		if (ar.IsLoadingToCurrentLayer())
		{
			pLayer = GetObjectManager()->GetIObjectLayerManager()->GetCurrentLayer();
		}
		else
		{
			if (xmlNode->getAttr("LayerGUID", layerGUID))
			{
				pLayer = GetObjectManager()->GetIObjectLayerManager()->FindLayer(layerGUID);
			}
			else if (xmlNode->getAttr("Layer", layerName))
			{
				pLayer = GetObjectManager()->GetIObjectLayerManager()->FindLayerByName(layerName);
			}
		}

		string name = m_name;
		string mtlName;

		// In Serialize do not use accessors bellow (GetPos(), .. ), as this will modify the level when saving, due to the potential delegated mode being activated
		Vec3 pos = m_pos;
		Vec3 scale = m_scale;
		Quat quat = m_rotate;
		Ang3 angles(0, 0, 0);
		uint32 nMinSpec = m_nMinSpec;

		COLORREF color = m_color;
		float flattenArea = m_flattenArea;

		CryGUID parentId = CryGUID::Null();
		CryGUID idInPrefab = CryGUID::Null();
		CryGUID lookatId = CryGUID::Null();

		xmlNode->getAttr("Name", name);
		xmlNode->getAttr("Pos", pos);
		if (!xmlNode->getAttr("Rotate", quat))
		{
			// Backward compatability.
			if (xmlNode->getAttr("Angles", angles))
			{
				angles = DEG2RAD(angles);
				//angles.z += gf_PI/2;
				quat.SetRotationXYZ(angles);
			}
		}

		xmlNode->getAttr("Scale", scale);
		xmlNode->getAttr("ColorRGB", color);
		xmlNode->getAttr("FlattenArea", flattenArea);
		xmlNode->getAttr("Flags", flags);
		xmlNode->getAttr("Parent", parentId);
		xmlNode->getAttr("IdInPrefab", idInPrefab);
		xmlNode->getAttr("LookAt", lookatId);
		xmlNode->getAttr("Material", mtlName);
		xmlNode->getAttr("MinSpec", nMinSpec);
		xmlNode->getAttr("FloorNumber", m_floorNumber);

		if (nMinSpec <= CONFIG_VERYHIGH_SPEC) // Ignore invalid values.
			m_nMinSpec = nMinSpec;

		if (m_bUseMaterialLayersMask)
		{
			uint32 nMask = 0;
			xmlNode->getAttr("MatLayersMask", nMask);
			m_nMaterialLayersMask = nMask;
		}

		bool bHidden = flags & OBJFLAG_HIDDEN;
		bool bFrozen = flags & OBJFLAG_FROZEN;

		m_flags = flags;
		m_flags &= ~OBJFLAG_PERSISTMASK;
		m_flags |= (oldFlags) & (~OBJFLAG_PERSISTMASK);
		//SetFlags( flags & OBJFLAG_PERSISTMASK );
		m_flags &= ~OBJFLAG_SHARED;  // clear shared flag
		m_flags &= ~OBJFLAG_DELETED; // clear deleted flag

		if (ar.bUndo)
		{
			DetachThis(false);
		}

		if (name != m_name)
		{
			// This may change object id.
			SetName(name);
		}

		//////////////////////////////////////////////////////////////////////////
		// Check if position is bad.
		if (fabs(pos.x) > INVALID_POSITION_EPSILON ||
		    fabs(pos.y) > INVALID_POSITION_EPSILON ||
		    fabs(pos.z) > INVALID_POSITION_EPSILON)
		{
			// File Not found.
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Object %s have invalid position (%f,%f,%f) %s", (const char*)GetName(), pos.x, pos.y, pos.z,
			           CryLinkService::CCryLinkUriFactory::GetUriV("Editor", "general.select_and_go_to_object %s", GetName()));
		}
		//////////////////////////////////////////////////////////////////////////

		if (!ar.bUndo)
			SetLocalTM(pos, quat, scale);
		else
			SetLocalTM(pos, quat, scale, eObjectUpdateFlags_Undo);

		if (idInPrefab != CryGUID::Null())
		{
			SetIdInPrefab(idInPrefab);
		}

		SetColor(color);
		SetArea(flattenArea);
		SetFrozen(bFrozen);
		SetHidden(bHidden);

		if (pLayer)
			SetLayer(pLayer);

		//////////////////////////////////////////////////////////////////////////
		// Load material.
		//////////////////////////////////////////////////////////////////////////
		SetMaterial(mtlName);

		ar.SetResolveCallback(this, parentId, functor(*this, &CBaseObject::ResolveParent));
		ar.SetResolveCallback(this, lookatId, functor(*this, &CBaseObject::SetLookAt));

		InvalidateTM(0);
		SetModified(false);

		//////////////////////////////////////////////////////////////////////////

		if (ar.bUndo)
		{
			// If we are selected update UI Panel.
			UpdateUIVars();
		}

		// We reseted the min spec and deserialized it so set it internally
		if (ar.ShouldResetInternalMembers())
		{
			SetMinSpec(m_nMinSpec);
		}
	}
	else
	{
		// Saving.
		const bool isPartOfPrefab = IsPartOfPrefab();

		// This attributed only readed by ObjectManager.
		xmlNode->setAttr("Type", GetTypeName());

		if (m_layer)
		{
			xmlNode->setAttr("Layer", m_layer->GetName());
			xmlNode->setAttr("LayerGUID", m_layer->GetGUID());
		}

		xmlNode->setAttr("Id", m_guid);

		if (isPartOfPrefab && !ar.IsSavingInPrefab())
		{
			xmlNode->setAttr("IdInPrefab", m_guidInPrefab);
		}

		xmlNode->setAttr("Name", GetName());

		if (m_parent)
			xmlNode->setAttr("Parent", m_parent->GetId());

		if (m_lookat)
			xmlNode->setAttr("LookAt", m_lookat->GetId());

		if (isPartOfPrefab || !IsEquivalent(GetPos(), Vec3(0, 0, 0), 0))
			xmlNode->setAttr("Pos", GetPos());

		xmlNode->setAttr("FloorNumber", m_floorNumber);

		xmlNode->setAttr("Rotate", m_rotate);

		auto scale = GetScale();

		if (!IsEquivalent(scale, Vec3(1, 1, 1), 0))
			xmlNode->setAttr("Scale", scale);

		xmlNode->setAttr("ColorRGB", GetColor());

		if (GetArea() != 0)
			xmlNode->setAttr("FlattenArea", GetArea());

		int flags = m_flags & OBJFLAG_PERSISTMASK;
		if (flags != 0)
			xmlNode->setAttr("Flags", flags);

		if (m_pMaterial)
			xmlNode->setAttr("Material", GetMaterialName());

		if (m_nMinSpec != 0)
			xmlNode->setAttr("MinSpec", (uint32)m_nMinSpec);

		if (m_bUseMaterialLayersMask)
		{
			uint32 nMask = m_nMaterialLayersMask;
			xmlNode->setAttr("MatLayersMask", nMask);
		}
	}

	// Serialize variables after default entity parameters.
	CVarObject::Serialize(xmlNode, ar.bLoading);

	m_pTransformDelegate = pTransformDelegate;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CBaseObject::Export(const string& levelPath, XmlNodeRef& xmlNode)
{
	// This function exports object to the Engine

	XmlNodeRef objNode = xmlNode->newChild("Object");

	objNode->setAttr("Type", GetTypeName());
	objNode->setAttr("Name", GetName());

	if (m_pMaterial)
		objNode->setAttr("Material", m_pMaterial->GetName());

	Vec3 pos;
	Vec3 scale;
	Quat rotate;

	if (m_parent)
	{
		// Export world coordinates.
		AffineParts ap;
		ap.SpectralDecompose(GetWorldTM());

		pos = ap.pos;
		rotate = ap.rot;
		scale = ap.scale;
	}
	else
	{
		// Not using acccessors(GetPos()...) here as we want to save the base object position and ignore delegates
		pos = m_pos;
		rotate = m_rotate;
		scale = m_scale;
	}

	if (!IsEquivalent(pos, Vec3(0, 0, 0), 0))
		objNode->setAttr("Pos", pos);

	if (!rotate.IsIdentity())
		objNode->setAttr("Rotate", rotate);

	if (!IsEquivalent(scale, Vec3(0, 0, 0), 0))
		objNode->setAttr("Scale", scale);

	if (m_nMinSpec != 0)
		objNode->setAttr("MinSpec", (uint32)m_nMinSpec);

	// Save variables.
	CVarObject::Serialize(objNode, false);

	return objNode;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CBaseObject::FindObject(CryGUID id) const
{
	return GetObjectManager()->FindObject(id);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StoreUndo(const char* UndoDescription, bool minimal, int flags)
{
	if (CUndo::IsRecording())
	{
		if (minimal)
			CUndo::Record(new CUndoBaseObjectMinimal(this, UndoDescription, flags));
		else
			CUndo::Record(new CUndoBaseObject(this, UndoDescription));
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsCreateGameObjects() const
{
	return GetObjectManager()->IsCreateGameObjects();
}

//////////////////////////////////////////////////////////////////////////
string CBaseObject::GetTypeName() const
{
	const char* className = m_classDesc->ClassName();
	const char* subClassName = strstr(className, "::");
	if (!subClassName)
		return className;

	string name;
	name.Append(className, subClassName - className);
	return name;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLayer(string layerFullName)
{
	auto pLayer = GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->FindLayerByFullName(layerFullName);
	if (pLayer)
	{
		SetLayer(pLayer);
	}
}

void CBaseObject::SetLayer(IObjectLayer* layer)
{
	if (layer == m_layer && !m_childs.size())
		return;

	assert(layer != 0);

	StoreUndo("Set Layer");

	// guard against same layer (we might have children!)
	if (layer != m_layer)
	{
		CObjectLayerChangeEvent changeEvt(this, m_layer);
		m_layer = layer;
		IObjectManager* iObjMng = GetIEditor()->GetObjectManager();
		iObjMng->signalObjectChanged(changeEvt);

		UpdateVisibility(layer->IsVisible());
	}

	// Set layer for all childs.
	for (int i = 0; i < m_childs.size(); i++)
	{
		m_childs[i]->SetLayer(layer);
	}

	// If object have target, target must also go into this layer.
	if (m_lookat)
	{
		m_lookat->SetLayer(layer);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IntersectRectBounds(const AABB& bbox)
{
	AABB aabb;
	GetBoundBox(aabb);

	return aabb.IsIntersectBox(bbox);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IntersectRayBounds(const Ray& ray)
{
	Vec3 tmpPnt;
	AABB aabb;
	GetBoundBox(aabb);

	return Intersect::Ray_AABB(ray, aabb, tmpPnt);
}

//////////////////////////////////////////////////////////////////////////
namespace
{
typedef std::pair<Vec2, Vec2> Edge2D;
}
bool IsIncludePointsInConvexHull(Edge2D* pEdgeArray0, int nEdgeArray0Size, Edge2D* pEdgeArray1, int nEdgeArray1Size)
{
	if (!pEdgeArray0 || !pEdgeArray1 || nEdgeArray0Size <= 0 || nEdgeArray1Size <= 0)
		return false;

	static const float kPointEdgeMaxInsideDistance(0.05f);

	bool bInside(true);
	for (int i = 0; i < nEdgeArray0Size; ++i)
	{
		const Vec2& v(pEdgeArray0[i].first);
		bInside = true;
		for (int k = 0; k < nEdgeArray1Size; ++k)
		{
			Vec2 v0 = pEdgeArray1[k].first;
			Vec2 v1 = pEdgeArray1[k].second;
			Vec3 direction(v1.x - v0.x, v1.y - v0.y, 0);
			Vec3 up(0, 0, 1);
			Vec3 z = up.Cross(direction);
			Vec2 normal;
			normal.x = z.x;
			normal.y = z.y;
			normal.Normalize();
			float distance = -normal.Dot(v0);
			if (normal.Dot(v) + distance > kPointEdgeMaxInsideDistance)
			{
				bInside = false;
				break;
			}
		}
		if (bInside)
			break;
	}
	return bInside;
}

void ModifyConvexEdgeDirection(Edge2D* pEdgeArray, int nEdgeArraySize)
{
	if (!pEdgeArray || nEdgeArraySize < 2)
		return;
	Vec3 v0(pEdgeArray[0].first.x - pEdgeArray[0].second.x, pEdgeArray[0].first.y - pEdgeArray[0].second.y, 0);
	Vec3 v1(pEdgeArray[1].second.x - pEdgeArray[1].first.x, pEdgeArray[1].second.y - pEdgeArray[1].first.y, 0);
	Vec3 vCross = v0.Cross(v1);
	if (vCross.z < 0)
	{
		for (int i = 0; i < nEdgeArraySize; ++i)
			std::swap(pEdgeArray[i].first, pEdgeArray[i].second);
	}
}

bool CBaseObject::HitTestRectBounds(HitContext& hc, const AABB& box)
{
	if (hc.bUseSelectionHelpers)
	{
		if (IsSkipSelectionHelper())
			return false;
	}

	static const int kNumberOfBoundBoxPt(8);

	// transform all 8 vertices into view space.
	CPoint p[kNumberOfBoundBoxPt] =
	{
		hc.view->WorldToView(Vec3(box.min.x, box.min.y, box.min.z)),
		hc.view->WorldToView(Vec3(box.min.x, box.max.y, box.min.z)),
		hc.view->WorldToView(Vec3(box.max.x, box.min.y, box.min.z)),
		hc.view->WorldToView(Vec3(box.max.x, box.max.y, box.min.z)),
		hc.view->WorldToView(Vec3(box.min.x, box.min.y, box.max.z)),
		hc.view->WorldToView(Vec3(box.min.x, box.max.y, box.max.z)),
		hc.view->WorldToView(Vec3(box.max.x, box.min.y, box.max.z)),
		hc.view->WorldToView(Vec3(box.max.x, box.max.y, box.max.z))
	};

	CRect objrc, temprc;
	objrc.left = 10000;
	objrc.top = 10000;
	objrc.right = -10000;
	objrc.bottom = -10000;

	// find new min/max values
	for (int i = 0; i < 8; i++)
	{
		objrc.left = min(objrc.left, p[i].x);
		objrc.right = max(objrc.right, p[i].x);
		objrc.top = min(objrc.top, p[i].y);
		objrc.bottom = max(objrc.bottom, p[i].y);
	}
	if (objrc.IsRectEmpty())
	{
		// Make objrc at least of size 1.
		objrc.bottom += 1;
		objrc.right += 1;
	}

	if (hc.rect.PtInRect(CPoint(objrc.left, objrc.top)) &&
	    hc.rect.PtInRect(CPoint(objrc.left, objrc.bottom)) &&
	    hc.rect.PtInRect(CPoint(objrc.right, objrc.top)) &&
	    hc.rect.PtInRect(CPoint(objrc.right, objrc.bottom)))
	{
		hc.object = this;
		return true;
	}

	if (temprc.IntersectRect(objrc, hc.rect))
	{
		AABB localAABB;
		GetLocalBounds(localAABB);
		CBaseObject* pOldObj = hc.object;
		hc.object = this;
		if (localAABB.IsEmpty())
			return true;

		const int kMaxSizeOfEdgeList0(4);
		Edge2D edgelist0[kMaxSizeOfEdgeList0] = {
			Edge2D(Vec2(hc.rect.left,  hc.rect.top),    Vec2(hc.rect.right, hc.rect.top)),
			Edge2D(Vec2(hc.rect.right, hc.rect.top),    Vec2(hc.rect.right, hc.rect.bottom)),
			Edge2D(Vec2(hc.rect.right, hc.rect.bottom), Vec2(hc.rect.left,  hc.rect.bottom)),
			Edge2D(Vec2(hc.rect.left,  hc.rect.bottom), Vec2(hc.rect.left,  hc.rect.top))
		};

		const int kMaxSizeOfEdgeList1(8);
		Edge2D edgelist1[kMaxSizeOfEdgeList1];
		int nEdgeList1Count(kMaxSizeOfEdgeList1);

		const Matrix34& worldTM(GetWorldTM());
		OBB obb = OBB::CreateOBBfromAABB(Matrix33(worldTM), localAABB);
		Vec3 ax = obb.m33.GetColumn0() * obb.h.x;
		Vec3 ay = obb.m33.GetColumn1() * obb.h.y;
		Vec3 az = obb.m33.GetColumn2() * obb.h.z;
		CPoint obb_p[kMaxSizeOfEdgeList1] =
		{
			hc.view->WorldToView(-ax - ay - az + worldTM.GetTranslation()),
			hc.view->WorldToView(-ax - ay + az + worldTM.GetTranslation()),
			hc.view->WorldToView(-ax + ay - az + worldTM.GetTranslation()),
			hc.view->WorldToView(-ax + ay + az + worldTM.GetTranslation()),
			hc.view->WorldToView(ax - ay - az + worldTM.GetTranslation()),
			hc.view->WorldToView(ax - ay + az + worldTM.GetTranslation()),
			hc.view->WorldToView(ax + ay - az + worldTM.GetTranslation()),
			hc.view->WorldToView(ax + ay + az + worldTM.GetTranslation())
		};

		std::vector<Vec3> pointsForRegion1;
		pointsForRegion1.reserve(kMaxSizeOfEdgeList1);
		for (int i = 0; i < kMaxSizeOfEdgeList1; ++i)
			pointsForRegion1.push_back(Vec3(obb_p[i].x, obb_p[i].y, 0));

		std::vector<Vec3> convexHullForRegion1;
		ConvexHull2D(convexHullForRegion1, pointsForRegion1);
		nEdgeList1Count = convexHullForRegion1.size();
		if (nEdgeList1Count < 3 || nEdgeList1Count > kMaxSizeOfEdgeList1)
			return true;

		for (int i = 0; i < nEdgeList1Count; ++i)
		{
			int iNextI = (i + 1) % nEdgeList1Count;
			edgelist1[i] = Edge2D(Vec2(convexHullForRegion1[i].x, convexHullForRegion1[i].y), Vec2(convexHullForRegion1[iNextI].x, convexHullForRegion1[iNextI].y));
		}

		ModifyConvexEdgeDirection(edgelist0, kMaxSizeOfEdgeList0);
		ModifyConvexEdgeDirection(edgelist1, nEdgeList1Count);

		bool bInside = IsIncludePointsInConvexHull(edgelist0, kMaxSizeOfEdgeList0, edgelist1, nEdgeList1Count);
		if (!bInside)
			bInside = IsIncludePointsInConvexHull(edgelist1, nEdgeList1Count, edgelist0, kMaxSizeOfEdgeList0);
		if (!bInside)
		{
			hc.object = pOldObj;
			return false;
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::HitTestRect(HitContext& hc)
{
	AABB box;

	if (hc.bUseSelectionHelpers)
	{
		if (IsSkipSelectionHelper())
			return false;
		box.min = GetWorldPos();
		box.max = box.min;
	}
	else
	{
		// Retrieve world space bound box.
		GetBoundBox(box);
	}

	bool bHit = HitTestRectBounds(hc, box);
	m_bInSelectionBox = bHit;
	if (bHit)
		hc.object = this;
	return bHit;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::HitHelperTest(HitContext& hc)
{
	return HitHelperAtTest(hc, GetWorldPos());
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::HitHelperAtTest(HitContext& hc, const Vec3& pos)
{
	bool bResult = false;

	if (m_nTextureIcon && (gViewportPreferences.showIcons || gViewportPreferences.showSizeBasedIcons) && !hc.bUseSelectionHelpers)
	{
		int iconSizeX = OBJECT_TEXTURE_ICON_SIZEX;
		int iconSizeY = OBJECT_TEXTURE_ICON_SIZEY;

		if (gViewportPreferences.distanceScaleIcons)
		{
			float fScreenScale = hc.view->GetScreenScaleFactor(pos);

			iconSizeX *= OBJECT_TEXTURE_ICON_SCALE / fScreenScale;
			iconSizeY *= OBJECT_TEXTURE_ICON_SCALE / fScreenScale;
		}

		// Hit Test icon of this object.
		Vec3 testPos = pos;
		int y0 = -(iconSizeY / 2);
		int y1 = +(iconSizeY / 2);
		if (CheckFlags(OBJFLAG_SHOW_ICONONTOP))
		{
			Vec3 objectPos = GetWorldPos();

			AABB box;
			GetBoundBox(box);
			testPos.z = (pos.z - objectPos.z) + box.max.z;
			y0 = -(iconSizeY);
			y1 = 0;
		}
		CPoint pnt = hc.view->WorldToView(testPos);

		if (hc.point2d.x >= pnt.x - (iconSizeX / 2) && hc.point2d.x <= pnt.x + (iconSizeX / 2) &&
		    hc.point2d.y >= pnt.y + y0 && hc.point2d.y <= pnt.y + y1)
		{
			hc.dist = hc.raySrc.GetDistance(testPos) - 0.2f;
			bResult = true;
		}
	}
	else if (hc.bUseSelectionHelpers)
	{
		// Check potentially children first
		bResult = HitHelperTestForChildObjects(hc);

		// If no hit check this object
		if (!bResult)
		{
			// Hit test helper.
			Vec3 w = pos - hc.raySrc;
			w = hc.rayDir.Cross(w);
			float d = w.GetLengthSquared();

			static const float screenScaleToRadiusFactor = 0.008f;
			const float radius = hc.view->GetScreenScaleFactor(pos) * screenScaleToRadiusFactor;
			const float pickDistance = hc.raySrc.GetDistance(pos);
			if (d < radius * radius + hc.distanceTolerance && hc.dist >= pickDistance)
			{
				hc.dist = pickDistance;
				hc.object = this;
				bResult = true;
			}
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CBaseObject::GetChild(size_t const i) const
{
	assert(i >= 0 && i < m_childs.size());
	return m_childs[i];
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsChildOf(CBaseObject* node)
{
	CBaseObject* p = m_parent;
	while (p && p != node)
	{
		p = p->m_parent;
	}
	if (p == node)
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::GetAllChildren(TBaseObjects& outAllChildren, CBaseObject* pObj) const
{
	const CBaseObject* pBaseObj = pObj ? pObj : this;

	for (int i = 0, iChildCount(pBaseObj->GetChildCount()); i < iChildCount; ++i)
	{
		CBaseObject* pChild = pBaseObj->GetChild(i);
		if (pChild == NULL)
			continue;
		outAllChildren.push_back(pChild);
		GetAllChildren(outAllChildren, pChild);
	}
}

void CBaseObject::GetAllChildren(DynArray<_smart_ptr<CBaseObject>>& outAllChildren, CBaseObject* pObj) const
{
	const CBaseObject* pBaseObj = pObj ? pObj : this;

	for (int i = 0, iChildCount(pBaseObj->GetChildCount()); i < iChildCount; ++i)
	{
		CBaseObject* pChild = pBaseObj->GetChild(i);
		if (pChild == NULL)
			continue;
		outAllChildren.push_back(pChild);
		GetAllChildren(outAllChildren, pChild);
	}
}

void CBaseObject::GetAllChildren(ISelectionGroup& outAllChildren, CBaseObject* pObj) const
{
	const CBaseObject* pBaseObj = pObj ? pObj : this;

	for (int i = 0, iChildCount(pBaseObj->GetChildCount()); i < iChildCount; ++i)
	{
		CBaseObject* pChild = pBaseObj->GetChild(i);
		if (pChild == NULL)
			continue;
		outAllChildren.AddObject(pChild);
		GetAllChildren(outAllChildren, pChild);
	}
}

void CBaseObject::GetAllPrefabFlagedChildren(TBaseObjects& outAllChildren, CBaseObject* pObj) const
{
	const CBaseObject* pBaseObj = pObj ? pObj : this;

	for (int i = 0, iChildCount(pBaseObj->GetChildCount()); i < iChildCount; ++i)
	{
		CBaseObject* pChild = pBaseObj->GetChild(i);
		if (pChild == NULL || !pChild->CheckFlags(OBJFLAG_PREFAB))
			continue;
		outAllChildren.push_back(pChild);

		if (GetIEditor()->IsCPrefabObject(pChild))
			continue;

		GetAllPrefabFlagedChildren(outAllChildren, pChild);
	}
}

void CBaseObject::GetAllPrefabFlagedChildren(ISelectionGroup& outAllChildren, CBaseObject* pObj) const
{
	const CBaseObject* pBaseObj = pObj ? pObj : this;

	for (int i = 0, iChildCount(pBaseObj->GetChildCount()); i < iChildCount; ++i)
	{
		CBaseObject* pChild = pBaseObj->GetChild(i);
		if (pChild == NULL || !pChild->CheckFlags(OBJFLAG_PREFAB))
			continue;
		outAllChildren.AddObject(pChild);

		if (GetIEditor()->IsCPrefabObject(pChild))
			continue;

		GetAllPrefabFlagedChildren(outAllChildren, pChild);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::CloneChildren(CBaseObject* pFromObject)
{
	if (pFromObject == NULL)
		return;

	for (int i = 0, nChildCount(pFromObject->GetChildCount()); i < nChildCount; ++i)
	{
		CBaseObject* pFromChildObject = pFromObject->GetChild(i);

		CBaseObject* pChildClone = GetObjectManager()->CloneObject(pFromChildObject);
		if (pChildClone == NULL)
			continue;

		pChildClone->CloneChildren(pFromChildObject);
		AddMember(pChildClone, false);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AttachChild(CBaseObject* child, bool bKeepPos)
{
	Matrix34 childTM;
	ITransformDelegate* pTransformDelegate;
	ITransformDelegate* pChildTransformDelegate;
	IObjectLayer* pOldLayer = child->GetLayer();

	{
		CScopedSuspendUndo suspendUndo;

		assert(child);
		if (!child || child == GetLookAt())
			return;

		// If not already attached to this node.
		if (child->m_parent == this)
			return;

		if (child->m_parent)
			child->DetachThis();

		if (child->GetLayer() != m_layer)
			child->SetLayer(m_layer);

		CObjectPreattachEvent preattachEvent(child, this);

		GetObjectManager()->signalObjectChanged(preattachEvent);
		child->NotifyListeners(bKeepPos ? OBJECT_ON_PREATTACHEDKEEPXFORM : OBJECT_ON_PREATTACHED);

		// Add to child list first to make sure node not get deleted while reattaching.
		m_childs.push_back(child);
		if (child->m_parent)
			child->DetachThis(bKeepPos);  // Detach node if attached to other parent.

		pTransformDelegate = m_pTransformDelegate;
		pChildTransformDelegate = child->m_pTransformDelegate;
		SetTransformDelegate(nullptr);
		child->SetTransformDelegate(nullptr);

		if (bKeepPos)
		{
			child->InvalidateTM(0);
			childTM = child->GetWorldTM();
		}

		{
			if (bKeepPos)
			{
				// Set the object's local transform relative to it's new parent
				Matrix34 invParentTM = GetWorldTM();
				invParentTM.Invert();
				Matrix34 localTM = invParentTM * child->GetWorldTM();
				child->SetLocalTM(localTM);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		child->m_parent = this; // Assign this node as parent to child node.
	}

	OnAttachChild(child);

	{
		CScopedSuspendUndo suspendUndo;

		SetTransformDelegate(pTransformDelegate);
		child->SetTransformDelegate(pChildTransformDelegate);

		CObjectAttachEvent attachEvent(child, pOldLayer);
		GetObjectManager()->signalObjectChanged(attachEvent);
		child->NotifyListeners(OBJECT_ON_ATTACHED);

		NotifyListeners(OBJECT_ON_CHILDATTACHED);
	}

	if (CUndo::IsRecording())
	{
		CUndo::Record(new CUndoAttachBaseObject(child, pOldLayer, bKeepPos, true));
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DetachAll(bool bKeepPos)
{
	while (!m_childs.empty())
	{
		CBaseObject* child = *m_childs.begin();
		child->DetachThis(bKeepPos);
		NotifyListeners(OBJECT_ON_CHILDDETACHED);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::DetachThis(bool bKeepPos)
{
	if (m_parent)
	{
		if (CUndo::IsRecording())
		{
			CUndo::Record(new CUndoAttachBaseObject(this, m_layer, bKeepPos, false));
		}

		Matrix34 worldTM;
		ITransformDelegate* pTransformDelegate;

		{
			CScopedSuspendUndo suspendUndo;
			GetObjectManager()->NotifyObjectListeners(this, OBJECT_ON_PREDETACHED);
			NotifyListeners(bKeepPos ? OBJECT_ON_PREDETACHEDKEEPXFORM : OBJECT_ON_PREDETACHED);

			pTransformDelegate = m_pTransformDelegate;
			SetTransformDelegate(nullptr);

			if (bKeepPos)
			{
				ITransformDelegate* pParentTransformDelegate = m_parent->m_pTransformDelegate;
				m_parent->SetTransformDelegate(nullptr);
				worldTM = GetWorldTM();
				m_parent->SetTransformDelegate(pParentTransformDelegate);
			}
		}

		OnDetachThis();

		{
			CScopedSuspendUndo suspendUndo;

			// Copy parent to temp var, erasing child from parent may delete this node if child referenced only from parent.
			CBaseObject* parent = m_parent;
			m_parent = 0;
			parent->RemoveChild(this);

			if (bKeepPos)
			{
				// Keep old world space transformation.
				SetWorldTM(worldTM);
			}

			SetTransformDelegate(pTransformDelegate);

			CObjectDetachEvent detachEvent(this, parent);
			GetObjectManager()->signalObjectChanged(detachEvent);
			NotifyListeners(OBJECT_ON_DETACHED);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveChild(CBaseObject* node)
{
	Childs::iterator it = std::find(m_childs.begin(), m_childs.end(), node);
	if (it != m_childs.end())
	{
		m_childs.erase(it);
		NotifyListeners(OBJECT_ON_CHILDDETACHED);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::ResolveParent(CBaseObject* parent)
{
	// even though parent is same as m_parent, adding the member to the parent must be done.
	if (parent)
	{
		bool bDerivedFromGroup = GetIEditor()->IsCGroup(parent);
		bool bSuspended(false);
		if (bDerivedFromGroup)
			bSuspended = GetIEditor()->SuspendUpdateCGroup(parent);
		parent->AddMember(this, false);
		if (bSuspended)
			GetIEditor()->ResumeUpdateCGroup(parent);
	}
	else
	{
		if (GetGroup())
			GetIEditor()->RemoveMemberCGroup(GetGroup(), this);
		else
			DetachThis(false);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::CalcLocalTM(Matrix34& tm) const
{
	tm.SetIdentity();

	if (m_lookat)
	{
		auto pos = GetPos();

		if (m_parent)
		{
			// Get our world position.
			pos = GetParentAttachPointWorldTM().TransformPoint(pos);
		}

		// Calculate local TM matrix differently.
		Vec3 lookatPos = m_lookat->GetWorldPos();
		if (lookatPos == pos) // if target at object pos
		{
			tm.SetTranslation(pos);
		}
		else
		{
			tm = Matrix34(Matrix33::CreateRotationVDir((lookatPos - pos).GetNormalized()), pos);
		}
		if (m_parent)
		{
			Matrix34 invParentTM = m_parent->GetWorldTM();
			invParentTM.Invert();
			tm = invParentTM * tm;
		}
	}
	else
	{
		tm = Matrix34::Create(GetScale(), GetRotation(), GetPos());
	}
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CBaseObject::GetWorldTM() const
{
	if (!m_bMatrixValid)
	{
		m_worldTM = GetLocalTM();
		m_bMatrixValid = true;
		m_bMatrixInWorldSpace = false;
		m_bWorldBoxValid = false;
	}
	if (!m_bMatrixInWorldSpace)
	{
		CBaseObject* parent = GetParent();
		if (parent)
		{
			m_worldTM = GetParentAttachPointWorldTM() * m_worldTM;
		}
		m_bMatrixInWorldSpace = true;
		m_bWorldBoxValid = false;
	}
	return m_worldTM;
}

bool CBaseObject::GetManipulatorMatrix(RefCoordSys coordSys, Matrix34& tm) const
{
	if (coordSys == COORDS_LOCAL)
	{
		tm = GetWorldTM();
		return true;
	}
	else if (coordSys == COORDS_PARENT)
	{
		if (GetParent())
		{
			tm = GetParent()->GetWorldTM();
		}
		else
		{
			tm = GetWorldTM();
		}
		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////
Matrix34 CBaseObject::GetParentAttachPointWorldTM() const
{
	CBaseObject* pParent = GetParent();
	if (pParent)
	{
		return pParent->GetWorldTM();
	}

	return Matrix34(IDENTITY);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsParentAttachmentValid() const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::InvalidateTM(int flags)
{
	bool bMatrixWasValid = m_bMatrixValid;

	m_bMatrixInWorldSpace = false;
	m_bMatrixValid = false;
	m_bWorldBoxValid = false;

	// If matrix was valid, invalidate all childs.
	if (bMatrixWasValid)
	{
		if (m_lookatSource)
			m_lookatSource->InvalidateTM(eObjectUpdateFlags_ParentChanged);

		// Invalidate matrices off all child objects.
		for (int i = 0; i < m_childs.size(); i++)
		{
			if (m_childs[i] != 0 && m_childs[i]->m_bMatrixValid)
			{
				m_childs[i]->InvalidateTM(eObjectUpdateFlags_ParentChanged);
			}
		}
		NotifyListeners(OBJECT_ON_TRANSFORM);

		// Notify parent that we were modified.
		if (m_parent)
		{
			m_parent->OnChildModified();
		}
	}

	if (m_pTransformDelegate)
	{
		m_pTransformDelegate->MatrixInvalidated();
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLocalTM(const Vec3& pos, const Quat& rotate, const Vec3& scale, int flags)
{
	const bool b1 = SetPos(pos, flags | eObjectUpdateFlags_DoNotInvalidate);
	const bool b2 = SetRotation(rotate, flags | eObjectUpdateFlags_DoNotInvalidate);
	const bool b3 = SetScale(scale, flags | eObjectUpdateFlags_DoNotInvalidate);

	if (b1 || b2 || b3 || flags == eObjectUpdateFlags_Animated)
	{
		flags = (b1) ? (flags | eObjectUpdateFlags_PositionChanged) : (flags & (~eObjectUpdateFlags_PositionChanged));
		flags = (b2) ? (flags | eObjectUpdateFlags_RotationChanged) : (flags & (~eObjectUpdateFlags_RotationChanged));
		flags = (b3) ? (flags | eObjectUpdateFlags_ScaleChanged) : (flags & (~eObjectUpdateFlags_ScaleChanged));
		InvalidateTM(flags);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLocalTM(const Matrix34& tm, int flags)
{
	if (m_lookat)
	{
		bool b1 = SetPos(tm.GetTranslation(), eObjectUpdateFlags_DoNotInvalidate);
		flags = (b1) ? (flags | eObjectUpdateFlags_PositionChanged) : (flags & (~eObjectUpdateFlags_PositionChanged));
		InvalidateTM(flags);
	}
	else
	{
		AffineParts affineParts;
		affineParts.SpectralDecompose(tm);
		SetLocalTM(affineParts.pos, affineParts.rot, affineParts.scale, flags);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetWorldPos(const Vec3& pos, int flags)
{
	if (GetParent())
	{
		Matrix34 invParentTM = GetParentAttachPointWorldTM();
		invParentTM.Invert();
		Vec3 posLocal = invParentTM * pos;
		SetPos(posLocal, flags);
	}
	else
	{
		SetPos(pos, flags);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetWorldTM(const Matrix34& tm, int flags)
{
	if (GetParent())
	{
		Matrix34 invParentTM = GetParentAttachPointWorldTM();
		invParentTM.Invert();
		Matrix34 localTM = invParentTM * tm;
		SetLocalTM(localTM, flags);
	}
	else
	{
		SetLocalTM(tm, flags);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVisibility(bool bVisible)
{
	bool bVisibleWithSpec = bVisible && !IsHiddenBySpec();
	if (bVisible == CheckFlags(OBJFLAG_INVISIBLE))
	{
		GetObjectManager()->InvalidateVisibleList();
		if (!bVisible)
			m_flags |= OBJFLAG_INVISIBLE;
		else
			m_flags &= ~OBJFLAG_INVISIBLE;

		NotifyListeners(OBJECT_ON_VISIBILITY);

		IPhysicalEntity* pPhEn = GetCollisionEntity();
		if (pPhEn && gEnv->pPhysicalWorld)
			gEnv->pPhysicalWorld->DestroyPhysicalEntity(pPhEn, bVisibleWithSpec ? 2 : 1);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLookAt(CBaseObject* target)
{
	if (m_lookat == target)
		return;

	StoreUndo("Change LookAt");

	if (m_lookat)
	{
		// Unbind current lookat.
		m_lookat->m_lookatSource = 0;
	}
	m_lookat = target;
	if (m_lookat)
	{
		m_lookat->m_lookatSource = this;
	}

	InvalidateTM(0);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsLookAtTarget() const
{
	return m_lookatSource != 0;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddEventListener(const EventCallback& cb)
{
	if (find(m_eventListeners.begin(), m_eventListeners.end(), cb) == m_eventListeners.end())
		m_eventListeners.push_back(cb);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveEventListener(const EventCallback& cb)
{
	std::vector<EventCallback>::iterator cbFound = find(m_eventListeners.begin(), m_eventListeners.end(), cb);
	if (cbFound != m_eventListeners.end())
	{
		(*cbFound) = EventCallback();
	}
}

//////////////////////////////////////////////////////////////////////////
bool IsBaseObjectEventCallbackNULL(const CBaseObject::EventCallback& cb) { return cb.getFunc() == NULL; }

//////////////////////////////////////////////////////////////////////////
void CBaseObject::NotifyListeners(EObjectListenerEvent event)
{
	for (std::vector<EventCallback>::iterator it = m_eventListeners.begin(), end = m_eventListeners.end(); it != end; ++it)
	{
		// Call listener callback.
		if ((*it).getFunc() != NULL)
			(*it)(this, event);
	}

	m_eventListeners.erase(remove_if(m_eventListeners.begin(), m_eventListeners.end(), IsBaseObjectEventCallbackNULL), std::end(m_eventListeners));
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::ConvertFromObject(CBaseObject* object)
{
	SetLocalTM(object->GetLocalTM());
	SetName(object->GetName());
	SetLayer(object->GetLayer());
	SetColor(object->GetColor());
	m_flattenArea = object->m_flattenArea;
	if (object->GetParent())
	{
		object->GetParent()->AttachChild(this);
	}
	SetMaterial(object->GetMaterial());
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsPotentiallyVisible() const
{
	if (!m_layer->IsVisible())
		return false;
	if (CheckFlags(OBJFLAG_HIDDEN))
		return false;
	if (gViewportDebugPreferences.GetObjectHideMask() & GetType())
		return false;

	CBaseObject* pGroup = GetGroup();
	if (pGroup)
	{
		if (!pGroup->IsPotentiallyVisible())
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
//! Analyze errors for this object.
void CBaseObject::Validate()
{
	// Not using acccessors(GetPos(), ... ) here as we want to validate the object position, scale and ignore delegates

	// Checks for invalid values in base object.

	//////////////////////////////////////////////////////////////////////////
	// Check if position is bad.
	auto pos = m_pos;

	if (fabs(pos.x) > INVALID_POSITION_EPSILON ||
	    fabs(pos.y) > INVALID_POSITION_EPSILON ||
	    fabs(pos.z) > INVALID_POSITION_EPSILON)
	{
		// File Not found.
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Object %s have invalid position (%f,%f,%f) %s", (const char*)GetName(), pos.x, pos.y, pos.z,
		           CryLinkService::CCryLinkUriFactory::GetUriV("Editor", "general.select_and_go_to_object %s", GetName()));
	}
	//////////////////////////////////////////////////////////////////////////

	float minScale = 0.01f;
	float maxScale = 1000.0f;
	//////////////////////////////////////////////////////////////////////////
	// Check if scale is out of range.
	if (m_scale.x < minScale || m_scale.x > maxScale ||
	    m_scale.y < minScale || m_scale.y > maxScale ||
	    m_scale.z < minScale || m_scale.z > maxScale)
	{
		// File Not found.
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Object %s have invalid scale (%f,%f,%f) %s", (const char*)GetName(), m_scale.x, m_scale.y, m_scale.z,
		           CryLinkService::CCryLinkUriFactory::GetUriV("Editor", "general.select_and_go_to_object %s", GetName()));
	} //////////////////////////////////////////////////////////////////////////

	if (GetMaterial() != NULL && GetMaterial()->IsDummy())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Material: %s for object: %s not found %s", GetMaterial()->GetName(), (const char*)GetName(),
		           CryLinkService::CCryLinkUriFactory::GetUriV("Editor", "general.select_and_go_to_object %s", GetName()));
	}

};

//////////////////////////////////////////////////////////////////////////
Ang3 CBaseObject::GetWorldAngles() const
{
	// Not using acccessors(GetScale()...) here as we want to validate the object scale and ignore delegates

	if (m_scale == Vec3(1, 1, 1))
	{
		Quat q = Quat(GetWorldTM());
		Ang3 angles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(q)));
		return angles;
	}
	else
	{
		Matrix34 tm = GetWorldTM();
		tm.OrthonormalizeFast();
		Quat q = Quat(tm);
		Ang3 angles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(q)));
		return angles;
	}
};

//////////////////////////////////////////////////////////////////////////
void CBaseObject::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
	CBaseObject* pFromParent = pFromObject->GetParent();
	if (pFromParent)
	{
		SetFloorNumber(pFromObject->GetFloorNumber());

		CBaseObject* pFromParentInContext = ctx.FindClone(pFromParent);
		if (pFromParentInContext)
			pFromParentInContext->AddMember(this, false);
		else
			pFromParent->AddMember(this, false);
	}
	for (int i = 0; i < pFromObject->GetChildCount(); i++)
	{
		CBaseObject* pChildObject = pFromObject->GetChild(i);
		CBaseObject* pClonedChild = GetObjectManager()->CloneObject(pChildObject);
		ctx.AddClone(pChildObject, pClonedChild);
	}
	for (int i = 0; i < pFromObject->GetChildCount(); i++)
	{
		CBaseObject* pChildObject = pFromObject->GetChild(i);
		CBaseObject* pClonedChild = ctx.FindClone(pChildObject);
		if (pClonedChild)
		{
			pClonedChild->PostClone(pChildObject, ctx);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::GatherUsedResources(CUsedResources& resources)
{
	if (GetVarBlock())
		GetVarBlock()->GatherUsedResources(resources);
	if (m_pMaterial)
	{
		m_pMaterial->GatherUsedResources(resources);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IsSimilarObject(CBaseObject* pObject)
{
	if (pObject->GetClassDesc() == GetClassDesc() && pObject->GetRuntimeClass() == GetRuntimeClass())
	{
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetMaterial(IEditorMaterial* mtl)
{
	if (m_pMaterial == mtl)
		return;

	StoreUndo("Assign Material");
	if (m_pMaterial)
	{
		m_pMaterial->Release();
	}
	m_pMaterial = mtl;
	if (m_pMaterial)
	{
		m_pMaterial->AddRef();
	}

	// Not sure if really needed, but add just in case
	UpdateUIVars();

	OnMaterialChanged(MATERIALCHANGE_ALL);
}

//////////////////////////////////////////////////////////////////////////
string CBaseObject::GetMaterialName() const
{
	if (m_pMaterial)
		return m_pMaterial->GetName();
	return "";
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetMaterial(const string& materialName)
{
	IEditorMaterial* pMaterial = NULL;
	if (!materialName.IsEmpty())
		pMaterial = GetIEditor()->LoadMaterial(materialName);
	SetMaterial(pMaterial);
}

void CBaseObject::OnMtlResolved(uint32 id, bool success, const char* orgName, const char* newName)
{
	if (success)
	{
		string materialName = newName;
		IEditorMaterial* pMaterial = NULL;
		if (!materialName.IsEmpty())
			pMaterial = GetIEditor()->LoadMaterial(materialName);
		SetMaterial(pMaterial);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetLayerModified()
{
	if (m_layer)
		m_layer->SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
	m_nMinSpec = nSpec;
	UpdateVisibility(!IsHidden());

	// Set min spec for all childs.
	if (bSetChildren)
	{
		for (int i = m_childs.size() - 1; i >= 0; --i)
		{
			m_childs[i]->SetMinSpec(nSpec, true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnPropertyChanged(IVariable*)
{
	SetLayerModified();
	UpdateGroup();
	UpdatePrefab();
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnMultiSelPropertyChanged(IVariable*)
{
	ISelectionGroup* grp = GetIEditor()->GetISelectionGroup();
	for (int i = 0; i < grp->GetCount(); i++)
	{
		grp->GetObject(i)->SetLayerModified();
		;
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnMenuProperties()
{
	if (!IsSelected())
	{
		CUndo undo("Select Object");
		GetIEditor()->GetObjectManager()->ClearSelection();
		GetIEditor()->SelectObject(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::OnContextMenu(CPopupMenuItem* menu)
{
	if (!menu->Empty())
	{
		menu->AddSeparator();
	}

	if (CBaseObject* pGroup = GetGroup())
	{
		if (GetIEditor()->IsGroupOpen(pGroup)) // If the group isn't open then we can't detach it's children
		{
			CPopupMenuItem& menuitem = menu->Add("Group");
			menuitem.Add("Detach From Group", [ = ] {
				BeginUndoAndEnsureSelection();
				GetIEditor()->OnGroupDetach();
				GetIEditor()->GetIUndoManager()->Accept("Detach From Group");
			});
		}
	}
	else
	{
		CPopupMenuItem& menuitem = menu->Add("Group");
		menuitem.Add("Create Group", [ = ] {
			BeginUndoAndEnsureSelection();
			GetIEditor()->OnGroupMake();
			GetIEditor()->GetIUndoManager()->Accept("Create Group");
		});

		menuitem.Add("Attach To Group", [ = ]
		{
			BeginUndoAndEnsureSelection();
			GetIEditor()->GetIUndoManager()->Accept("Select Object");
			GetIEditor()->OnGroupAttach();
		});
	}
	CPopupMenuItem& prefabMenu = menu->Add("Prefab");
	prefabMenu.Add("Create Prefab", [ = ] {
		BeginUndoAndEnsureSelection();
		GetIEditor()->OnPrefabMake();
		GetIEditor()->GetIUndoManager()->Accept("Create Prefab From Selection");
	});
	prefabMenu.Add("Attach To Prefab", [ = ]
	{
		BeginUndoAndEnsureSelection();
		GetIEditor()->GetIUndoManager()->Accept("Select Object");
		GetIEditor()->OnPrefabAttach();
	});

	menu->Add("Delete", [ = ]
	{
		IObjectManager* manager = GetObjectManager();
		BeginUndoAndEnsureSelection();
		manager->DeleteSelection();
		GetIEditor()->GetIUndoManager()->Accept("Delete Selection");
	});

	CPopupMenuItem& transformMenu = menu->Add("Transform");
	transformMenu.Add("Clear Rotation", [ = ]
	{
		BeginUndoAndEnsureSelection();
		ISelectionGroup* pGroup = GetIEditor()->GetISelectionGroup();
		Quat q(IDENTITY);
		for (int i = 0; i < pGroup->GetCount(); ++i)
		{
		  CBaseObject* pObj = pGroup->GetObject(i);
		  pObj->SetRotation(q);
		}
		GetIEditor()->GetIUndoManager()->Accept("Clear Rotation");
	});

	transformMenu.Add("Clear Scale", [ = ]
	{
		BeginUndoAndEnsureSelection();
		ISelectionGroup* pGroup = GetIEditor()->GetISelectionGroup();
		Vec3 s(1.0f, 1.0f, 1.0f);
		for (int i = 0; i < pGroup->GetCount(); ++i)
		{
		  CBaseObject* pObj = pGroup->GetObject(i);
		  pObj->SetScale(s);
		}
		GetIEditor()->GetIUndoManager()->Accept("Clear Scale");
	});

	transformMenu.Add("Clear All", [ = ]
	{
		BeginUndoAndEnsureSelection();
		ISelectionGroup* pGroup = GetIEditor()->GetISelectionGroup();
		Quat q(IDENTITY);
		Vec3 s(1.0f, 1.0f, 1.0f);
		for (int i = 0; i < pGroup->GetCount(); ++i)
		{
		  CBaseObject* pObj = pGroup->GetObject(i);
		  pObj->SetRotation(q);
		  pObj->SetScale(s);
		}
		GetIEditor()->GetIUndoManager()->Accept("Clear Rotation/Scale");
	});

	menu->AddSeparator();

	CUsedResources resources;
	GatherUsedResources(resources);

	//TODO: Object context menu
	// need to add a proper context menu here with all possible actions on the object !

	//Not necessary now that we have inspector, this mostly changed the rollup bar
	//menu->Add("Properties", functor(*this, &CBaseObject::OnMenuProperties));

	GetIEditor()->OnObjectContextMenuOpened(menu, this);

	//TODO: to be re-added when AssetBrowser is loading fast
	//menu->Add("Show in Asset Browser", functor(*this, &CBaseObject::OnMenuShowInAssetBrowser)).Enable(!resources.files.empty());
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::IntersectRayMesh(const Vec3& raySrc, const Vec3& rayDir, SRayHitInfo& outHitInfo) const
{
	const float fRenderMeshTestDistance = 0.2f;
	IRenderNode* pRenderNode = GetEngineNode();
	if (!pRenderNode)
		return false;

	int cntSlots = pRenderNode->GetSlotCount();

	Matrix34A worldTM;
	IStatObj* pStatObj = pRenderNode->GetEntityStatObj(0, 0, &worldTM);
	if (!pStatObj)
		return false;

	// transform decal into object space
	Matrix34A worldTM_Inverted = worldTM.GetInverted();
	Matrix33 worldRot(worldTM_Inverted);
	worldRot.Transpose();
	// put hit direction into the object space
	Vec3 vRayDir = rayDir.GetNormalized() * worldRot;
	// put hit position into the object space
	Vec3 vHitPos = worldTM_Inverted.TransformPoint(raySrc);
	Vec3 vLineP1 = vHitPos - vRayDir * fRenderMeshTestDistance;

	memset(&outHitInfo, 0, sizeof(outHitInfo));
	outHitInfo.inReferencePoint = vHitPos;
	outHitInfo.inRay.origin = vLineP1;
	outHitInfo.inRay.direction = vRayDir;
	outHitInfo.bInFirstHit = false;
	outHitInfo.bUseCache = false;

	return pStatObj->RayIntersection(outHitInfo, 0);
}

//////////////////////////////////////////////////////////////////////////
EScaleWarningLevel CBaseObject::GetScaleWarningLevel() const
{
	EScaleWarningLevel scaleWarningLevel = eScaleWarningLevel_None;

	const float kScaleThreshold = 0.001f;
	auto scale = GetScale();

	if (fabs(scale.x - 1.0f) > kScaleThreshold
	    || fabs(scale.y - 1.0f) > kScaleThreshold
	    || fabs(scale.z - 1.0f) > kScaleThreshold)
	{
		if (fabs(scale.x - scale.y) < kScaleThreshold
		    && fabs(scale.y - scale.z) < kScaleThreshold)
		{
			scaleWarningLevel = eScaleWarningLevel_Rescaled;
		}
		else
		{
			scaleWarningLevel = eScaleWarningLevel_RescaledNonUniform;
		}
	}

	return scaleWarningLevel;
}

//////////////////////////////////////////////////////////////////////////
ERotationWarningLevel CBaseObject::GetRotationWarningLevel() const
{
	ERotationWarningLevel rotationWarningLevel = eRotationWarningLevel_None;

	const float kRotationThreshold = 0.01f;

	Ang3 eulerRotation = Ang3(GetRotation());

	if (fabs(eulerRotation.x) > kRotationThreshold
	    || fabs(eulerRotation.y) > kRotationThreshold
	    || fabs(eulerRotation.z) > kRotationThreshold)
	{
		const float xRotationMod = fabs(fmod(eulerRotation.x, gf_PI / 2.0f));
		const float yRotationMod = fabs(fmod(eulerRotation.y, gf_PI / 2.0f));
		const float zRotationMod = fabs(fmod(eulerRotation.z, gf_PI / 2.0f));

		if ((xRotationMod < kRotationThreshold || xRotationMod > (gf_PI / 2.0f - kRotationThreshold))
		    && (yRotationMod < kRotationThreshold || yRotationMod > (gf_PI / 2.0f - kRotationThreshold))
		    && (zRotationMod < kRotationThreshold || zRotationMod > (gf_PI / 2.0f - kRotationThreshold)))
		{
			rotationWarningLevel = eRotationWarningLevel_Rotated;
		}
		else
		{
			rotationWarningLevel = eRotationWarningLevel_RotatedNonRectangular;
		}
	}

	return rotationWarningLevel;
}

bool CBaseObject::IsSkipSelectionHelper() const
{
	CEditTool* pEditTool(GetIEditor()->GetEditTool());
	if (pEditTool && pEditTool->IsNeedToSkipPivotBoxForObjects())
		return true;
	return false;
}

CBaseObject* CBaseObject::GetGroup() const
{
	CBaseObject* pParentObject = GetParent();
	while (pParentObject)
	{
		if (GetIEditor()->IsCGroup(pParentObject))
			return pParentObject;
		pParentObject = pParentObject->GetParent();
	}
	return NULL;
}

CBaseObject* CBaseObject::GetPrefab() const
{
	CBaseObject* pParentObject = GetParent();
	while (pParentObject)
	{
		if (GetIEditor()->IsCPrefabObject(pParentObject))
			return pParentObject;
		pParentObject = pParentObject->GetParent();
	}
	return nullptr;
}

bool CBaseObject::IsPartOfPrefab() const
{
	return CheckFlags(OBJFLAG_PREFAB);
}

void CBaseObject::UpdateGroup()
{
	CBaseObject* pParentObject = GetParent();
	while (pParentObject)
	{
		if (GetIEditor()->IsCGroup(pParentObject))
			GetIEditor()->SyncCGroup(pParentObject);
		pParentObject = pParentObject->GetParent();
	}
}

void CBaseObject::UpdatePrefab(EObjectChangedOpType updateReason)
{
	CBaseObject* pPrefab = GetPrefab();
	if (!pPrefab || GetIEditor()->IsModifyInProgressCPrefabObject(pPrefab))
		return;

	SObjectChangedContext context;
	context.m_modifiedObjectGlobalId = GetId();
	context.m_modifiedObjectGuidInPrefab = GetIdInPrefab();
	context.m_operation = updateReason;

	GetIEditor()->SyncPrefabCPrefabObject(pPrefab, context);
}

bool CBaseObject::CanBeHightlighted() const
{
	CBaseObject* pGroup = GetGroup();
	return !pGroup || GetIEditor()->IsGroupOpen(pGroup);
}

Matrix33 CBaseObject::GetWorldRotTM() const
{
	if (GetParent())
		return GetParent()->GetWorldRotTM() * Matrix33(GetRotation());
	return Matrix33(GetRotation());
}

void CBaseObject::BeginUndoAndEnsureSelection()
{
	IObjectManager* manager = GetObjectManager();

	GetIEditor()->GetIUndoManager()->Begin();
	if (!CheckFlags(OBJFLAG_SELECTED) || GetIEditor()->GetISelectionGroup()->GetCount() == 0)
	{
		// Select just this object, so we need to clear selection
		manager->ClearSelection();
		manager->SelectObject(this);
	}
}

Matrix33 CBaseObject::GetWorldScaleTM() const
{
	if (GetParent())
		return GetParent()->GetWorldScaleTM() * Matrix33::CreateScale(GetScale());
	return Matrix33::CreateScale(GetScale());
}

void CBaseObject::SetTransformDelegate(ITransformDelegate* pTransformDelegate)
{
	m_pTransformDelegate = pTransformDelegate;
	InvalidateTM(0);
}

void CBaseObject::AddMember(CBaseObject* pMember, bool bKeepPos /*=true */)
{
	AttachChild(pMember, bKeepPos);
	if (CBaseObject* pPrefab = GetPrefab())
		pPrefab->AddMember(pMember, bKeepPos);
}

void CBaseObject::OnBeforeAreaChange()
{
	AABB aabb;
	GetBoundBox(aabb);
	GetIEditor()->OnAreaModified(aabb);
}
//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetMaterialByName(const char* mtlName)
{

}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SerializeUiProperties(Serialization::IArchive& ar, bool bMultiEdit)
{
	SerializeProperties(ar, bMultiEdit);

	if (ar.isInput())
	{
		SetModified(false);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SerializeProperties(Serialization::IArchive& ar, bool bMultiEdit)
{
	if (ar.openBlock("General", "General"))
	{
		string typeDescr = GetTypeDescription();
		ar(typeDescr, "type_name", "!Type");
		string name = (const char*)m_name;
		ar(name, "name", "Name");

		int abgr = m_color;
		ColorB color = ColorB((uint32)abgr);
		ar(color, "color", "Color");

		string layer = (const char*)m_layer->GetFullName();
		ar(Serialization::LevelLayerPicker(layer), "layer", "Layer");

		string mtl = GetMaterialName();
		ar(Serialization::MaterialPicker(mtl), "mtl", "Override Material");

		ESystemConfigSpec minSpec = (ESystemConfigSpec)m_nMinSpec;
		ar(minSpec, "MinSpec", "MinSpec");

		if (ar.isInput())
		{
			bool objectChanged = false;
			abgr = color.pack_abgr8888();
			if (abgr != m_color)
			{
				objectChanged = true;
				SetColor(abgr);
			}

			if (!name.empty() && strcmp(name.c_str(), (LPCSTR) m_name) != 0)
			{
				// This may also change object id.
				GetObjectManager()->ChangeObjectName(this, name.c_str(), bMultiEdit); // Force a unique name if doing a multi-select change
				objectChanged = true;
			}

			if (minSpec != GetMinSpec())
			{
				StoreUndo("Change MinSpec", true);
				SetMinSpec(minSpec);
				objectChanged = true;
			}

			if (m_pMaterial)
			{
				if (mtl.empty())
				{
					SetMaterial(0);
					objectChanged = true;
				}
				else if (strcmp(mtl.c_str(), (LPCSTR)m_pMaterial->GetName()) != 0)
				{
					SetMaterial(mtl.c_str());
					objectChanged = true;
				}
			}
			else if (!mtl.empty())
			{
				SetMaterial(mtl.c_str());
				objectChanged = true;
			}

			if (layer != m_layer->GetFullName())
			{
				SetLayer(layer);
				objectChanged = true;
			}

			// if we had a change make sure to propagate it to the prefab
			if (objectChanged)
			{
				UpdatePrefab();
			}
		}

		ar.closeBlock();
	}

	if (ar.openBlock("xform", "Transform"))
	{
		static bool uniform = GetUniformScale();
		bool uniformScale = uniform;

		// Using delegate here as we want the properties to display and edit the delegate transform
		Vec3 pos = GetPos();
		Quat rot = GetRotation();
		Vec3 scl = GetScale();

		ar(Serialization::SPosition(pos), "xform.pos", "Position");
		ar(Serialization::SRotation(rot), "xform.rot", "Rotation");
		ar(Serialization::SUniformScale(scl, uniformScale), "xform.scl", "Scale");
		if (ar.isInput())
		{
			if (uniform != uniformScale)
			{
				uniform = uniformScale;
				SetUniformScale(uniform);
			}

			SetPos(pos);

			if (!Quat::IsEquivalent(m_rotate, rot, 0.001f))
				SetRotation(rot);

			SetScale(scl);
		}
		ar.closeBlock();
	}

	// Same interface as prefabs - they are handled from Prefab object in old UI, though it might make sense to simplify here.
	if (!CBaseObject::GetPrefab())
	{
		if (ar.openBlock("group", "Group"))
		{
			if (ar.openBlock("group_operators", "<Group"))
			{
				if (GetGroup())
				{
					ar(Serialization::ActionButton([]() { GetIEditor()->OnGroupDetach(); }), "detach", "^Detach");
				}
				else
				{
					ar(Serialization::ActionButton([]() { GetIEditor()->OnGroupMake(); }), "group", "^Create");
					ar(Serialization::ActionButton([]() { GetIEditor()->OnGroupAttach(); }), "attach", "^Attach to...");
				}
				ar.closeBlock();
			}
			ar.closeBlock();
		}
	}

	CVarBlock* varBlock = GetVarBlock();
	if (varBlock)
		ar(*varBlock, "properties", "|Properties");
}

void CBaseObject::RegisterOnEngine()
{
	IRenderNode* pRenderNode = GetEngineNode();
	if (pRenderNode && pRenderNode->GetEntityStatObj())
	{
		GetIEditor()->RegisterEntity(pRenderNode);
	}
}

void CBaseObject::UnRegisterFromEngine()
{
	IRenderNode* pRenderNode = GetEngineNode();
	if (pRenderNode && pRenderNode->GetEntityStatObj())
	{
		GetIEditor()->UnRegisterEntityAsJob(pRenderNode);
	}
}

const char* CBaseObject::ObjectEventToString(EObjectListenerEvent e)
{
#define STRINGIFY_EVENT(e) case e: \
  return # e;
	switch (e)
	{
		STRINGIFY_EVENT(OBJECT_ON_DELETE)
		STRINGIFY_EVENT(OBJECT_ON_ADD)
		STRINGIFY_EVENT(OBJECT_ON_SELECT)
		STRINGIFY_EVENT(OBJECT_ON_UNSELECT)
		STRINGIFY_EVENT(OBJECT_ON_TRANSFORM)
		STRINGIFY_EVENT(OBJECT_ON_VISIBILITY)
		STRINGIFY_EVENT(OBJECT_ON_RENAME)
		STRINGIFY_EVENT(OBJECT_ON_CHILDATTACHED)
		STRINGIFY_EVENT(OBJECT_ON_PREDELETE)
		STRINGIFY_EVENT(OBJECT_ON_CHILDDETACHED)
		STRINGIFY_EVENT(OBJECT_ON_DETACHFROMPARENT)
		STRINGIFY_EVENT(OBJECT_ON_PREATTACHED)
		STRINGIFY_EVENT(OBJECT_ON_PREATTACHEDKEEPXFORM)
		STRINGIFY_EVENT(OBJECT_ON_ATTACHED)
		STRINGIFY_EVENT(OBJECT_ON_PREDETACHED)
		STRINGIFY_EVENT(OBJECT_ON_PREDETACHEDKEEPXFORM)
		STRINGIFY_EVENT(OBJECT_ON_DETACHED)
		STRINGIFY_EVENT(OBJECT_ON_PREFAB_CHANGED)
		STRINGIFY_EVENT(OBJECT_ON_UI_PROPERTY_CHANGED)
		STRINGIFY_EVENT(OBJECT_ON_LAYERCHANGE)
	}
#undef STRINGIFY_EVENT

	return "Unknown Event";
}
