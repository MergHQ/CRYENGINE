// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "OpticsElement.h"

class COpticsGroup : public COpticsElement
{

protected:
	std::vector<IOpticsElementBasePtr> children;
	void _init();

public:

	COpticsGroup(const char* name = "[Unnamed_Group]") : COpticsElement(name) { _init(); };
	COpticsGroup(const char* name, COpticsElement* elem, ...);
	virtual ~COpticsGroup(){}

	COpticsGroup&       Add(IOpticsElementBase* pElement);
	void                Remove(int i);
	void                RemoveAll();
	virtual int         GetElementCount() const;
	IOpticsElementBase* GetElementAt(int i) const;

	void                AddElement(IOpticsElementBase* pElement) { Add(pElement);    }
	void                InsertElement(int nPos, IOpticsElementBase* pElement);
	void                SetElementAt(int i, IOpticsElementBase* elem);
	void                Invalidate();

	bool                IsGroup() const { return true; }
	void                validateChildrenGlobalVars(SAuxParams& aux);

	virtual void        GetMemoryUsage(ICrySizer* pSizer) const;
	virtual EFlareType  GetType() { return eFT_Group; }
	virtual void        validateGlobalVars(SAuxParams& aux);

	void                Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux);

#if defined(FLARES_SUPPORT_EDITING)
	virtual void InitEditorParamGroups(DynArray<FuncVariableGroup>& groups);
#endif

public:
	static COpticsGroup predef_simpleCamGhost;
	static COpticsGroup predef_cheapCamGhost;
	static COpticsGroup predef_multiGlassGhost;
};
