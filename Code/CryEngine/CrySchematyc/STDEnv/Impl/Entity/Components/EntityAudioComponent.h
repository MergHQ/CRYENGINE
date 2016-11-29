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
	void         Serialize(Serialization::IArchive& archive);
	static SGUID ReflectSchematycType(CTypeInfo<SAudioTriggerSerializeHelper>& typeInfo);

	AudioControlId m_triggerId = INVALID_AUDIO_CONTROL_ID;
	string         m_triggerName;
};

struct SAudioParameterSerializeHelper
{
	void         Serialize(Serialization::IArchive& archive);
	static SGUID ReflectSchematycType(CTypeInfo<SAudioParameterSerializeHelper>& typeInfo);

	AudioControlId m_parameterId = INVALID_AUDIO_CONTROL_ID;
	string         m_parameterName;
};

struct SAudioSwitchWithStateSerializeHelper
{
	void         Serialize(Serialization::IArchive& archive);
	static SGUID ReflectSchematycType(CTypeInfo<SAudioSwitchWithStateSerializeHelper>& typeInfo);

	AudioControlId m_switchId = INVALID_AUDIO_CONTROL_ID;
	string         m_switchName;
	AudioControlId m_switchStateId = INVALID_AUDIO_CONTROL_ID;
	string         m_switchStateName;
};

class CEntityAudioComponent final : public CComponent
{
public:

	struct SAudioTriggerFinishedSignal
	{
		SAudioTriggerFinishedSignal() {}
		SAudioTriggerFinishedSignal(uint32 instanceId, uint32 triggerId, bool bSuccess);

		static SGUID ReflectSchematycType(CTypeInfo<SAudioTriggerFinishedSignal>& typeInfo);

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

	static SGUID ReflectSchematycType(CTypeInfo<CEntityAudioComponent>& typeInfo);

	static void  Register(IEnvRegistrar& registrar);

	//audio callback
	static void OnAudioCallback(SAudioRequestInfo const* const pAudioRequestInfo);

private:
	AudioProxyId         m_audioProxyId = INVALID_AUDIO_PROXY_ID;
	IEntityAudioComponent*   m_pAudioProxy;
};
} // Schematyc
