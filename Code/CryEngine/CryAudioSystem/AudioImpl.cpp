// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioImpl.h"

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::Null;

struct SNullAudioEvent final : IAudioEvent
{
	virtual ERequestStatus Stop() override { return eRequestStatus_Success; }
};

struct SNullAudioListener final : IAudioListener
{
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override { return eRequestStatus_Success; }
};

struct SNullAudioTrigger final : IAudioTrigger
{
	virtual ERequestStatus Load() const override                                       { return eRequestStatus_Success; }
	virtual ERequestStatus Unload() const override                                     { return eRequestStatus_Success; }
	virtual ERequestStatus LoadAsync(IAudioEvent* const pIAudioEvent) const override   { return eRequestStatus_Success; }
	virtual ERequestStatus UnloadAsync(IAudioEvent* const pIAudioEvent) const override { return eRequestStatus_Success; }
};

struct SNullAudioObject final : IAudioObject
{
	virtual ERequestStatus Update() override                                                                                   { return eRequestStatus_Success; }
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override                                     { return eRequestStatus_Success; }
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) override      { return eRequestStatus_Success; }
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioParameter, float const value) override                  { return eRequestStatus_Success; }
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) override                          { return eRequestStatus_Success; }
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) override                    { return eRequestStatus_Success; }
	virtual ERequestStatus ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent) override { return eRequestStatus_Success; }
	virtual ERequestStatus StopAllTriggers() override                                                                          { return eRequestStatus_Success; }
	virtual ERequestStatus PlayFile(IAudioStandaloneFile* const pIFile) override                                               { return eRequestStatus_Success; }
	virtual ERequestStatus StopFile(IAudioStandaloneFile* const pIFile) override                                               { return eRequestStatus_Success; }
	virtual ERequestStatus SetName(char const* const szName) override                                                          { return eRequestStatus_Success; }
};

///////////////////////////////////////////////////////////////////////////
void CAudioImpl::Update(float const deltaTime)
{
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnBeforeShutDown()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::ShutDown()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::Release()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnLoseFocus()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::OnGetFocus()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::MuteAll()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnmuteAll()
{
	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::StopAllSounds()
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::RegisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry)
{
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioImpl::UnregisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry)
{
	return eRequestStatus_Success;
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
	return eRequestStatus_Failure; // This is the correct behavior: the NULL implementation does not recognize any EntryNodes.
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
