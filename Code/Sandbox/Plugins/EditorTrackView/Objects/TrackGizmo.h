// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Gizmos/Gizmo.h"

class CTrackViewAnimNode;
class CTrackViewTrack;

// Gizmo of Objects animation track.
class CTrackGizmo : public CGizmo
{
public:
	CTrackGizmo();

	// Overrides from CGizmo
	virtual void GetWorldBounds(AABB& bbox);
	virtual void Display(SDisplayContext& dc);
	virtual bool HitTest(HitContext& hc);
	// TODO implement this
	virtual bool MouseCallback(IDisplayViewport* view, EMouseEvent event, CPoint& point, int nFlags) { return false; }

	void         SetMatrix(const Matrix34& tm);
	void         SetAnimNode(CTrackViewAnimNode* pNode);
	void         DrawAxis(SDisplayContext& dc, const Vec3& pos);
	void         DrawKeys(SDisplayContext& dc, CTrackViewTrack* pTrack, CTrackViewTrack* pKeysTrack);

private:
	mutable Matrix34    m_matrix;
	CTrackViewAnimNode* m_pAnimNode;
	AABB                m_worldBbox;
};
