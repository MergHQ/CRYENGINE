// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CameraSource.h"
#include "IViewSystem.h"

//------------------------------------------------------------------------
CCameraSource::CCameraSource()
{
}

//------------------------------------------------------------------------
CCameraSource::~CCameraSource()
{
}

//------------------------------------------------------------------------
bool CCameraSource::Init(IGameObject* pGameObject)
{
	SetGameObject(pGameObject);
	return true;
}

//------------------------------------------------------------------------
void CCameraSource::PostInit(IGameObject* pGameObject)
{
	pGameObject->CaptureView(this);
}

//------------------------------------------------------------------------
void CCameraSource::Release()
{
	GetGameObject()->ReleaseView(this);
	delete this;
}

//------------------------------------------------------------------------
void CCameraSource::Serialize(TSerialize ser, EEntityAspects aspects)
{
}

//------------------------------------------------------------------------
void CCameraSource::Update(SEntityUpdateContext& ctx, int updateSlot)
{
}

//------------------------------------------------------------------------
void CCameraSource::HandleEvent(const SGameObjectEvent& event)
{
}

//------------------------------------------------------------------------
void CCameraSource::ProcessEvent(SEntityEvent& event)
{
}

//------------------------------------------------------------------------
void CCameraSource::SetAuthority(bool auth)
{
}

//------------------------------------------------------------------------
void CCameraSource::UpdateView(SViewParams& params)
{
	// update params
	const Matrix34& mat = GetEntity()->GetWorldTM();
	params.position = mat.GetTranslation();
	params.rotation = Quat(mat);
}
