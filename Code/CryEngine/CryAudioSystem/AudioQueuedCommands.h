// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

enum EQueuedAudioCommandType : AudioEnumFlagsType
{
	eQueuedAudioCommandType_None                   = 0,
	eQueuedAudioCommandType_PlayFile               = 1,
	eQueuedAudioCommandType_StopFile               = 2,
	eQueuedAudioCommandType_ExecuteTrigger         = 3,
	eQueuedAudioCommandType_StopTrigger            = 4,
	eQueuedAudioCommandType_SetSwitchState         = 5,
	eQueuedAudioCommandType_SetRtpcValue           = 6,
	eQueuedAudioCommandType_SetTransformation      = 7,
	eQueuedAudioCommandType_SetEnvironmentAmount   = 8,
	eQueuedAudioCommandType_SetCurrentEnvironments = 9,
	eQueuedAudioCommandType_ClearEnvironments      = 10,
	eQueuedAudioCommandType_Reset                  = 11,
	eQueuedAudioCommandType_Release                = 12,
	eQueuedAudioCommandType_Initialize             = 13,
};

//////////////////////////////////////////////////////////////////////////
struct SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommandBase(EQueuedAudioCommandType const _type)
		: type(_type)
	{}

	virtual ~SQueuedAudioCommandBase() {}

	EQueuedAudioCommandType const type;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommandBase);
	PREVENT_OBJECT_COPY(SQueuedAudioCommandBase);
};

//////////////////////////////////////////////////////////////////////////
template<EQueuedAudioCommandType T>
struct SQueuedAudioCommand final : public SQueuedAudioCommandBase
{
	SQueuedAudioCommand()
		: SQueuedAudioCommandBase(T)
	{}

	virtual ~SQueuedAudioCommand() {}

	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_PlayFile> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(
	  AudioControlId const _audioTriggerId,
	  void* const _pOwnerOverride,
	  void* const _pUserData,
	  void* const _pUserDataOwner,
	  AudioEnumFlagsType const _requestFlags,
	  bool const _bLocalized,
	  char const* const szFileName)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_PlayFile)
		, audioTriggerId(_audioTriggerId)
		, pOwnerOverride(_pOwnerOverride)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
		, requestFlags(_requestFlags)
		, bLocalized(_bLocalized)
		, fileName(szFileName)
	{}

	virtual ~SQueuedAudioCommand() {}

	AudioControlId const                              audioTriggerId;
	void* const                                       pOwnerOverride;
	void* const                                       pUserData;
	void* const                                       pUserDataOwner;
	AudioEnumFlagsType const                          requestFlags;
	bool const                                        bLocalized;
	CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> const fileName;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_StopFile> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(char const* const szFileName)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_StopFile)
		, fileName(szFileName)
	{}

	virtual ~SQueuedAudioCommand() {}

	CryFixedStringT<MAX_AUDIO_FILE_NAME_LENGTH> const fileName;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_ExecuteTrigger> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(
	  AudioControlId const _audioTriggerId,
	  void* const _pOwnerOverride,
	  void* const _pUserData,
	  void* const _pUserDataOwner,
	  AudioEnumFlagsType const _requestFlags)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_ExecuteTrigger)
		, audioTriggerId(_audioTriggerId)
		, pOwnerOverride(_pOwnerOverride)
		, pUserData(_pUserData)
		, pUserDataOwner(_pUserDataOwner)
		, requestFlags(_requestFlags)
	{}

	virtual ~SQueuedAudioCommand() {}

	AudioControlId const     audioTriggerId;
	void* const              pOwnerOverride;
	void* const              pUserData;
	void* const              pUserDataOwner;
	AudioEnumFlagsType const requestFlags;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_StopTrigger> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(AudioControlId const _audioTriggerId)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_StopTrigger)
		, audioTriggerId(_audioTriggerId)
	{}

	virtual ~SQueuedAudioCommand() {}

	AudioControlId const audioTriggerId;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(AudioControlId const _audioSwitchId, AudioSwitchStateId const _audioSwitchStateId)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_SetSwitchState)
		, audioSwitchId(_audioSwitchId)
		, audioSwitchStateId(_audioSwitchStateId)
	{}

	virtual ~SQueuedAudioCommand() {}

	AudioControlId const     audioSwitchId;
	AudioSwitchStateId const audioSwitchStateId;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(AudioControlId const _audioRtpcId, float const _value)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_SetRtpcValue)
		, audioRtpcId(_audioRtpcId)
		, value(_value)
	{}

	virtual ~SQueuedAudioCommand() {}

	AudioControlId const audioRtpcId;
	float const          value;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_SetTransformation> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(Matrix34 const& _transformation)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_SetTransformation)
		, transformation(_transformation)
	{}

	virtual ~SQueuedAudioCommand() {}

	Matrix34 const transformation;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(AudioEnvironmentId const _audioEnvironmentId, float const _value)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_SetEnvironmentAmount)
		, audioEnvironmentId(_audioEnvironmentId)
		, value(_value)
	{}

	virtual ~SQueuedAudioCommand() {}

	AudioEnvironmentId const audioEnvironmentId;
	float const              value;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_SetCurrentEnvironments> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(EntityId const _entityId)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_SetCurrentEnvironments)
		, entityId(_entityId)
	{}

	virtual ~SQueuedAudioCommand() {}

	EntityId const entityId;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_ClearEnvironments> final : public SQueuedAudioCommandBase
{
	SQueuedAudioCommand()
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_ClearEnvironments)
	{}

	virtual ~SQueuedAudioCommand() {}

	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_Reset> final : public SQueuedAudioCommandBase
{
	SQueuedAudioCommand()
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_Reset)
	{}

	virtual ~SQueuedAudioCommand() {}

	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_Release> final : public SQueuedAudioCommandBase
{
	SQueuedAudioCommand()
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_Release)
	{}

	virtual ~SQueuedAudioCommand() {}

	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};

//////////////////////////////////////////////////////////////////////////
template<>
struct SQueuedAudioCommand<eQueuedAudioCommandType_Initialize> final : public SQueuedAudioCommandBase
{
	explicit SQueuedAudioCommand(char const* const szAudioObjectName)
		: SQueuedAudioCommandBase(eQueuedAudioCommandType_Initialize)
		, audioObjectName(szAudioObjectName)
	{}

	virtual ~SQueuedAudioCommand() {}

	CryFixedStringT<MAX_AUDIO_OBJECT_NAME_LENGTH> const audioObjectName;

	DELETE_DEFAULT_CONSTRUCTOR(SQueuedAudioCommand);
	PREVENT_OBJECT_COPY(SQueuedAudioCommand);
};
