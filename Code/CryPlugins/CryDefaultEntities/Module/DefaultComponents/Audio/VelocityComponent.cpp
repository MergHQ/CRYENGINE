// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "VelocityComponent.h"

namespace Cry
{
namespace Audio
{
namespace DefaultComponents
{
//////////////////////////////////////////////////////////////////////////
void CVelocityComponent::ReflectType(Schematyc::CTypeDesc<CVelocityComponent>& desc)
{
	desc.SetGUID("1228C261-9E89-478B-B5B5-7E162CD57E73"_cry_guid);
	desc.SetEditorCategory("Audio");
	desc.SetLabel("Velocity Tracking");
	desc.SetDescription("Used to toggle absolute or relative velocity tracking.");
	desc.SetIcon("icons:Audio/component_velocity_tracking.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly, IEntityComponent::EFlags::HideFromInspector });

	desc.AddMember(&CVelocityComponent::m_trackAbsoluteVelocity, 'absv', "trackAbsoluteVelocity", "Track Absolute Velocity", "If enabled calculates the absolute velocity of the object.", false);
	desc.AddMember(&CVelocityComponent::m_trackRelativeVelocity, 'relv', "trackRelativeVelocity", "Track Relative Velocity", "If enabled calculates the relative velocity of the object.", false);
}

//////////////////////////////////////////////////////////////////////////
void CVelocityComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
}

//////////////////////////////////////////////////////////////////////////
void CVelocityComponent::Initialize()
{
	if (m_pIEntityAudioComponent == nullptr)
	{
		m_pIEntityAudioComponent = m_pEntity->GetOrCreateComponent<IEntityAudioComponent>();
		m_pIEntityAudioComponent->ToggleAbsoluteVelocityTracking(m_trackAbsoluteVelocity, CryAudio::InvalidAuxObjectId);
		m_pIEntityAudioComponent->ToggleRelativeVelocityTracking(m_trackRelativeVelocity, CryAudio::InvalidAuxObjectId);
	}
}

//////////////////////////////////////////////////////////////////////////
void CVelocityComponent::OnShutDown()
{
	if (m_pIEntityAudioComponent != nullptr)
	{
		m_pIEntityAudioComponent->ToggleAbsoluteVelocityTracking(false, CryAudio::InvalidAuxObjectId);
		m_pIEntityAudioComponent->ToggleRelativeVelocityTracking(false, CryAudio::InvalidAuxObjectId);
		m_pIEntityAudioComponent = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
Cry::Entity::EventFlags CVelocityComponent::GetEventMask() const
{
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	return ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED;
#else
	return Cry::Entity::EventFlags();
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CVelocityComponent::ProcessEvent(const SEntityEvent& event)
{
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
		if (m_pIEntityAudioComponent != nullptr)
		{
			m_pIEntityAudioComponent->ToggleAbsoluteVelocityTracking(m_trackAbsoluteVelocity, CryAudio::InvalidAuxObjectId);
			m_pIEntityAudioComponent->ToggleRelativeVelocityTracking(m_trackRelativeVelocity, CryAudio::InvalidAuxObjectId);
		}
	}
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
}
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
