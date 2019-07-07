// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include "SandboxAPI.h"
#include <CryCore/smartptr.h>
#include <CryMath/Cry_Math.h>

struct AABB;
struct HitContext;
struct IIndexedMesh;
struct IStatObj;
struct SDisplayContext;
struct SSubObjSelectionModifyContext;
class CObjectArchive;

// Basic supported geometry types.
enum EEdGeometryType
{
	GEOM_TYPE_MESH = 0, // Mesh geometry.
	GEOM_TYPE_BRUSH,    // Solid brush geometry.
	GEOM_TYPE_PATCH,    // Bezier patch surface geometry.
	GEOM_TYPE_NURB,     // Nurbs surface geometry.
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    CEdGeometry is a base class for all supported editable geometries.
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CEdGeometry : public _i_reference_target_t
{
	DECLARE_DYNAMIC(CEdGeometry)
public:
	CEdGeometry() {}

	// Query the type of the geometry mesh.
	virtual EEdGeometryType GetType() const = 0;

	// Return geometry axis aligned bounding box.
	virtual void GetBounds(AABB& box) = 0;

	// Clones Geometry, returns exact copy of the original geometry.
	virtual CEdGeometry* Clone() = 0;

	// Access to the indexed mesh.
	// Return false if geometry can not be represented by an indexed mesh.
	virtual IIndexedMesh* GetIndexedMesh(size_t idx = 0) = 0;
	virtual IStatObj*     GetIStatObj() const = 0;
	virtual void          GetTM(Matrix34* pTM, size_t idx = 0) = 0;

	//////////////////////////////////////////////////////////////////////////
	// Advanced geometry interface for SubObject selection and modification.
	//////////////////////////////////////////////////////////////////////////
	virtual bool StartSubObjSelection(const Matrix34& nodeWorldTM, int elemType, int nFlags) = 0;
	virtual void EndSubObjSelection() = 0;

	// Display geometry for sub object selection.
	virtual void Display(SDisplayContext& dc) = 0;

	// Sub geometry hit testing and selection.
	virtual bool HitTest(HitContext& hit) = 0;

protected:
	~CEdGeometry() {}
};
