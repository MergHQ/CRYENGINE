// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MFCToolsDefines.h"
#include "Controls/PropertyCtrl.h"
#include "Util/Variable.h"

class MFC_TOOLS_PLUGIN_API CPropertiesPanel : public CDialog
{
public:
	CPropertiesPanel(CWnd* pParent = NULL);

	typedef Functor1<IVariable*> UpdateCallback;

	void ReloadValues();

	void DeleteVars();
	void AddVars(class CVarBlock* vb, const UpdateCallback& func = NULL);

	void SetVarBlock(class CVarBlock* vb, const UpdateCallback& func = NULL, const bool resizeToFit = true);

	void         SetMultiSelect(bool bEnable);
	bool         IsMultiSelect() const { return m_multiSelect; }

	void         SetTitle(const char* title);
	virtual void ResizeToFitProperties();

	//////////////////////////////////////////////////////////////////////////
	// Helper functions.
	//////////////////////////////////////////////////////////////////////////
	template<class T>
	void SyncValue(CSmartVariable<T>& var, T& value, bool bCopyToUI, IVariable* pSrcVar = NULL)
	{
		if (bCopyToUI)
			var = value;
		else
		{
			if (!pSrcVar || pSrcVar == var.GetVar())
				value = var;
		}
	}
	void SyncValueFlag(CSmartVariable<bool>& var, int& nFlags, int flag, bool bCopyToUI, IVariable* pVar = NULL)
	{
		if (bCopyToUI)
		{
			var = (nFlags & flag) != 0;
		}
		else
		{
			if (!pVar || var.GetVar() == pVar)
				nFlags = (var) ? (nFlags | flag) : (nFlags & (~flag));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
	{
		if (varName)
			var.SetName(varName);
		var.SetDataType(dataType);
		varArray.AddVariable(&var);
	}
	//////////////////////////////////////////////////////////////////////////
	void AddVariable(CVarBlock* vars, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
	{
		if (varName)
			var.SetName(varName);
		var.SetDataType(dataType);
		vars->AddVariable(&var);
	}
	void UpdateVarBlock(CVarBlock* vars);

	//////////////////////////////////////////////////////////////////////////
	CPropertyCtrl* GetPropertyCtrl() { return m_pWndProps.get(); }

protected:
	void         OnPropertyChanged(IVariable* pVar);

	virtual void OnOK()     {}
	virtual void OnCancel() {}

	//{{AFX_VIRTUAL(CPropertiesPanel)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CPropertiesPanel)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	std::unique_ptr<CPropertyCtrl> m_pWndProps;
	XmlNodeRef                   m_template;
	bool                         m_multiSelect;
	_smart_ptr<CVarBlock>        m_varBlock;

	std::list<UpdateCallback>    m_updateCallbacks;
	int                          m_titleAdjust;
};
