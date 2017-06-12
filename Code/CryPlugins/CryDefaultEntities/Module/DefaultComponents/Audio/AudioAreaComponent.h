// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include "AudioSpotComponent.h"  // SAudioParameterSerializeHelper
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;
struct CEntityComponentAudio;

namespace Cry
{
namespace DefaultComponents
{

struct SAudioEnvironmentSerializeHelper
{
	void Serialize(Serialization::IArchive& archive);
	bool operator==(const SAudioEnvironmentSerializeHelper& other) const { return m_envName == other.m_envName; }

	CryAudio::ControlId m_envId = CryAudio::InvalidControlId;
	string              m_envName;
};

class CEntityAudioAreaComponent final : public IEntityComponent
{
protected:
	friend CPlugin_CryDefaultEntities;
	static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void   Initialize() final;
	virtual void   OnShutDown() final;
	virtual uint64 GetEventMask() const final;
	virtual void   ProcessEvent(SEntityEvent& event) final;
	// ~IEntityComponent

public:
	//this class sends schematyc events when an area is entered/left. It also sends the current fade-factor, so that a graph can do fading based on this value
	//for convenience sake it also has options to:
	//	- automatically sync an audio-parameter with the current fade-factor, 
	//	- can fade in an audio-environment 
	//	- and can toggle (ambiance)-audio-trigger that are defined in normal audio-spot-components added to the same entity.

	struct SAreaFadingFactorChangedSignal
	{
		float m_factor;
	};

	struct SOutsideAreaEnteredSignal
	{
	};

	struct SFadingAreaEnteredSignal
	{
		float m_factor;
	};

	struct SInsideAreaEnteredSignal
	{
	};

	struct IListener
	{
		virtual void OnFadingFactorChanged(float newValue, EAreaState currentState) = 0;
	};

	static void ReflectType(Schematyc::CTypeDesc<CEntityAudioAreaComponent>& desc);

	static CryGUID& IID()
	{
		static CryGUID id = "88f8417f-0f4c-48a5-8f51-ecabee83d955"_cry_guid;
		return id;
	}

	float GetCurrentFadeFactor() const { return m_currentFadeFactor; }
	
	//property getter/setter
	virtual void  SetEnabled(bool bEnabled);
	bool  GetEnabled() const { return m_bEnabled; }

	virtual void SetFadeDistance(float value) { m_fadeDistance = value; }
	float GetFadeDistance() const { return m_fadeDistance; }

	virtual void SetTriggerEntityName(const char* szValue) { m_triggerEntityName = szValue; }
	const char* GetTriggerEntityName() const { return m_triggerEntityName.c_str(); }

	virtual void SetMoveAlongWithTriggeringEntity(bool bValue) { m_bMoveAlongWithTriggeringEntity = bValue; }
	float GetMoveAlongWithTriggeringEntity() const { return m_bMoveAlongWithTriggeringEntity; }

	virtual void SetToggleAudioTriggerSpots(bool bValue) { m_bToggleAudioTriggerSpots = bValue; }
	float GetToggleAudioTriggerSpots() const { return m_bToggleAudioTriggerSpots; }

	virtual void SetControlledAudioParameter(const char* szValue) { m_controlledAudioParameter.m_parameterName = szValue; gEnv->pAudioSystem->GetParameterId(szValue, m_controlledAudioParameter.m_parameterId); }
	const char* GetControlledAudioParameter() const { return m_controlledAudioParameter.m_parameterName.c_str(); }

	virtual void SetEnvironment(const char* szValue) { m_environment.m_envName = szValue; gEnv->pAudioSystem->GetEnvironmentId(szValue, m_environment.m_envId); }
	const char* GetEnvironment() const { return m_environment.m_envName.c_str(); }

private:
	void SetFadingFactor(float factor, Schematyc::IObject* pSchematycObject, bool bForceSet = false);
	void ToggleAudioSpots(bool bEnable);

	IEntityAudioComponent* m_pAudioComp = nullptr;
	EAreaState             m_areaState = EAreaState::Outside;
	EntityId               m_triggeringEntityId = INVALID_ENTITYID;
	float                  m_currentFadeFactor = 0.0f;
	bool                   m_bActive = false;

	//Properties exposed to UI
	bool                             m_bEnabled = true;
	float                            m_fadeDistance = 5.0f;
	Schematyc::CSharedString         m_triggerEntityName = "Player";
	bool                             m_bMoveAlongWithTriggeringEntity = false;
	bool                             m_bToggleAudioTriggerSpots = false;
	SAudioParameterSerializeHelper   m_controlledAudioParameter;
	SAudioEnvironmentSerializeHelper m_environment;
};

}  // DefaultComponents
}  //Cry
