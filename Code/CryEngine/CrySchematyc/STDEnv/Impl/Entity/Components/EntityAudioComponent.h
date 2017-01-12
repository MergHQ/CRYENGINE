// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/MathTypes.h>
#include <CrySerialization/Forward.h>

struct IEntityAudioComponent;

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvRegistrar;

struct SAudioTriggerSerializeHelper
{
	void        Serialize(Serialization::IArchive& archive);
	static void ReflectType(CTypeDesc<SAudioTriggerSerializeHelper>& desc);

	CryAudio::ControlId m_triggerId = CryAudio::InvalidControlId;
	string              m_triggerName;
};

struct SAudioParameterSerializeHelper
{
	void        Serialize(Serialization::IArchive& archive);
	static void ReflectType(CTypeDesc<SAudioParameterSerializeHelper>& desc);

	CryAudio::ControlId m_parameterId = CryAudio::InvalidControlId;
	string              m_parameterName;
};

struct SAudioSwitchWithStateSerializeHelper
{
	void        Serialize(Serialization::IArchive& archive);
	static void ReflectType(CTypeDesc<SAudioSwitchWithStateSerializeHelper>& desc);

	CryAudio::ControlId     m_switchId = CryAudio::InvalidControlId;
	string                  m_switchName;
	CryAudio::SwitchStateId m_switchStateId = CryAudio::InvalidSwitchStateId;
	string                  m_switchStateName;
};

class CEntityAudioComponent final : public CComponent
{
public:

	struct SAudioTriggerFinishedSignal
	{
		SAudioTriggerFinishedSignal() {}
		SAudioTriggerFinishedSignal(uint32 instanceId, uint32 triggerId, bool bSuccess);

		static void ReflectType(CTypeDesc<SAudioTriggerFinishedSignal>& desc);

		uint32       m_instanceId = 0;
		uint32       m_triggerId = 0;
		bool         m_bSuccess = false;
	};

	// CComponent
	virtual bool Init() override;
	virtual void Shutdown() override;
	// ~CComponent

	void         ExecuteTrigger(const SAudioTriggerSerializeHelper trigger, uint32& instanceId, uint32& triggerId);
	void         StopTrigger(const SAudioTriggerSerializeHelper trigger);
	void         SetParameter(const SAudioParameterSerializeHelper parameter, float value);
	void         SetSwitchState(const SAudioSwitchWithStateSerializeHelper switchAndState);

	static void ReflectType(CTypeDesc<CEntityAudioComponent>& desc);

	static void  Register(IEnvRegistrar& registrar);

	//audio callback
	static void OnAudioCallback(CryAudio::SRequestInfo const* const pAudioRequestInfo);

private:

	CryAudio::AuxObjectId  m_audioProxyId = CryAudio::InvalidAuxObjectId;
	IEntityAudioComponent* m_pAudioProxy;
};
} // Schematyc
