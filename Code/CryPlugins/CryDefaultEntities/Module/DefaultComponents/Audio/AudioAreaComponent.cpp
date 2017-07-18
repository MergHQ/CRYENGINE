// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioAreaComponent.h"

#include <CryAudio/IAudioSystem.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <CrySchematyc/FundamentalTypes.h>

namespace Cry
{
namespace DefaultComponents
{
static void ReflectType(Schematyc::CTypeDesc<CEntityAudioAreaComponent::SAreaFadingFactorChangedSignal>& desc)
{
	desc.SetGUID("0113ca63-bb62-4702-b439-c9b805e6f4c7"_cry_guid);
	desc.SetLabel("Fading Area Factor Changed");
	desc.AddMember(&CEntityAudioAreaComponent::SAreaFadingFactorChangedSignal::m_factor, 'fac', "fadingFactor", "FadingFactor", "The current fading factor, from 0.0 (FadeDistance meter away from the area) to 1.0 (inside area)", 0.0f);
}

static void ReflectType(Schematyc::CTypeDesc<CEntityAudioAreaComponent::SOutsideAreaEnteredSignal>& desc)
{
	desc.SetGUID("3c14f36d-3c1e-45e7-949f-d9a9dffdd1ce"_cry_guid);
	desc.SetLabel("Outside Area Entered");
}

static void ReflectType(Schematyc::CTypeDesc<CEntityAudioAreaComponent::SFadingAreaEnteredSignal>& desc)
{
	desc.SetGUID("3167cb2d-1d4c-4d92-bff5-8c4178f7d9c5"_cry_guid);
	desc.SetLabel("Fading Area Entered");
	desc.AddMember(&CEntityAudioAreaComponent::SFadingAreaEnteredSignal::m_factor, 'fac', "fadingFactor", "FadingFactor", "The current fading factor, from 0.0 (from FadeDistance meter away from the area) to 1.0 (in area)", 0.0f);
}

static void ReflectType(Schematyc::CTypeDesc<CEntityAudioAreaComponent::SInsideAreaEnteredSignal>& desc)
{
	desc.SetGUID("141c9773-ad11-4587-8e3a-fa88679d08b0"_cry_guid);
	desc.SetLabel("Inside Area Entered");
}

void CEntityAudioAreaComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	// Functions
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityAudioAreaComponent::SetEnabled, "2aa27775-8cd5-4102-83a0-6cae12b96268"_cry_guid, "SetEnabled");
		pFunction->SetDescription("Enables/Disables sending of \"Fade Changed\" signals");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		pFunction->BindInput(1, 'val', "Value");
		componentScope.Register(pFunction);
	}
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEntityAudioAreaComponent::GetCurrentFadeFactor, "336e2e26-8e81-4e40-9170-ff6c2053f377"_cry_guid, "GetCurrentFadeFactor");
		pFunction->SetDescription("Gets the current FadeFactor based on the specified Fading Distance");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::None);
		pFunction->BindOutput(0, 'fac', "Factor");
		componentScope.Register(pFunction);
	}
	// Signals
	{
		componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(CEntityAudioAreaComponent::SAreaFadingFactorChangedSignal));
		componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(CEntityAudioAreaComponent::SOutsideAreaEnteredSignal));
		componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(CEntityAudioAreaComponent::SFadingAreaEnteredSignal));
		componentScope.Register(SCHEMATYC_MAKE_ENV_SIGNAL(CEntityAudioAreaComponent::SInsideAreaEnteredSignal));
	}
}

void CEntityAudioAreaComponent::Initialize()
{
	m_pAudioComp = m_pEntity->GetOrCreateComponent<IEntityAudioComponent>();
	m_pAudioComp->GetComponentFlags().Add(EEntityComponentFlags::UserAdded);
}

void CEntityAudioAreaComponent::OnShutDown()
{
}

uint64 CEntityAudioAreaComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_MOVENEARAREA)
	       | BIT64(ENTITY_EVENT_ENTERAREA)
	       | BIT64(ENTITY_EVENT_MOVEINSIDEAREA)
	       | BIT64(ENTITY_EVENT_LEAVEAREA)
	       | BIT64(ENTITY_EVENT_ENTERNEARAREA)
	       | BIT64(ENTITY_EVENT_LEAVENEARAREA)
	       | BIT64(ENTITY_EVENT_RESET);
}

void CEntityAudioAreaComponent::SetEnabled(bool bEnabled)
{
	if (bEnabled && m_bEnabled != bEnabled)
	{
		m_bEnabled = true;
		SetFadingFactor(m_currentFadeFactor, GetEntity()->GetSchematycObject(), true);
	}
	else
	{
		m_bEnabled = bEnabled;
	}
}

void CEntityAudioAreaComponent::ProcessEvent(SEntityEvent& event)
{
	if (gEnv->IsDedicated())
		return;

	if (event.nParam[0] != m_triggeringEntityId
	    && m_triggeringEntityId != INVALID_ENTITYID
	    && event.event != ENTITY_EVENT_RESET)
	{
		return;         //early out, this is not the triggering object we are looking for.
	}

	Schematyc::IObject* pSchematycObject = GetEntity()->GetSchematycObject();

	switch (event.event)
	{
	case ENTITY_EVENT_ENTERNEARAREA:
		{
			if (m_areaState != EAreaState::Near)
			{
				if (pSchematycObject && m_bActive && m_bEnabled)
				{
					pSchematycObject->ProcessSignal(SFadingAreaEnteredSignal { 0.0f }, GetGUID());
				}
				m_areaState = EAreaState::Near;
				SetFadingFactor(0.0f, pSchematycObject, true);
				ToggleAudioSpots(true);
			}
		}
		break;
	case ENTITY_EVENT_ENTERAREA:
		{
			if (m_areaState != EAreaState::Inside)
			{
				if (pSchematycObject && m_bActive && m_bEnabled)
				{
					pSchematycObject->ProcessSignal(SInsideAreaEnteredSignal(), GetGUID());
				}
				m_areaState = EAreaState::Inside;
				SetFadingFactor(1.0f, pSchematycObject, true);
			}
		}
		break;
	case ENTITY_EVENT_LEAVEAREA:
		{
			if (m_areaState != EAreaState::Near)
			{
				if (pSchematycObject && m_bActive && m_bEnabled)
				{
					pSchematycObject->ProcessSignal(SFadingAreaEnteredSignal { 1.0f }, GetGUID());
				}
				m_areaState = EAreaState::Near;
				SetFadingFactor(1.0f, pSchematycObject, true);
			}
		}
		break;
	case ENTITY_EVENT_LEAVENEARAREA:
		{
			if (m_areaState != EAreaState::Outside)
			{
				if (pSchematycObject && m_bActive && m_bEnabled)
				{
					pSchematycObject->ProcessSignal(SOutsideAreaEnteredSignal(), GetGUID());
				}
				m_areaState = EAreaState::Outside;
				SetFadingFactor(0.f, pSchematycObject, true);
				ToggleAudioSpots(false);
			}
		}
		break;
	case ENTITY_EVENT_MOVEINSIDEAREA:
		{
			const float factor = event.fParam[0];
			m_areaState = EAreaState::Inside;
			SetFadingFactor(crymath::clamp(factor, 0.0f, 1.0f), pSchematycObject);
		}
		break;
	case ENTITY_EVENT_MOVENEARAREA:
		{
			const float distance = event.fParam[0];
			m_areaState = EAreaState::Near;
			SetFadingFactor(crymath::clamp((m_fadeDistance - distance) / m_fadeDistance, 0.0f, 1.0f), pSchematycObject);
		}
		break;
	case ENTITY_EVENT_RESET:
		{
			if (event.nParam[0] == 1)     //entering game
			{
				m_bActive = true;
				if (m_bMoveAlongWithTriggeringEntity)
				{
					m_pEntity->SetFlags(m_pEntity->GetFlags() | ENTITY_FLAG_VOLUME_SOUND);
				}
				if (m_pAudioComp)
				{
					m_pAudioComp->SetFadeDistance(m_fadeDistance);
					if (m_environment.m_envId != CryAudio::InvalidControlId)
					{
						m_pAudioComp->SetEnvironmentId(m_environment.m_envId);
						m_pAudioComp->SetEnvironmentFadeDistance(m_fadeDistance);  //for now, we use the same fading distance
					}
				}
				IAreaManager* const pAreaManager = gEnv->pEntitySystem->GetAreaManager();
				pAreaManager->SetAreasDirty();

				m_triggeringEntityId = INVALID_ENTITYID;
				if (!m_triggerEntityName.empty())
				{
					IEntity* pEntity = nullptr;
					if (m_triggerEntityName == "Player")
						pEntity = gEnv->pGameFramework->GetClientEntity();
					else
						pEntity = gEnv->pEntitySystem->FindEntityByName(m_triggerEntityName.c_str());
					if (pEntity)
					{
						m_triggeringEntityId = pEntity->GetId();
					}
					else
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "<Audio>: Could not find specified triggering entity '%s'!", m_triggerEntityName.c_str());
					}
				}
			}
			else
			{
				m_bActive = false;
			}
		}
		break;
	}
}

void CEntityAudioAreaComponent::SetFadingFactor(float factor, Schematyc::IObject* pSchematycObject, bool bForceSet)
{
	if(fabs_tpl(m_currentFadeFactor - factor) > 0.01f || bForceSet)  //we only update if the new factor is noticeably different to the current
	{
		m_currentFadeFactor = factor;

		if (pSchematycObject && m_bEnabled && m_bActive)
		{
			pSchematycObject->ProcessSignal(SAreaFadingFactorChangedSignal { factor }, GetGUID());

			if (m_controlledAudioParameter.m_parameterId != CryAudio::InvalidControlId && m_pAudioComp)
			{
				m_pAudioComp->SetParameter(m_controlledAudioParameter.m_parameterId, m_currentFadeFactor);
			}
		}
	}
}

void CEntityAudioAreaComponent::ToggleAudioSpots(bool bEnable)
{
	if (m_bActive && m_bEnabled && m_bToggleAudioTriggerSpots)
	{
		const IEntity::ComponentsVisitor visitComponent = [bEnable](IEntityComponent* pComponent)
		{
			if (pComponent->GetClassDesc().GetGUID() == Schematyc::GetTypeDesc<CEntityAudioSpotComponent>().GetGUID())
			{
				CEntityAudioSpotComponent* pAudioSpotComp = static_cast<CEntityAudioSpotComponent*>(pComponent);
				pAudioSpotComp->Enable(bEnable);
			}
		};
		m_pEntity->VisitComponents(visitComponent);
	}
}

void SAudioEnvironmentSerializeHelper::Serialize(Serialization::IArchive& archive)
{
	archive(Serialization::AudioEnvironment<string>(m_envName), "environment", "^Name ");

	if (archive.isInput())
	{
		gEnv->pAudioSystem->GetEnvironmentId(m_envName.c_str(), m_envId);
	}
}

} //DefaultComponents
} //Cry
