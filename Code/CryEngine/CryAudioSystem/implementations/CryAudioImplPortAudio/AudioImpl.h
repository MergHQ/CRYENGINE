// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <IAudioImpl.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CAudioObject;

class CAudioImpl final : public IAudioImpl
{
public:

	CAudioImpl() = default;
	virtual ~CAudioImpl() override = default;
	CAudioImpl(CAudioImpl const&) = delete;
	CAudioImpl(CAudioImpl&&) = delete;
	CAudioImpl& operator=(CAudioImpl const&) = delete;
	CAudioImpl& operator=(CAudioImpl&&) = delete;

	// IAudioImpl
	virtual void                     Update(float const deltaTime) override {}
	virtual ERequestStatus           Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize) override;
	virtual ERequestStatus           OnBeforeShutDown() override;
	virtual ERequestStatus           ShutDown() override;
	virtual ERequestStatus           Release() override;
	virtual ERequestStatus           OnLoseFocus() override;
	virtual ERequestStatus           OnGetFocus() override;
	virtual ERequestStatus           MuteAll() override;
	virtual ERequestStatus           UnmuteAll() override;
	virtual ERequestStatus           StopAllSounds() override;
	virtual ERequestStatus           RegisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry) override;
	virtual ERequestStatus           UnregisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry) override;
	virtual ERequestStatus           ParseAudioFileEntry(XmlNodeRef const pAudioFileEntryNode, SAudioFileEntryInfo* const pFileEntryInfo) override;
	virtual void                     DeleteAudioFileEntry(IAudioFileEntry* const pOldAudioFileEntry) override;
	virtual char const* const        GetAudioFileLocation(SAudioFileEntryInfo* const pFileEntryInfo) override;
	virtual IAudioTrigger const*     NewAudioTrigger(XmlNodeRef const pAudioTriggerNode) override;
	virtual void                     DeleteAudioTrigger(IAudioTrigger const* const pOldAudioTrigger) override;
	virtual IParameter const*        NewAudioParameter(XmlNodeRef const pAudioRtpcNode) override;
	virtual void                     DeleteAudioParameter(IParameter const* const pOldAudioRtpc) override;
	virtual IAudioSwitchState const* NewAudioSwitchState(XmlNodeRef const pAudioSwitchStateImplNode) override;
	virtual void                     DeleteAudioSwitchState(IAudioSwitchState const* const pOldAudioSwitchState) override;
	virtual IAudioEnvironment const* NewAudioEnvironment(XmlNodeRef const pAudioEnvironmentNode) override;
	virtual void                     DeleteAudioEnvironment(IAudioEnvironment const* const pOldAudioEnvironment) override;
	virtual IAudioObject*            ConstructGlobalAudioObject() override;
	virtual IAudioObject*            ConstructAudioObject(char const* const szAudioObjectName = "") override;
	virtual void                     DestructAudioObject(IAudioObject const* const pAudioObject) override;
	virtual IAudioListener*          ConstructAudioListener() override;
	virtual void                     DestructAudioListener(IAudioListener* const pListenerData) override;
	virtual IAudioEvent*             ConstructAudioEvent(CATLEvent& audioEvent) override;
	virtual void                     DestructAudioEvent(IAudioEvent const* const pAudioEvent) override;
	virtual IAudioStandaloneFile*    ConstructAudioStandaloneFile(CATLStandaloneFile& atlStandaloneFile, char const* const szFile, bool const bLocalized, IAudioTrigger const* pTrigger = nullptr) override;
	virtual void                     DestructAudioStandaloneFile(IAudioStandaloneFile const* const pAudioStandaloneFile) override;
	virtual void                     GamepadConnected(TAudioGamepadUniqueID const deviceUniqueID) override;
	virtual void                     GamepadDisconnected(TAudioGamepadUniqueID const deviceUniqueID) override;
	virtual void                     OnAudioSystemRefresh() override;
	virtual void                     SetLanguage(char const* const szLanguage) override;

	// Below data is only used when INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE is defined!
	virtual char const* const GetImplementationNameString() const override;
	virtual void              GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const override;
	virtual void              GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const override;
	// ~IAudioImpl

private:

	static char const* const           s_szPortAudioEventTag;
	static char const* const           s_szPortAudioEventNameAttribute;
	static char const* const           s_szPortAudioEventNumLoopsAttribute;
	static char const* const           s_szPortAudioEventTypeAttribute;

	std::vector<CAudioObject*>         m_constructedAudioObjects;

	CryFixedStringT<MaxFilePathLength> m_regularSoundBankFolder;
	CryFixedStringT<MaxFilePathLength> m_localizedSoundBankFolder;

#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxMiscStringLength> m_fullImplString;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
};
}
}
}
