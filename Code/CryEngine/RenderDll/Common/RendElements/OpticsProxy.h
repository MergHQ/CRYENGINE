// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "OpticsReference.h"

class COpticsProxy : public IOpticsElementBase
{
public:

	COpticsProxy(const char* name);
	~COpticsProxy(){}

	EFlareType          GetType()                    { return eFT_Proxy; }
	bool                IsGroup() const              { return false; }

	string              GetName() const              { return m_name;  }
	void                SetName(const char* ch_name) { m_name = ch_name; }
	void                Load(IXmlNode* pNode);

	IOpticsElementBase* GetParent() const                                     { return NULL;  }

	bool                IsEnabled() const                                     { return m_bEnable; }
	void                SetEnabled(bool b)                                    { m_bEnable = b;    }

	void                AddElement(IOpticsElementBase* pElement)              {}
	void                InsertElement(int nPos, IOpticsElementBase* pElement) {}
	void                Remove(int i)                                         {}
	void                RemoveAll()                                           {}
	int                 GetElementCount() const                               { return 0; }
	IOpticsElementBase* GetElementAt(int i) const                             { return NULL;  }

	void                GetMemoryUsage(ICrySizer* pSizer) const;
	void                Invalidate();

	void                Render(SLensFlareRenderParam* pParam, const Vec3& vPos);

	void                SetOpticsReference(IOpticsElementBase* pReference)
	{
		if (pReference->GetType() == eFT_Reference)
			m_pOpticsReference = (COpticsReference*)pReference;
	}
	IOpticsElementBase* GetOpticsReference() const
	{
		return m_pOpticsReference;
	}
#if defined(FLARES_SUPPORT_EDITING)
	DynArray<FuncVariableGroup> GetEditorParamGroups();
#endif

public:

	bool                         m_bEnable;
	string                       m_name;
	_smart_ptr<COpticsReference> m_pOpticsReference;

};
