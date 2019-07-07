// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>
#include <CrySerialization/Forward.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CryEntitySystem/IEntityComponent.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
namespace Audio
{
namespace DefaultComponents
{
class CDefaultTriggerComponent final : public IEntityComponent
{
protected:

	friend CPlugin_CryDefaultEntities;
	static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void                    Initialize() override;
	virtual void                    OnShutDown() override                            {}
	virtual Cry::Entity::EventFlags GetEventMask() const override                    { return Cry::Entity::EventFlags(); }
	virtual void                    ProcessEvent(const SEntityEvent& event) override {}
	// ~IEntityComponent

public:

	CDefaultTriggerComponent() = default;

	static void ReflectType(Schematyc::CTypeDesc<CDefaultTriggerComponent>& desc);
	void        Execute();

protected:

	// Properties exposed to UI
	CryAudio::EDefaultTriggerType m_triggerType;

#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	Serialization::FunctorActionButton<std::function<void()>> m_executeButton;
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
};
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
