// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioEvent.h"
#include "AudioObject.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CAudioListener final : public IAudioListener
{
public:

	explicit CAudioListener(int const _id)
		: m_id(_id)
	{
		ZeroStruct(m_attributes);
	}

	virtual ~CAudioListener() override = default;

	CAudioListener(CAudioListener const&) = delete;
	CAudioListener(CAudioListener&&) = delete;
	CAudioListener&           operator=(CAudioListener const&) = delete;
	CAudioListener&           operator=(CAudioListener&&) = delete;

	ILINE int                 GetId() const     { return m_id; }
	ILINE FMOD_3D_ATTRIBUTES& Get3DAttributes() { return m_attributes; }

	// IAudioListener
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override
	{
		FillFmodObjectPosition(attributes, m_attributes);
		FMOD_RESULT const fmodResult = s_pSystem->setListenerAttributes(m_id, &m_attributes);
		ASSERT_FMOD_OK;
		return eRequestStatus_Success;
	}
	// ~IAudioListener

private:

	int                m_id;
	FMOD_3D_ATTRIBUTES m_attributes;

public:
	static FMOD::Studio::System* s_pSystem;
};

enum EFmodEventType : EnumFlagsType
{
	eFmodEventType_None,
	eFmodEventType_Start,
	eFmodEventType_Stop,
};

class CAudioTrigger final : public IAudioTrigger
{
public:

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	explicit CAudioTrigger(
	  uint32 const eventPathId,
	  EnumFlagsType const eventType,
	  FMOD::Studio::EventDescription* const pEventDescription,
	  FMOD_GUID const guid,
	  char const* const szEventPath)
		: m_eventPathId(eventPathId)
		, m_eventType(eventType)
		, m_pEventDescription(pEventDescription)
		, m_guid(guid)
		, m_eventPath(szEventPath)
	{}
#else
	explicit CAudioTrigger(
	  uint32 const eventPathId,
	  EnumFlagsType const eventType,
	  FMOD::Studio::EventDescription* const pEventDescription,
	  FMOD_GUID const guid)
		: m_eventPathId(eventPathId)
		, m_eventType(eventType)
		, m_pEventDescription(pEventDescription)
		, m_guid(guid)
	{}
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

	virtual ~CAudioTrigger() override = default;

	CAudioTrigger(CAudioTrigger const&) = delete;
	CAudioTrigger(CAudioTrigger&&) = delete;
	CAudioTrigger& operator=(CAudioTrigger const&) = delete;
	CAudioTrigger& operator=(CAudioTrigger&&) = delete;

	// IAudioTrigger
	virtual ERequestStatus Load()  const override                                      { return eRequestStatus_Success; }
	virtual ERequestStatus Unload() const override                                     { return eRequestStatus_Success; }
	virtual ERequestStatus LoadAsync(IAudioEvent* const pIAudioEvent) const override   { return eRequestStatus_Success; }
	virtual ERequestStatus UnloadAsync(IAudioEvent* const pIAudioEvent) const override { return eRequestStatus_Success; }
	// ~IAudioTrigger

	uint32 const                          m_eventPathId;
	EnumFlagsType const                   m_eventType;
	FMOD::Studio::EventDescription* const m_pEventDescription;
	FMOD_GUID const                       m_guid;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	CryFixedStringT<512> const m_eventPath;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};

class CAudioParameter final : public IParameter
{
public:

	explicit CAudioParameter(
	  uint32 const eventPathId,
	  float const multiplier,
	  float const shift,
	  char const* const szName)
		: m_eventPathId(eventPathId)
		, m_multiplier(multiplier)
		, m_shift(shift)
		, m_name(szName)
	{}

	virtual ~CAudioParameter() override = default;

	CAudioParameter(CAudioParameter const&) = delete;
	CAudioParameter(CAudioParameter&&) = delete;
	CAudioParameter&                                      operator=(CAudioParameter const&) = delete;
	CAudioParameter&                                      operator=(CAudioParameter&&) = delete;

	uint32                                                GetEventPathId() const     { return m_eventPathId; }
	float                                                 GetValueMultiplier() const { return m_multiplier; }
	float                                                 GetValueShift() const      { return m_shift; }
	CryFixedStringT<CryAudio::MaxObjectNameLength> const& GetName() const            { return m_name; }

private:

	uint32 const m_eventPathId;
	float const  m_multiplier;
	float const  m_shift;
	CryFixedStringT<CryAudio::MaxObjectNameLength> const m_name;
};

class CAudioSwitchState final : public IAudioSwitchState
{
public:

	explicit CAudioSwitchState(
	  uint32 const _eventPathId,
	  float const _value,
	  char const* const _szName)
		: eventPathId(_eventPathId)
		, value(_value)
		, name(_szName)
	{}

	virtual ~CAudioSwitchState() override = default;

	CAudioSwitchState(CAudioSwitchState const&) = delete;
	CAudioSwitchState(CAudioSwitchState&&) = delete;
	CAudioSwitchState& operator=(CAudioSwitchState const&) = delete;
	CAudioSwitchState& operator=(CAudioSwitchState&&) = delete;

	uint32 const eventPathId;
	float const  value;
	CryFixedStringT<CryAudio::MaxObjectNameLength> const name;
};

class CAudioEnvironment final : public IAudioEnvironment
{
public:

	explicit CAudioEnvironment(
	  FMOD::Studio::EventDescription* const _pEventDescription,
	  FMOD::Studio::Bus* const _pBus)
		: pEventDescription(_pEventDescription)
		, pBus(_pBus)
	{}

	virtual ~CAudioEnvironment() override = default;

	CAudioEnvironment(CAudioEnvironment const&) = delete;
	CAudioEnvironment(CAudioEnvironment&&) = delete;
	CAudioEnvironment& operator=(CAudioEnvironment const&) = delete;
	CAudioEnvironment& operator=(CAudioEnvironment&&) = delete;

	FMOD::Studio::EventDescription* const pEventDescription;
	FMOD::Studio::Bus* const              pBus;
};

class CAudioFileEntry final : public IAudioFileEntry
{
public:

	CAudioFileEntry() = default;
	virtual ~CAudioFileEntry() override = default;

	CAudioFileEntry(CAudioFileEntry const&) = delete;
	CAudioFileEntry(CAudioFileEntry&&) = delete;
	CAudioFileEntry& operator=(CAudioFileEntry const&) = delete;
	CAudioFileEntry& operator=(CAudioFileEntry&&) = delete;

	FMOD::Studio::Bank* pBank = nullptr;
};

class CAudioFileBase : public IAudioStandaloneFile
{
public:

	explicit CAudioFileBase(char const* const szFile, CATLStandaloneFile& atlStandaloneFile);
	virtual ~CAudioFileBase() override;

	virtual void StartLoading() = 0;
	virtual bool IsReady() = 0;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) = 0;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) = 0;
	virtual void Stop() = 0;

	void         ReportFileStarted();
	void         ReportFileFinished();

	CATLStandaloneFile&                          m_atlStandaloneFile;
	CryFixedStringT<CryAudio::MaxFilePathLength> m_fileName;
	CAudioObjectBase*                            m_pAudioObject = nullptr;
	static FMOD::System*                         s_pLowLevelSystem;

};

class CAudioStandaloneFile final : public CAudioFileBase
{
public:

	explicit CAudioStandaloneFile(char const* const szFile, CATLStandaloneFile& atlStandaloneFile)
		: CAudioFileBase(szFile, atlStandaloneFile)
	{}

	// CAudioFileBase
	virtual void StartLoading() override;
	virtual bool IsReady() override;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Stop() override;
	// ~CAudioFileBase

	FMOD::Sound*   m_pLowLevelSound = nullptr;
	FMOD::Channel* m_pChannel = nullptr;

};

class CProgrammerSoundAudioFile final : public CAudioFileBase
{
public:

	explicit CProgrammerSoundAudioFile(char const* const szFile, FMOD_GUID const eventGuid, CATLStandaloneFile& atlStandaloneFile)
		: CAudioFileBase(szFile, atlStandaloneFile)
		, m_eventGuid(eventGuid)
	{}

	// CAudioFileBase
	virtual void StartLoading() override;
	virtual bool IsReady() override;
	virtual void Play(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes) override;
	virtual void Stop() override;
	// ~CAudioFileBase

private:
	FMOD_GUID const              m_eventGuid;
	FMOD::Studio::EventInstance* m_pEventInstance = nullptr;

};
class CAudioObjectBase;

typedef std::vector<CAudioObjectBase*>                AudioObjects;
typedef std::vector<CAudioEvent*>                     AudioEvents;
typedef std::vector<CAudioStandaloneFile*>            StandaloneFiles;

typedef std::map<CAudioParameter const* const, int>   AudioParameterToIndexMap;
typedef std::map<CAudioSwitchState const* const, int> FmodSwitchToIndexMap;
}
}
}
