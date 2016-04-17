// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   CameraProxy.h
//  Version:     v1.00
//  Created:     5/12/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CameraProxy.h"
#include <CryNetwork/ISerialize.h>

//////////////////////////////////////////////////////////////////////////
CCameraProxy::CCameraProxy()
	: m_pEntity(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
void CCameraProxy::Initialize(const SComponentInitializer& init)
{
	m_pEntity = (CEntity*)init.m_pEntity;
	UpdateMaterialCamera();
}

//////////////////////////////////////////////////////////////////////////
void CCameraProxy::Release()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
bool CCameraProxy::Init(IEntity* pEntity, SEntitySpawnParams& params)
{
	UpdateMaterialCamera();

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CCameraProxy::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
	UpdateMaterialCamera();
}

//////////////////////////////////////////////////////////////////////////
void CCameraProxy::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_INIT:
	case ENTITY_EVENT_XFORM:
		{
			UpdateMaterialCamera();
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CCameraProxy::UpdateMaterialCamera()
{
	float fov = m_camera.GetFov();
	m_camera = GetISystem()->GetViewCamera();
	Matrix34 wtm = m_pEntity->GetWorldTM();
	wtm.OrthonormalizeFast();
	m_camera.SetMatrix(wtm);
	m_camera.SetFrustum(m_camera.GetViewSurfaceX(), m_camera.GetViewSurfaceZ(), fov, m_camera.GetNearPlane(), m_camera.GetFarPlane(), m_camera.GetPixelAspectRatio());

	IMaterial* pMaterial = m_pEntity->GetMaterial();
	if (pMaterial)
		pMaterial->SetCamera(m_camera);
}

//////////////////////////////////////////////////////////////////////////
void CCameraProxy::SetCamera(CCamera& cam)
{
	m_camera = cam;
	UpdateMaterialCamera();
}

//////////////////////////////////////////////////////////////////////////
bool CCameraProxy::GetSignature(TSerialize signature)
{
	signature.BeginGroup("CameraProxy");
	signature.EndGroup();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CCameraProxy::Serialize(TSerialize ser)
{

}
