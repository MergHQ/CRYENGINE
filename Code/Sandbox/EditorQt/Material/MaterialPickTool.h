// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <LevelEditor/Tools/EditTool.h>
#include <Cry3DEngine/IStatObj.h>

class CMaterialPickTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CMaterialPickTool)

	CMaterialPickTool();

	virtual string GetDisplayName() const override { return "Pick Material"; }
	virtual bool   MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags);
	virtual void   Display(SDisplayContext& dc);

protected:

	bool OnMouseMove(CViewport* view, UINT nFlags, CPoint point);
	void SetMaterial(IMaterial* pMaterial);

	virtual ~CMaterialPickTool();
	// Delete itself.
	void DeleteThis() { delete this; }

	_smart_ptr<IMaterial> m_pMaterial;
	string                m_displayString;
	CPoint                m_Mouse2DPosition;
	SRayHitInfo           m_HitInfo;
};
