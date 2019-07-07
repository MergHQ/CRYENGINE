// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GenericSelectItemDialog.h"

class CSelectSequenceDialog : public CGenericSelectItemDialog
{
	DECLARE_DYNAMIC(CSelectSequenceDialog)
	CSelectSequenceDialog(CWnd* pParent = NULL);

protected:
	virtual BOOL OnInitDialog();

	// Derived Dialogs should override this
	virtual void GetItems(std::vector<SItem>& outItems);
};
