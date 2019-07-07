// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DefaultTriggerComponent.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
static void ReflectType(Schematyc::CTypeDesc<EDefaultTriggerType>& desc)
{
	desc.SetGUID("{6EDDB14E-61A2-4D19-B629-B82632C4491C}"_cry_guid);
	desc.SetLabel("Trigger");
	desc.SetDescription("Default trigger to execute");
	desc.SetDefaultValue(EDefaultTriggerType::MuteAll);
	desc.AddConstant(EDefaultTriggerType::MuteAll, "mute_all", "mute_all");
	desc.AddConstant(EDefaultTriggerType::UnmuteAll, "unmute_all", "unmute_all");
	desc.AddConstant(EDefaultTriggerType::PauseAll, "pause_all", "pause_all");
	desc.AddConstant(EDefaultTriggerType::ResumeAll, "resume_all", "resume_all");
}
} // namespace CryAudio

namespace Cry
{
namespace Audio
{
namespace DefaultComponents
{
//////////////////////////////////////////////////////////////////////////
void CDefaultTriggerComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CDefaultTriggerComponent::Execute, "B4C3D6CD-2E06-4BFF-9810-6D21C48FD9AD"_cry_guid, "Execute");
		pFunction->SetDescription("Executes the Trigger");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		componentScope.Register(pFunction);
	}
}

//////////////////////////////////////////////////////////////////////////
void CDefaultTriggerComponent::ReflectType(Schematyc::CTypeDesc<CDefaultTriggerComponent>& desc)
{
	desc.SetGUID("354639D9-62FF-474A-928A-459E3C795E89"_cry_guid);
	desc.SetEditorCategory("Audio");
	desc.SetLabel("DefaultTrigger");
	desc.SetDescription("Allows for execution of a default audio trigger.");
	desc.SetIcon("icons:Audio/component_trigger.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly, IEntityComponent::EFlags::HideFromInspector });

	desc.AddMember(&CDefaultTriggerComponent::m_triggerType, 'tri1', "trigger", "Trigger", "This trigger gets executed when Execute is called.", CryAudio::EDefaultTriggerType::MuteAll);

#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	desc.AddMember(&CDefaultTriggerComponent::m_executeButton, 'btn1', "execute", "Execute", "Executes the default trigger.", Serialization::FunctorActionButton<std::function<void()>>());
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CDefaultTriggerComponent::Initialize()
{
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	m_executeButton = Serialization::ActionButton(std::function<void()>([this]() { Execute(); }));
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CDefaultTriggerComponent::Execute()
{
	switch (m_triggerType)
	{
	case CryAudio::EDefaultTriggerType::MuteAll:
		{
			GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_MUTE, 0, 0);

			break;
		}
	case CryAudio::EDefaultTriggerType::UnmuteAll:
		{
			GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_UNMUTE, 0, 0);

			break;
		}
	case CryAudio::EDefaultTriggerType::PauseAll:
		{
			GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_PAUSE, 0, 0);

			break;
		}
	case CryAudio::EDefaultTriggerType::ResumeAll:
		{
			GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_RESUME, 0, 0);

			break;
		}
	default:
		break;
	}
}
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
