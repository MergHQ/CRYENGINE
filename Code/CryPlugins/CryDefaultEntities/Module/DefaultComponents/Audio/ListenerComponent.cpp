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
constexpr float g_positionUpdateThreshold = 0.01f; // Listener needs to move at least 1 cm to trigger an update.

//////////////////////////////////////////////////////////////////////////
void CListenerComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CListenerComponent::SetOffset, "339E91ED-31F3-4FA1-B3C2-2DF9FA87DA94"_cry_guid, "SetOffset");
		pFunction->SetDescription("Sets an offset to the listener component.");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'val', "Offset");
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
	return ENTITY_EVENT_XFORM | ENTITY_EVENT_RESET | ENTITY_EVENT_DONE | ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED;
#else
	return ENTITY_EVENT_XFORM | ENTITY_EVENT_RESET | ENTITY_EVENT_DONE;
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
				int const flags = static_cast<int>(event.nParam[0]);

				if ((flags & (ENTITY_XFORM_POS | ENTITY_XFORM_ROT)) != 0)
				{
					OnTransformChanged();
				}

				break;
			}
		case ENTITY_EVENT_RESET:
		case ENTITY_EVENT_DONE:
			{
				m_listenerHelper.m_offset = ZERO;

				break;
			}
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
		case ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED:
			{
				if (m_previousListenerId != m_listenerHelper.m_id)
				{
					m_pIListener = gEnv->pAudioSystem->GetListener(m_listenerHelper.m_id);
					m_previousListenerId = m_listenerHelper.m_id;
				}

				OnTransformChanged();

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
	if (!m_listenerHelper.m_hasOffset)
	{
		Matrix34 const tm = GetWorldTransformMatrix();

		if (!m_previousTransformation.IsEquivalent(tm, g_positionUpdateThreshold))
		{
			m_previousTransformation = tm;
			m_pIListener->SetTransformation(m_previousTransformation);
		}
	}
	else
	{
		Matrix34 const tm = GetWorldTransformMatrix() * Matrix34(IDENTITY, m_listenerHelper.m_offset);

		if (!m_previousTransformation.IsEquivalent(tm, g_positionUpdateThreshold))
		{
			m_previousTransformation = tm;
			m_pIListener->SetTransformation(m_previousTransformation);
		}
	}
}
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
