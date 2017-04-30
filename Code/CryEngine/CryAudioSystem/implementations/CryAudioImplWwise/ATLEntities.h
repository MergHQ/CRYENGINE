// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>
#include <IAudioImpl.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CAudioObject final : public IAudioObject, public CPoolObject<CAudioObject, stl::PSyncNone>
{
public:

	typedef std::map<AkAuxBusID, float> EnvironmentImplMap;

	explicit CAudioObject(AkGameObjectID const id)
		: m_id(id)
		, m_bNeedsToUpdateEnvironments(false)
	{}

	virtual ~CAudioObject() override = default;

	CAudioObject(CAudioObject const&) = delete;
	CAudioObject(CAudioObject&&) = delete;
	CAudioObject& operator=(CAudioObject const&) = delete;
	CAudioObject& operator=(CAudioObject&&) = delete;

	// IAudioObject
	virtual ERequestStatus Update() override;
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override;
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) override;
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioRtpc, float const value) override;
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) override;
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual ERequestStatus ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent) override;
	virtual ERequestStatus StopAllTriggers() override;
	virtual ERequestStatus PlayFile(IAudioStandaloneFile* const pIFile) override { return ERequestStatus::Success; }
	virtual ERequestStatus StopFile(IAudioStandaloneFile* const pIFile) override { return ERequestStatus::Success; }
	virtual ERequestStatus SetName(char const* const szName) override;
	// ~IAudioObject

	AkGameObjectID const  m_id;
	bool                  m_bNeedsToUpdateEnvironments;
	EnvironmentImplMap    m_environemntImplAmounts;

	static AkGameObjectID s_dummyGameObjectId;

private:

	ERequestStatus PostEnvironmentAmounts();
};

struct SAudioListener final : public IAudioListener
{
	explicit SAudioListener(AkUniqueID const id_)
		: id(id_)
	{}

	virtual ~SAudioListener() override = default;

	SAudioListener(SAudioListener const&) = delete;
	SAudioListener(SAudioListener&&) = delete;
	SAudioListener& operator=(SAudioListener const&) = delete;
	SAudioListener& operator=(SAudioListener&&) = delete;

	// IAudioListener
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override;
	// ~IAudioListener

	AkUniqueID const id;
};

struct SAudioTrigger final : public IAudioTrigger
{
	explicit SAudioTrigger(AkUniqueID const id_)
		: id(id_)
	{}

	virtual ~SAudioTrigger() override = default;

	SAudioTrigger(SAudioTrigger const&) = delete;
	SAudioTrigger(SAudioTrigger&&) = delete;
	SAudioTrigger& operator=(SAudioTrigger const&) = delete;
	SAudioTrigger& operator=(SAudioTrigger&&) = delete;

	// IAudioTrigger
	virtual ERequestStatus Load() const override;
	virtual ERequestStatus Unload() const override;
	virtual ERequestStatus LoadAsync(IAudioEvent* const pIAudioEvent) const override;
	virtual ERequestStatus UnloadAsync(IAudioEvent* const pIAudioEvent) const override;
	// ~IAudioTrigger

	AkUniqueID const id;

private:

	ERequestStatus SetLoaded(bool bLoad) const;
	ERequestStatus SetLoadedAsync(IAudioEvent* const pIAudioEvent, bool bLoad) const;
};

struct SAudioRtpc final : public IParameter
{
	explicit SAudioRtpc(AkRtpcID const id_, float const mult_, float const shift_)
		: mult(mult_)
		, shift(shift_)
		, id(id_)
	{}

	virtual ~SAudioRtpc() override = default;

	SAudioRtpc(SAudioRtpc const&) = delete;
	SAudioRtpc(SAudioRtpc&&) = delete;
	SAudioRtpc& operator=(SAudioRtpc const&) = delete;
	SAudioRtpc& operator=(SAudioRtpc&&) = delete;

	float const    mult;
	float const    shift;
	AkRtpcID const id;
};

enum class ESwitchType : EnumFlagsType
{
	None,
	Rtpc,
	StateGroup,
	SwitchGroup,
};

struct SAudioSwitchState final : public IAudioSwitchState
{
	explicit SAudioSwitchState(
	  ESwitchType const type_,
	  AkUInt32 const stateOrSwitchGroupId_,
	  AkUInt32 const stateOrSwitchId_,
	  float const rtpcValue_ = 0.0f)
		: type(type_)
		, stateOrSwitchGroupId(stateOrSwitchGroupId_)
		, stateOrSwitchId(stateOrSwitchId_)
		, rtpcValue(rtpcValue_)
	{}

	virtual ~SAudioSwitchState() override = default;

	SAudioSwitchState(SAudioSwitchState const&) = delete;
	SAudioSwitchState(SAudioSwitchState&&) = delete;
	SAudioSwitchState& operator=(SAudioSwitchState const&) = delete;
	SAudioSwitchState& operator=(SAudioSwitchState&&) = delete;

	ESwitchType const type;
	AkUInt32 const    stateOrSwitchGroupId;
	AkUInt32 const    stateOrSwitchId;
	float const       rtpcValue;
};

enum class EEnvironmentType : EnumFlagsType
{
	None,
	AuxBus,
	Rtpc,
};

struct SAudioEnvironment final : public IAudioEnvironment
{
	explicit SAudioEnvironment(EEnvironmentType const type_, AkAuxBusID const busId_)
		: type(type_)
		, busId(busId_)
	{
		CRY_ASSERT(type_ == EEnvironmentType::AuxBus);
	}

	explicit SAudioEnvironment(
	  EEnvironmentType const type_,
	  AkRtpcID const rtpcId_,
	  float const multiplier_,
	  float const shift_)
		: type(type_)
		, rtpcId(rtpcId_)
		, multiplier(multiplier_)
		, shift(shift_)
	{
		CRY_ASSERT(type_ == EEnvironmentType::Rtpc);
	}

	virtual ~SAudioEnvironment() override = default;

	SAudioEnvironment(SAudioEnvironment const&) = delete;
	SAudioEnvironment(SAudioEnvironment&&) = delete;
	SAudioEnvironment& operator=(SAudioEnvironment const&) = delete;
	SAudioEnvironment& operator=(SAudioEnvironment&&) = delete;

	EEnvironmentType const type;

	union
	{
		// Aux Bus implementation
		struct
		{
			AkAuxBusID const busId;
		};

		// Rtpc implementation
		struct
		{
			AkRtpcID const rtpcId;
			float const    multiplier;
			float const    shift;
		};
	};
};

struct SAudioEvent final : public IAudioEvent, public CPoolObject<SAudioEvent, stl::PSyncNone>
{
	explicit SAudioEvent(CATLEvent& atlEvent_)
		: state(EEventState::None)
		, id(AK_INVALID_UNIQUE_ID)
		, atlEvent(atlEvent_)
	{}

	virtual ~SAudioEvent() override = default;

	SAudioEvent(SAudioEvent const&) = delete;
	SAudioEvent(SAudioEvent&&) = delete;
	SAudioEvent& operator=(SAudioEvent const&) = delete;
	SAudioEvent& operator=(SAudioEvent&&) = delete;

	// IAudioEvent
	virtual ERequestStatus Stop() override;
	// ~IAudioEvent

	EEventState state;
	AkUniqueID  id;
	CATLEvent&  atlEvent;
};

struct SAudioFileEntry final : public IAudioFileEntry
{
	SAudioFileEntry() = default;
	virtual ~SAudioFileEntry() override = default;

	SAudioFileEntry(SAudioFileEntry const&) = delete;
	SAudioFileEntry(SAudioFileEntry&&) = delete;
	SAudioFileEntry& operator=(SAudioFileEntry const&) = delete;
	SAudioFileEntry& operator=(SAudioFileEntry&&) = delete;

	AkBankID bankId = AK_INVALID_BANK_ID;
};

struct SAudioStandaloneFile final : public IAudioStandaloneFile
{
	SAudioStandaloneFile() = default;
	virtual ~SAudioStandaloneFile() override = default;

	SAudioStandaloneFile(SAudioStandaloneFile const&) = delete;
	SAudioStandaloneFile(SAudioStandaloneFile&&) = delete;
	SAudioStandaloneFile& operator=(SAudioStandaloneFile const&) = delete;
	SAudioStandaloneFile& operator=(SAudioStandaloneFile&&) = delete;

};

struct SEnvPairCompare
{
	bool operator()(std::pair<AkAuxBusID, float> const& pair1, std::pair<AkAuxBusID, float> const& pair2) const
	{
		return (pair1.second > pair2.second);
	}
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
