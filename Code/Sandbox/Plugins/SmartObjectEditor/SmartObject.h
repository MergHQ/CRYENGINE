// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "Objects/EntityObject.h"
#include "SmartObjectsEditorDialog.h"

/*!
 *	CSmartObject is an special entity visible in editor but invisible in game, that's registered with AI system as an smart object.
 *
 */
class CSmartObject : public CEntityObject
{
private:
	Matrix34                              m_statObjWorldMatrix;

protected:
	IStatObj*                             m_pStatObj;
	IMaterial*                            m_pHelperMtl;

	CSOLibrary::CClassTemplateData const* m_pClassTemplate;

public:
	DECLARE_DYNCREATE(CSmartObject)

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CBaseObject.
	//////////////////////////////////////////////////////////////////////////
	virtual bool Init(CBaseObject* prev, const string& file) override;
	virtual void InitVariables() override {}
	virtual void Done() override;
	virtual void Display(CObjectRenderHelper& objRenderHelper) override;
	virtual bool HitTest(HitContext& hc) override;
	virtual void GetLocalBounds(AABB& box) override;
	bool         IsScalable() const override { return false; }
	virtual void OnEvent(ObjectEvent eventID) override;
	virtual void GetScriptProperties(XmlNodeRef xmlEntityNode) override;
	virtual void DrawDefault(SDisplayContext& dc, COLORREF labelColor = RGB(255, 255, 255)) override;

	//////////////////////////////////////////////////////////////////////////

	IStatObj* GetIStatObj();

protected:
	//! Dtor must be protected.
	CSmartObject();
	~CSmartObject();

	float      GetRadius();

	void       DeleteThis() { delete this; }
	IMaterial* GetHelperMaterial();

public:
	void OnPropertyChange(IVariable* var);
};

/*!
 * Class Description of SmartObject.
 */
class CSmartObjectClassDesc : public CObjectClassDesc
{
public:
	ObjectType     GetObjectType()              { return OBJTYPE_AIPOINT; }
	const char*    ClassName()                  { return "SmartObject"; }
	const char*    Category()                   { return "AI"; }
	CRuntimeClass* GetRuntimeClass()            { return RUNTIME_CLASS(CSmartObject); }
	virtual bool   IsCreatable() const override { return gEnv->pEntitySystem->GetClassRegistry()->FindClass("SmartObject") != nullptr; }
};
