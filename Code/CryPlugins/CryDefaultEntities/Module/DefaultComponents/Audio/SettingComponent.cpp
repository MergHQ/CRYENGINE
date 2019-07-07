// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SettingComponent.h"
#include <CryAudio/IAudioSystem.h>

namespace Cry
{
namespace Audio
{
namespace DefaultComponents
{
//////////////////////////////////////////////////////////////////////////
void CSettingComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSettingComponent::Load, "D95940E7-D164-4187-A6BC-F93259B2EA70"_cry_guid, "Load");
		pFunction->SetDescription("Loads the setting");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		componentScope.Register(pFunction);
	}

	{
		auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CSettingComponent::Unload, "2E9E81B0-CF99-4C91-ACD3-54ACDA377D71"_cry_guid, "Unload");
		pFunction->SetDescription("Unloads the setting");
		pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
		componentScope.Register(pFunction);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSettingComponent::ReflectType(Schematyc::CTypeDesc<CSettingComponent>& desc)
{
	desc.SetGUID("D4C1E55D-A765-4C54-9C33-8B2632C36650"_cry_guid);
	desc.SetEditorCategory("Audio");
	desc.SetLabel("Setting");
	desc.SetDescription("Allows to load and unload a setting.");
	desc.SetIcon("icons:Audio/component_setting.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly, IEntityComponent::EFlags::HideFromInspector });

	desc.AddMember(&CSettingComponent::m_setting, 'sett', "setting", "Setting", "The setting which gets loaded or unloaded.", SSettingSerializeHelper());

#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	desc.AddMember(&CSettingComponent::m_loadButton, 'btn1', "load", "Load", "Loads the setting.", Serialization::FunctorActionButton<std::function<void()>>());
	desc.AddMember(&CSettingComponent::m_unloadButton, 'btn2', "unload", "Unload", "Unloads the setting.", Serialization::FunctorActionButton<std::function<void()>>());
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSettingComponent::Initialize()
{
#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	m_loadButton = Serialization::ActionButton(std::function<void()>([this]() { Load(); }));
	m_unloadButton = Serialization::ActionButton(std::function<void()>([this]() { Unload(); }));
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSettingComponent::Load()
{
	if (m_setting.m_id != CryAudio::InvalidControlId)
	{
		gEnv->pAudioSystem->LoadSetting(m_setting.m_id);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSettingComponent::Unload()
{
	if (m_setting.m_id != CryAudio::InvalidControlId)
	{
		gEnv->pAudioSystem->UnloadSetting(m_setting.m_id);
	}
}
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
