// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "Controls/PropertyCtrl.h"

class CPropertyCtrlExt : public CPropertyCtrl
{
	DECLARE_DYNAMIC(CPropertyCtrlExt)

public:
	typedef Functor1<CPropertyItem*> PreSelChangeCallback;

	CPropertyCtrlExt();

	void SetPreSelChangeCallback(PreSelChangeCallback& callback) { m_preSelChangeFunc = callback; }
	void SelectItem(CPropertyItem* item);
	void OnItemChange(CPropertyItem* item);

	void SetVehicleVar(IVariable* pVar) { m_pVehicleVar = pVar; }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	void         OnAddChild();
	void         OnDeleteChild(CPropertyItem* pItem);
	void         OnGetEffect(CPropertyItem* pItem);

	void         ReloadItem(CPropertyItem* pItem);

	PreSelChangeCallback m_preSelChangeFunc;

	IVariable*           m_pVehicleVar;
};
