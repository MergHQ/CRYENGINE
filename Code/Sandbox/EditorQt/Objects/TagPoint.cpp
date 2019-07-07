// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TagPoint.h"

#include <Gizmos/AxisHelper.h>
#include <Viewport.h>

#include <CryAISystem/IAgent.h>
#include <CryAISystem/IAISystem.h>

REGISTER_CLASS_DESC(CTagPointClassDesc);
REGISTER_CLASS_DESC(CNavigationSeedPointClassDesc);

IMPLEMENT_DYNCREATE(CTagPoint, CEntityObject)
IMPLEMENT_DYNCREATE(CNavigationSeedPoint, CTagPoint)

STagPointPreferences tagPointPreferences;
REGISTER_PREFERENCES_PAGE_PTR(STagPointPreferences, &tagPointPreferences)

CTagPoint::CTagPoint()
{
	m_entityClass = "TagPoint";
}

bool CTagPoint::Init(CBaseObject* prev, const string& file)
{
	SetColor(ColorB(0, 0, 255));
	SetTextureIcon(GetClassDesc()->GetTextureIconId());

	bool res = CEntityObject::Init(prev, file);

	UseMaterialLayersMask(false);

	return res;
}

void CTagPoint::InitVariables()
{
}

float CTagPoint::GetRadius()
{
	return GetHelperScale() * gGizmoPreferences.helperScale * tagPointPreferences.tagpointScaleMulti;
}

int CTagPoint::MouseCreateCallback(IDisplayViewport* view, EMouseEvent event, CPoint& point, int flags)
{
	if (event == eMouseMove || event == eMouseLDown)
	{
		Vec3 pos = ((CViewport*)view)->MapViewToCP(point, CLevelEditorSharedState::Axis::None, true, GetCreationOffsetFromTerrain());

		SetPos(pos);
		if (event == eMouseLDown)
		{
			return MOUSECREATE_OK;
		}
		return MOUSECREATE_CONTINUE;
	}
	return CBaseObject::MouseCreateCallback(view, event, point, flags);
}

void CTagPoint::Display(CObjectRenderHelper& objRenderHelper)
{
	SDisplayContext& dc = objRenderHelper.GetDisplayContextRef();

	const Matrix34& wtm = GetWorldTM();

	float fHelperScale = 1 * m_helperScale * gGizmoPreferences.helperScale;
	Vec3 dir = wtm.TransformVector(Vec3(0, fHelperScale, 0));
	Vec3 wp = wtm.GetTranslation();
	if (IsFrozen())
		dc.SetFreezeColor();
	else
		dc.SetColor(1, 1, 0);
	dc.DrawArrow(wp, wp + dir * 2, fHelperScale);

	if (IsFrozen())
	{
		dc.SetFreezeColor();
	}
	else
	{
		ColorB color = GetColor();
		color.a = 204;
		dc.SetColor(color);
	}

	dc.DrawBall(wp, GetRadius());

	if (IsSelected())
	{
		dc.SetSelectedColor(0.6f);
		dc.DrawBall(wp, GetRadius() + 0.02f);
	}

	DrawDefault(dc);
}

bool CTagPoint::HitTest(HitContext& hc)
{
	// Must use icon..

	Vec3 origin = GetWorldPos();
	float radius = GetRadius();

	Vec3 w = origin - hc.raySrc;
	Vec3 wcross = hc.rayDir.Cross(w);
	float d = wcross.GetLengthSquared();

	if (d < radius * radius + hc.distanceTolerance &&
	    w.GetLengthSquared() > radius * radius)           // Check if we inside TagPoint, then do not select!
	{
		Vec3 i0;
		hc.object = this;
		if (Intersect::Ray_SphereFirst(Ray(hc.raySrc, hc.rayDir), Sphere(origin, radius), i0))
		{
			hc.dist = hc.raySrc.GetDistance(i0);
			return true;
		}
		hc.dist = hc.raySrc.GetDistance(origin);
		return true;
	}

	return false;
}

void CTagPoint::GetBoundBox(AABB& box)
{
	Vec3 pos = GetWorldPos();
	float r = GetRadius();
	box.min = pos - Vec3(r, r, r);
	box.max = pos + Vec3(r, r, r);
}

void CTagPoint::GetLocalBounds(AABB& box)
{
	float r = GetRadius();
	box.min = -Vec3(r, r, r);
	box.max = Vec3(r, r, r);
}
