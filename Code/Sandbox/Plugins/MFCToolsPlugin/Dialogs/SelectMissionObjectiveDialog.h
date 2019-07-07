// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GenericSelectItemDialog.h"

class CSelectMissionObjectiveDialog : public CGenericSelectItemDialog
{
	DECLARE_DYNAMIC(CSelectMissionObjectiveDialog)
	CSelectMissionObjectiveDialog(CWnd* pParent = NULL);

protected:
	virtual BOOL OnInitDialog();

	// Derived Dialogs should override this
	virtual void GetItems(std::vector<SItem>& outItems);
	void         GetItemsInternal(std::vector<SItem>& outItems, const char* path, const bool isOptional);

	// Called whenever an item gets selected
	virtual void ItemSelected();

	struct SObjective
	{
		CString shortText;
		CString longText;
	};

	typedef std::map<CString, SObjective, stl::less_stricmp<CString>> TObjMap;
	TObjMap m_objMap;
};
