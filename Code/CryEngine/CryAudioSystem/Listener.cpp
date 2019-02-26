// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Listener.h"
#include "Common.h"
#include "System.h"
#include "ListenerRequestData.h"
#include "Common/IListener.h"

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	#include "GlobalObject.h"
#endif // CRY_AUDIO_USE_DEBUG_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void CListener::SetTransformation(CTransformation const& transformation, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SListenerRequestData<EListenerRequestType::SetTransformation> requestData(transformation, this);
	CRequest const request(&requestData, userData);
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

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	m_transformation = transformation;
	g_previewObject.HandleSetTransformation(transformation);
#endif // CRY_AUDIO_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
CTransformation const& CListener::GetTransformation() const
{
	return m_pImplData->GetTransformation();
}

//////////////////////////////////////////////////////////////////////////
void CListener::SetName(char const* const szName, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	SListenerRequestData<EListenerRequestType::SetName> requestData(szName, this);
	CRequest const request(&requestData, userData);
	g_system.PushRequest(request);
#endif // CRY_AUDIO_USE_DEBUG_CODE
}

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
//////////////////////////////////////////////////////////////////////////
void CListener::HandleSetName(char const* const szName)
{
	m_name = szName;
	m_pImplData->SetName(m_name);
}
#endif // CRY_AUDIO_USE_DEBUG_CODE
}      // namespace CryAudio
