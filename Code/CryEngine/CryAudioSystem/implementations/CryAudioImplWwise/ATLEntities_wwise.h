// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <IAudioSystemImplementation.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
typedef std::vector<AkUniqueID, STLSoundAllocator<AkUniqueID>> TAKUniqueIDVector;

struct SAudioObject_wwise final : public IAudioObject
{
	typedef std::map<AkAuxBusID, float, std::less<AkAuxBusID>,
	                 STLSoundAllocator<std::pair<AkAuxBusID, float>>> TEnvironmentImplMap;

	explicit SAudioObject_wwise(AkGameObjectID const _id, bool const _bHasPosition)
		: id(_id)
		, bHasPosition(_bHasPosition)
		, bNeedsToUpdateEnvironments(false)
	{}

	virtual ~SAudioObject_wwise() {}

	AkGameObjectID const id;
	bool const           bHasPosition;
	bool                 bNeedsToUpdateEnvironments;
	TEnvironmentImplMap  cEnvironemntImplAmounts;

	DELETE_DEFAULT_CONSTRUCTOR(SAudioObject_wwise);
	PREVENT_OBJECT_COPY(SAudioObject_wwise);
};

struct SAudioListener_wwise final : public IAudioListener
{
	explicit SAudioListener_wwise(AkUniqueID const _id)
		: id(_id)
	{}

	virtual ~SAudioListener_wwise() {}

	AkUniqueID const id;

	DELETE_DEFAULT_CONSTRUCTOR(SAudioListener_wwise);
	PREVENT_OBJECT_COPY(SAudioListener_wwise);
};

struct SAudioTrigger_wwise final : public IAudioTrigger
{
	explicit SAudioTrigger_wwise(AkUniqueID const _id)
		: id(_id)
	{}

	virtual ~SAudioTrigger_wwise() {}

	AkUniqueID const id;

	DELETE_DEFAULT_CONSTRUCTOR(SAudioTrigger_wwise);
	PREVENT_OBJECT_COPY(SAudioTrigger_wwise);
};

struct SAudioRtpc_wwise final : public IAudioRtpc
{
	explicit SAudioRtpc_wwise(AkRtpcID const _id, float const _mult, float const _shift)
		: mult(_mult)
		, shift(_shift)
		, id(_id)
	{}

	virtual ~SAudioRtpc_wwise() {}

	float const    mult;
	float const    shift;
	AkRtpcID const id;

	DELETE_DEFAULT_CONSTRUCTOR(SAudioRtpc_wwise);
	PREVENT_OBJECT_COPY(SAudioRtpc_wwise);
};

enum EWwiseSwitchType : AudioEnumFlagsType
{
	eWST_NONE   = 0,
	eWST_SWITCH = 1,
	eWST_STATE  = 2,
	eWST_RTPC   = 3,
};

struct SAudioSwitchState_wwise final : public IAudioSwitchState
{
	explicit SAudioSwitchState_wwise(
	  EWwiseSwitchType const _type,
	  AkUInt32 const _switchId,
	  AkUInt32 const _stateId,
	  float const rtpcValue = 0.0f)
		: type(_type)
		, switchId(_switchId)
		, stateId(_stateId)
		, rtpcValue(rtpcValue)
	{}

	virtual ~SAudioSwitchState_wwise() {}

	EWwiseSwitchType const type;
	AkUInt32 const         switchId;
	AkUInt32 const         stateId;
	float const            rtpcValue;

	DELETE_DEFAULT_CONSTRUCTOR(SAudioSwitchState_wwise);
	PREVENT_OBJECT_COPY(SAudioSwitchState_wwise);
};

enum EWwiseAudioEnvironmentType : AudioEnumFlagsType
{
	eWAET_NONE    = 0,
	eWAET_AUX_BUS = 1,
	eWAET_RTPC    = 2,
};

struct SAudioEnvironment_wwise final : public IAudioEnvironment
{
	explicit SAudioEnvironment_wwise(EWwiseAudioEnvironmentType const _type)
		: type(_type)
	{}

	explicit SAudioEnvironment_wwise(EWwiseAudioEnvironmentType const _type, AkAuxBusID const _busId)
		: type(_type)
		, busId(_busId)
	{
		CRY_ASSERT(_type == eWAET_AUX_BUS);
	}

	explicit SAudioEnvironment_wwise(
	  EWwiseAudioEnvironmentType const _type,
	  AkRtpcID const _rtpcId,
	  float const _multiplier,
	  float const _shift)
		: type(_type)
		, rtpcId(_rtpcId)
		, multiplier(_multiplier)
		, shift(_shift)
	{
		CRY_ASSERT(_type == eWAET_RTPC);
	}

	virtual ~SAudioEnvironment_wwise() {}

	EWwiseAudioEnvironmentType const type;

	union
	{
		// Aux Bus implementation
		struct
		{
			AkAuxBusID busId;
		};

		// Rtpc implementation
		struct
		{
			AkRtpcID rtpcId;
			float    multiplier;
			float    shift;
		};
	};

	DELETE_DEFAULT_CONSTRUCTOR(SAudioEnvironment_wwise);
	PREVENT_OBJECT_COPY(SAudioEnvironment_wwise);
};

struct SAudioEvent_wwise final : public IAudioEvent
{
	explicit SAudioEvent_wwise(AudioEventId const _id)
		: audioEventState(eAudioEventState_None)
		, id(AK_INVALID_UNIQUE_ID)
		, audioEventId(_id)
	{}

	virtual ~SAudioEvent_wwise() {}

	EAudioEventState   audioEventState;
	AkUniqueID         id;
	AudioEventId const audioEventId;

	DELETE_DEFAULT_CONSTRUCTOR(SAudioEvent_wwise);
	PREVENT_OBJECT_COPY(SAudioEvent_wwise);
};

struct SAudioFileEntry_wwise final : public IAudioFileEntry
{
	SAudioFileEntry_wwise()
		: bankId(AK_INVALID_BANK_ID)
	{}

	virtual ~SAudioFileEntry_wwise() {}

	AkBankID bankId;

	PREVENT_OBJECT_COPY(SAudioFileEntry_wwise);
};

struct SAudioStandaloneFile_wwise final : public IAudioStandaloneFile
{
};
}
}
