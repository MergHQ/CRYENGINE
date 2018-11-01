// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Listener.h"
#include "Common.h"
#include "System.h"
#include "Object.h"
#include "ListenerRequestData.h"
#include "Common/IListener.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void CListener::SetTransformation(CTransformation const& transformation, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SListenerRequestData<EListenerRequestType::SetTransformation> requestData(transformation, this);
	CRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	g_system.PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CListener::Update(float const deltaTime)
{
	m_pImplData->Update(deltaTime);
}

//////////////////////////////////////////////////////////////////////////
void CListener::HandleSetTransformation(CTransformation const& transformation)
{
	m_pImplData->SetTransformation(transformation);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_transformation = transformation;
	g_previewObject.HandleSetTransformation(transformation);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
CTransformation const& CListener::GetTransformation() const
{
	return m_pImplData->GetTransformation();
}

//////////////////////////////////////////////////////////////////////////
void CListener::SetName(char const* const szName, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SListenerRequestData<EListenerRequestType::SetName> requestData(szName, this);
	CRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	g_system.PushRequest(request);
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CListener::HandleSetName(char const* const szName)
{
	m_name = szName;
	m_pImplData->SetName(m_name);
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
