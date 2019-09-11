// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CryAudio/IListener.h>
#include <CrySerialization/Decorators/ResourcesAudio.h>
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
struct SListenerSerializeHelper final
{
	SListenerSerializeHelper() = default;

	explicit SListenerSerializeHelper(char const* const szName)
		: m_name(szName)
	{
		m_id = CryAudio::StringToId(m_name.c_str());
	}

	bool        operator==(SListenerSerializeHelper const& other) const { return (m_id == other.m_id); }
	bool        operator!=(SListenerSerializeHelper const& other) const { return (m_id != other.m_id); }

	static void ReflectType(Schematyc::CTypeDesc<SListenerSerializeHelper>& desc)
	{
		desc.SetGUID("FFBB308D-AD8D-428A-BDCA-F9A992157CFA"_cry_guid);
	}

	void Serialize(Serialization::IArchive& archive)
	{
		archive(Serialization::AudioListener<string>(m_name), "name", "Name");
		archive(m_offset, "offset", "Offset");
		archive.doc("Offset of the listener position.");

		if (archive.isInput())
		{
			if (!m_name.empty())
			{
				m_id = CryAudio::StringToId(m_name.c_str());
			}
			else
			{
				m_name = CryAudio::g_szDefaultListenerName;
				m_id = CryAudio::DefaultListenerId;
			}

			m_hasOffset = m_offset != ZERO;
		}
	}

	CryAudio::ListenerId m_id = CryAudio::DefaultListenerId;
	string               m_name = CryAudio::g_szDefaultListenerName;
	Vec3                 m_offset = ZERO;
	bool                 m_hasOffset = false;
};

class CListenerComponent final : public IEntityComponent
{
protected:

	friend CPlugin_CryDefaultEntities;
	static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void                    Initialize() override;
	virtual void                    OnShutDown() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void                    ProcessEvent(const SEntityEvent& event) override;
	virtual void                    OnTransformChanged() override;
	// ~IEntityComponent

public:

	CListenerComponent() = default;

	static void ReflectType(Schematyc::CTypeDesc<CListenerComponent>& desc)
	{
		desc.SetGUID("BAE91D5C-8CFB-40B2-8397-F5A9EEDB9AC4"_cry_guid);
		desc.SetEditorCategory("Audio");
		desc.SetLabel("Listener");
		desc.SetDescription("Audio Listener from which transformation the audio is recorded.");
		desc.SetIcon("icons:Audio/component_audio_listener.ico");
		desc.SetComponentFlags({ IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly });

		desc.AddMember(&CListenerComponent::m_listenerHelper, 'list', "listener", "Listener", "The listener of this component.", SListenerSerializeHelper());
	}

	void SetOffset(Vec3 const& offset)
	{
		m_listenerHelper.m_offset = offset;
		m_listenerHelper.m_hasOffset = m_listenerHelper.m_offset != ZERO;
	}

private:

	CryAudio::IListener*      m_pIListener = nullptr;
	CryAudio::CTransformation m_previousTransformation;
	SListenerSerializeHelper  m_listenerHelper;

#if defined(INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE)
	CryAudio::ListenerId m_previousListenerId = CryAudio::InvalidListenerId;
#endif  // INCLUDE_DEFAULT_PLUGINS_PRODUCTION_CODE
};
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
