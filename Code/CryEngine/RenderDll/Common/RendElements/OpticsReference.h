// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryRenderer/IFlares.h>

class COpticsReference : public IOpticsElementBase
{
public:

#if defined(FLARES_SUPPORT_EDITING)
	DynArray<FuncVariableGroup> GetEditorParamGroups();
#endif

	COpticsReference(const char* name);
	~COpticsReference(){}

	EFlareType          GetType()                    { return eFT_Reference; }
	bool                IsGroup() const              { return false; }

	string              GetName() const              { return m_name;  }
	void                SetName(const char* ch_name) { m_name = ch_name; }
	void                Load(IXmlNode* pNode);

	IOpticsElementBase* GetParent() const  { return NULL;  }

	bool                IsEnabled() const  { return true;  }
	void                SetEnabled(bool b) {}
	void                AddElement(IOpticsElementBase* pElement);
	void                InsertElement(int nPos, IOpticsElementBase* pElement);
	void                Remove(int i);
	void                RemoveAll();
	int                 GetElementCount() const;
	IOpticsElementBase* GetElementAt(int i) const;

	void                GetMemoryUsage(ICrySizer* pSizer) const;
	void                Invalidate();

	void                Render(SLensFlareRenderParam* pParam, const Vec3& vPos);

public:
	string                             m_name;
	std::vector<IOpticsElementBasePtr> m_OpticsList;
};
