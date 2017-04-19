// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::Null;

struct SNullAudioEvent final : IAudioEvent
{
	virtual ERequestStatus Stop() override { return ERequestStatus::Success; }
};

struct SNullAudioListener final : IAudioListener
{
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override { return ERequestStatus::Success; }
};

struct SNullAudioTrigger final : IAudioTrigger
{
	virtual ERequestStatus Load() const override                                       { return ERequestStatus::Success; }
	virtual ERequestStatus Unload() const override                                     { return ERequestStatus::Success; }
	virtual ERequestStatus LoadAsync(IAudioEvent* const pIAudioEvent) const override   { return ERequestStatus::Success; }
	virtual ERequestStatus UnloadAsync(IAudioEvent* const pIAudioEvent) const override { return ERequestStatus::Success; }
};

struct SNullAudioObject final : IAudioObject
{
	virtual ERequestStatus Update() override                                                                                   { return ERequestStatus::Success; }
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override                                     { return ERequestStatus::Success; }
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) override      { return ERequestStatus::Success; }
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioParameter, float const value) override                  { return ERequestStatus::Success; }
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) override                          { return ERequestStatus::Success; }
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) override                    { return ERequestStatus::Success; }
	virtual ERequestStatus ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent) override { return ERequestStatus::Success; }
	virtual ERequestStatus StopAllTriggers() override                                                                          { return ERequestStatus::Success; }
	virtual ERequestStatus PlayFile(IAudioStandaloneFile* const pIFile) override                                               { return ERequestStatus::Success; }
	virtual ERequestStatus StopFile(IAudioStandaloneFile* const pIFile) override                                               { return ERequestStatus::Success; }
	virtual ERequestStatus SetName(char const* const szName) override                                                          { return ERequestStatus::Success; }
};

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::Update(float const deltaTime)
{
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnBeforeShutDown()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ShutDown()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Release()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnLoseFocus()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnGetFocus()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::MuteAll()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnmuteAll()
{
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::StopAllSounds()
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::RegisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnregisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry)
{
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ParseAudioFileEntry(XmlNodeRef const pAudioFileEntryNode, SAudioFileEntryInfo* const pFileEntryInfo)
{
	pFileEntryInfo->memoryBlockAlignment = 0;
	pFileEntryInfo->size = 0;
	pFileEntryInfo->bLocalized = false;
	pFileEntryInfo->pFileData = nullptr;
	pFileEntryInfo->pImplData = nullptr;
	pFileEntryInfo->szFileName = nullptr;
	return ERequestStatus::Failure; // This is the correct behavior: the NULL implementation does not recognize any EntryNodes.
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioFileEntry(IAudioFileEntry* const pOldAudioFileEntry)
{
}

//////////////////////////////////////////////////////////////////////////
char const* const CAudioImpl::GetAudioFileLocation(SAudioFileEntryInfo* const pFileEntryInfo)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructGlobalAudioObject()
{
	return new SNullAudioObject();
}

///////////////////////////////////////////////////////////////////////////
IAudioObject* CAudioImpl::ConstructAudioObject(char const* const szAudioObjectName)
{
	return new SNullAudioObject();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioObject(IAudioObject const* const pAudioObject)
{
	delete pAudioObject;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioListener(IAudioListener* const pListener)
{
	delete pListener;
}

///////////////////////////////////////////////////////////////////////////
IAudioListener* CAudioImpl::ConstructAudioListener()
{
	return new SNullAudioListener();
}

///////////////////////////////////////////////////////////////////////////
IAudioEvent* CAudioImpl::ConstructAudioEvent(CATLEvent& audioEvent)
{
	return new SNullAudioEvent();
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioEvent(IAudioEvent const* const pEvent)
{
	delete pEvent;
}

//////////////////////////////////////////////////////////////////////////
IAudioStandaloneFile* CAudioImpl::ConstructAudioStandaloneFile(CATLStandaloneFile& atlStandaloneFile, char const* const szFile, bool const bLocalized, IAudioTrigger const* pTrigger /*= nullptr*/)
{
	return new IAudioStandaloneFile();
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::DestructAudioStandaloneFile(IAudioStandaloneFile const* const pAudioStandaloneFile)
{
	delete pAudioStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadConnected(TAudioGamepadUniqueID const deviceUniqueID)
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GamepadDisconnected(TAudioGamepadUniqueID const deviceUniqueID)
{
}

///////////////////////////////////////////////////////////////////////////
IAudioTrigger const* CAudioImpl::NewAudioTrigger(XmlNodeRef const pAudioTriggerNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioTrigger(IAudioTrigger const* const pOldTriggerImpl)
{
}

///////////////////////////////////////////////////////////////////////////
IParameter const* CAudioImpl::NewAudioParameter(XmlNodeRef const pAudioParameterNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioParameter(IParameter const* const pParameter)
{
}

///////////////////////////////////////////////////////////////////////////
IAudioSwitchState const* CAudioImpl::NewAudioSwitchState(XmlNodeRef const pAudioSwitchNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioSwitchState(IAudioSwitchState const* const pOldAudioSwitchStateImplData)
{
}

///////////////////////////////////////////////////////////////////////////
IAudioEnvironment const* CAudioImpl::NewAudioEnvironment(XmlNodeRef const pAudioEnvironmentNode)
{
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::DeleteAudioEnvironment(IAudioEnvironment const* const pOldEnvironmentImpl)
{
}

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const
{
	ZeroStruct(memoryInfo);
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::OnAudioSystemRefresh()
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::SetLanguage(char const* const szLanguage)
{
}

//////////////////////////////////////////////////////////////////////////
void CAudioImpl::GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const
{
}
