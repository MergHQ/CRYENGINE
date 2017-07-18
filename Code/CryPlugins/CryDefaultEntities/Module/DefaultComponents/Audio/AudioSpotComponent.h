// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>

class CPlugin_CryDefaultEntities;

namespace Schematyc
{
	static void ReflectType(CTypeDesc<CryAudio::EOcclusionType>& desc)
	{
		desc.SetGUID("ed87ee11-3733-4d3f-90de-fab399d011f1"_cry_guid);
	}
}

namespace Cry
{
	namespace DefaultComponents
	{
		struct SAudioTriggerSerializeHelper
		{
			void        Serialize(Serialization::IArchive& archive);
			bool operator == (const SAudioTriggerSerializeHelper & other) const { return m_triggerName == other.m_triggerName; }

			static void ReflectType(Schematyc::CTypeDesc<SAudioTriggerSerializeHelper>& desc)
			{
				desc.SetGUID("C5DE4974-ECAB-4D6F-A93D-02C1F5C55C31"_cry_guid);
			}

			CryAudio::ControlId m_triggerId = CryAudio::InvalidControlId;
			string              m_triggerName;
		};

		struct SAudioParameterSerializeHelper
		{
			void        Serialize(Serialization::IArchive& archive);
			bool operator == (const SAudioParameterSerializeHelper & other) const { return m_parameterName == other.m_parameterName; }

			static void ReflectType(Schematyc::CTypeDesc<SAudioParameterSerializeHelper>& desc)
			{
				desc.SetGUID("5287D8F9-7638-41BB-BFDD-2F5B47DEEA07"_cry_guid);
			}

			CryAudio::ControlId m_parameterId = CryAudio::InvalidControlId;
			string              m_parameterName;
		};

		struct SAudioSwitchWithStateSerializeHelper
		{
			void        Serialize(Serialization::IArchive& archive);
			bool operator == (const SAudioSwitchWithStateSerializeHelper & other) const { return m_switchName == other.m_switchName && m_switchStateName == other.m_switchStateName; }

			static void ReflectType(Schematyc::CTypeDesc<SAudioSwitchWithStateSerializeHelper>& desc)
			{
				desc.SetGUID("9DB56B33-57FE-4E97-BED2-F0BBD3012967"_cry_guid);
			}

			CryAudio::ControlId     m_switchId = CryAudio::InvalidControlId;
			string                  m_switchName;
			CryAudio::SwitchStateId m_switchStateId = CryAudio::InvalidSwitchStateId;
			string                  m_switchStateName;
		};

		class CEntityAudioSpotComponent : public IEntityComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;
			virtual void OnShutDown() final;
			virtual uint64 GetEventMask() const final;
			virtual void ProcessEvent(SEntityEvent& event) final;
			// ~IEntityComponent

		public:
			enum class EPlayMode
			{
				None = 0, //default trigger not executed automatically
				TriggerOnce = 1, //default trigger executed once on start
				ReTriggerConstantly = 2, //default trigger executed constantly after a delay
				ReTriggerWhenDone = 3, //default trigger executed constantly when finished after a delay
			};

			struct SAudioTriggerFinishedSignal
			{
				SAudioTriggerFinishedSignal() {}
				SAudioTriggerFinishedSignal(uint32 instanceId, uint32 triggerId, bool bSuccess);

				uint32      m_instanceId = 0;
				uint32      m_triggerId = 0;
				bool        m_bSuccess = false;
			};

			static void ReflectType(Schematyc::CTypeDesc<CEntityAudioSpotComponent>& desc)
			{
				desc.SetGUID("c86954ca-d759-40a0-891e-152f7125d60d"_cry_guid);
				desc.SetEditorCategory("Audio");
				desc.SetLabel("Audio Spot");
				desc.SetDescription("Entity audio spot component");
				desc.SetIcon("icons:schematyc/entity_audio_component.ico");
				desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly });

				desc.AddMember(&CEntityAudioSpotComponent::m_occlusionType, 'occ', "occlusionType", "Occlusion Type", "Specifies the occlusion type for all sounds played via this component.", CryAudio::EOcclusionType::Ignore);
				desc.AddMember(&CEntityAudioSpotComponent::m_defaultTrigger, 'tri', "defaultTrigger", "Default Trigger", "The default trigger that should be used.", SAudioTriggerSerializeHelper());
				desc.AddMember(&CEntityAudioSpotComponent::m_playMode, 'mode', "playMode", "Play Mode", "PlayMode used for the DefaultTrigger", CEntityAudioSpotComponent::EPlayMode::TriggerOnce);
				desc.AddMember(&CEntityAudioSpotComponent::m_minDelay, 'min', "minDelay", "Min Delay", "Depending on the PlayMode: The min time between triggering or the min delay of re-triggering, after the trigger has finished", 1.0f);
				desc.AddMember(&CEntityAudioSpotComponent::m_maxDelay, 'max', "maxDelay", "Max Delay", "Depending on the PlayMode: The max time between triggering or the max delay of re-triggering, after the trigger has finished", 2.0f);
				desc.AddMember(&CEntityAudioSpotComponent::m_bEnabled, 'ena', "enabled", "Enabled", "Enables/Disables the looping of the default Trigger", true);
				desc.AddMember(&CEntityAudioSpotComponent::m_randomOffset, 'rand', "randomOffset", "Random Offset", "Randomized offset that is added to the proxy position on each triggering", Vec3(0.f));
			}

			virtual void ExecuteTrigger(const SAudioTriggerSerializeHelper& trigger, uint32& _instanceId, uint32& _triggerId)
			{
				if (m_pAudioComp && trigger.m_triggerId != CryAudio::InvalidControlId)
				{
					static uint32 currentInstanceId = 1;
					CryAudio::SRequestUserData const userData(CryAudio::ERequestFlags::DoneCallbackOnExternalThread, this, (void*)static_cast<UINT_PTR>(currentInstanceId));
					if (m_pAudioComp->ExecuteTrigger(trigger.m_triggerId, m_auxAudioObjectId, userData))
					{
						_instanceId = currentInstanceId++;
					}
				}
				else
				{
					_triggerId = 0;
					_instanceId = 0;
				}
			}

			virtual void StopTrigger(const SAudioTriggerSerializeHelper& trigger)
			{
				if (m_pAudioComp && trigger.m_triggerId != CryAudio::InvalidControlId)
				{
					m_pAudioComp->StopTrigger(trigger.m_triggerId, m_auxAudioObjectId);
				}
			}

			virtual void SetParameter(const SAudioParameterSerializeHelper& parameter, float value)
			{
				if (m_pAudioComp && parameter.m_parameterId != CryAudio::InvalidControlId)
				{
					m_pAudioComp->SetParameter(parameter.m_parameterId, value, m_auxAudioObjectId);
				}
			}

			virtual void SetSwitchState(const SAudioSwitchWithStateSerializeHelper& switchAndState)
			{
				if (m_pAudioComp && switchAndState.m_switchId != CryAudio::InvalidControlId && switchAndState.m_switchStateId != CryAudio::InvalidControlId)
				{
					m_pAudioComp->SetSwitchState(switchAndState.m_switchId, switchAndState.m_switchStateId, m_auxAudioObjectId);
				}
			}
			
			virtual void Enable(bool bEnabled)
			{ 
				m_bEnabled = bEnabled;
				if (!bEnabled)
				{
					GetEntity()->KillTimer('ats');
					if (m_pAudioComp)
					{
						m_pAudioComp->StopTrigger(m_defaultTrigger.m_triggerId, m_auxAudioObjectId);
					}
				}
				else
				{
					ExecuteDefaultTrigger();
				}
			}

			bool IsEnabled() const { return m_bEnabled; }

			virtual void SetDefaultTrigger(const char* szName);
			const char*  GetDefaultTrigger() const { return m_defaultTrigger.m_triggerName;}

			virtual void SetPlayMode(EPlayMode mode) { m_playMode = mode; }
			EPlayMode GetPlayMode() const { return m_playMode; }

			virtual void SetMinimumDelay(float delay) { m_minDelay = delay; }
			float GetMinimumDelay() const { return m_minDelay; }
			virtual void SetMaximumDelay(float delay) { m_maxDelay = delay; }
			float GetMaximumDelay() const { return m_maxDelay; }

			virtual void SetOcclusionType(CryAudio::EOcclusionType type) { m_occlusionType = type; }
			CryAudio::EOcclusionType GetOcclusionType() const { return m_occlusionType; }

			//audio callback
			static void OnAudioCallback(CryAudio::SRequestInfo const* const pAudioRequestInfo);

		protected:
			void Update(const Schematyc::SUpdateContext& updateContext);
			bool ExecuteDefaultTrigger();

			CryAudio::AuxObjectId        m_auxAudioObjectId = CryAudio::InvalidAuxObjectId;
			IEntityAudioComponent*       m_pAudioComp = nullptr;
			bool                         m_bActive = false;
			int                          m_timerId = IEntity::CREATE_NEW_UNIQUE_TIMER_ID;

			//Properties exposed to UI
			SAudioTriggerSerializeHelper m_defaultTrigger;
			EPlayMode                    m_playMode = EPlayMode::ReTriggerWhenDone;
			Schematyc::Range<0, 10000>   m_minDelay = 1.0f;
			Schematyc::Range<0, 10000>   m_maxDelay = 2.0f;
			CryAudio::EOcclusionType     m_occlusionType = CryAudio::EOcclusionType::Ignore;
			bool                         m_bEnabled = true;
			Vec3						 m_randomOffset;
		};

		static void ReflectType(Schematyc::CTypeDesc<CEntityAudioSpotComponent::EPlayMode>& desc)
		{
			desc.SetGUID("f40378ca-fd06-4f6e-b84d-b974b57e2ada"_cry_guid);
		}
	} // DefaultComponents
}  //Cry
