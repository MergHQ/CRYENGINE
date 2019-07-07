// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <Objects/BaseObject.h>
#include <Util/Variable.h>

class CCloudObject : public CBaseObject
{
public:
	DECLARE_DYNCREATE(CCloudObject)

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CBaseObject.
	//////////////////////////////////////////////////////////////////////////
	void          Display(CObjectRenderHelper& objRenderHelper);
	void          GetLocalBounds(AABB& box);
	bool          HitTest(HitContext& hc);

	virtual float GetCreationOffsetFromTerrain() const override { return 0.f; }

	virtual void  CreateInspectorWidgets(CInspectorWidgetCreator& creator) override;

	//////////////////////////////////////////////////////////////////////////
	// Cloud parameters.
	//////////////////////////////////////////////////////////////////////////
	CVariable<int>   mv_spriteRow;
	CVariable<float> m_width;
	CVariable<float> m_height;
	CVariable<float> m_length;
	CVariable<float> m_angleVariations;
	CVariable<float> mv_sizeSprites;
	CVariable<float> mv_randomSizeValue;

protected:
	CCloudObject();

	void DeleteThis() { delete this; }
	void OnSizeChange(IVariable* pVar);

	bool m_bNotSharedGeom;
	bool m_bIgnoreNodeUpdate;
};

/*!
 * Class Description of CCloudObject.
 */
class CCloudObjectClassDesc : public CObjectClassDesc
{
public:
	ObjectType     GetObjectType()   { return OBJTYPE_CLOUD; }
	const char*    ClassName()       { return "CloudVolume"; }
	const char*    Category()        { return ""; }
	CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CCloudObject); }
	const char*    GetFileSpec()     { return ""; }
};
