// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SandboxAPI.h"
#include "EntityObject.h"

struct IGeometry;
struct IStatObj;

//////////////////////////////////////////////////////////////////////////
// WindArea entity.
//////////////////////////////////////////////////////////////////////////
class CWindAreaEntity : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CWindAreaEntity)

	virtual void Display(CObjectRenderHelper& objRenderHelper);
};

/*!
 * Class Description of Entity
 */
class CWindAreaEntityClassDesc : public CObjectClassDesc
{
public:
	ObjectType          GetObjectType()                     { return OBJTYPE_ENTITY; }
	const char*         ClassName()                         { return "Entity::WindArea"; }
	const char*         Category()                          { return ""; }
	CRuntimeClass*      GetRuntimeClass()                   { return RUNTIME_CLASS(CWindAreaEntity); }
	const char*         GetFileSpec()                       { return "*.cgf;*.chr;*.cga;*.cdf"; }
	virtual const char* GetDataFilesFilterString() override { return GetFileSpec(); }
};

// Constraint entity.
//////////////////////////////////////////////////////////////////////////
class CConstraintEntity : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CConstraintEntity)

	//////////////////////////////////////////////////////////////////////////
	CConstraintEntity() : m_body0(NULL), m_body1(NULL) {}

	virtual void Display(CObjectRenderHelper& objRenderHelper);

private:
	// the orientation of the constraint in world, body0 and body1 space
	quaternionf m_qframe, m_qloc0, m_qloc1;

	// the position of the constraint in world space
	Vec3 m_posLoc;

	// pointers to the two joined bodies
	IPhysicalEntity* m_body0, * m_body1;
};

/*!
 * Class Description of Entity
 */
class CConstraintEntityClassDesc : public CObjectClassDesc
{
public:
	ObjectType          GetObjectType()                     { return OBJTYPE_ENTITY; }
	// this was done because the string was prefixed in the object manager and would otherwise return by default CEntityObject
	const char*         ClassName()                         { return "Entity::Constraint"; }
	const char*         Category()                          { return ""; }
	CRuntimeClass*      GetRuntimeClass()                   { return RUNTIME_CLASS(CConstraintEntity); }
	const char*         GetFileSpec()                       { return "*.cgf;*.chr;*.cga;*.cdf"; }
	virtual const char* GetDataFilesFilterString() override { return GetFileSpec(); }
};

#if defined(USE_GEOM_CACHES)
// GeomCache entity.
class SANDBOX_API CGeomCacheEntity : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CGeomCacheEntity)

	CGeomCacheEntity() {}

	virtual bool HitTestEntity(HitContext& hc, bool& bHavePhysics) override;
};
#endif

#if defined(USE_GEOM_CACHES)
/*!
 * Class Description of Entity
 */
class CGeomCacheEntityClassDesc : public CObjectClassDesc
{
public:
	ObjectType          GetObjectType()                     { return OBJTYPE_GEOMCACHE; }
	const char*         ClassName()                         { return "Entity::GeomCache"; }
	const char*         Category()                          { return ""; }
	CRuntimeClass*      GetRuntimeClass()                   { return RUNTIME_CLASS(CGeomCacheEntity); }
	const char*         GetFileSpec()                       { return "*.cax"; }
	virtual const char* GetDataFilesFilterString() override { return GetFileSpec(); }
	virtual bool        RenderTextureOnTop() const override { return true; }
};
#endif

//////////////////////////////////////////////////////////////////////////
// JointGen entity
//////////////////////////////////////////////////////////////////////////
class CJointGenEntity : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CJointGenEntity)
	CJointGenEntity();
	virtual void Display(CObjectRenderHelper& objRenderHelper);
private:
	string                m_fname;
	_smart_ptr<IStatObj>  m_pObj;
	_smart_ptr<IGeometry> m_pSph;
};

class CJointGenEntityClassDesc : public CObjectClassDesc
{
public:
	ObjectType          GetObjectType()                     { return OBJTYPE_ENTITY; }
	const char*         ClassName()                         { return "Entity::JointGen"; }
	const char*         Category()                          { return ""; }
	CRuntimeClass*      GetRuntimeClass()                   { return RUNTIME_CLASS(CJointGenEntity); }
	const char*         GetFileSpec()                       { return "*.cgf"; }
	virtual const char* GetDataFilesFilterString() override { return GetFileSpec(); }
};
