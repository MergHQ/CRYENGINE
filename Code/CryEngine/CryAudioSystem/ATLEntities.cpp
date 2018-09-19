// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLEntities.h"
#include "Managers.h"
#include "AudioEventManager.h"
#include "AudioStandaloneFileManager.h"
#include "AudioSystem.h"
#include "ATLAudioObject.h"
#include "Common/IAudioImpl.h"
#include "Common/Logger.h"

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
			CRY_ASSERT_MESSAGE(pControl->GetDataScope() == EDataScope::Global, "Default controls must always have global data scope! (%s)", pControl->GetName());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pAudioObject = g_pObject;
			pEvent->SetDataScope(pControl->GetDataScope());
			pEvent->SetTriggerId(pControl->GetId());
			pEvent->m_audioTriggerImplId = pConnection->m_audioTriggerImplId;
			pEvent->m_audioTriggerInstanceId = s_triggerInstanceIdCounter;

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
	SListenerRequestData<EListenerRequestType::SetTransformation> requestData(transformation, this);
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
	m_pImplData->Update(deltaTime);
}

//////////////////////////////////////////////////////////////////////////
void CATLListener::HandleSetTransformation(CObjectTransformation const& transformation)
{
	m_pImplData->SetTransformation(transformation);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_transformation = transformation;
	g_previewObject.HandleSetTransformation(transformation);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CATLListener::SetName(char const* const szName, SRequestUserData const& userData /*= SRequestUserData::GetEmptyObject()*/)
{
	SListenerRequestData<EListenerRequestType::SetName> requestData(szName, this);
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
	m_pImplData->SetName(m_name);
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
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction of CParameterImpl");
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
CSwitchStateImpl::~CSwitchStateImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction of CSwitchStateImpl");
	g_pIImpl->DestructSwitchState(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
void CSwitchStateImpl::Set(CATLAudioObject const& audioObject) const
{
	audioObject.GetImplDataPtr()->SetSwitchState(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
CATLSwitchState::~CATLSwitchState()
{
	for (auto const pStateImpl : m_connections)
	{
		delete pStateImpl;
	}
}

//////////////////////////////////////////////////////////////////////////
void CATLSwitchState::Set(CATLAudioObject const& object) const
{
	for (auto const pSwitchStateImpl : m_connections)
	{
		pSwitchStateImpl->Set(object);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	const_cast<CATLAudioObject&>(object).StoreSwitchValue(m_switchId, m_switchStateId);
#endif   // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
CATLSwitch::~CATLSwitch()
{
	for (auto const& statePair : m_states)
	{
		delete statePair.second;
	}
}

//////////////////////////////////////////////////////////////////////////
CATLTriggerImpl::~CATLTriggerImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction of CATLTriggerImpl");
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
	object.UpdateOcclusion();

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
			pEvent->SetTriggerRadius(m_radius);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pAudioObject = &object;
			pEvent->SetTriggerId(GetId());
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
	object.UpdateOcclusion();

	for (auto const pConnection : m_connections)
	{
		CATLEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = pConnection->Execute(object.GetImplDataPtr(), pEvent->m_pImplData);

		if (activateResult == ERequestStatus::Success || activateResult == ERequestStatus::Pending)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerName(GetName());
			pEvent->SetTriggerRadius(m_radius);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pAudioObject = &object;
			pEvent->SetTriggerId(GetId());
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
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerRadius(m_radius);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pAudioObject = &object;
			pEvent->SetTriggerId(GetId());
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
				pFile->m_state = EStandaloneFileState::Playing;
			}
			else if (status == ERequestStatus::Pending)
			{
				pFile->m_state = EStandaloneFileState::Loading;
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
				pFile->m_state = EStandaloneFileState::Playing;
			}
			else if (status == ERequestStatus::Pending)
			{
				pFile->m_state = EStandaloneFileState::Loading;
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
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during LoseFocusTrigger destruction!");
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
void CLoseFocusTrigger::AddConnections(TriggerConnections const& connections)
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during CLoseFocusTrigger::AddConnections!");
	m_connections = connections;
}

//////////////////////////////////////////////////////////////////////////
void CLoseFocusTrigger::Clear()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}

	m_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
CGetFocusTrigger::~CGetFocusTrigger()
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during GetFocusTrigger destruction!");
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
void CGetFocusTrigger::AddConnections(TriggerConnections const& connections)
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during CGetFocusTrigger::AddConnections!");
	m_connections = connections;
}

//////////////////////////////////////////////////////////////////////////
void CGetFocusTrigger::Clear()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}

	m_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
CMuteAllTrigger::~CMuteAllTrigger()
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during MuteAllTrigger destruction!");
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

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	g_systemStates |= ESystemStates::IsMuted;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CMuteAllTrigger::AddConnections(TriggerConnections const& connections)
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during CMuteAllTrigger::AddConnections!");
	m_connections = connections;
}

//////////////////////////////////////////////////////////////////////////
void CMuteAllTrigger::Clear()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}

	m_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
CUnmuteAllTrigger::~CUnmuteAllTrigger()
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during UnmuteAllTrigger destruction!");
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

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	g_systemStates &= ~ESystemStates::IsMuted;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CUnmuteAllTrigger::AddConnections(TriggerConnections const& connections)
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during CUnmuteAllTrigger::AddConnections!");
	m_connections = connections;
}

//////////////////////////////////////////////////////////////////////////
void CUnmuteAllTrigger::Clear()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}

	m_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
CPauseAllTrigger::~CPauseAllTrigger()
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during PauseAllTrigger destruction!");
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
void CPauseAllTrigger::AddConnections(TriggerConnections const& connections)
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during CPauseAllTrigger::AddConnections!");
	m_connections = connections;
}

//////////////////////////////////////////////////////////////////////////
void CPauseAllTrigger::Clear()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}

	m_connections.clear();
}

//////////////////////////////////////////////////////////////////////////
CResumeAllTrigger::~CResumeAllTrigger()
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during ResumeAllTrigger destruction!");
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
void CResumeAllTrigger::AddConnections(TriggerConnections const& connections)
{
	CRY_ASSERT_MESSAGE(m_connections.empty(), "There are still connections during CResumeAllTrigger::AddConnections!");
	m_connections = connections;
}

//////////////////////////////////////////////////////////////////////////
void CResumeAllTrigger::Clear()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}

	m_connections.clear();
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
CPreviewTrigger::CPreviewTrigger()
	: Control(PreviewTriggerId, EDataScope::Global, s_szPreviewTriggerName)
	, m_pConnection(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
CPreviewTrigger::~CPreviewTrigger()
{
	CRY_ASSERT_MESSAGE(m_pConnection == nullptr, "There is still a connection during CPreviewTrigger destruction!");
}

//////////////////////////////////////////////////////////////////////////
void CPreviewTrigger::Execute(Impl::ITriggerInfo const& triggerInfo)
{
	Impl::ITrigger const* const pITrigger = g_pIImpl->ConstructTrigger(&triggerInfo);

	if (pITrigger != nullptr)
	{
		delete m_pConnection;
		m_pConnection = new CATLTriggerImpl(++g_uniqueConnectionId, pITrigger);

		SAudioTriggerInstanceState triggerInstanceState;
		triggerInstanceState.triggerId = GetId();

		CATLEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = m_pConnection->Execute(g_previewObject.GetImplDataPtr(), pEvent->m_pImplData);

		if (activateResult == ERequestStatus::Success || activateResult == ERequestStatus::Pending)
		{
			pEvent->SetTriggerName(GetName());
			pEvent->m_pAudioObject = &g_previewObject;
			pEvent->SetTriggerId(GetId());
			pEvent->m_audioTriggerImplId = m_pConnection->m_audioTriggerImplId;
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

			g_previewObject.AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

			if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				// No TriggerImpl generated an active event.
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s")", GetName(), g_previewObject.m_name.c_str());
			}
		}

		if (triggerInstanceState.numPlayingEvents > 0 || triggerInstanceState.numLoadingEvents > 0)
		{
			triggerInstanceState.flags |= ETriggerStatus::Playing;
			g_previewObject.AddTriggerState(s_triggerInstanceIdCounter++, triggerInstanceState);
		}
		else
		{
			// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
			g_previewObject.SendFinishedTriggerInstanceRequest(triggerInstanceState);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPreviewTrigger::Stop()
{
	for (auto const pEvent : g_previewObject.GetActiveEvents())
	{
		CRY_ASSERT_MESSAGE((pEvent != nullptr) && (pEvent->IsPlaying() || pEvent->IsVirtual()), "Invalid event during CPreviewTrigger::Stop");
		pEvent->Stop();
	}
}

//////////////////////////////////////////////////////////////////////////
void CPreviewTrigger::Clear()
{
	delete m_pConnection;
	m_pConnection = nullptr;
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
CATLEnvironmentImpl::~CATLEnvironmentImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction of CATLEnvironmentImpl");
	g_pIImpl->DestructEnvironment(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
CSettingImpl::~CSettingImpl()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during destruction of CSettingImpl");
	g_pIImpl->DestructSetting(m_pImplData);
}

//////////////////////////////////////////////////////////////////////////
CSetting::~CSetting()
{
	for (auto const pConnection : m_connections)
	{
		delete pConnection;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSetting::Load() const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->GetImplData()->Load();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSetting::Unload() const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->GetImplData()->Unload();
	}
}
} // namespace CryAudio
