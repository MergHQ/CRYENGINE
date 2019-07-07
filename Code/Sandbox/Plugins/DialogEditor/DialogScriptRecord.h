// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DialogManager.h"

class CDialogScriptRecord : public CXTPReportRecord
{
public:
	CDialogScriptRecord();
	CDialogScriptRecord(CEditorDialogScript* pScript, const CEditorDialogScript::SScriptLine* pLine);
	const CEditorDialogScript::SScriptLine* GetLine() const { return &m_line; }
	void                                    Swap(CDialogScriptRecord* pOther);

protected:
	void OnBrowseAudioTrigger(string& value, CDialogScriptRecord* pRecord);

	void OnBrowseFacial(string& value, CDialogScriptRecord* pRecord);
	void OnBrowseAG(string& value, CDialogScriptRecord* pRecord);
	void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	void FillItems();

protected:
	CEditorDialogScript*             m_pScript;
	CEditorDialogScript::SScriptLine m_line;
};
