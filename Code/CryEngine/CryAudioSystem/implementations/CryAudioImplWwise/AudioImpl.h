// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "FileIOHandler.h"
#include "ATLEntities.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CAudioImpl final : public IAudioImpl
{
public:

	CAudioImpl();
	virtual ~CAudioImpl() override = default;
	CAudioImpl(CAudioImpl const&) = delete;
	CAudioImpl(CAudioImpl&&) = delete;
	CAudioImpl& operator=(CAudioImpl const&) = delete;
	CAudioImpl& operator=(CAudioImpl&&) = delete;

	// IAudioImpl
	virtual void                     Update(float const deltaTime) override;
	virtual ERequestStatus           Init(uint32 const audioObjectPoolSize, uint32 const eventPoolSize) override;
	virtual ERequestStatus           OnBeforeShutDown() override;
	virtual ERequestStatus           ShutDown() override;
	virtual ERequestStatus           Release() override;
	virtual ERequestStatus           OnLoseFocus() override;
	virtual ERequestStatus           OnGetFocus() override;
	virtual ERequestStatus           MuteAll() override;
	virtual ERequestStatus           UnmuteAll() override;
	virtual ERequestStatus           StopAllSounds() override;
	virtual void                     GamepadConnected(AudioGamepadUniqueId const deviceUniqueID) override;
	virtual void                     GamepadDisconnected(AudioGamepadUniqueId const deviceUniqueID) override;
	virtual void                     OnAudioSystemRefresh() override;
	virtual void                     SetLanguage(char const* const szLanguage) override;
	virtual ERequestStatus           RegisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry) override;
	virtual ERequestStatus           UnregisterInMemoryFile(SAudioFileEntryInfo* const pAudioFileEntry) override;
	virtual ERequestStatus           ParseAudioFileEntry(XmlNodeRef const pAudioFileEntryNode, SAudioFileEntryInfo* const pFileEntryInfo) override;
	virtual void                     DeleteAudioFileEntry(IAudioFileEntry* const pOldAudioFileEntry) override;
	virtual char const* const        GetAudioFileLocation(SAudioFileEntryInfo* const pFileEntryInfo) override;
	virtual IAudioTrigger const*     NewAudioTrigger(XmlNodeRef const pAudioTriggerNode) override;
	virtual void                     DeleteAudioTrigger(IAudioTrigger const* const pOldAudioTrigger) override;
	virtual IParameter const*        NewAudioParameter(XmlNodeRef const pParameterNode) override;
	virtual void                     DeleteAudioParameter(IParameter const* const pParameter) override;
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

	// Below data is only used when INCLUDE_WWISE_IMPL_PRODUCTION_CODE is defined!
	virtual char const* const GetImplementationNameString() const override;
	virtual void              GetMemoryInfo(SAudioImplMemoryInfo& memoryInfo) const override;
	virtual void              GetAudioFileData(char const* const szFilename, SFileData& audioFileData) const override;
	// ~IAudioImpl

private:

	static char const* const s_szWwiseEventTag;
	static char const* const s_szWwiseRtpcTag;
	static char const* const s_szWwiseSwitchTag;
	static char const* const s_szWwiseStateTag;
	static char const* const s_szWwiseRtpcSwitchTag;
	static char const* const s_szWwiseFileTag;
	static char const* const s_szWwiseAuxBusTag;
	static char const* const s_szWwiseValueTag;
	static char const* const s_szWwiseNameAttribute;
	static char const* const s_szWwiseValueAttribute;
	static char const* const s_szWwiseMutiplierAttribute;
	static char const* const s_szWwiseShiftAttribute;
	static char const* const s_szWwiseLocalisedAttribute;

	bool                     ParseSwitchOrState(XmlNodeRef const pNode, AkUInt32& outStateOrSwitchGroupId, AkUInt32& outStateOrSwitchId);
	SAudioSwitchState const* ParseWwiseRtpcSwitch(XmlNodeRef pNode);
	void                     ParseRtpcImpl(XmlNodeRef const pNode, AkRtpcID& rtpcId, float& multiplier, float& shift);
	void                     SignalAuxAudioThread();
	void                     WaitForAuxAudioThread();

	AkBankID                           m_initBankId;
	CFileIOHandler                     m_fileIOHandler;

	CryFixedStringT<MaxFilePathLength> m_regularSoundBankFolder;
	CryFixedStringT<MaxFilePathLength> m_localizedSoundBankFolder;

	typedef std::map<AudioGamepadUniqueId, AkUInt8> AudioInputDevices;
	AudioInputDevices m_mapInputDevices;

#if !defined(WWISE_FOR_RELEASE)
	bool m_bCommSystemInitialized;
#endif  // !WWISE_FOR_RELEASE

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	CryFixedStringT<MaxFilePathLength> m_fullImplString;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

#if defined(WWISE_USE_OCULUS)
	void* m_pOculusSpatializerLibrary;
#endif  // WWISE_USE_OCULUS
};
}
}
}
