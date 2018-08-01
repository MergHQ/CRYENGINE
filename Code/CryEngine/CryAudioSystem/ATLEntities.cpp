// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLEntities.h"
#include "AudioSystem.h"
#include "ATLAudioObject.h"
#include "Common/IAudioImpl.h"
#include "Common/Logger.h"
#include "Common.h"
#include "AudioEventManager.h"
#include "AudioStandaloneFileManager.h"

namespace CryAudio
{
#if CRY_PLATFORM_WINDOWS
char const* const SATLXMLTags::szPlatform = "pc";
#elif CRY_PLATFORM_DURANGO
char const* const SATLXMLTags::szPlatform = "xboxone";
#elif CRY_PLATFORM_ORBIS
char const* const SATLXMLTags::szPlatform = "ps4";
#elif CRY_PLATFORM_MAC
char const* const SATLXMLTags::szPlatform = "mac";
#elif CRY_PLATFORM_LINUX
char const* const SATLXMLTags::szPlatform = "linux";
#elif CRY_PLATFORM_IOS
char const* const SATLXMLTags::szPlatform = "ios";
#elif CRY_PLATFORM_ANDROID
char const* const SATLXMLTags::szPlatform = "linux";
#else
	#error "Undefined platform."
#endif

static TriggerInstanceId s_triggerInstanceIdCounter = 0;

//////////////////////////////////////////////////////////////////////////
void ExecuteDefaultTriggerConnections(Control const* const pControl, TriggerConnections const& connections)
{
	SAudioTriggerInstanceState triggerInstanceState;
	triggerInstanceState.triggerId = pControl->GetId();

	for (auto const pConnection : connections)
	{
		CATLEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = pConnection->Execute(g_pObject->GetImplDataPtr(), pEvent->m_pImplData);

		if (activateResult == ERequestStatus::Success || activateResult == ERequestStatus::Pending)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerName(pControl->GetName());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pAudioObject = g_pObject;
			pEvent->SetTriggerId(pControl->GetId());
			pEvent->m_audioTriggerImplId = pConnection->m_audioTriggerImplId;
			pEvent->m_audioTriggerInstanceId = s_triggerInstanceIdCounter;

			CRY_ASSERT_MESSAGE(pControl->GetDataScope() == EDataScope::Global, "Default controls must always have global data scope! (%s)", pControl->GetName());
			pEvent->SetDataScope(pControl->GetDataScope());

			if (activateResult == ERequestStatus::Success)
			{
				pEvent->m_state = EEventState::Playing;
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::Pending)
			{
				pEvent->m_state = EEventState::Loading;
				++(triggerInstanceState.numLoadingEvents);
			}

			g_pObject->AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				// No TriggerImpl generated an active event.
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s")", pControl->GetName(), g_pObject->m_name.c_str());
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

	if (triggerInstanceState.numPlayingEvents > 0 || triggerInstanceState.numLoadingEvents > 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::Playing;
		g_pObject->AddTriggerState(s_triggerInstanceIdCounter++, triggerInstanceState);
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		g_pObject->SendFinishedTriggerInstanceRequest(triggerInstanceState);
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLListener::SetTransformation(CObjectTransformation const& transformation, SRequestUserData const& userData /* = SAudioRequestUserData::GetEmptyObject() */)
{
	SAudioListenerRequestData<EAudioListenerRequestType::SetTransformation> requestData(transformation, this);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	g_system.PushRequest(request);
}

//////////////////////////////////////////////////////////////////////////
void CATLListener::Update(float const deltaTime)
{
	if (m_isMovingOrDecaying)
	{
		Vec3 const deltaPos(m_transformation.GetPosition() - m_previousPositionForVelocityCalculation);

		if (!deltaPos.IsZero())
		{
			m_velocity = deltaPos / deltaTime;
			m_previousPositionForVelocityCalculation = m_transformation.GetPosition();
		}
		else if (!m_velocity.IsZero())
		{
			// We did not move last frame, begin exponential decay towards zero.
			float const decay = std::max(1.0f - deltaTime / 0.05f, 0.0f);
			m_velocity *= decay;

			if (m_velocity.GetLengthSquared() < FloatEpsilon)
			{
				m_velocity = ZERO;
				m_isMovingOrDecaying = false;
			}
		}

		// TODO: propagate listener velocity down to the middleware.
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLListener::HandleSetTransformation(CObjectTransformation const& transformation)
{
	m_transformation = transformation;
	m_isMovingOrDecaying = true;

	// Immediately propagate the new transformation down to the middleware, calculation of velocity can be safely delayed to next audio frame.
	m_pImplData->SetTransformation(m_transformation);
}

//////////////////////////////////////////////////////////////////////////
void CATLListener::SetName(char const* const szName, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SAudioListenerRequestData<EAudioListenerRequestType::SetName> requestData(szName, this);
	CAudioRequest request(&requestData);
	request.flags = userData.flags;
	request.pOwner = userData.pOwner;
	request.pUserData = userData.pUserData;
	request.pUserDataOwner = userData.pUserDataOwner;
	g_system.PushRequest(request);
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CATLListener::HandleSetName(char const* const szName)
{
	m_name = szName;
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CATLEvent::Stop()
{
	m_pImplData->Stop();
}

//////////////////////////////////////////////////////////////////////////
void CATLEvent::Release()
{
	m_pImplData = nullptr;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_szTriggerName = nullptr;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CParameterImpl::Set(CATLAudioObject const& object, float const value) const
{
	object.GetImplDataPtr()->SetParameter(m_pImplData, value);
}

//////////////////////////////////////////////////////////////////////////
CParameterImpl::~CParameterImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction");
	g_pIImpl->DestructParameter(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
CParameter::~CParameter()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameter::Set(CATLAudioObject const& object, float const value) const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->Set(object, value);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// Log the "no-connections" case only on user generated controls.
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Parameter "%s" set on object "%s" without connections)", GetName(), object.m_name.c_str());
	}

	const_cast<CATLAudioObject&>(object).StoreParameterValue(GetId(), value);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
CAbsoluteVelocityParameter::~CAbsoluteVelocityParameter()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAbsoluteVelocityParameter::Set(CATLAudioObject const& object, float const value) const
{
	// Log the "no-connections" case only on user generated controls.
	for (auto const pConnection : m_connections)
	{
		pConnection->Set(object, value);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	const_cast<CATLAudioObject&>(object).StoreParameterValue(GetId(), value);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
CRelativeVelocityParameter::~CRelativeVelocityParameter()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CRelativeVelocityParameter::Set(CATLAudioObject const& object, float const value) const
{
	// Log the "no-connections" case only on user generated controls.
	for (auto const pConnection : m_connections)
	{
		pConnection->Set(object, value);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	const_cast<CATLAudioObject&>(object).StoreParameterValue(GetId(), value);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CExternalAudioSwitchStateImpl::Set(CATLAudioObject& audioObject) const
{
	audioObject.GetImplDataPtr()->SetSwitchState(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
CExternalAudioSwitchStateImpl::~CExternalAudioSwitchStateImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction");
	g_pIImpl->DestructSwitchState(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
CATLTriggerImpl::~CATLTriggerImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction");
	g_pIImpl->DestructTrigger(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CATLTriggerImpl::Execute(Impl::IObject* const pImplObject, Impl::IEvent* const pImplEvent) const
{
	return pImplObject->ExecuteTrigger(m_pImplData, pImplEvent);
}

//////////////////////////////////////////////////////////////////////////
CTrigger::~CTrigger()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Execute(
	CATLAudioObject& object,
	void* const pOwner /* = nullptr */,
	void* const pUserData /* = nullptr */,
	void* const pUserDataOwner /* = nullptr */,
	ERequestFlags const flags /* = ERequestFlags::None */) const
{
	SAudioTriggerInstanceState triggerInstanceState;
	triggerInstanceState.triggerId = GetId();
	triggerInstanceState.pOwnerOverride = pOwner;
	triggerInstanceState.pUserData = pUserData;
	triggerInstanceState.pUserDataOwner = pUserDataOwner;

	if ((flags& ERequestFlags::DoneCallbackOnAudioThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnAudioThread;
	}
	else if ((flags& ERequestFlags::DoneCallbackOnExternalThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnExternalThread;
	}

	for (auto const pConnection : m_connections)
	{
		CATLEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = pConnection->Execute(object.GetImplDataPtr(), pEvent->m_pImplData);

		if (activateResult == ERequestStatus::Success || activateResult == ERequestStatus::Pending)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerName(GetName());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pAudioObject = &object;
			pEvent->SetTriggerId(GetId());
			pEvent->SetTriggerRadius(m_radius);
			pEvent->m_audioTriggerImplId = pConnection->m_audioTriggerImplId;
			pEvent->m_audioTriggerInstanceId = s_triggerInstanceIdCounter;
			pEvent->SetDataScope(GetDataScope());

			if (activateResult == ERequestStatus::Success)
			{
				pEvent->m_state = EEventState::Playing;
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::Pending)
			{
				pEvent->m_state = EEventState::Loading;
				++(triggerInstanceState.numLoadingEvents);
			}

			object.AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				// No TriggerImpl generated an active event.
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s")", GetName(), object.m_name.c_str());
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

	if (triggerInstanceState.numPlayingEvents > 0 || triggerInstanceState.numLoadingEvents > 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::Playing;
		object.AddTriggerState(s_triggerInstanceIdCounter++, triggerInstanceState);
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		object.SendFinishedTriggerInstanceRequest(triggerInstanceState);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), object.m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Execute(
	CATLAudioObject& object,
	TriggerInstanceId const triggerInstanceId,
	SAudioTriggerInstanceState& triggerInstanceState) const
{
	for (auto const pConnection : m_connections)
	{
		CATLEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = pConnection->Execute(object.GetImplDataPtr(), pEvent->m_pImplData);

		if (activateResult == ERequestStatus::Success || activateResult == ERequestStatus::Pending)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerName(GetName());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pAudioObject = &object;
			pEvent->SetTriggerId(GetId());
			pEvent->SetTriggerRadius(m_radius);
			pEvent->m_audioTriggerImplId = pConnection->m_audioTriggerImplId;
			pEvent->m_audioTriggerInstanceId = triggerInstanceId;
			pEvent->SetDataScope(GetDataScope());

			if (activateResult == ERequestStatus::Success)
			{
				pEvent->m_state = EEventState::Playing;
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::Pending)
			{
				pEvent->m_state = EEventState::Loading;
				++(triggerInstanceState.numLoadingEvents);
			}

			object.AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				// No TriggerImpl generated an active event.
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s")", GetName(), object.m_name.c_str());
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), object.m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::LoadAsync(CATLAudioObject& object, bool const doLoad) const
{
	// TODO: This needs proper implementation!
	SAudioTriggerInstanceState triggerInstanceState;
	triggerInstanceState.triggerId = GetId();

	for (auto const pConnection : m_connections)
	{
		CATLEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus prepUnprepResult = ERequestStatus::Failure;

		if (doLoad)
		{
			if (((triggerInstanceState.flags & ETriggerStatus::Loaded) == 0) && ((triggerInstanceState.flags & ETriggerStatus::Loading) == 0))
			{
				prepUnprepResult = pConnection->m_pImplData->LoadAsync(pEvent->m_pImplData);
			}
		}
		else
		{
			if (((triggerInstanceState.flags & ETriggerStatus::Loaded) != 0) && ((triggerInstanceState.flags & ETriggerStatus::Unloading) == 0))
			{
				prepUnprepResult = pConnection->m_pImplData->UnloadAsync(pEvent->m_pImplData);
			}
		}

		if (prepUnprepResult == ERequestStatus::Success)
		{
			pEvent->m_pAudioObject = &object;
			pEvent->SetTriggerId(GetId());
			pEvent->SetTriggerRadius(m_radius);
			pEvent->m_audioTriggerImplId = pConnection->m_audioTriggerImplId;
			pEvent->m_audioTriggerInstanceId = s_triggerInstanceIdCounter;
			pEvent->SetDataScope(GetDataScope());
			pEvent->m_state = doLoad ? EEventState::Loading : EEventState::Unloading;

			object.AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, R"(LoadAsync failed on trigger "%s" for object "%s")", GetName(), object.m_name.c_str());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

	object.AddTriggerState(s_triggerInstanceIdCounter++, triggerInstanceState);
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::PlayFile(
	CATLAudioObject& object,
	char const* const szName,
	bool const isLocalized,
	void* const pOwner /* = nullptr */,
	void* const pUserData /* = nullptr */,
	void* const pUserDataOwner /* = nullptr */) const
{
	if (!m_connections.empty())
	{
		Impl::ITrigger const* const pITrigger = m_connections[0]->m_pImplData;
		CATLStandaloneFile* const pFile = g_fileManager.ConstructStandaloneFile(szName, isLocalized, pITrigger);
		ERequestStatus const status = object.GetImplDataPtr()->PlayFile(pFile->m_pImplData);

		if (status == ERequestStatus::Success || status == ERequestStatus::Pending)
		{
			if (status == ERequestStatus::Success)
			{
				pFile->m_state = EAudioStandaloneFileState::Playing;
			}
			else if (status == ERequestStatus::Pending)
			{
				pFile->m_state = EAudioStandaloneFileState::Loading;
			}

			pFile->m_pAudioObject = &object;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pFile->m_triggerId = GetId();
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			object.AddStandaloneFile(pFile, SUserDataBase(pOwner, pUserData, pUserDataOwner));
		}
		else
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, R"(PlayFile failed with "%s" on object "%s")", pFile->m_hashedFilename.GetText().c_str(), object.m_name.c_str());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			g_fileManager.ReleaseStandaloneFile(pFile);
		}
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CTrigger::PlayFile(CATLAudioObject& object, CATLStandaloneFile* const pFile) const
{
	CRY_ASSERT_MESSAGE(pFile->m_pImplData == nullptr, "Standalone file impl data survided a middleware switch!");

	if (!m_connections.empty())
	{
		Impl::ITrigger const* const pITrigger = m_connections[0]->m_pImplData;
		pFile->m_pImplData = g_pIImpl->ConstructStandaloneFile(*pFile, pFile->m_hashedFilename.GetText().c_str(), pFile->m_isLocalized, pITrigger);
		ERequestStatus const status = object.GetImplDataPtr()->PlayFile(pFile->m_pImplData);

		if (status == ERequestStatus::Success || status == ERequestStatus::Pending)
		{
			if (status == ERequestStatus::Success)
			{
				pFile->m_state = EAudioStandaloneFileState::Playing;
			}
			else if (status == ERequestStatus::Pending)
			{
				pFile->m_state = EAudioStandaloneFileState::Loading;
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, R"(PlayFile failed with "%s" on object "%s")", pFile->m_hashedFilename.GetText().c_str(), object.m_name.c_str());

			g_pIImpl->DestructStandaloneFile(pFile->m_pImplData);
			pFile->m_pImplData = nullptr;
		}
	}
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
CLoseFocusTrigger::~CLoseFocusTrigger()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CLoseFocusTrigger::Execute() const
{
	if (!m_connections.empty())
	{
		ExecuteDefaultTriggerConnections(this, m_connections);
	}
	else
	{
		g_pIImpl->OnLoseFocus();
	}
}

//////////////////////////////////////////////////////////////////////////
CGetFocusTrigger::~CGetFocusTrigger()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CGetFocusTrigger::Execute() const
{
	if (!m_connections.empty())
	{
		ExecuteDefaultTriggerConnections(this, m_connections);
	}
	else
	{
		g_pIImpl->OnGetFocus();
	}
}

//////////////////////////////////////////////////////////////////////////
CMuteAllTrigger::~CMuteAllTrigger()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CMuteAllTrigger::Execute() const
{
	if (!m_connections.empty())
	{
		ExecuteDefaultTriggerConnections(this, m_connections);
	}
	else
	{
		g_pIImpl->MuteAll();
	}
}

//////////////////////////////////////////////////////////////////////////
CUnmuteAllTrigger::~CUnmuteAllTrigger()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CUnmuteAllTrigger::Execute() const
{
	if (!m_connections.empty())
	{
		ExecuteDefaultTriggerConnections(this, m_connections);
	}
	else
	{
		g_pIImpl->UnmuteAll();
	}
}

//////////////////////////////////////////////////////////////////////////
CPauseAllTrigger::~CPauseAllTrigger()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPauseAllTrigger::Execute() const
{
	if (!m_connections.empty())
	{
		ExecuteDefaultTriggerConnections(this, m_connections);
	}
	else
	{
		g_pIImpl->PauseAll();
	}
}

//////////////////////////////////////////////////////////////////////////
CResumeAllTrigger::~CResumeAllTrigger()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CResumeAllTrigger::Execute() const
{
	if (!m_connections.empty())
	{
		ExecuteDefaultTriggerConnections(this, m_connections);
	}
	else
	{
		g_pIImpl->ResumeAll();
	}
}

//////////////////////////////////////////////////////////////////////////
CATLEnvironmentImpl::~CATLEnvironmentImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction");
	g_pIImpl->DestructEnvironment(m_pImplData);
}
} // namespace CryAudio
