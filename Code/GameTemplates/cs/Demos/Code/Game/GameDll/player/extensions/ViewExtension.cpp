// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#include "StdAfx.h"
#include "ViewExtension.h"
#include <IViewSystem.h>


CViewExtension::CViewExtension()
	: m_viewId(0)
	, m_camFOV(0.0f)
{
}

CViewExtension::~CViewExtension()
{
}

void CViewExtension::Release()
{
	GetGameObject()->ReleaseView(this);

	gEnv->pGame->GetIGameFramework()->GetIViewSystem()->RemoveView(m_viewId);
	gEnv->pConsole->UnregisterVariable("gamezero_cam_fov", true);

	ISimpleExtension::Release();
}

void CViewExtension::PostInit(IGameObject* pGameObject)
{
	CreateView();
	pGameObject->CaptureView(this);

	REGISTER_CVAR2("gamezero_cam_fov", &m_camFOV, 60.0f, VF_NULL, "Camera FOV.");
}

void CViewExtension::CreateView()
{
	IViewSystem* pViewSystem = gEnv->pGame->GetIGameFramework()->GetIViewSystem();
	IView* pView = pViewSystem->CreateView();

	pView->LinkTo(GetGameObject());
	pViewSystem->SetActiveView(pView);

	m_viewId = pViewSystem->GetViewId(pView);
}

void CViewExtension::UpdateView(SViewParams& params)
{
	params.position = GetEntity()->GetWorldPos();
	params.rotation = GetEntity()->GetWorldRotation();
	params.fov = DEG2RAD(m_camFOV);
}
