// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include <LevelEditor/Tools/EditTool.h>

struct IGeomCacheRenderNode;

class CLinkTool : public CEditTool
{
public:
	DECLARE_DYNAMIC(CLinkTool)

	CLinkTool();

	// Overrides from CEditTool
	virtual string GetDisplayName() const override { return "Link Objects"; }
	bool           MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags);

	virtual void   Display(SDisplayContext& dc);
	virtual bool   OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
	virtual bool   OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return false; }

	virtual void   DrawObjectHelpers(CBaseObject* pObject, SDisplayContext& dc) override;
	virtual bool   HitTest(CBaseObject* pObject, HitContext& hc) override;

	static void    PickObject();

protected:
	void DeleteThis() { delete this; }

private:
	CBaseObject*          m_pChild;
	Vec3                  m_StartDrag;
	Vec3                  m_EndDrag;

	HCURSOR               m_hLinkCursor;
	HCURSOR               m_hLinkNowCursor;
	HCURSOR               m_hCurrCursor;

	const char*           m_nodeName;
	uint                  m_hitNodeIndex;
	IGeomCacheRenderNode* m_pGeomCacheRenderNode;
};
