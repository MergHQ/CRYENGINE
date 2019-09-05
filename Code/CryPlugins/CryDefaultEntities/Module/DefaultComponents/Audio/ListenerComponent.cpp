// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ListenerComponent.h"
#include <CryAudio/IAudioSystem.h>

namespace Cry
{
namespace Audio
{
namespace DefaultComponents
{
//////////////////////////////////////////////////////////////////////////
void CListenerComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CListenerComponent::SetUpdatePosition, "7EA9C2D4-1C50-4485-9136-625DAE4ADFC6"_cry_guid, "UpdatePosition");
		pFunction->SetDescription("Enables/Disables the component.");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'val', "Enable");
		componentScope.Register(pFunction);
	}
}

//////////////////////////////////////////////////////////////////////////
void CListenerComponent::Initialize()
{
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	m_previousListenerId = m_listenerHelper.m_id;
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE

	if (m_pIListener == nullptr)
	{
		m_pIListener = gEnv->pAudioSystem->GetListener(m_listenerHelper.m_id);

		if (m_pIListener != nullptr)
		{
			m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_TRIGGER_AREAS);
			m_pEntity->SetFlagsExtended(m_pEntity->GetFlagsExtended() | ENTITY_FLAG_EXTENDED_AUDIO_LISTENER);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CListenerComponent::OnShutDown()
{
	m_pIListener = nullptr;
}

//////////////////////////////////////////////////////////////////////////
Cry::Entity::EventFlags CListenerComponent::GetEventMask() const
{
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	return ENTITY_EVENT_XFORM | ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED;
#else
	return ENTITY_EVENT_XFORM;
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CListenerComponent::ProcessEvent(const SEntityEvent& event)
{
	if (m_pIListener != nullptr)
	{
		switch (event.event)
		{
		case ENTITY_EVENT_XFORM:
			{
				if (m_listenerHelper.m_updatePosition)
				{
					int const flags = static_cast<int>(event.nParam[0]);

					if ((flags & (ENTITY_XFORM_POS | ENTITY_XFORM_ROT)) != 0)
					{
						OnTransformChanged();
					}
				}

				break;
			}
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
		case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
			{
				if (m_previousListenerId != m_listenerHelper.m_id)
				{
					m_pIListener = gEnv->pAudioSystem->GetListener(m_listenerHelper.m_id);
					m_previousListenerId = m_listenerHelper.m_id;

					if (m_listenerHelper.m_updatePosition)
					{
						// Force transformation update for newly selected listener.
						m_previousTransformation = GetWorldTransformMatrix();
						m_pIListener->SetTransformation(m_previousTransformation);
					}
				}

				break;
			}
#endif      // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
		default:
			{
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CListenerComponent::OnTransformChanged()
{
	Matrix34 const tm = GetWorldTransformMatrix();

	// Listener needs to move at least 1 cm to trigger an update.
	if (!m_previousTransformation.IsEquivalent(tm, 0.01f))
	{
		m_previousTransformation = tm;
		m_pIListener->SetTransformation(m_previousTransformation);
	}
}
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
