// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseLibraryItem.h"
#include <CryRenderer/IFlares.h>

class CEntityObject;

class CLensFlareItem : public CBaseLibraryItem
{
public:

	CLensFlareItem();
	~CLensFlareItem();

	EDataBaseItemType GetType() const
	{
		return EDB_TYPE_FLARE;
	}

	void                  SetName(const string& name);
	void                  SetName(const string& name, bool bRefreshWhenUndo, bool bRefreshWhenRedo);
	void                  Serialize(SerializeContext& ctx);

	void                  CreateOptics();

	IOpticsElementBasePtr GetOptics() const { return m_pOptics; }

	void                  ReplaceOptics(IOpticsElementBasePtr pNewData);

	XmlNodeRef            CreateXmlData() const;
	void                  UpdateLights(IOpticsElementBasePtr pSrcOptics = NULL);

private:

	void        GetLightEntityObjects(std::vector<CEntityObject*>& outEntityLights) const;
	static void AddChildOptics(IOpticsElementBasePtr pParentOptics, XmlNodeRef& pNode);

	IOpticsElementBasePtr m_pOptics;
};
