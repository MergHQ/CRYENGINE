// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Entity.h"
#include "Common.h"

namespace CryAudio
{
enum class ETriggerStatus : EnumFlagsType
{
	None                     = 0,
	Playing                  = BIT(0),
	CallbackOnExternalThread = BIT(1),
	CallbackOnAudioThread    = BIT(2),
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ETriggerStatus);

struct STriggerInstanceState final
{
	STriggerInstanceState() = delete;
	STriggerInstanceState& operator=(STriggerInstanceState const&) = delete;

	explicit STriggerInstanceState(
		ControlId const triggerId_,
		void* const pOwnerOverride_ = nullptr,
		void* const pUserData_ = nullptr,
		void* const pUserDataOwner_ = nullptr)
		: triggerId(triggerId_)
		, pOwnerOverride(pOwnerOverride_)
		, pUserData(pUserData_)
		, pUserDataOwner(pUserDataOwner_)
		, flags(ETriggerStatus::None)
		, numPlayingInstances(0)
		, numPendingInstances(0)
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		, radius(0.0f)
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
	{}

	ControlId const triggerId;

	void* const     pOwnerOverride;
	void* const     pUserData;
	void* const     pUserDataOwner;

	ETriggerStatus  flags;
	uint16          numPlayingInstances;
	uint16          numPendingInstances;
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	float           radius;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
};

using TriggerInstanceStates = std::map<TriggerInstanceId, STriggerInstanceState>;

void SendFinishedTriggerInstanceRequest(STriggerInstanceState const& triggerInstanceState, EntityId const entityId);
void ExecuteDefaultTriggerConnections(Control const* const pControl, TriggerConnections const& connections);
} // namespace CryAudio
