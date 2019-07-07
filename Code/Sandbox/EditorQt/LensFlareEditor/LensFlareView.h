// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ILensFlareListener.h"
#include "Controls/PreviewModelCtrl.h"

struct IVariable;

class CLensFlareView : public CPreviewModelCtrl, public ILensFlareChangeItemListener, public ILensFlareChangeElementListener
{
public:

	CLensFlareView();
	virtual ~CLensFlareView();

	void OnInternalVariableChange(IVariable* pVar);
	void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:

	void ProcessKeys() {}
	void RenderEffect(IMaterial* pMaterial, const SRenderingPassInfo& passInfo);

	void OnLensFlareChangeItem(CLensFlareItem* pLensFlareItem);
	void OnLensFlareDeleteItem(CLensFlareItem* pLensFlareItem);
	void OnLensFlareChangeElement(CLensFlareElement* pLensFlareElement);

	DECLARE_MESSAGE_MAP()

	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);

private:

	CPoint                     m_prevPoint;
	Vec3                       m_InitCameraPos;

	_smart_ptr<CMaterial>      m_pLensFlareMaterial;
	_smart_ptr<CLensFlareItem> m_pLensFlareItem;
};
