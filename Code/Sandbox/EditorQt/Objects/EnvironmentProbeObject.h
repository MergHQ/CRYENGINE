// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EntityObject.h"

/*!
 *	CEnvironmentProbeObject is an object that represent an environment map generator
 *	also it encapsulates a light
 */

struct IBackgroundScheduleItemWork;
struct IMaterial;

class SANDBOX_API CEnvironementProbeObject : public CEntityObject
{
public:
	DECLARE_DYNCREATE(CEnvironementProbeObject)

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CBaseObject.
	//////////////////////////////////////////////////////////////////////////
	bool                         Init(CBaseObject* prev, const string& file);
	void                         InitVariables();
	void                         Serialize(CObjectArchive& ar);

	void                         CreateInspectorWidgets(CInspectorWidgetCreator& creator) override;

	virtual void                 Display(CObjectRenderHelper& objRenderHelper) override;
	void                         GetDisplayBoundBox(AABB& box) override;
	void                         GetBoundBox(AABB& box);
	void                         GetLocalBounds(AABB& aabb);
	//! Generates the cubemap file and creates a task for task manager
	IBackgroundScheduleItemWork* GenerateCubemapTask();
	void                         GenerateCubemap();
	void                         OnPreviewCubemap(IVariable* piVariable);
	IMaterial*                   CreateMaterial();
	virtual void                 UpdateLinks();

	virtual int                  AddEntityLink(const string& name, CryGUID targetEntityId) override;
	void                         OnPropertyChanged(IVariable* pVar);
	void                         OnMultiSelPropertyChanged(IVariable* pVar);

	// HACK: We override this function because environment probes need to manually set their light param
	// after the entity has set/initialized the script
	void SetScriptName(const string& file, CBaseObject* pPrev) override;

protected:
	//! Dtor must be protected.
	CEnvironementProbeObject();

	CSmartVariable<bool>    m_preview_cubemap;
	CSmartVariableEnum<int> m_cubemap_resolution;
};

/*!
 * Class Description of EnvironmentProbeObject.
 */
class CEnvironmentProbeObjectClassDesc : public CObjectClassDesc
{
public:
	ObjectType     GetObjectType()              { return OBJTYPE_ENTITY; }
	const char*    ClassName()                  { return "EnvironmentProbe"; }
	const char*    Category()                   { return "Misc"; }
	CRuntimeClass* GetRuntimeClass()            { return RUNTIME_CLASS(CEnvironementProbeObject); }
	const char*    GetTextureIcon()             { return "%EDITOR%/ObjectIcons/environmentProbe.bmp"; }
	virtual bool   IsCreatable() const override { return gEnv->pEntitySystem->GetClassRegistry()->FindClass("EnvironmentLight"); }
};
