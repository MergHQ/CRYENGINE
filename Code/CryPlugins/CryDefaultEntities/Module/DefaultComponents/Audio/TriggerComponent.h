// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CrySerialization/Forward.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <CryString/CryString.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
namespace Audio
{
namespace DefaultComponents
{
struct SPlayTriggerSerializeHelper final
{
	void Serialize(Serialization::IArchive& archive);
	bool operator==(SPlayTriggerSerializeHelper const& other) const { return (m_autoPlay == other.m_autoPlay) && (m_id == other.m_id); }

	bool                m_autoPlay = true;
	CryAudio::ControlId m_id = CryAudio::InvalidControlId;
	string              m_name;
};

struct SStopTriggerSerializeHelper final
{
	void Serialize(Serialization::IArchive& archive);
	bool operator==(SStopTriggerSerializeHelper const& other) const { return (m_canStop == other.m_canStop) && (m_id == other.m_id); }

	bool                m_canStop = true;
	CryAudio::ControlId m_id = CryAudio::InvalidControlId;
	string              m_name;
};

#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
class CTriggerComponent;

class CDebugSerializeHelper final
{
public:

	CDebugSerializeHelper() = default;

	void Serialize(Serialization::IArchive& archive);
	bool operator==(CDebugSerializeHelper const& other) const { return 0 == memcmp(this, &other, sizeof(other)); }

	void SetComponent(CTriggerComponent* const pComponent)    { m_pComponent = pComponent; }
	void UpdateDebugInfo();
	void DrawDebugInfo();

private:

	void OnPlay();
	void OnStop();

	CTriggerComponent* m_pComponent = nullptr;
	bool               m_drawActivityRadius = false;
	bool               m_drawStopTriggerRadius = false;
	bool               m_canDrawPlayTriggerRadius = false;
	bool               m_canDrawStopTriggerRadius = false;
	float              m_playTriggerRadius = 0.0f;
	float              m_stopTriggerRadius = 0.0f;
	Serialization::FunctorActionButton<std::function<void()>> m_playButton;
	Serialization::FunctorActionButton<std::function<void()>> m_stopButton;
};
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE

class CTriggerComponent final : public IEntityComponent
{
protected:

	friend CPlugin_CryDefaultEntities;
	static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void                    Initialize() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void                    ProcessEvent(const SEntityEvent& event) override;
	// ~IEntityComponent

public:

	CTriggerComponent() = default;

	static void ReflectType(Schematyc::CTypeDesc<CTriggerComponent>& desc);

	void        Play();
	void        Stop();

	struct SFinishedSignal
	{
		SFinishedSignal() = default;
	};

protected:

	CryAudio::AuxObjectId  m_auxObjectId = CryAudio::InvalidAuxObjectId;
	IEntityAudioComponent* m_pIEntityAudioComponent = nullptr;
	uint32                 m_numActiveTriggerInstances = 0;

	// Properties exposed to UI
	SPlayTriggerSerializeHelper m_playTrigger;
	SStopTriggerSerializeHelper m_stopTrigger;

#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
public:

	CryAudio::ControlId GetPlayTriggerId() const { return m_playTrigger.m_id; }
	CryAudio::ControlId GetStopTriggerId() const { return m_stopTrigger.m_id; }
	bool                CanStop() const          { return m_stopTrigger.m_canStop; }

protected:

	CDebugSerializeHelper m_debugInfo;
	CryAudio::ControlId   m_previousPlayTriggerId = CryAudio::InvalidControlId;
	CryAudio::ControlId   m_previousStopTriggerId = CryAudio::InvalidControlId;
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
};
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
