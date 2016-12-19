// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <IAudioImpl.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CAudioImpl final : public IAudioImpl
{
public:

	CAudioImpl();
	virtual ~CAudioImpl();

	// IAudioImpl
	virtual void                     Update(float const deltaTime) override;
	virtual ERequestStatus           Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize) override;
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

	// Below data is only used when INCLUDE_FMOD_IMPL_PRODUCTION_CODE is defined!
	virtual char const* const GetImplementationNameString() const override;
	virtual void              GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const override;
	virtual void              GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const override;
	// ~IAudioImpl

private:

	static char const* const s_szFmodEventTag;
	static char const* const s_szFmodSnapshotTag;
	static char const* const s_szFmodEventParameterTag;
	static char const* const s_szFmodSnapshotParameterTag;
	static char const* const s_szFmodFileTag;
	static char const* const s_szFmodBusTag;
	static char const* const s_szFmodNameAttribute;
	static char const* const s_szFmodValueAttribute;
	static char const* const s_szFmodMutiplierAttribute;
	static char const* const s_szFmodShiftAttribute;
	static char const* const s_szFmodPathAttribute;
	static char const* const s_szFmodLocalizedAttribute;
	static char const* const s_szFmodEventTypeAttribute;
	static char const* const s_szFmodEventPrefix;
	static char const* const s_szFmodSnapshotPrefix;
	static char const* const s_szFmodBusPrefix;

	void           CreateVersionString(CryFixedStringT<MaxMiscStringLength>& stringOut) const;
	bool           LoadMasterBanks();
	void           UnloadMasterBanks();
	ERequestStatus MuteMasterBus(bool const bMute);

	AudioObjects                          m_constructedAudioObjects;

	CryFixedStringT<MaxFilePathLength>    m_regularSoundBankFolder;
	CryFixedStringT<MaxFilePathLength>    m_localizedSoundBankFolder;

	FMOD::Studio::System*                 m_pSystem;
	FMOD::System*                         m_pLowLevelSystem;
	FMOD::Studio::Bank*                   m_pMasterBank;
	FMOD::Studio::Bank*                   m_pStringsBank;
	CryFixedStringT<MaxControlNameLength> m_language;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxMiscStringLength> m_fullImplString;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};
}
}
}
