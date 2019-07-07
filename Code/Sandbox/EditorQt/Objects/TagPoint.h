// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EntityObject.h"
#include <EditorFramework/Preferences.h>

// Preferences
struct STagPointPreferences : public SPreferencePage
{
	STagPointPreferences()
		: SPreferencePage("Tag Point", "Viewport/Gizmo")
		, tagpointScaleMulti(0.5f)
	{
	}

	virtual bool Serialize(yasli::Archive& ar) override
	{
		ar.openBlock("tagPoint", "Tag Point");
		ar(tagpointScaleMulti, "tagpointScaleMulti", "Tagpoint Scale Multiplier");

		return true;
	}

	float tagpointScaleMulti;
};

/*!
 *	CTagPoint is an object that represent named 3d position in world.
 *
 */

class CTagPoint : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CTagPoint)

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CBaseObject.
	//////////////////////////////////////////////////////////////////////////
	bool         Init(CBaseObject* prev, const string& file);
	void         InitVariables();
	virtual void Display(CObjectRenderHelper& objRenderHelper) override;
	bool         IsScalable() const override { return false; }

	//! Called when object is being created.
	int  MouseCreateCallback(IDisplayViewport* view, EMouseEvent event, CPoint& point, int flags);
	bool HitTest(HitContext& hc);

	void GetLocalBounds(AABB& box);
	void GetBoundBox(AABB& box);
	//////////////////////////////////////////////////////////////////////////

protected:
	CTagPoint();

	float GetRadius();

	void  DeleteThis() { delete this; }
};

/*
   NavigationSeedPoint is a special tag point used to generate the reachable
   part of a navigation mesh starting from his position
 */
class CNavigationSeedPoint : public CTagPoint
{
public:
	DECLARE_DYNCREATE(CNavigationSeedPoint)
	virtual void Display(CObjectRenderHelper& objRenderHelper) override
	{
		DrawDefault(objRenderHelper.GetDisplayContextRef());
	}

protected:
	CNavigationSeedPoint()
	{
		m_entityClass = "NavigationSeedPoint";
	}
};

/*!
 * Class Description of TagPoint.
 */
class CTagPointClassDesc : public CObjectClassDesc
{
public:
	ObjectType          GetObjectType()              { return OBJTYPE_TAGPOINT; }
	const char*         ClassName()                  { return "StdTagPoint"; }
	const char*         Category()                   { return ""; }
	CRuntimeClass*      GetRuntimeClass()            { return RUNTIME_CLASS(CTagPoint); }
	virtual const char* GetTextureIcon()             { return "%EDITOR%/ObjectIcons/TagPoint.bmp"; }
	virtual bool        IsCreatable() const override { return gEnv->pEntitySystem->GetClassRegistry()->FindClass("TagPoint") != nullptr; }
};

class CNavigationSeedPointClassDesc : public CObjectClassDesc
{
public:
	ObjectType     GetObjectType()              { return OBJTYPE_TAGPOINT; }
	const char*    ClassName()                  { return "NavigationSeedPoint"; }
	const char*    Category()                   { return "AI"; }
	CRuntimeClass* GetRuntimeClass()            { return RUNTIME_CLASS(CNavigationSeedPoint); }
	virtual bool   IsCreatable() const override { return gEnv->pEntitySystem->GetClassRegistry()->FindClass("NavigationSeedPoint") != nullptr; }
};
